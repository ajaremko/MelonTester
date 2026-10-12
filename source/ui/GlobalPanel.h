#pragma once

#include "ParamControls.h"
#include "PluginProcessor.h"

// Settings and actions for the whole sample: polyphony, loading and auto-slicing.
class GlobalPanel : public juce::Component
{
public:
    explicit GlobalPanel (PluginProcessor& processorToUse);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void chooseSample();

    PluginProcessor& processor;

    juce::TextButton loadButton { "Load sample..." };
    ParamChoice polyMode;
    juce::TextButton divideButton { "Divide equally" };
    juce::TextButton transientsButton { "Detect transients" };
    ParamKnob sensitivity;

    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GlobalPanel)
};
