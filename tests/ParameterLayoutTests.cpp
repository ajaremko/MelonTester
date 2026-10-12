#include <PluginProcessor.h>
#include <catch2/catch_test_macros.hpp>
#include <set>

TEST_CASE ("Parameter layout", "[params]")
{
    PluginProcessor plugin;
    auto& state = plugin.getValueTreeState();

    SECTION ("has the global set, the mirror set and one set per pad")
    {
        const auto numGlobal = 5;
        CHECK (plugin.getParameters().size() == numGlobal + kNumPadParams * (kNumPads + 1));
    }

    SECTION ("every parameter ID is unique")
    {
        std::set<juce::String> ids;

        for (auto* param : plugin.getParameters())
            if (auto* withID = dynamic_cast<juce::AudioProcessorParameterWithID*> (param))
                ids.insert (withID->paramID);

        CHECK (static_cast<int> (ids.size()) == plugin.getParameters().size());
    }

    SECTION ("the mirror set matches a pad's set")
    {
        for (const auto param : allPadParams)
        {
            auto* mirror = state.getParameter (selectedPadParamID (param));
            auto* pad = state.getParameter (padParamID (0, param));

            REQUIRE (mirror != nullptr);
            REQUIRE (pad != nullptr);
            CHECK (mirror->getNumSteps() == pad->getNumSteps());
            CHECK (mirror->isDiscrete() == pad->isDiscrete());
        }
    }

    SECTION ("pad IDs are stable")
    {
        CHECK (padParamID (0, PadParam::start) == "pad01_start");
        CHECK (padParamID (15, PadParam::chokeGroup) == "pad16_chokeGroup");
        CHECK (selectedPadParamID (PadParam::end) == "sel_end");
    }
}

TEST_CASE ("Switch parameters only hold on or off", "[params]")
{
    PluginProcessor plugin;
    auto& state = plugin.getValueTreeState();

    for (const auto param : { PadParam::reverse, PadParam::loop, PadParam::assigned })
    {
        for (const auto& id : { padParamID (0, param), selectedPadParamID (param) })
        {
            auto* parameter = state.getParameter (id);
            REQUIRE (parameter != nullptr);

            // A host can write any value from 0 to 1, but a saved project only records on or off,
            // so an in-between value could never be restored
            parameter->setValueNotifyingHost (0.38f);
            CHECK (juce::exactlyEqual (parameter->getValue(), 0.0f));

            parameter->setValueNotifyingHost (0.7f);
            CHECK (juce::exactlyEqual (parameter->getValue(), 1.0f));
        }
    }
}
