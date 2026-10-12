#pragma once

#include "SampleData.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_events/juce_events.h>

/* Holds the one loaded sample and hands it to the audio thread.

   Everything is message thread only, except getSampleForAudio(). Sends a change
   message when the sample changes.
*/
class SampleStore : public juce::ChangeBroadcaster
{
public:
    SampleStore();

    // Reads the whole file into memory. Returns false and keeps the current sample if it can't be read.
    bool loadFile (const juce::File& file);

    // For a saved project whose sample has moved: remembers the path with no audio loaded
    void setMissing (const juce::File& file);

    void setSample (SampleData::Ptr newSample, const juce::File& sourceFile = {});
    void clear();

    SampleData::Ptr getSample() const { return current; }
    juce::File getFile() const { return file; }
    bool isMissing() const { return current == nullptr && file != juce::File(); }

    juce::String getSupportedWildcard() const { return formatManager.getWildcardForAllFormats(); }

    // Audio thread: call once per block. The returned sample stays valid until the next call.
    SampleData* getSampleForAudio() noexcept;

    // Frees a sample the audio thread has stopped using
    void releaseRetired();

private:
    juce::AudioFormatManager formatManager;

    SampleData::Ptr current;
    juce::File file;

    // pending holds the next sample on its way to the audio thread, then the old one on its way back
    juce::SpinLock handoffLock;
    SampleData::Ptr pending;
    SampleData::Ptr audioSample;
    std::atomic<bool> hasPending { false };
    std::atomic<bool> hasRetired { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleStore)
};
