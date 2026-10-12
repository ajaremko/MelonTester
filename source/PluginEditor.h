#pragma once

#include "PluginProcessor.h"
#include "melatonin_inspector/melatonin_inspector.h"
#include "ui/GlobalPanel.h"
#include "ui/PadGrid.h"
#include "ui/PadParamsPanel.h"
#include "ui/WaveformPanel.h"

//==============================================================================
class PluginEditor : public juce::AudioProcessorEditor
{
public:
    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    GlobalPanel globalPanel;
    WaveformPanel waveformPanel;
    PadParamsPanel padParamsPanel;
    PadGrid padGrid;

   #if JUCE_DEBUG
    std::unique_ptr<melatonin::Inspector> inspector;
    juce::TextButton inspectButton { "Inspect the UI" };
   #endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
