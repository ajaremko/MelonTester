#pragma once

#include <array>
#include <juce_core/juce_core.h>

inline constexpr int kNumPads = 16;
inline constexpr int kNumChokeGroups = 8;
inline constexpr int kNumPadColours = 16;

// Parameter IDs are written into saved projects, so they must never change once released.
namespace ParamIDs
{
    inline constexpr auto polyMode = "polyMode";
    inline constexpr auto zoom = "zoom";
    inline constexpr auto viewX = "viewX";
    inline constexpr auto sensitivity = "sensitivity";
    inline constexpr auto selectedPad = "selectedPad";
}

// The settings every pad has. The "selected pad" mirror set repeats this list once.
enum class PadParam
{
    start,
    end,
    pitch,
    gain,
    attack,
    release,
    playMode,
    reverse,
    loop,
    chokeGroup,
    colour,
    assigned
};

inline constexpr int kNumPadParams = 12;

inline constexpr std::array<const char*, kNumPadParams> padParamNames {
    "start", "end", "pitch", "gain", "attack", "release", "playMode", "reverse", "loop", "chokeGroup", "colour", "assigned"
};

inline constexpr std::array<PadParam, kNumPadParams> allPadParams {
    PadParam::start, PadParam::end, PadParam::pitch, PadParam::gain, PadParam::attack, PadParam::release, PadParam::playMode, PadParam::reverse, PadParam::loop, PadParam::chokeGroup, PadParam::colour, PadParam::assigned
};

constexpr size_t toIndex (PadParam param) noexcept { return static_cast<size_t> (param); }

// padIndex is zero-based: pad 0 gives "pad01_start"
inline juce::String padParamID (int padIndex, PadParam param)
{
    return "pad" + juce::String (padIndex + 1).paddedLeft ('0', 2) + "_" + padParamNames[toIndex (param)];
}

inline juce::String selectedPadParamID (PadParam param)
{
    return juce::String ("sel_") + padParamNames[toIndex (param)];
}
