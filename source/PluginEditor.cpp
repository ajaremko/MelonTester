#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), globalPanel (p), waveformPanel (p), padParamsPanel (p), padGrid (p)
{
    addAndMakeVisible (globalPanel);
    addAndMakeVisible (waveformPanel);
    addAndMakeVisible (padParamsPanel);
    addAndMakeVisible (padGrid);

   #if JUCE_DEBUG
    addAndMakeVisible (inspectButton);

    // this chunk of code instantiates and opens the melatonin inspector
    inspectButton.onClick = [&] {
        if (!inspector)
        {
            inspector = std::make_unique<melatonin::Inspector> (*this);
            inspector->onClose = [this]() { inspector.reset(); };
        }

        inspector->setVisible (true);
    };
   #endif

    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (920, 680);
}

PluginEditor::~PluginEditor()
{
}

void PluginEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void PluginEditor::resized()
{
    // The four areas from the README: global settings and waveform on top, pad settings and pads below
    auto area = getLocalBounds().reduced (8);

    auto top = area.removeFromTop (280);
    area.removeFromTop (8);

    auto globalArea = top.removeFromLeft (190);
    top.removeFromLeft (8);

   #if JUCE_DEBUG
    inspectButton.setBounds (globalArea.removeFromBottom (24));
    globalArea.removeFromBottom (4);
   #endif

    globalPanel.setBounds (globalArea);
    waveformPanel.setBounds (top);

    padGrid.setBounds (area.removeFromRight (area.getHeight()));
    area.removeFromRight (8);
    padParamsPanel.setBounds (area);
}
