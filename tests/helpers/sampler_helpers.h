#pragma once
#include <PluginProcessor.h>

// A mono sample where every value equals its position, scaled to stay below 1, so rendered audio shows which part of the sample played
[[maybe_unused]] static SampleData::Ptr makeRampSample (int numSamples, double sampleRate = 48000.0)
{
    SampleData::Ptr sample = new SampleData();
    sample->sampleRate = sampleRate;
    sample->buffer.setSize (1, numSamples);

    for (int i = 0; i < numSamples; ++i)
        sample->buffer.setSample (0, i, static_cast<float> (i) / static_cast<float> (numSamples));

    return sample;
}

[[maybe_unused]] static int noteForPad (const PluginProcessor& plugin, int padIndex)
{
    const auto& map = plugin.getMidiMap();

    for (int note = 0; note < 128; ++note)
        if (map.noteToPad[static_cast<size_t> (note)] == padIndex)
            return note;

    return -1;
}

[[maybe_unused]] static int ccForTarget (const PluginProcessor& plugin, ControlTarget target)
{
    const auto& map = plugin.getMidiMap();

    for (int cc = 0; cc < 128; ++cc)
        if (map.controls[static_cast<size_t> (cc)].target == target)
            return cc;

    return -1;
}

[[maybe_unused]] static float getParam (PluginProcessor& plugin, const juce::String& paramID)
{
    return plugin.getValueTreeState().getRawParameterValue (paramID)->load();
}

// Runs one block and returns what was rendered
[[maybe_unused]] static juce::AudioBuffer<float> processBlock (PluginProcessor& plugin, const juce::MidiBuffer& midi, int numSamples = 512)
{
    juce::AudioBuffer<float> buffer (2, numSamples);
    auto midiCopy = midi;

    plugin.processBlock (buffer, midiCopy);
    return buffer;
}

[[maybe_unused]] static bool same (float a, float b)
{
    return juce::exactlyEqual (a, b);
}

[[maybe_unused]] static bool isSilent (const juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    return juce::exactlyEqual (buffer.getMagnitude (startSample, numSamples), 0.0f);
}
