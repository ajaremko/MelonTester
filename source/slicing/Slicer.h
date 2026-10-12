#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

// A slice of the sample, in samples. end is one past the last sample played.
struct SliceRegion
{
    juce::int64 start = 0;
    juce::int64 end = 0;

    juce::int64 getLength() const noexcept { return end - start; }
};

namespace Slicer
{
    // Slices that cover the whole sample with no gaps or overlaps
    std::vector<SliceRegion> divideEqually (juce::int64 totalSamples, int numSlices);

    // Slices starting at detected hits, in time order. When more than maxSlices are found the strongest are kept.
    // Not implemented yet: always returns no slices.
    std::vector<SliceRegion> detectTransients (const juce::AudioBuffer<float>& audio, float sensitivity, int maxSlices);
}
