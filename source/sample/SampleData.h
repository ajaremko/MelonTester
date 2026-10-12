#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

// A loaded sample. Never modified after it is handed to the SampleStore, so both threads can read it.
struct SampleData : public juce::ReferenceCountedObject
{
    using Ptr = juce::ReferenceCountedObjectPtr<SampleData>;

    int getNumSamples() const noexcept { return buffer.getNumSamples(); }

    juce::AudioBuffer<float> buffer;
    double sampleRate = 44100.0;
};
