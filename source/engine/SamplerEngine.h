#pragma once

#include "PadVoice.h"
#include "params/PadParams.h"
#include "sample/SampleStore.h"

/* Turns pad hits into audio. One voice per pad, so hitting a pad again restarts it.

   Audio thread only, except isPadPlaying().

   Not implemented yet: pitch, attack/release, gate mode, reverse, loop, choke groups
   and polyphony mode. Their parameters exist and are ignored here.
*/
class SamplerEngine
{
public:
    SamplerEngine (const ParamRefs& params, SampleStore& sampleStore);

    void prepare (double sampleRate) noexcept;

    // Call at the start of every block, before any triggers or rendering
    void beginBlock() noexcept;

    // Take effect from the next render() call, which is how hits land on the right sample within a block
    void triggerPad (int padIndex, float velocity) noexcept;
    void releasePad (int padIndex) noexcept;

    // Adds the playing pads to part of the buffer
    void render (juce::AudioBuffer<float>& buffer, int startSample, int numSamples) noexcept;

    // Safe to call from the message thread, for drawing
    bool isPadPlaying (int padIndex) const noexcept;

private:
    void updatePlayingFlags() noexcept;

    const ParamRefs& params;
    SampleStore& sampleStore;

    SampleData* sample = nullptr;
    double hostSampleRate = 44100.0;

    std::array<PadVoice, kNumPads> voices;
    std::array<std::atomic<bool>, kNumPads> playing {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplerEngine)
};
