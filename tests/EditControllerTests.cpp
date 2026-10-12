#include "helpers/sampler_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <params/ViewRange.h>

using Catch::Matchers::WithinAbs;

TEST_CASE ("Editing from the controller", "[controller]")
{
    PluginProcessor plugin;
    plugin.setRateAndBufferSizeDetails (48000.0, 512);
    plugin.prepareToPlay (48000.0, 512);

    plugin.getSampleStore().setSample (makeRampSample (1600));
    plugin.getController().divideEqually();

    // Hits a pad and lets the message-thread side catch up, as the timer would
    const auto hitPad = [&plugin] (int pad) {
        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (1, noteForPad (plugin, pad), static_cast<juce::uint8> (100)), 0);
        processBlock (plugin, midi);
        plugin.getController().processPending();
    };

    const auto turn = [&plugin] (ControlTarget target, int value) {
        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::controllerEvent (1, ccForTarget (plugin, target), value), 0);
        processBlock (plugin, midi);
        plugin.getController().processPending();
    };

    SECTION ("loading spreads the pads across the sample")
    {
        CHECK_THAT (getParam (plugin, padParamID (3, PadParam::start)), WithinAbs (3.0 / 16.0, 1.0e-6));
        CHECK_THAT (getParam (plugin, padParamID (3, PadParam::end)), WithinAbs (4.0 / 16.0, 1.0e-6));
        CHECK (same (getParam (plugin, padParamID (3, PadParam::assigned)), 1.0f));
    }

    SECTION ("hitting a pad selects it and the mirror shows its values")
    {
        hitPad (3);

        CHECK (plugin.getController().getSelectedPad() == 3);
        CHECK (same (getParam (plugin, ParamIDs::selectedPad), 4.0f));
        CHECK_THAT (getParam (plugin, selectedPadParamID (PadParam::start)), WithinAbs (3.0 / 16.0, 1.0e-6));
        CHECK_THAT (getParam (plugin, selectedPadParamID (PadParam::end)), WithinAbs (4.0 / 16.0, 1.0e-6));
    }

    SECTION ("the start encoder moves only the selected pad's start")
    {
        hitPad (3);

        const auto before = getParam (plugin, padParamID (3, PadParam::start));
        const auto otherBefore = getParam (plugin, padParamID (4, PadParam::start));

        turn (ControlTarget::start, 5);

        const auto after = getParam (plugin, padParamID (3, PadParam::start));

        CHECK (after > before);
        CHECK (same (getParam (plugin, selectedPadParamID (PadParam::start)), after));
        CHECK (same (getParam (plugin, padParamID (4, PadParam::start)), otherBefore));

        turn (ControlTarget::start, 127); // one step back
        CHECK (getParam (plugin, padParamID (3, PadParam::start)) < after);
    }

    SECTION ("start can't pass end")
    {
        hitPad (3);

        for (int i = 0; i < 40; ++i)
            turn (ControlTarget::start, 63);

        CHECK (same (getParam (plugin, padParamID (3, PadParam::start)), getParam (plugin, padParamID (3, PadParam::end))));
    }

    SECTION ("changing a mirror parameter changes the selected pad")
    {
        hitPad (5);

        auto* mirrorGain = plugin.getValueTreeState().getParameter (selectedPadParamID (PadParam::gain));
        mirrorGain->setValueNotifyingHost (mirrorGain->convertTo0to1 (-12.0f));
        plugin.getController().processPending();

        CHECK_THAT (getParam (plugin, padParamID (5, PadParam::gain)), WithinAbs (-12.0, 0.05));
        CHECK_THAT (getParam (plugin, padParamID (4, PadParam::gain)), WithinAbs (0.0, 0.05));

        // Selecting another pad shows that pad's value, and leaves pad 6's alone
        hitPad (4);
        CHECK_THAT (getParam (plugin, selectedPadParamID (PadParam::gain)), WithinAbs (0.0, 0.05));
        CHECK_THAT (getParam (plugin, padParamID (5, PadParam::gain)), WithinAbs (-12.0, 0.05));
    }

    SECTION ("a toggle button flips the selected pad's setting")
    {
        hitPad (1);
        turn (ControlTarget::reverse, 127);

        CHECK (same (getParam (plugin, padParamID (1, PadParam::reverse)), 1.0f));
        CHECK (same (getParam (plugin, padParamID (0, PadParam::reverse)), 0.0f));
    }

    SECTION ("selecting a pad scrolls a zoomed view to its region")
    {
        auto* zoom = plugin.getValueTreeState().getParameter (ParamIDs::zoom);
        zoom->setValueNotifyingHost (zoom->convertTo0to1 (8.0f));

        hitPad (12);

        const auto view = getViewRange (getParam (plugin, ParamIDs::zoom), getParam (plugin, ParamIDs::viewX));
        const auto regionStart = static_cast<double> (getParam (plugin, padParamID (12, PadParam::start)));

        CHECK (regionStart >= view.start);
        CHECK (regionStart < view.getEnd());
    }
}

