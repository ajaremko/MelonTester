#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

// Plays one region of the sample. Audio thread only.
class PadVoice
{
public:
    // Positions are in source samples. increment is how far to move through the source per output sample.
    void start (juce::int64 startSample, juce::int64 endSample, float gainToUse, double incrementToUse) noexcept;
    void stop() noexcept { active = false; }

    bool isActive() const noexcept { return active; }

    // Adds this voice's output to the buffer
    void render (const juce::AudioBuffer<float>& source, juce::AudioBuffer<float>& output, int startSample, int numSamples) noexcept;

private:
    double position = 0.0;
    double endPosition = 0.0;
    double increment = 1.0;
    float gain = 1.0f;
    bool active = false;
};
