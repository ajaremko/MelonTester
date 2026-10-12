#pragma once

#include "control/Commands.h"
#include "control/EditController.h"
#include "engine/SamplerEngine.h"
#include "midi/MidiRouter.h"
#include "params/PadParams.h"
#include "sample/SampleStore.h"
#include <juce_audio_processors/juce_audio_processors.h>

class PluginProcessor : public juce::AudioProcessor
{
public:
    PluginProcessor();
    ~PluginProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState& getValueTreeState() { return apvts; }
    const ParamRefs& getParams() const { return params; }
    SampleStore& getSampleStore() { return sampleStore; }
    EditController& getController() { return controller; }
    const SamplerEngine& getEngine() const { return engine; }
    const MidiMap& getMidiMap() const { return router.getMap(); }

    // Plays a pad from the editor. Message thread.
    void auditionPad (int padIndex, bool isDown);

private:
    void handleMidiEvent (const RoutedEvent& event);
    void handleAudition (const EditCommand& command);

    juce::AudioProcessorValueTreeState apvts;
    ParamRefs params;
    SampleStore sampleStore;

    CommandQueue editQueue; // audio thread -> message thread
    CommandQueue auditionQueue; // message thread -> audio thread

    MidiRouter router;
    SamplerEngine engine;
    EditController controller;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};
