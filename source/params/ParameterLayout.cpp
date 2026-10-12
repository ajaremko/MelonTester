#include "ParameterLayout.h"
#include "ParamIDs.h"

namespace
{
    using Param = std::unique_ptr<juce::RangedAudioParameter>;
    using Range = juce::NormalisableRange<float>;

    juce::ParameterID versioned (const juce::String& id)
    {
        return { id, 1 };
    }

    Range skewedRange (float min, float max, float interval, float skew)
    {
        return { min, max, interval, skew };
    }

    // Continuous settings would otherwise show seven decimal places
    juce::AudioParameterFloatAttributes withDecimals (int numDecimals)
    {
        return juce::AudioParameterFloatAttributes().withStringFromValueFunction ([numDecimals] (float value, int) { return juce::String (value, numDecimals); });
    }

    juce::StringArray chokeGroupNames()
    {
        juce::StringArray names { "Off" };

        for (int group = 1; group <= kNumChokeGroups; ++group)
            names.add (juce::String (group));

        return names;
    }

    // defaultStart and defaultEnd spread the pads evenly across the sample
    Param makePadParam (const juce::String& id, const juce::String& name, PadParam param, float defaultStart, float defaultEnd)
    {
        switch (param)
        {
            case PadParam::start:
                return std::make_unique<juce::AudioParameterFloat> (versioned (id), name, Range (0.0f, 1.0f), defaultStart, withDecimals (4));
            case PadParam::end:
                return std::make_unique<juce::AudioParameterFloat> (versioned (id), name, Range (0.0f, 1.0f), defaultEnd, withDecimals (4));
            case PadParam::pitch:
                return std::make_unique<juce::AudioParameterFloat> (versioned (id), name, Range (-24.0f, 24.0f, 0.01f), 0.0f);
            case PadParam::gain:
                return std::make_unique<juce::AudioParameterFloat> (versioned (id), name, Range (-60.0f, 12.0f, 0.1f), 0.0f);
            case PadParam::attack:
                return std::make_unique<juce::AudioParameterFloat> (versioned (id), name, skewedRange (0.0f, 1000.0f, 0.1f, 0.3f), 1.0f);
            case PadParam::release:
                return std::make_unique<juce::AudioParameterFloat> (versioned (id), name, skewedRange (0.0f, 5000.0f, 0.1f, 0.3f), 10.0f);
            case PadParam::playMode:
                return std::make_unique<juce::AudioParameterChoice> (versioned (id), name, juce::StringArray { "One-shot", "Gate" }, 0);
            case PadParam::reverse:
                return std::make_unique<juce::AudioParameterBool> (versioned (id), name, false);
            case PadParam::loop:
                return std::make_unique<juce::AudioParameterBool> (versioned (id), name, false);
            case PadParam::chokeGroup:
                return std::make_unique<juce::AudioParameterChoice> (versioned (id), name, chokeGroupNames(), 0);
            case PadParam::colour:
                return std::make_unique<juce::AudioParameterInt> (versioned (id), name, 0, kNumPadColours, 0);
            case PadParam::assigned:
                return std::make_unique<juce::AudioParameterBool> (versioned (id), name, false);
        }

        jassertfalse;
        return {};
    }

    juce::String displayName (PadParam param)
    {
        switch (param)
        {
            case PadParam::start:      return "Start";
            case PadParam::end:        return "End";
            case PadParam::pitch:      return "Pitch";
            case PadParam::gain:       return "Gain";
            case PadParam::attack:     return "Attack";
            case PadParam::release:    return "Release";
            case PadParam::playMode:   return "Play Mode";
            case PadParam::reverse:    return "Reverse";
            case PadParam::loop:       return "Loop";
            case PadParam::chokeGroup: return "Choke Group";
            case PadParam::colour:     return "Colour";
            case PadParam::assigned:   return "Assigned";
        }

        jassertfalse;
        return {};
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto global = std::make_unique<juce::AudioProcessorParameterGroup> ("global", "Global", "|");
    global->addChild (std::make_unique<juce::AudioParameterChoice> (versioned (ParamIDs::polyMode), "Polyphony", juce::StringArray { "Poly", "Mono" }, 0));
    global->addChild (std::make_unique<juce::AudioParameterFloat> (versioned (ParamIDs::zoom), "Zoom", skewedRange (1.0f, 1024.0f, 0.0f, 0.3f), 1.0f, withDecimals (1)));
    global->addChild (std::make_unique<juce::AudioParameterFloat> (versioned (ParamIDs::viewX), "View Position", Range (0.0f, 1.0f), 0.0f, withDecimals (3)));
    global->addChild (std::make_unique<juce::AudioParameterFloat> (versioned (ParamIDs::sensitivity), "Transient Sensitivity", Range (0.0f, 1.0f), 0.5f, withDecimals (2)));
    global->addChild (std::make_unique<juce::AudioParameterInt> (versioned (ParamIDs::selectedPad), "Selected Pad", 1, kNumPads, 1));
    layout.add (std::move (global));

    // The mirror set follows whichever pad is selected, see SelectedPadMirror
    auto selected = std::make_unique<juce::AudioProcessorParameterGroup> ("selected", "Selected Pad", "|");

    for (const auto param : allPadParams)
        selected->addChild (makePadParam (selectedPadParamID (param), "Selected " + displayName (param), param, 0.0f, 1.0f / static_cast<float> (kNumPads)));

    layout.add (std::move (selected));

    for (int pad = 0; pad < kNumPads; ++pad)
    {
        const auto padName = "Pad " + juce::String (pad + 1);
        const auto defaultStart = static_cast<float> (pad) / static_cast<float> (kNumPads);
        const auto defaultEnd = static_cast<float> (pad + 1) / static_cast<float> (kNumPads);

        auto group = std::make_unique<juce::AudioProcessorParameterGroup> ("pad" + juce::String (pad + 1), padName, "|");

        for (const auto param : allPadParams)
            group->addChild (makePadParam (padParamID (pad, param), padName + " " + displayName (param), param, defaultStart, defaultEnd));

        layout.add (std::move (group));
    }

    return layout;
}
