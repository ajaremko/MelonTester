#include "EditController.h"
#include "params/ParamUtils.h"
#include "params/ViewRange.h"
#include "slicing/Slicer.h"

namespace
{
    // One encoder step moves a region edge by this fraction of what the waveform shows, so zooming in gives finer control
    constexpr double regionStepsPerView = 512.0;

    // One encoder step moves a continuous setting by this fraction of its range
    constexpr float continuousStep = 0.01f;
}

EditController::EditController (juce::AudioProcessorValueTreeState& stateToUse, const ParamRefs& paramsToUse, SampleStore& sampleStoreToUse, CommandQueue& incomingToUse)
    : apvts (stateToUse), params (paramsToUse), sampleStore (sampleStoreToUse), incoming (incomingToUse), mirror (stateToUse)
{
    mirror.sync();
    startTimerHz (60);
}

EditController::~EditController()
{
    stopTimer();
}

void EditController::timerCallback()
{
    processPending();
}

void EditController::processPending()
{
    incoming.drain ([this] (const EditCommand& command) { apply (command); });
    mirror.sync();
    sampleStore.releaseRetired();
}

void EditController::apply (const EditCommand& command)
{
    switch (command.type)
    {
        case EditCommand::Type::selectPad:
            selectPad (command.pad);
            break;
        case EditCommand::Type::adjust:
            adjust (command.target, command.value);
            break;
        case EditCommand::Type::setNormalised:
            setNormalised (command.target, command.value);
            break;
        case EditCommand::Type::toggle:
            toggle (command.target);
            break;
        case EditCommand::Type::trigger:
            if (command.target == ControlTarget::divideEqually)
                divideEqually();
            else if (command.target == ControlTarget::detectTransients)
                detectTransients();
            break;
        case EditCommand::Type::padOn:
        case EditCommand::Type::padOff:
            break;
    }
}

//==============================================================================
juce::RangedAudioParameter& EditController::getPadParameter (int padIndex, PadParam param) const
{
    return *apvts.getParameter (padParamID (padIndex, param));
}

juce::RangedAudioParameter* EditController::getParameter (ControlTarget target) const
{
    if (isPadTarget (target))
        return &getPadParameter (getSelectedPad(), toPadParam (target));

    if (target == ControlTarget::polyMode)
        return apvts.getParameter (ParamIDs::polyMode);

    if (target == ControlTarget::zoom)
        return apvts.getParameter (ParamIDs::zoom);

    if (target == ControlTarget::viewX)
        return apvts.getParameter (ParamIDs::viewX);

    if (target == ControlTarget::sensitivity)
        return apvts.getParameter (ParamIDs::sensitivity);

    return nullptr;
}

//==============================================================================
void EditController::selectPad (int padIndex)
{
    if (! juce::isPositiveAndBelow (padIndex, kNumPads))
        return;

    setParameterValue (*apvts.getParameter (ParamIDs::selectedPad), static_cast<float> (padIndex + 1));
    mirror.sync();
    scrollToSelectedPad();
}

void EditController::scrollToSelectedPad()
{
    const auto& pad = params.pad (getSelectedPad());
    const auto zoom = params.zoom->load();
    const auto view = getViewRange (zoom, params.viewX->load());
    const auto regionStart = static_cast<double> (pad.get (PadParam::start));

    if (regionStart >= view.start && regionStart < view.getEnd())
        return;

    // Leave a little of the sample visible before the region
    const auto newViewStart = regionStart - view.length * 0.1;
    setParameterValue (*apvts.getParameter (ParamIDs::viewX), viewXForStart (newViewStart, zoom));
}

void EditController::adjust (ControlTarget target, float steps)
{
    if (target == ControlTarget::start || target == ControlTarget::end)
    {
        adjustRegionEdge (target, steps);
        return;
    }

    if (auto* param = getParameter (target))
    {
        const auto stepSize = param->isDiscrete() ? 1.0f / static_cast<float> (juce::jmax (1, param->getNumSteps() - 1))
                                                  : continuousStep;

        setParameterNormalised (*param, param->getValue() + steps * stepSize);
        mirror.sync();
    }
}

void EditController::adjustRegionEdge (ControlTarget edge, float steps)
{
    const auto selected = getSelectedPad();
    const auto& pad = params.pad (selected);

    const auto view = getViewRange (params.zoom->load(), params.viewX->load());
    const auto delta = static_cast<double> (steps) * view.length / regionStepsPerView;

    const auto start = static_cast<double> (pad.get (PadParam::start));
    const auto end = static_cast<double> (pad.get (PadParam::end));

    if (edge == ControlTarget::start)
        setParameterValue (getPadParameter (selected, PadParam::start), static_cast<float> (juce::jlimit (0.0, end, start + delta)));
    else
        setParameterValue (getPadParameter (selected, PadParam::end), static_cast<float> (juce::jlimit (start, 1.0, end + delta)));

    mirror.sync();
}

void EditController::setNormalised (ControlTarget target, float normalised)
{
    if (auto* param = getParameter (target))
    {
        setParameterNormalised (*param, normalised);
        mirror.sync();
    }
}

void EditController::toggle (ControlTarget target)
{
    if (auto* param = getParameter (target))
    {
        // Settings with more than two choices step to the next one and wrap around
        const auto numSteps = juce::jmax (2, param->getNumSteps());
        const auto current = juce::roundToInt (param->getValue() * static_cast<float> (numSteps - 1));
        const auto next = (current + 1) % numSteps;

        setParameterNormalised (*param, static_cast<float> (next) / static_cast<float> (numSteps - 1));
        mirror.sync();
    }
}

//==============================================================================
bool EditController::loadSample (const juce::File& file)
{
    if (! sampleStore.loadFile (file))
        return false;

    divideEqually();
    return true;
}

void EditController::divideEqually()
{
    if (const auto sample = sampleStore.getSample())
        assignSlices (Slicer::divideEqually (sample->getNumSamples(), kNumPads), sample->getNumSamples());
}

void EditController::detectTransients()
{
    if (const auto sample = sampleStore.getSample())
    {
        const auto slices = Slicer::detectTransients (sample->buffer, params.sensitivity->load(), kNumPads);

        if (! slices.empty())
            assignSlices (slices, sample->getNumSamples());
    }
}

void EditController::assignSlices (const std::vector<SliceRegion>& slices, juce::int64 totalSamples)
{
    if (totalSamples <= 0)
        return;

    for (int pad = 0; pad < kNumPads; ++pad)
    {
        const auto hasSlice = static_cast<size_t> (pad) < slices.size();

        if (hasSlice)
        {
            const auto& slice = slices[static_cast<size_t> (pad)];
            const auto total = static_cast<double> (totalSamples);

            setParameterValue (getPadParameter (pad, PadParam::start), static_cast<float> (static_cast<double> (slice.start) / total));
            setParameterValue (getPadParameter (pad, PadParam::end), static_cast<float> (static_cast<double> (slice.end) / total));
        }

        setParameterValue (getPadParameter (pad, PadParam::assigned), hasSlice ? 1.0f : 0.0f);
    }

    mirror.sync();
}
