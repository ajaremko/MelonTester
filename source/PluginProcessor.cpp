#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "params/ParameterLayout.h"

namespace
{
    const juce::Identifier samplePathProperty { "samplePath" };
}

//==============================================================================
PluginProcessor::PluginProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MelonTester", createParameterLayout()),
      params (apvts),
      router (MidiMap::load()),
      engine (params, sampleStore),
      controller (apvts, params, sampleStore, editQueue)
{
}

PluginProcessor::~PluginProcessor()
{
}

//==============================================================================
const juce::String PluginProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PluginProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PluginProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PluginProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int PluginProcessor::getCurrentProgram()
{
    return 0;
}

void PluginProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String PluginProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    // Steinberg's VST3 validator fails plugins whose single default program has no name
    return "Default";
}

void PluginProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    engine.prepare (sampleRate);
}

void PluginProcessor::releaseResources()
{
}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const auto numSamples = buffer.getNumSamples();
    buffer.clear();

    engine.beginBlock();
    auditionQueue.drain ([this] (const EditCommand& command) { handleAudition (command); });

    // Render up to each MIDI message before acting on it, so pad hits start on the right sample
    int position = 0;

    for (const auto metadata : midiMessages)
    {
        const auto eventPosition = juce::jlimit (position, numSamples, metadata.samplePosition);

        engine.render (buffer, position, eventPosition - position);
        position = eventPosition;

        handleMidiEvent (router.route (metadata.getMessage()));
    }

    engine.render (buffer, position, numSamples - position);
}

void PluginProcessor::handleMidiEvent (const RoutedEvent& event)
{
    switch (event.kind)
    {
        case RoutedEvent::Kind::padOn:
            engine.triggerPad (event.pad, event.velocity);
            editQueue.push (EditCommand::forPad (EditCommand::Type::selectPad, event.pad));
            break;
        case RoutedEvent::Kind::padOff:
            engine.releasePad (event.pad);
            break;
        case RoutedEvent::Kind::edit:
            editQueue.push (event.command);
            break;
        case RoutedEvent::Kind::none:
            break;
    }
}

void PluginProcessor::handleAudition (const EditCommand& command)
{
    if (command.type == EditCommand::Type::padOn)
        engine.triggerPad (command.pad, command.value);
    else if (command.type == EditCommand::Type::padOff)
        engine.releasePad (command.pad);
}

void PluginProcessor::auditionPad (int padIndex, bool isDown)
{
    const auto type = isDown ? EditCommand::Type::padOn : EditCommand::Type::padOff;
    auditionQueue.push (EditCommand::forPad (type, padIndex, 1.0f));
}

//==============================================================================
bool PluginProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor (*this);
}

//==============================================================================
void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty (samplePathProperty, sampleStore.getFile().getFullPathName(), nullptr);

    if (const auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    const auto xml = getXmlFromBinary (data, sizeInBytes);

    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;

    const auto state = juce::ValueTree::fromXml (*xml);
    const auto samplePath = state.getProperty (samplePathProperty).toString();

    apvts.replaceState (state);
    controller.stateWasRestored();

    // The pad regions come back with the parameters, so the sample is loaded without re-slicing
    if (! juce::File::isAbsolutePath (samplePath))
    {
        sampleStore.clear();
        return;
    }

    const juce::File sampleFile (samplePath);

    if (! sampleStore.loadFile (sampleFile))
        sampleStore.setMissing (sampleFile);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