TEST_CASE ("Saving and restoring", "[state]")
{
    const auto sampleFile = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("melontester_state_test.wav");
    sampleFile.deleteFile();

    {
        const auto ramp = makeRampSample (1600);
        std::unique_ptr<juce::OutputStream> stream = sampleFile.createOutputStream();
        REQUIRE (stream != nullptr);

        juce::WavAudioFormat wav;
        const auto options = juce::AudioFormatWriterOptions().withSampleRate (48000.0).withNumChannels (1).withBitsPerSample (24);
        auto writer = wav.createWriterFor (stream, options);

        REQUIRE (writer != nullptr);
        REQUIRE (writer->writeFromAudioSampleBuffer (ramp->buffer, 0, 1600));
    }

    juce::MemoryBlock saved;

    {
        PluginProcessor original;
        REQUIRE (original.getController().loadSample (sampleFile));

        original.getController().selectPad (7);
        original.getController().adjust (ControlTarget::start, 20.0f);
        original.getController().adjust (ControlTarget::gain, -10.0f);
        original.getStateInformation (saved);

        PluginProcessor restored;
        restored.setStateInformation (saved.getData(), static_cast<int> (saved.getSize()));
        restored.getController().processPending();

        SECTION ("parameters come back")
        {
            for (const auto param : { PadParam::start, PadParam::end, PadParam::gain, PadParam::assigned })
            {
                CHECK_THAT (getParam (restored, padParamID (7, param)), WithinAbs (getParam (original, padParamID (7, param)), 1.0e-6));
                CHECK_THAT (getParam (restored, selectedPadParamID (param)), WithinAbs (getParam (original, padParamID (7, param)), 1.0e-6));
            }

            CHECK (restored.getController().getSelectedPad() == 7);
        }

        SECTION ("the sample is reloaded from its path without re-slicing")
        {
            REQUIRE (restored.getSampleStore().getSample() != nullptr);
            CHECK (restored.getSampleStore().getSample()->getNumSamples() == 1600);
            CHECK (restored.getSampleStore().getFile() == sampleFile);
            CHECK (getParam (restored, padParamID (7, PadParam::start)) > 7.0f / 16.0f);
        }
    }

    SECTION ("a sample that has moved is reported as missing")
    {
        sampleFile.deleteFile();

        PluginProcessor restored;
        restored.setStateInformation (saved.getData(), static_cast<int> (saved.getSize()));

        CHECK (restored.getSampleStore().getSample() == nullptr);
        CHECK (restored.getSampleStore().isMissing());
        CHECK (restored.getSampleStore().getFile() == sampleFile);
    }

    sampleFile.deleteFile();
}

TEST_CASE ("A restored state stays as the host set it", "[state]")
{
    // What pluginval's state restoration test does: write every parameter, save, restore, and expect
    // every value to be what was written. The plugin's timer runs in between, so the selected-pad
    // mirror gets the chance to change things.
    for (const auto seed : { 1, 2, 3, 4, 5 })
    {
        PluginProcessor plugin;
        juce::Random random (seed);

        for (auto* parameter : plugin.getParameters())
            parameter->setValueNotifyingHost (random.nextFloat());

        std::vector<float> expected;

        for (auto* parameter : plugin.getParameters())
            expected.push_back (parameter->getValue());

        juce::MemoryBlock saved;
        plugin.getStateInformation (saved);

        plugin.getController().processPending();
        plugin.setStateInformation (saved.getData(), static_cast<int> (saved.getSize()));
        plugin.getController().processPending();

        for (int i = 0; i < plugin.getParameters().size(); ++i)
        {
            INFO ("seed " << seed << ", " << plugin.getParameters()[i]->getName (64).toStdString());
            CHECK_THAT (plugin.getParameters()[i]->getValue(), WithinAbs (expected[static_cast<size_t> (i)], 1.0e-3));
        }
    }
}
