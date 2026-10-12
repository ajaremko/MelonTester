#pragma once

#include "ParamControls.h"
#include "PluginProcessor.h"

// The selected pad's settings. Bound to the mirror parameters, so the controls follow the selection by themselves.
class PadParamsPanel : public juce::Component,
                       private juce::Timer
{
public:
    explicit PadParamsPanel (PluginProcessor& processorToUse);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    PluginProcessor& processor;
    int shownPad = -1;

    juce::Label title;
    ParamKnob pitch;
    ParamKnob gain;
    ParamKnob attack;
    ParamKnob release;
    ParamChoice playMode;
    ParamChoice chokeGroup;
    ParamToggle reverse;
    ParamToggle loop;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PadParamsPanel)
};
