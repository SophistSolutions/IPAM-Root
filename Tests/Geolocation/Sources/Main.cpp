/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2023.  All rights reserved
 */

#include <cstdlib>
#include <iostream>

#include "Stroika/Foundation/Characters/RegularExpression.h"
#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Common/StroikaVersion.h"
#include "Stroika/Foundation/Debug/TimingTrace.h"
#include "Stroika/Foundation/Debug/Trace.h"
#include "Stroika/Foundation/Execution/CommandLine.h"

#include "Stroika/Frameworks/Test/TestHarness.h"

#include "LibIPAM/Common/Geolocation.h"

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Frameworks;

using namespace IPAM::LibIPAM::Common;

#if qHasFeature_GoogleTest
namespace {
    GTEST_TEST (Geolocation, Latitude_Tests_)
    {
        Geolocation::Latitude a{30.6};
        EXPECT_EQ (a.degrees (), 30);
        EXPECT_EQ (a.minutes (), 36);
        EXPECT_NEAR (a.seconds (), 0, 0.00001);

        Geolocation::Latitude b{-30.634343};
        EXPECT_EQ (b.degrees (), -30);
        EXPECT_EQ (b.minutes (), -38);
        EXPECT_NEAR (abs (b.seconds ()), 3.6348, 0.00001);

        Geolocation::Latitude c{5655};
        EXPECT_EQ (c.degrees (), 5655 % 360);
        EXPECT_EQ (c.minutes (), 0);
        EXPECT_NEAR (c.seconds (), 0, 0.00001);

        EXPECT_TRUE (a != b);
        EXPECT_TRUE (b != c);

        // ISO 6709: a decimal fraction belongs to the last unit given - here minutes, so .23' is 13.8"
        Geolocation::Latitude d{"+1234.23"};
        EXPECT_EQ (d.degrees (), 12);
        EXPECT_EQ (d.minutes (), 34);
        EXPECT_NEAR (d.seconds (), 13.8, 0.00001);

        Geolocation::Latitude e{"-1234.23"};
        EXPECT_EQ (e.degrees (), -12);
        EXPECT_EQ (e.minutes (), -34);
        EXPECT_NEAR (abs (e.seconds ()), 13.8, 0.00001);

        EXPECT_EQ (Geolocation::Latitude{"+48.8577"}, Geolocation::Latitude{48.8577});
        EXPECT_EQ (Geolocation::Latitude{"+1234.23"}, Geolocation::Latitude{12 + 34.23 / 60});
        EXPECT_EQ (Geolocation::Latitude{"+124543.891234"}, Geolocation::Latitude{12 + 45 / 60.0 + 43.891234 / 3600});

        // a parsed coordinate prints back in the form it was given
        EXPECT_EQ (Geolocation::Latitude{"+10"}.ToISOString (), "+10");
        EXPECT_EQ (Geolocation::Latitude{"+1245"}.ToISOString (), "+1245");
        EXPECT_EQ (Geolocation::Latitude{"+1200"}.ToISOString (), "+1200");
        EXPECT_EQ (Geolocation::Latitude{"+124543"}.ToISOString (), "+124543");
        EXPECT_EQ (Geolocation::Latitude{"-2134.23"}.ToISOString (), "-2134.23");
        EXPECT_EQ (Geolocation::Latitude{"-0030"}.ToISOString (), "-0030");
        EXPECT_EQ (Geolocation::Latitude{"+124543.891234"}.ToISOString (), "+124543.891234");

        // one made from a number (as ImportMetadata does) prints every unit down to the last nonzero one, zero-padded after the sign
        EXPECT_EQ (Geolocation::Latitude{-5.5}.ToISOString (), "-0530");
        EXPECT_EQ (Geolocation::Latitude{30 + 5 / 3600.0}.ToISOString (), "+300005");
        EXPECT_EQ (Geolocation::Latitude{30 + 42 / 60.0 + 0.5 / 3600}.ToISOString (), "+304200.5");
    }

