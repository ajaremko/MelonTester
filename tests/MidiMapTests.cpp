#include <catch2/catch_test_macros.hpp>
#include <midi/MidiMap.h>

TEST_CASE ("Default MIDI map", "[midi]")
{
    const auto map = MidiMap::createDefault();

    SECTION ("maps 16 notes to the pads in order")
    {
        int numPadNotes = 0;

        for (const auto pad : map.noteToPad)
            if (pad >= 0)
                ++numPadNotes;

        CHECK (numPadNotes == kNumPads);
        CHECK (map.noteToPad[36] == 0);
        CHECK (map.noteToPad[51] == 15);
        CHECK (map.noteToPad[35] == -1);
    }

    SECTION ("maps start and end to relative encoders")
    {
        CHECK (map.controls[16].target == ControlTarget::start);
        CHECK (map.controls[16].mode == MidiMap::Mode::relative);
        CHECK (map.controls[17].target == ControlTarget::end);
        CHECK (map.controls[17].mode == MidiMap::Mode::relative);
    }
}

TEST_CASE ("MIDI map from JSON", "[midi]")
{
    SECTION ("reads a custom map")
    {
        const auto map = MidiMap::fromJson (R"({
            "channel": 3,
            "relativeEncoding": "binaryOffset",
            "padNotes": [60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75],
            "controls": [ { "cc": 7, "target": "gain", "mode": "absolute" } ]
        })");

        REQUIRE (map.has_value());
        CHECK (map->channel == 3);
        CHECK (map->relativeEncoding == MidiMap::RelativeEncoding::binaryOffset);
        CHECK (map->noteToPad[60] == 0);
        CHECK (map->noteToPad[36] == -1);
        CHECK (map->controls[7].target == ControlTarget::gain);
        CHECK (map->controls[7].mode == MidiMap::Mode::absolute);
    }

    SECTION ("rejects text that isn't JSON")
    {
        CHECK_FALSE (MidiMap::fromJson ("not json").has_value());
    }

    SECTION ("rejects a map without 16 pad notes")
    {
        CHECK_FALSE (MidiMap::fromJson (R"({ "padNotes": [36, 37, 38] })").has_value());
        CHECK_FALSE (MidiMap::fromJson (R"({ "controls": [] })").has_value());
    }

    SECTION ("rejects an unknown target")
    {
        const auto map = MidiMap::fromJson (R"({
            "padNotes": [36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51],
            "controls": [ { "cc": 7, "target": "wobble", "mode": "absolute" } ]
        })");

        CHECK_FALSE (map.has_value());
    }
}
