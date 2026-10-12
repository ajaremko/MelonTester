#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// Message thread only: changes a parameter the way a user gesture would, so the host records it.
inline void setParameterNormalised (juce::RangedAudioParameter& param, float normalised)
{
    normalised = juce::jlimit (0.0f, 1.0f, normalised);

    if (juce::exactlyEqual (param.getValue(), normalised))
        return;

    param.beginChangeGesture();
    param.setValueNotifyingHost (normalised);
    param.endChangeGesture();
}

inline void setParameterValue (juce::RangedAudioParameter& param, float value)
{
    setParameterNormalised (param, param.convertTo0to1 (value));
}
