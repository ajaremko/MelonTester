#include "SampleStore.h"

SampleStore::SampleStore()
{
    formatManager.registerBasicFormats();
}

bool SampleStore::loadFile (const juce::File& fileToLoad)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (fileToLoad));

    if (reader == nullptr || reader->lengthInSamples <= 0 || reader->lengthInSamples > std::numeric_limits<int>::max())
        return false;

    const auto numSamples = static_cast<int> (reader->lengthInSamples);
    const auto numChannels = static_cast<int> (juce::jmin (reader->numChannels, 2u));

    SampleData::Ptr sample = new SampleData();
    sample->sampleRate = reader->sampleRate;
    sample->buffer.setSize (numChannels, numSamples);

    if (! reader->read (&sample->buffer, 0, numSamples, 0, true, true))
        return false;

    setSample (sample, fileToLoad);
    return true;
}

void SampleStore::setMissing (const juce::File& missingFile)
{
    setSample (nullptr, missingFile);
}

void SampleStore::clear()
{
    setSample (nullptr);
}

void SampleStore::setSample (SampleData::Ptr newSample, const juce::File& sourceFile)
{
    auto previouslyPending = newSample;

    {
        const juce::SpinLock::ScopedLockType lock (handoffLock);
        std::swap (pending, previouslyPending);
        hasPending = true;
    }

    current = newSample;
    file = sourceFile;
    sendChangeMessage();
}

SampleData* SampleStore::getSampleForAudio() noexcept
{
    if (hasPending.load())
    {
        const juce::SpinLock::ScopedTryLockType lock (handoffLock);

        if (lock.isLocked())
        {
            std::swap (audioSample, pending);
            hasPending = false;
            hasRetired = true;
        }
    }

    return audioSample.get();
}

void SampleStore::releaseRetired()
{
    if (! hasRetired.load())
        return;

    SampleData::Ptr retired;

    {
        const juce::SpinLock::ScopedLockType lock (handoffLock);

        if (! hasPending.load())
        {
            std::swap (retired, pending);
            hasRetired = false;
        }
    }
}
