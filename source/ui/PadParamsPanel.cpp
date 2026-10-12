#include "PadParamsPanel.h"

PadParamsPanel::PadParamsPanel (PluginProcessor& processorToUse)
    : processor (processorToUse),
      pitch (processorToUse.getValueTreeState(), selectedPadParamID (PadParam::pitch), "Pitch"),
      gain (processorToUse.getValueTreeState(), selectedPadParamID (PadParam::gain), "Gain"),
      attack (processorToUse.getValueTreeState(), selectedPadParamID (PadParam::attack), "Attack"),
      release (processorToUse.getValueTreeState(), selectedPadParamID (PadParam::release), "Release"),
      playMode (processorToUse.getValueTreeState(), selectedPadParamID (PadParam::playMode), "Play mode"),
      chokeGroup (processorToUse.getValueTreeState(), selectedPadParamID (PadParam::chokeGroup), "Choke group"),
      reverse (processorToUse.getValueTreeState(), selectedPadParamID (PadParam::reverse), "Reverse"),
      loop (processorToUse.getValueTreeState(), selectedPadParamID (PadParam::loop), "Loop")
{
    title.setFont (juce::Font (juce::FontOptions (18.0f, juce::Font::bold)));
    addAndMakeVisible (title);

    addAndMakeVisible (pitch);
    addAndMakeVisible (gain);
    addAndMakeVisible (attack);
    addAndMakeVisible (release);
    addAndMakeVisible (playMode);
    addAndMakeVisible (chokeGroup);
    addAndMakeVisible (reverse);
    addAndMakeVisible (loop);

    timerCallback();
    startTimerHz (30);
}

void PadParamsPanel::timerCallback()
{
    const auto selected = processor.getParams().getSelectedPad();

    if (selected != shownPad)
    {
        shownPad = selected;
        title.setText ("Pad " + juce::String (selected + 1), juce::dontSendNotification);
    }
}

void PadParamsPanel::paint (juce::Graphics& g)
{
    fillPanelBackground (g, getLocalBounds());
}

void PadParamsPanel::resized()
{
    auto area = getLocalBounds().reduced (8);

    title.setBounds (area.removeFromTop (26));
    area.removeFromTop (4);

    auto knobs = area.removeFromTop (juce::jmin (100, area.getHeight() / 2));
    const auto knobWidth = knobs.getWidth() / 4;

    pitch.setBounds (knobs.removeFromLeft (knobWidth));
    gain.setBounds (knobs.removeFromLeft (knobWidth));
    attack.setBounds (knobs.removeFromLeft (knobWidth));
    release.setBounds (knobs);

    area.removeFromTop (12);

    auto choices = area.removeFromTop (44);
    playMode.setBounds (choices.removeFromLeft (choices.getWidth() / 2).withTrimmedRight (4));
    chokeGroup.setBounds (choices.withTrimmedLeft (4));

    area.removeFromTop (8);

    auto toggles = area.removeFromTop (24);
    reverse.setBounds (toggles.removeFromLeft (toggles.getWidth() / 2));
    loop.setBounds (toggles);
}
