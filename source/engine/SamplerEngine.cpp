#include "SamplerEngine.h"

SamplerEngine::SamplerEngine (const ParamRefs& paramsToUse, SampleStore& sampleStoreToUse)
    : params (paramsToUse), sampleStore (sampleStoreToUse)
{
}

void SamplerEngine::prepare (double sampleRate) noexcept
{
    hostSampleRate = sampleRate;

    for (auto& voice : voices)
        voice.stop();

    updatePlayingFlags();
}

void SamplerEngine::beginBlock() noexcept
{
    auto* latest = sampleStore.getSampleForAudio();

    if (latest == sample)
        return;

    for (auto& voice : voices)
        voice.stop();

    sample = latest;
    updatePlayingFlags();
}

void SamplerEngine::triggerPad (int padIndex, float velocity) noexcept
{
    if (sample == nullptr || hostSampleRate <= 0.0 || ! juce::isPositiveAndBelow (padIndex, kNumPads))
        return;

    const auto& pad = params.pad (padIndex);

    if (pad.get (PadParam::assigned) < 0.5f)
        return;

    const auto total = static_cast<double> (sample->getNumSamples());
    const auto startSample = static_cast<juce::int64> (std::llround (static_cast<double> (pad.get (PadParam::start)) * total));
    const auto endSample = static_cast<juce::int64> (std::llround (static_cast<double> (pad.get (PadParam::end)) * total));
    const auto gain = juce::Decibels::decibelsToGain (pad.get (PadParam::gain)) * velocity;

    voices[static_cast<size_t> (padIndex)].start (startSample, endSample, gain, sample->sampleRate / hostSampleRate);
}

void SamplerEngine::releasePad (int padIndex) noexcept
{
    // Only matters in gate mode, which isn't implemented yet
    juce::ignoreUnused (padIndex);
}

void SamplerEngine::render (juce::AudioBuffer<float>& buffer, int startSample, int numSamples) noexcept
{
    if (sample == nullptr || numSamples <= 0)
        return;

    for (auto& voice : voices)
        voice.render (sample->buffer, buffer, startSample, numSamples);

    updatePlayingFlags();
}

bool SamplerEngine::isPadPlaying (int padIndex) const noexcept
{
    return juce::isPositiveAndBelow (padIndex, kNumPads) && playing[static_cast<size_t> (padIndex)].load (std::memory_order_relaxed);
}

void SamplerEngine::updatePlayingFlags() noexcept
{
    for (size_t i = 0; i < voices.size(); ++i)
        playing[i].store (voices[i].isActive(), std::memory_order_relaxed);
}
