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
