#include "Slicer.h"

std::vector<SliceRegion> Slicer::divideEqually (juce::int64 totalSamples, int numSlices)
{
    std::vector<SliceRegion> slices;

    if (totalSamples <= 0 || numSlices <= 0)
        return slices;

    slices.reserve (static_cast<size_t> (numSlices));

    for (int i = 0; i < numSlices; ++i)
        slices.push_back ({ totalSamples * i / numSlices, totalSamples * (i + 1) / numSlices });

    return slices;
}

std::vector<SliceRegion> Slicer::detectTransients (const juce::AudioBuffer<float>& audio, float sensitivity, int maxSlices)
{
    juce::ignoreUnused (audio, sensitivity, maxSlices);
    return {};
}
