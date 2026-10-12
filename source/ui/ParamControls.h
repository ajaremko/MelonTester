#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

// Small controls that bind themselves to a parameter by ID.

inline void fillPanelBackground (juce::Graphics& g, juce::Rectangle<int> bounds)
{
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRoundedRectangle (bounds.toFloat(), 6.0f);
}

class ParamKnob : public juce::Component
{
public:
    ParamKnob (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID, const juce::String& labelText)
        : attachment (*apvts.getParameter (paramID), slider)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 16);
        addAndMakeVisible (slider);

        label.setText (labelText, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (label);
    }

    void resized() override
    {
        auto area = getLocalBounds();
        label.setBounds (area.removeFromTop (16));
        slider.setBounds (area);
    }

private:
    juce::Slider slider;
    juce::Label label;
    juce::SliderParameterAttachment attachment;
};

class ParamToggle : public juce::Component
{
public:
    ParamToggle (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID, const juce::String& labelText)
        : button (labelText), attachment (*apvts.getParameter (paramID), button)
    {
        addAndMakeVisible (button);
    }

    void resized() override { button.setBounds (getLocalBounds()); }

private:
    juce::ToggleButton button;
    juce::ButtonParameterAttachment attachment;
};

class ParamChoice : public juce::Component
{
public:
    ParamChoice (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID, const juce::String& labelText)
    {
        auto* param = apvts.getParameter (paramID);

        // The items have to exist before the attachment sets the current one
        comboBox.addItemList (param->getAllValueStrings(), 1);
        attachment = std::make_unique<juce::ComboBoxParameterAttachment> (*param, comboBox);
        addAndMakeVisible (comboBox);

        label.setText (labelText, juce::dontSendNotification);
        addAndMakeVisible (label);
    }

    void resized() override
    {
        auto area = getLocalBounds();
        label.setBounds (area.removeFromTop (16));
        comboBox.setBounds (area.removeFromTop (24));
    }

private:
    juce::ComboBox comboBox;
    juce::Label label;
    std::unique_ptr<juce::ComboBoxParameterAttachment> attachment;
};
