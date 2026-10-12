#pragma once

#include "MidiMap.h"
#include <juce_audio_basics/juce_audio_basics.h>

// What one MIDI message means for the plugin.
struct RoutedEvent
{
    enum class Kind
    {
        none,
        padOn, // pad, velocity
        padOff, // pad
        edit // command, to be applied on the message thread
    };

    Kind kind = Kind::none;
    int pad = 0;
    float velocity = 0.0f;
    EditCommand command;
};

// Translates MIDI using a MidiMap. Does nothing itself, so it is safe on the audio thread.
class MidiRouter
{
public:
    explicit MidiRouter (const MidiMap& mapToUse) : map (mapToUse) {}

    RoutedEvent route (const juce::MidiMessage& message) const noexcept;

    const MidiMap& getMap() const noexcept { return map; }

private:
    int decodeRelative (int value) const noexcept;

    MidiMap map;
};
