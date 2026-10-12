#pragma once

#include "ParamIDs.h"
#include <juce_audio_processors/juce_audio_processors.h>

/* Keeps the "selected pad" parameters and the selected pad's own parameters equal.

   Selecting another pad copies that pad's values into the mirror set. A change to a
   mirror parameter is written to the pad, and a change to the pad is shown in the mirror.
   The UI and the controller bind to the mirror set, so they never re-bind on pad change.

   Message thread only.
*/
class SelectedPadMirror
{
public:
    explicit SelectedPadMirror (juce::AudioProcessorValueTreeState& apvts);

    void sync();

    // Takes the parameters as they are now as the starting point, without changing any of them.
    // For after a saved state is restored: the host expects to get back exactly what it set.
    void adoptCurrentValues();

private:
    int getSelectedPad() const;

    std::array<std::array<juce::RangedAudioParameter*, kNumPadParams>, kNumPads> padParams {};
    std::array<juce::RangedAudioParameter*, kNumPadParams> mirrorParams {};
    std::atomic<float>* selectedPad = nullptr;

    int lastSelected = -1;
    std::array<float, kNumPadParams> lastMirrorValues {};
    std::array<float, kNumPadParams> lastPadValues {};
};
