#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// Every host parameter: the global settings, one set per pad, and the selected-pad mirror set.
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
