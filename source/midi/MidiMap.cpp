#include "MidiMap.h"
#include "BinaryData.h"

namespace
{
    std::optional<int> readInt (const juce::var& value, int min, int max)
    {
        if (! (value.isInt() || value.isInt64()))
            return {};

        const auto number = static_cast<int> (value);

        if (number < min || number > max)
            return {};

        return number;
    }

    std::optional<ControlTarget> targetFromName (const juce::String& name)
    {
        for (const auto param : allPadParams)
            if (name == padParamNames[toIndex (param)])
                return static_cast<ControlTarget> (param);

        if (name == "polyMode")         return ControlTarget::polyMode;
        if (name == "zoom")             return ControlTarget::zoom;
        if (name == "viewX")            return ControlTarget::viewX;
        if (name == "sensitivity")      return ControlTarget::sensitivity;
        if (name == "divideEqually")    return ControlTarget::divideEqually;
        if (name == "detectTransients") return ControlTarget::detectTransients;

        return {};
    }

    std::optional<MidiMap::Mode> modeFromName (const juce::String& name)
    {
        if (name == "relative") return MidiMap::Mode::relative;
        if (name == "absolute") return MidiMap::Mode::absolute;
        if (name == "toggle")   return MidiMap::Mode::toggle;
        if (name == "trigger")  return MidiMap::Mode::trigger;

        return {};
    }

    std::optional<MidiMap::RelativeEncoding> encodingFromName (const juce::String& name)
    {
        if (name == "twosComplement") return MidiMap::RelativeEncoding::twosComplement;
        if (name == "signedBit")      return MidiMap::RelativeEncoding::signedBit;
        if (name == "binaryOffset")   return MidiMap::RelativeEncoding::binaryOffset;

        return {};
    }
}

MidiMap::MidiMap()
{
    noteToPad.fill (-1);
}

std::optional<MidiMap> MidiMap::fromJson (const juce::String& json)
{
    juce::var root;

    if (juce::JSON::parse (json, root).failed() || ! root.isObject())
        return {};

    MidiMap map;

    if (root.hasProperty ("channel"))
    {
        const auto channelNumber = readInt (root["channel"], 0, 16);

        if (! channelNumber)
            return {};

        map.channel = *channelNumber;
    }

    if (root.hasProperty ("relativeEncoding"))
    {
        const auto encoding = encodingFromName (root["relativeEncoding"].toString());

        if (! encoding)
            return {};

        map.relativeEncoding = *encoding;
    }

    const auto* padNotes = root["padNotes"].getArray();

    if (padNotes == nullptr || padNotes->size() != kNumPads)
        return {};

    for (int pad = 0; pad < kNumPads; ++pad)
    {
        const auto note = readInt (padNotes->getReference (pad), 0, 127);

        if (! note)
            return {};

        map.noteToPad[static_cast<size_t> (*note)] = static_cast<juce::int8> (pad);
    }

    if (const auto* controlList = root["controls"].getArray())
    {
        for (const auto& entry : *controlList)
        {
            const auto cc = readInt (entry["cc"], 0, 127);
            const auto target = targetFromName (entry["target"].toString());
            const auto mode = modeFromName (entry["mode"].toString());

            if (! cc || ! target || ! mode)
                return {};

            map.controls[static_cast<size_t> (*cc)] = { *mode, *target };
        }
    }

    return map;
}

MidiMap MidiMap::createDefault()
{
    const auto json = juce::String::fromUTF8 (BinaryData::default_map_json, BinaryData::default_map_jsonSize);

    if (auto map = fromJson (json))
        return *map;

    jassertfalse; // assets/midi/default_map.json is broken
    return {};
}

juce::File MidiMap::getUserMapFile()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("MelonTester")
        .getChildFile ("midi_map.json");
}

MidiMap MidiMap::load()
{
    const auto userFile = getUserMapFile();

    if (userFile.existsAsFile())
        if (auto map = fromJson (userFile.loadFileAsString()))
            return *map;

    return createDefault();
}
