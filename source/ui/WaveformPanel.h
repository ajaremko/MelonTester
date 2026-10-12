#pragma once

#include "ParamControls.h"
#include "PluginProcessor.h"
#include "params/ViewRange.h"

// The loaded sample with the pad regions drawn over it, plus the zoom, view, start and end knobs.
// Drop an audio file on it to load it.
class WaveformPanel : public juce::Component,
                      public juce::FileDragAndDropTarget,
                      private juce::ChangeListener,
                      private juce::Timer
{
public:
    explicit WaveformPanel (PluginProcessor& processorToUse);
    ~WaveformPanel() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

private:
    void changeListenerCallback (juce::ChangeBroadcaster* source) override;
    void timerCallback() override;

    // What the drawing depends on, so the timer only repaints when something moved
    std::array<float, 5> getDrawState() const;

    void rebuildPeaks (const SampleData& sample, ViewRange view);
    void paintWaveform (juce::Graphics& g, const SampleData& sample, ViewRange view);
    void paintRegions (juce::Graphics& g, ViewRange view);

    PluginProcessor& processor;

    ParamKnob zoom;
    ParamKnob viewX;
    ParamKnob start;
    ParamKnob end;

    juce::Rectangle<int> waveformArea;
    std::array<float, 5> lastDrawState {};

    // One min/max pair per pixel column, rebuilt when the sample, the view or the width changes
    std::vector<juce::Range<float>> peaks;
    const SampleData* peaksSample = nullptr;
    ViewRange peaksView;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformPanel)
};
