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

void SelectedPadMirror::adoptCurrentValues()
{
    lastSelected = getSelectedPad();
    const auto& pad = padParams[static_cast<size_t> (lastSelected)];

    for (size_t i = 0; i < mirrorParams.size(); ++i)
    {
        lastMirrorValues[i] = mirrorParams[i]->getValue();
        lastPadValues[i] = pad[i]->getValue();
    }
}

void SelectedPadMirror::sync()
{
    const auto selected = getSelectedPad();
    const auto& pad = padParams[static_cast<size_t> (selected)];

    if (selected != lastSelected)
    {
        for (size_t i = 0; i < mirrorParams.size(); ++i)
            setParameterNormalised (*mirrorParams[i], pad[i]->getValue());

        adoptCurrentValues();
        return;
    }

    for (size_t i = 0; i < mirrorParams.size(); ++i)
    {
        // Each side is compared with its own last value, so a restored state where the two differ is left alone
        if (! juce::exactlyEqual (mirrorParams[i]->getValue(), lastMirrorValues[i]))
            setParameterNormalised (*pad[i], mirrorParams[i]->getValue());
        else if (! juce::exactlyEqual (pad[i]->getValue(), lastPadValues[i]))
            setParameterNormalised (*mirrorParams[i], pad[i]->getValue());

        lastMirrorValues[i] = mirrorParams[i]->getValue();
        lastPadValues[i] = pad[i]->getValue();
    }
}
