#include "MidiRouter.h"

RoutedEvent MidiRouter::route (const juce::MidiMessage& message) const noexcept
{
    RoutedEvent event;

    if (map.channel != 0 && message.getChannel() != map.channel)
        return event;

    if (message.isNoteOnOrOff())
    {
        const auto pad = map.noteToPad[static_cast<size_t> (message.getNoteNumber())];

        if (pad < 0)
            return event;

        event.kind = message.isNoteOn() ? RoutedEvent::Kind::padOn : RoutedEvent::Kind::padOff;
        event.pad = pad;
        event.velocity = message.getFloatVelocity();
        return event;
    }

    if (! message.isController())
        return event;

    const auto& control = map.controls[static_cast<size_t> (message.getControllerNumber())];
    const auto value = message.getControllerValue();
    const auto isPress = value >= 64;

    switch (control.mode)
    {
        case MidiMap::Mode::relative:
            if (const auto steps = decodeRelative (value); steps != 0)
            {
                event.kind = RoutedEvent::Kind::edit;
                event.command = EditCommand::forTarget (EditCommand::Type::adjust, control.target, static_cast<float> (steps));
            }
            break;
        case MidiMap::Mode::absolute:
            event.kind = RoutedEvent::Kind::edit;
            event.command = EditCommand::forTarget (EditCommand::Type::setNormalised, control.target, static_cast<float> (value) / 127.0f);
            break;
        case MidiMap::Mode::toggle:
            if (isPress)
            {
                event.kind = RoutedEvent::Kind::edit;
                event.command = EditCommand::forTarget (EditCommand::Type::toggle, control.target);
            }
            break;
        case MidiMap::Mode::trigger:
            if (isPress)
            {
                event.kind = RoutedEvent::Kind::edit;
                event.command = EditCommand::forTarget (EditCommand::Type::trigger, control.target);
            }
            break;
        case MidiMap::Mode::none:
            break;
    }

    return event;
}

int MidiRouter::decodeRelative (int value) const noexcept
{
    switch (map.relativeEncoding)
    {
        case MidiMap::RelativeEncoding::twosComplement:
            return value < 64 ? value : value - 128;
        case MidiMap::RelativeEncoding::signedBit:
            return value < 64 ? value : -(value - 64);
        case MidiMap::RelativeEncoding::binaryOffset:
            return value - 64;
    }

    return 0;
}
