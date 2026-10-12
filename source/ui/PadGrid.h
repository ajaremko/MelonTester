#pragma once

#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>

// The 16 pads, laid out like the hardware with pad 1 at the bottom left. Click a pad to play and select it.
class PadGrid : public juce::Component,
                private juce::Timer
{
public:
    explicit PadGrid (PluginProcessor& processorToUse);

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;

private:
    void timerCallback() override;

    juce::Rectangle<int> getPadBounds (int padIndex) const;
    int getPadAt (juce::Point<int> position) const;

    // Selected pad plus which pads are assigned and playing, packed so a change is cheap to spot
    juce::uint64 getDrawState() const;

    PluginProcessor& processor;
    juce::uint64 lastDrawState = 0;
    int heldPad = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PadGrid)
};
