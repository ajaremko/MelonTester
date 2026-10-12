#pragma once

#include "Commands.h"
#include "params/PadParams.h"
#include "params/SelectedPadMirror.h"
#include "sample/SampleStore.h"
#include "slicing/Slicer.h"
#include <juce_audio_processors/juce_audio_processors.h>

/* The one place that changes plugin state. Message thread only.

   Edits from the MIDI controller arrive through the command queue; the editor calls
   the same functions directly, so both take the same path.
*/
class EditController : private juce::Timer
{
public:
    EditController (juce::AudioProcessorValueTreeState& apvts, const ParamRefs& params, SampleStore& sampleStore, CommandQueue& incoming);
    ~EditController() override;

    // Applies queued commands and keeps the selected-pad mirror in sync. Runs on a timer.
    void processPending();

    void apply (const EditCommand& command);

    // Call after a saved state has been restored, from any thread. The restored values are then
    // kept as they are, not treated as edits to react to.
    void stateWasRestored() noexcept { stateRestored = true; }

    void selectPad (int padIndex);
    int getSelectedPad() const { return params.getSelectedPad(); }

    void adjust (ControlTarget target, float steps);
    void setNormalised (ControlTarget target, float normalised);
    void toggle (ControlTarget target);

    // Loads a file chosen by the user and spreads the pads evenly across it
    bool loadSample (const juce::File& file);

    void divideEqually();
    void detectTransients();

private:
    void timerCallback() override;

    juce::RangedAudioParameter* getParameter (ControlTarget target) const;
    juce::RangedAudioParameter& getPadParameter (int padIndex, PadParam param) const;

    void adjustRegionEdge (ControlTarget edge, float steps);
    void assignSlices (const std::vector<SliceRegion>& slices, juce::int64 totalSamples);
    void scrollToSelectedPad();

    juce::AudioProcessorValueTreeState& apvts;
    const ParamRefs& params;
    SampleStore& sampleStore;
    CommandQueue& incoming;
    SelectedPadMirror mirror;
    std::atomic<bool> stateRestored { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EditController)
};
