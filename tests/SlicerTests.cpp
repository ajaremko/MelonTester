#include <catch2/catch_test_macros.hpp>
#include <slicing/Slicer.h>

TEST_CASE ("Equal division", "[slicer]")
{
    SECTION ("covers the sample with no gaps or overlaps")
    {
        // A length that doesn't divide evenly by 16
        const auto slices = Slicer::divideEqually (1001, 16);

        REQUIRE (slices.size() == 16);
        CHECK (slices.front().start == 0);
        CHECK (slices.back().end == 1001);

        for (size_t i = 1; i < slices.size(); ++i)
            CHECK (slices[i].start == slices[i - 1].end);

        for (const auto& slice : slices)
        {
            CHECK (slice.getLength() >= 62);
            CHECK (slice.getLength() <= 63);
        }
    }

    SECTION ("gives nothing for an empty sample or no slices")
    {
        CHECK (Slicer::divideEqually (0, 16).empty());
        CHECK (Slicer::divideEqually (1000, 0).empty());
    }
}
