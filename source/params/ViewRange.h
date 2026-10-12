#pragma once

#include <juce_core/juce_core.h>

// The part of the sample the waveform shows, as fractions of its length.
struct ViewRange
{
    double start = 0.0;
    double length = 1.0;

    double getEnd() const noexcept { return start + length; }
};

// zoom is a magnification factor (1 shows everything), viewX scrolls 0-1 across what is hidden
inline ViewRange getViewRange (float zoom, float viewX) noexcept
{
    const auto length = 1.0 / juce::jmax (1.0, static_cast<double> (zoom));
    return { static_cast<double> (viewX) * (1.0 - length), length };
}

// The viewX value that puts the given fraction at the left edge of the view
inline float viewXForStart (double start, float zoom) noexcept
{
    const auto hidden = 1.0 - getViewRange (zoom, 0.0f).length;

    if (hidden <= 0.0)
        return 0.0f;

    return static_cast<float> (juce::jlimit (0.0, 1.0, start / hidden));
}
