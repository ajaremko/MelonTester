#include "WaveformPanel.h"

WaveformPanel::WaveformPanel (PluginProcessor& processorToUse)
    : processor (processorToUse),
      zoom (processorToUse.getValueTreeState(), ParamIDs::zoom, "Zoom"),
      viewX (processorToUse.getValueTreeState(), ParamIDs::viewX, "View"),
      start (processorToUse.getValueTreeState(), selectedPadParamID (PadParam::start), "Start"),
      end (processorToUse.getValueTreeState(), selectedPadParamID (PadParam::end), "End")
{
    addAndMakeVisible (zoom);
    addAndMakeVisible (viewX);
    addAndMakeVisible (start);
    addAndMakeVisible (end);

    processor.getSampleStore().addChangeListener (this);
    startTimerHz (30);
}

WaveformPanel::~WaveformPanel()
{
    processor.getSampleStore().removeChangeListener (this);
}

void WaveformPanel::resized()
{
    auto area = getLocalBounds().reduced (8);
    auto knobs = area.removeFromBottom (84);
    const auto knobWidth = knobs.getWidth() / 4;

    zoom.setBounds (knobs.removeFromLeft (knobWidth));
    viewX.setBounds (knobs.removeFromLeft (knobWidth));
    start.setBounds (knobs.removeFromLeft (knobWidth));
    end.setBounds (knobs);

    waveformArea = area.withTrimmedBottom (4);
    peaksSample = nullptr;
}

//==============================================================================
std::array<float, 5> WaveformPanel::getDrawState() const
{
    const auto& params = processor.getParams();
    const auto selected = params.getSelectedPad();

    return { params.zoom->load(), params.viewX->load(), static_cast<float> (selected), params.pad (selected).get (PadParam::start), params.pad (selected).get (PadParam::end) };
}

void WaveformPanel::timerCallback()
{
    const auto drawState = getDrawState();

    if (drawState != lastDrawState)
    {
        lastDrawState = drawState;
        repaint();
    }
}

void WaveformPanel::changeListenerCallback (juce::ChangeBroadcaster*)
{
    peaksSample = nullptr;
    repaint();
}

//==============================================================================
void WaveformPanel::paint (juce::Graphics& g)
{
    fillPanelBackground (g, getLocalBounds());

    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillRect (waveformArea);

    auto& sampleStore = processor.getSampleStore();
    const auto sample = sampleStore.getSample();

    if (sample == nullptr)
    {
        const auto message = sampleStore.isMissing() ? "Missing sample: " + sampleStore.getFile().getFullPathName()
                                                     : juce::String ("Drop a sample here");

        g.setColour (juce::Colours::white.withAlpha (0.6f));
        g.drawFittedText (message, waveformArea.reduced (8), juce::Justification::centred, 2);
        return;
    }

    const auto& params = processor.getParams();
    const auto view = getViewRange (params.zoom->load(), params.viewX->load());

    paintWaveform (g, *sample, view);
    paintRegions (g, view);
}

void WaveformPanel::rebuildPeaks (const SampleData& sample, ViewRange view)
{
    const auto numColumns = juce::jmax (0, waveformArea.getWidth());
    const auto numSamples = sample.getNumSamples();
    const auto samplesPerColumn = view.length * numSamples / juce::jmax (1, numColumns);

    peaks.assign (static_cast<size_t> (numColumns), {});

    for (int column = 0; column < numColumns; ++column)
    {
        const auto first = juce::jlimit (0, numSamples - 1, static_cast<int> (view.start * numSamples + column * samplesPerColumn));
        const auto count = juce::jlimit (1, numSamples - first, static_cast<int> (std::ceil (samplesPerColumn)));

        auto range = sample.buffer.findMinMax (0, first, count);

        for (int channel = 1; channel < sample.buffer.getNumChannels(); ++channel)
            range = range.getUnionWith (sample.buffer.findMinMax (channel, first, count));

        peaks[static_cast<size_t> (column)] = range;
    }

    peaksSample = &sample;
    peaksView = view;
}

void WaveformPanel::paintWaveform (juce::Graphics& g, const SampleData& sample, ViewRange view)
{
    const auto viewChanged = ! juce::exactlyEqual (view.start, peaksView.start) || ! juce::exactlyEqual (view.length, peaksView.length);

    if (peaksSample != &sample || viewChanged || static_cast<int> (peaks.size()) != waveformArea.getWidth())
        rebuildPeaks (sample, view);

    const auto centreY = static_cast<float> (waveformArea.getCentreY());
    const auto halfHeight = static_cast<float> (waveformArea.getHeight()) * 0.5f;

    g.setColour (juce::Colours::lightgrey);

    for (size_t column = 0; column < peaks.size(); ++column)
    {
        const auto x = static_cast<float> (waveformArea.getX()) + static_cast<float> (column);
        const auto top = centreY - juce::jlimit (-1.0f, 1.0f, peaks[column].getEnd()) * halfHeight;
        const auto bottom = centreY - juce::jlimit (-1.0f, 1.0f, peaks[column].getStart()) * halfHeight;

        g.drawVerticalLine (juce::roundToInt (x), top, juce::jmax (bottom, top + 1.0f));
    }
}

void WaveformPanel::paintRegions (juce::Graphics& g, ViewRange view)
{
    const auto& params = processor.getParams();
    const auto selected = params.getSelectedPad();

    const auto toX = [this, view] (float fraction) {
        return static_cast<float> (waveformArea.getX()) + static_cast<float> ((static_cast<double> (fraction) - view.start) / view.length * waveformArea.getWidth());
    };

    g.saveState();
    g.reduceClipRegion (waveformArea);

    // Faint markers where the other pads start
    g.setColour (juce::Colours::white.withAlpha (0.25f));

    for (int pad = 0; pad < kNumPads; ++pad)
        if (pad != selected && params.pad (pad).get (PadParam::assigned) >= 0.5f)
            g.fillRect (toX (params.pad (pad).get (PadParam::start)), static_cast<float> (waveformArea.getY()), 1.0f, static_cast<float> (waveformArea.getHeight()));

    const auto startX = toX (params.pad (selected).get (PadParam::start));
    const auto endX = toX (params.pad (selected).get (PadParam::end));
    const auto region = juce::Rectangle<float> (startX, static_cast<float> (waveformArea.getY()), juce::jmax (1.0f, endX - startX), static_cast<float> (waveformArea.getHeight()));

    g.setColour (juce::Colours::orange.withAlpha (0.3f));
    g.fillRect (region);
    g.setColour (juce::Colours::orange);
    g.fillRect (region.withWidth (1.5f));
    g.fillRect (region.withLeft (region.getRight() - 1.5f));

    g.restoreState();
}

//==============================================================================
bool WaveformPanel::isInterestedInFileDrag (const juce::StringArray& files)
{
    return ! files.isEmpty();
}

void WaveformPanel::filesDropped (const juce::StringArray& files, int, int)
{
    if (! files.isEmpty())
        processor.getController().loadSample (juce::File (files[0]));
}