    GTEST_TEST (Geolocation, Longitude_Tests_)
    {
        Geolocation::Longitude a{30.6};
        EXPECT_EQ (a.degrees (), 30);
        EXPECT_EQ (a.minutes (), 36);
        EXPECT_NEAR (a.seconds (), 0, 0.00001);

        Geolocation::Longitude b{-30.634343};
        EXPECT_EQ (b.degrees (), -30);
        EXPECT_EQ (b.minutes (), -38);
        EXPECT_NEAR (abs (b.seconds ()), 3.6348, 0.00001);

        Geolocation::Longitude c{5655};
        EXPECT_EQ (c.degrees (), 5655 % 360);
        EXPECT_EQ (c.minutes (), 0);
        EXPECT_NEAR (c.seconds (), 0, 0.00001);

        EXPECT_TRUE (a != b);
        EXPECT_TRUE (b != c);

        Geolocation::Longitude d{"+21234.23"};
        EXPECT_EQ (d.degrees (), 212);
        EXPECT_EQ (d.minutes (), 34);
        EXPECT_NEAR (d.seconds (), 13.8, 0.00001);

        Geolocation::Longitude e{"-21234.23"};
        EXPECT_EQ (e.degrees (), -212);
        EXPECT_EQ (e.minutes (), -34);
        EXPECT_NEAR (abs (e.seconds ()), 13.8, 0.00001);

        EXPECT_EQ (Geolocation::Longitude{"-074.0447"}, Geolocation::Longitude{-74.0447});
        EXPECT_EQ (Geolocation::Longitude{"+21234.23"}, Geolocation::Longitude{212 + 34.23 / 60});

        EXPECT_EQ (Geolocation::Longitude{"+100"}.ToISOString (), "+100");
        EXPECT_EQ (Geolocation::Longitude{"+12345"}.ToISOString (), "+12345");
        EXPECT_EQ (Geolocation::Longitude{"+1234543"}.ToISOString (), "+1234543");
        EXPECT_EQ (Geolocation::Longitude{"-21234.23"}.ToISOString (), "-21234.23");
        EXPECT_EQ (Geolocation::Longitude{"-00030"}.ToISOString (), "-00030");
        EXPECT_EQ (Geolocation::Longitude{"+1234543.891234"}.ToISOString (), "+1234543.891234");

        EXPECT_EQ (Geolocation::Longitude{-74.0447}.ToISOString (), "-0740240.92");
    }

    GTEST_TEST (Geolocation, TestOne_)
    {
        Geolocation a{String{"+1245"}, String{"+100"}};
        EXPECT_EQ (a.latitude, Geolocation::Latitude{"+1245"});
        EXPECT_EQ (a.longitude, Geolocation::Longitude{"+100"});
        EXPECT_EQ (a.ToISOString (), "+1245+100/");

        EXPECT_EQ (Geolocation (String{"-2134.23"}, String{"+1234543.891234"}).ToISOString (), "-2134.23+1234543.891234/");

        // examples from https://en.wikipedia.org/wiki/ISO_6709
        EXPECT_EQ (Geolocation{"+00-025/"}.ToISOString (), "+00-025/");                                                 // Atlantic Ocean
        EXPECT_EQ (Geolocation{"+48.8577+002.295/"}.ToISOString (), "+48.8577+002.295/");                               // Eiffel Tower
        EXPECT_EQ (Geolocation{"+27.5916+086.5640+8850CRSWGS_84/"}.ToISOString (), "+27.5916+086.5640+8850CRSWGS_84/"); // Mount Everest
        EXPECT_EQ (Geolocation{"-90+000+2800CRSWGS_84/"}.ToISOString (), "-90+000+2800CRSWGS_84/");                     // South Pole
        EXPECT_EQ (Geolocation{"+40.6894-074.0447/"}.ToISOString (), "+40.6894-074.0447/");                             // Statue of Liberty

        Geolocation eiffelTower{"+48.8577+002.295/"};
        EXPECT_EQ (eiffelTower.latitude, Geolocation::Latitude{48.8577});
        EXPECT_EQ (eiffelTower.longitude, Geolocation::Longitude{2.295});
    }
}
#endif

int main (int argc, const char* argv[])
{
    Debug::TraceContextBumper ctx{"main", "argv={}"_f, Execution::CommandLine{argc, argv}};
    Test::Setup (argc, argv);
#if qHasFeature_GoogleTest
    return RUN_ALL_TESTS ();
#else
    cerr << "IPAM regression tests require building with google test feature" << endl;
#endif
}
