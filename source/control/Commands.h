#pragma once

#include "params/ParamIDs.h"
#include <juce_core/juce_core.h>

// Everything a controller or the editor can act on.
enum class ControlTarget : juce::uint8
{
    // Settings of the selected pad, in PadParam order
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
    assigned,

    // Global settings
    polyMode,
    zoom,
    viewX,
    sensitivity,

    // Actions
    divideEqually,
    detectTransients,

    none
};

constexpr bool isPadTarget (ControlTarget target) noexcept
{
    return static_cast<int> (target) < kNumPadParams;
}

constexpr PadParam toPadParam (ControlTarget target) noexcept
{
    return static_cast<PadParam> (target);
}

// A small message passed between the audio thread and the message thread.
struct EditCommand
{
    enum class Type : juce::uint8
    {
        selectPad, // pad
        adjust, // target, value = encoder steps (signed)
        setNormalised, // target, value = 0-1
        toggle, // target
        trigger, // target is an action
        padOn, // pad, value = velocity. Editor to audio thread only.
        padOff // pad. Editor to audio thread only.
    };

    Type type = Type::trigger;
    ControlTarget target = ControlTarget::none;
    int pad = 0;
    float value = 0.0f;

    static EditCommand forPad (Type commandType, int padIndex, float commandValue = 0.0f) noexcept
    {
        return { commandType, ControlTarget::none, padIndex, commandValue };
    }

    static EditCommand forTarget (Type commandType, ControlTarget commandTarget, float commandValue = 0.0f) noexcept
    {
        return { commandType, commandTarget, 0, commandValue };
    }
};

// Fixed-size lock-free queue for one writing thread and one reading thread.
class CommandQueue
{
public:
    // Returns false and drops the command if the queue is full
    bool push (const EditCommand& command) noexcept
    {
        const auto scope = fifo.write (1);

        if (scope.blockSize1 > 0)
            buffer[static_cast<size_t> (scope.startIndex1)] = command;
        else if (scope.blockSize2 > 0)
            buffer[static_cast<size_t> (scope.startIndex2)] = command;
        else
            return false;

        return true;
    }

    template <typename Callback>
    void drain (Callback&& callback)
    {
        const auto scope = fifo.read (fifo.getNumReady());
        scope.forEach ([&] (int index) { callback (buffer[static_cast<size_t> (index)]); });
    }

private:
    static constexpr int capacity = 256;

    juce::AbstractFifo fifo { capacity };
    std::array<EditCommand, capacity> buffer {};
};
