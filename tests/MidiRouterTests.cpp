#include <catch2/catch_test_macros.hpp>
#include <midi/MidiRouter.h>

TEST_CASE ("MIDI routing with the default map", "[midi]")
{
    const MidiRouter router (MidiMap::createDefault());

    SECTION ("a pad note becomes a pad hit")
    {
        const auto event = router.route (juce::MidiMessage::noteOn (1, 38, static_cast<juce::uint8> (127)));

        CHECK (event.kind == RoutedEvent::Kind::padOn);
        CHECK (event.pad == 2);
        CHECK (event.velocity > 0.99f);
    }

    SECTION ("releasing a pad note becomes a pad release")
    {
        const auto event = router.route (juce::MidiMessage::noteOff (1, 38));

        CHECK (event.kind == RoutedEvent::Kind::padOff);
        CHECK (event.pad == 2);
    }

    SECTION ("other notes are ignored")
    {
        CHECK (router.route (juce::MidiMessage::noteOn (1, 20, static_cast<juce::uint8> (100))).kind == RoutedEvent::Kind::none);
    }

    SECTION ("an encoder turn becomes a signed adjustment")
    {
        const auto right = router.route (juce::MidiMessage::controllerEvent (1, 16, 2));

        CHECK (right.kind == RoutedEvent::Kind::edit);
        CHECK (right.command.type == EditCommand::Type::adjust);
        CHECK (right.command.target == ControlTarget::start);
        CHECK (juce::exactlyEqual (right.command.value, 2.0f));

        const auto left = router.route (juce::MidiMessage::controllerEvent (1, 16, 127));

        CHECK (left.command.type == EditCommand::Type::adjust);
        CHECK (juce::exactlyEqual (left.command.value, -1.0f));
    }

    SECTION ("a button acts on press, not release")
    {
        const auto press = router.route (juce::MidiMessage::controllerEvent (1, 28, 127));

        CHECK (press.kind == RoutedEvent::Kind::edit);
        CHECK (press.command.type == EditCommand::Type::trigger);
        CHECK (press.command.target == ControlTarget::divideEqually);

        CHECK (router.route (juce::MidiMessage::controllerEvent (1, 28, 0)).kind == RoutedEvent::Kind::none);
    }

    SECTION ("unmapped controllers are ignored")
    {
        CHECK (router.route (juce::MidiMessage::controllerEvent (1, 100, 64)).kind == RoutedEvent::Kind::none);
    }
}

TEST_CASE ("MIDI routing honours the map's channel", "[midi]")
{
    auto map = MidiMap::createDefault();
    map.channel = 2;

    const MidiRouter router (map);

    CHECK (router.route (juce::MidiMessage::noteOn (1, 36, static_cast<juce::uint8> (100))).kind == RoutedEvent::Kind::none);
    CHECK (router.route (juce::MidiMessage::noteOn (2, 36, static_cast<juce::uint8> (100))).kind == RoutedEvent::Kind::padOn);
}
