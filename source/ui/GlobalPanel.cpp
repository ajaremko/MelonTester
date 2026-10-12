#include "GlobalPanel.h"

GlobalPanel::GlobalPanel (PluginProcessor& processorToUse)
    : processor (processorToUse),
      polyMode (processorToUse.getValueTreeState(), ParamIDs::polyMode, "Polyphony"),
      sensitivity (processorToUse.getValueTreeState(), ParamIDs::sensitivity, "Sensitivity")
{
    loadButton.onClick = [this] { chooseSample(); };
    divideButton.onClick = [this] { processor.getController().divideEqually(); };
    transientsButton.onClick = [this] { processor.getController().detectTransients(); };

    // Transient detection isn't implemented yet
    transientsButton.setEnabled (false);

    addAndMakeVisible (loadButton);
    addAndMakeVisible (polyMode);
    addAndMakeVisible (divideButton);
    addAndMakeVisible (transientsButton);
    addAndMakeVisible (sensitivity);
}

void GlobalPanel::paint (juce::Graphics& g)
{
    fillPanelBackground (g, getLocalBounds());
}

void GlobalPanel::resized()
{
    auto area = getLocalBounds().reduced (8);

    loadButton.setBounds (area.removeFromTop (26));
    area.removeFromTop (8);
    polyMode.setBounds (area.removeFromTop (44));
    area.removeFromTop (8);
    divideButton.setBounds (area.removeFromTop (26));
    area.removeFromTop (4);
    transientsButton.setBounds (area.removeFromTop (26));
    area.removeFromTop (4);
    sensitivity.setBounds (area.removeFromTop (juce::jmin (90, area.getHeight())));
}

void GlobalPanel::chooseSample()
{
    fileChooser = std::make_unique<juce::FileChooser> ("Load a sample", processor.getSampleStore().getFile(), processor.getSampleStore().getSupportedWildcard());

    fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this] (const juce::FileChooser& chooser) {
            const auto file = chooser.getResult();

            if (file.existsAsFile())
                processor.getController().loadSample (file);
        });
}
