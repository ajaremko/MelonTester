#include "PadVoice.h"

void PadVoice::start (juce::int64 startSample, juce::int64 endSample, float gainToUse, double incrementToUse) noexcept
{
    position = static_cast<double> (startSample);
    endPosition = static_cast<double> (endSample);
    gain = gainToUse;
    increment = incrementToUse;
    active = endSample > startSample && incrementToUse > 0.0;
}

void PadVoice::render (const juce::AudioBuffer<float>& source, juce::AudioBuffer<float>& output, int startSample, int numSamples) noexcept
{
    if (! active)
        return;

    const auto sourceChannels = source.getNumChannels();
    const auto lastIndex = source.getNumSamples() - 1;
    const auto end = juce::jmin (endPosition, static_cast<double> (source.getNumSamples()));

    for (int i = 0; i < numSamples; ++i)
    {
        if (position >= end)
        {
            active = false;
            return;
        }

        const auto index = static_cast<int> (position);
        const auto next = juce::jmin (index + 1, lastIndex);
        const auto fraction = static_cast<float> (position - static_cast<double> (index));

        for (int channel = 0; channel < output.getNumChannels(); ++channel)
        {
            // A mono sample plays on every output channel
            const auto* data = source.getReadPointer (juce::jmin (channel, sourceChannels - 1));
            const auto value = data[index] + fraction * (data[next] - data[index]);

            output.addSample (channel, startSample + i, value * gain);
        }

        position += increment;
    }

    if (position >= end)
        active = false;
}
