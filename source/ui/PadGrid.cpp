#include "PadGrid.h"

namespace
{
    constexpr int padsPerRow = 4;
}

PadGrid::PadGrid (PluginProcessor& processorToUse)
    : processor (processorToUse)
{
    startTimerHz (30);
}

juce::Rectangle<int> PadGrid::getPadBounds (int padIndex) const
{
    const auto cellWidth = getWidth() / padsPerRow;
    const auto cellHeight = getHeight() / padsPerRow;
    const auto column = padIndex % padsPerRow;
    const auto rowFromBottom = padIndex / padsPerRow;

    return juce::Rectangle<int> (column * cellWidth, (padsPerRow - 1 - rowFromBottom) * cellHeight, cellWidth, cellHeight).reduced (3);
}

int PadGrid::getPadAt (juce::Point<int> position) const
{
    for (int pad = 0; pad < kNumPads; ++pad)
        if (getPadBounds (pad).contains (position))
            return pad;

    return -1;
}

juce::uint64 PadGrid::getDrawState() const
{
    const auto& params = processor.getParams();
    auto state = static_cast<juce::uint64> (params.getSelectedPad());

    for (int pad = 0; pad < kNumPads; ++pad)
    {
        const auto assigned = params.pad (pad).get (PadParam::assigned) >= 0.5f;
        const auto playing = processor.getEngine().isPadPlaying (pad);

        state = (state << 2) | (assigned ? 1u : 0u) | (playing ? 2u : 0u);
    }

    return state;
}

void PadGrid::timerCallback()
{
    const auto drawState = getDrawState();

    if (drawState != lastDrawState)
    {
        lastDrawState = drawState;
        repaint();
    }
}

void PadGrid::paint (juce::Graphics& g)
{
    const auto& params = processor.getParams();
    const auto selected = params.getSelectedPad();

    for (int pad = 0; pad < kNumPads; ++pad)
    {
        const auto bounds = getPadBounds (pad).toFloat();
        const auto assigned = params.pad (pad).get (PadParam::assigned) >= 0.5f;
        const auto playing = processor.getEngine().isPadPlaying (pad);

        // Pad colours aren't implemented yet, so every assigned pad is the same grey
        auto colour = assigned ? juce::Colours::grey : juce::Colours::darkgrey.darker();

        if (playing)
            colour = juce::Colours::orange;

        g.setColour (colour);
        g.fillRoundedRectangle (bounds, 5.0f);

        if (pad == selected)
        {
            g.setColour (juce::Colours::white);
            g.drawRoundedRectangle (bounds.reduced (1.0f), 5.0f, 2.0f);
        }

        g.setColour (juce::Colours::white.withAlpha (0.8f));
        g.drawText (juce::String (pad + 1), bounds.reduced (6.0f), juce::Justification::topLeft);
    }
}

void PadGrid::mouseDown (const juce::MouseEvent& event)
{
    heldPad = getPadAt (event.getPosition());

    if (heldPad < 0)
        return;

    processor.auditionPad (heldPad, true);
    processor.getController().selectPad (heldPad);
}

void PadGrid::mouseUp (const juce::MouseEvent&)
{
    if (heldPad >= 0)
        processor.auditionPad (heldPad, false);

    heldPad = -1;
}
