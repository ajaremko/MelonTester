#pragma once

#include "ParamIDs.h"
#include <juce_audio_processors/juce_audio_processors.h>

// Pointers to the parameters' atomic values, looked up once so the audio thread can read them without searching.
struct PadParams
{
    std::array<std::atomic<float>*, kNumPadParams> values {};

    float get (PadParam param) const noexcept
    {
        return values[toIndex (param)]->load (std::memory_order_relaxed);
    }
};

struct ParamRefs
{
    explicit ParamRefs (juce::AudioProcessorValueTreeState& apvts)
    {
        for (int pad = 0; pad < kNumPads; ++pad)
            for (const auto param : allPadParams)
                pads[static_cast<size_t> (pad)].values[toIndex (param)] = apvts.getRawParameterValue (padParamID (pad, param));

        for (const auto param : allPadParams)
            selected.values[toIndex (param)] = apvts.getRawParameterValue (selectedPadParamID (param));

        polyMode = apvts.getRawParameterValue (ParamIDs::polyMode);
        zoom = apvts.getRawParameterValue (ParamIDs::zoom);
        viewX = apvts.getRawParameterValue (ParamIDs::viewX);
        sensitivity = apvts.getRawParameterValue (ParamIDs::sensitivity);
        selectedPad = apvts.getRawParameterValue (ParamIDs::selectedPad);
    }

    const PadParams& pad (int padIndex) const noexcept
    {
        return pads[static_cast<size_t> (padIndex)];
    }

    // Zero-based, unlike the host-facing parameter which counts from 1
    int getSelectedPad() const noexcept
    {
        return juce::jlimit (0, kNumPads - 1, juce::roundToInt (selectedPad->load (std::memory_order_relaxed)) - 1);
    }

    std::array<PadParams, kNumPads> pads;
    PadParams selected;

    std::atomic<float>* polyMode = nullptr;
    std::atomic<float>* zoom = nullptr;
    std::atomic<float>* viewX = nullptr;
    std::atomic<float>* sensitivity = nullptr;
    std::atomic<float>* selectedPad = nullptr;
};
