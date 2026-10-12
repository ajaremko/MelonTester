#include "SelectedPadMirror.h"
#include "ParamUtils.h"

SelectedPadMirror::SelectedPadMirror (juce::AudioProcessorValueTreeState& apvts)
    : selectedPad (apvts.getRawParameterValue (ParamIDs::selectedPad))
{
    for (const auto param : allPadParams)
    {
        mirrorParams[toIndex (param)] = apvts.getParameter (selectedPadParamID (param));

        for (int pad = 0; pad < kNumPads; ++pad)
            padParams[static_cast<size_t> (pad)][toIndex (param)] = apvts.getParameter (padParamID (pad, param));
    }
}

int SelectedPadMirror::getSelectedPad() const
{
    return juce::jlimit (0, kNumPads - 1, juce::roundToInt (selectedPad->load()) - 1);
}

void SelectedPadMirror::sync()
{
    const auto selected = getSelectedPad();
    const auto& pad = padParams[static_cast<size_t> (selected)];

    if (selected != lastSelected)
    {
        for (size_t i = 0; i < mirrorParams.size(); ++i)
        {
            setParameterNormalised (*mirrorParams[i], pad[i]->getValue());
            lastValues[i] = mirrorParams[i]->getValue();
        }

        lastSelected = selected;
        return;
    }

    for (size_t i = 0; i < mirrorParams.size(); ++i)
    {
        const auto mirrorValue = mirrorParams[i]->getValue();
        const auto padValue = pad[i]->getValue();

        if (! juce::exactlyEqual (mirrorValue, lastValues[i]))
        {
            setParameterNormalised (*pad[i], mirrorValue);
            lastValues[i] = mirrorValue;
        }
        else if (! juce::exactlyEqual (padValue, lastValues[i]))
        {
            setParameterNormalised (*mirrorParams[i], padValue);
            lastValues[i] = padValue;
        }
    }
}
