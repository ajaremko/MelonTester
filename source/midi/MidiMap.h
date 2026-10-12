#pragma once

#include "control/Commands.h"
#include <juce_core/juce_core.h>

/* Which MIDI notes and CCs do what. Plain data, read from a JSON file:

   {
       "channel": 0,                             0 listens on every channel, 1-16 on one
       "relativeEncoding": "twosComplement",     or "signedBit", "binaryOffset"
       "padNotes": [36, 37, ...],                exactly 16 note numbers, pad 1 first
       "controls": [
           { "cc": 16, "target": "start", "mode": "relative" }
       ]
   }

   target is a pad setting (start, end, pitch, gain, attack, release, playMode, reverse,
   loop, chokeGroup, colour), a global setting (polyMode, zoom, viewX, sensitivity) or an
   action (divideEqually, detectTransients). Pad settings act on the selected pad.

   mode is relative (endless encoder), absolute (0-127 knob or fader), toggle or
   trigger (buttons, acting on press).
*/
struct MidiMap
{
    enum class Mode : juce::uint8
    {
        none,
        relative,
        absolute,
        toggle,
        trigger
    };

    // How an endless encoder reports a turn. For one step right / one step left:
    enum class RelativeEncoding : juce::uint8
    {
        twosComplement, // 1 / 127
        signedBit, // 1 / 65
        binaryOffset // 65 / 63
    };

    struct Control
    {
        Mode mode = Mode::none;
        ControlTarget target = ControlTarget::none;
    };

    MidiMap();

    // Returns nothing if the text isn't a valid map
    static std::optional<MidiMap> fromJson (const juce::String& json);

    // The map built into the plugin, from assets/midi/default_map.json
    static MidiMap createDefault();

    // A map at this location replaces the built-in one
    static juce::File getUserMapFile();

    // The user's map if there is a valid one, otherwise the default
    static MidiMap load();

    int channel = 0;
    RelativeEncoding relativeEncoding = RelativeEncoding::twosComplement;
    std::array<juce::int8, 128> noteToPad; // -1 where a note isn't a pad
    std::array<Control, 128> controls {};
};
