#include "helpers/sampler_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

TEST_CASE ("Pad playback", "[engine]")
{
    constexpr int sampleLength = 1600; // 16 slices of 100 samples
    constexpr int sliceLength = sampleLength / kNumPads;

    PluginProcessor plugin;
    plugin.setRateAndBufferSizeDetails (48000.0, 512);
    plugin.prepareToPlay (48000.0, 512);

    SECTION ("nothing plays without a sample")
    {
        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (1, noteForPad (plugin, 0), static_cast<juce::uint8> (127)), 0);

        CHECK (isSilent (processBlock (plugin, midi), 0, 512));
    }

    SECTION ("a pad hit plays that pad's region and then stops")
    {
        const auto ramp = makeRampSample (sampleLength);
        plugin.getSampleStore().setSample (ramp);
        plugin.getController().divideEqually();

        const int pad = 2;
        const int hitPosition = 10;

        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (1, noteForPad (plugin, pad), static_cast<juce::uint8> (127)), hitPosition);

        const auto output = processBlock (plugin, midi);

        // Silent before the hit
        CHECK (isSilent (output, 0, hitPosition));

        // Then the region, on both channels since the sample is mono
        for (int i = 0; i < sliceLength; ++i)
        {
            const auto expected = ramp->buffer.getSample (0, pad * sliceLength + i);

            REQUIRE_THAT (output.getSample (0, hitPosition + i), Catch::Matchers::WithinAbs (expected, 1.0e-5));
            REQUIRE_THAT (output.getSample (1, hitPosition + i), Catch::Matchers::WithinAbs (expected, 1.0e-5));
        }

        // And silence once the region ends
        const auto afterRegion = hitPosition + sliceLength;
        CHECK (isSilent (output, afterRegion, 512 - afterRegion));
        CHECK_FALSE (plugin.getEngine().isPadPlaying (pad));
    }

    SECTION ("a region longer than the block carries on into the next one")
    {
        plugin.getSampleStore().setSample (makeRampSample (16 * 1000));
        plugin.getController().divideEqually();

        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (1, noteForPad (plugin, 0), static_cast<juce::uint8> (127)), 0);

        processBlock (plugin, midi);
        CHECK (plugin.getEngine().isPadPlaying (0));

        const auto second = processBlock (plugin, {});
        CHECK (second.getMagnitude (0, 400) > 0.0f);
        CHECK (isSilent (second, 500, 12));
        CHECK_FALSE (plugin.getEngine().isPadPlaying (0));
    }
}
