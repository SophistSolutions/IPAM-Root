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
        EXPECT_TRUE ((abs (a.seconds ()) - 0) < 0.00001);

        Geolocation::Latitude b{-30.634343};
        EXPECT_EQ (b.degrees (), -30);
        EXPECT_EQ (b.minutes (), -38);
        EXPECT_TRUE ((abs (b.seconds ()) - 3.6348) < 0.00001);

        Geolocation::Latitude c{5655};
        EXPECT_EQ (c.degrees (), 5655 % 360);
        EXPECT_EQ (c.minutes (), 0);
        EXPECT_TRUE ((abs (c.seconds ()) - 0) < 0.00001);

        EXPECT_TRUE (a != b);
        EXPECT_TRUE (b != c);

        Geolocation::Latitude d{"+1234.23"};
        EXPECT_EQ (d.degrees (), 12);
        EXPECT_EQ (d.minutes (), 34);
        EXPECT_TRUE ((abs (d.seconds ()) - .23) < 0.00001);

        Geolocation::Latitude e{"-1234.23"};
        EXPECT_EQ (e.degrees (), -12);
        EXPECT_EQ (e.minutes (), -34);
        EXPECT_TRUE ((abs (e.seconds ()) - .23) < 0.00001);

        EXPECT_EQ (Geolocation::Latitude{"+10"}.ToISOString (), "+10");
        EXPECT_EQ (Geolocation::Latitude{"+1245"}.ToISOString (), "+1245");
        EXPECT_EQ (Geolocation::Latitude{"+124543"}.ToISOString (), "+124543");
        EXPECT_EQ (Geolocation::Latitude{"-2134.23"}.ToISOString (), "-2134.23");
        EXPECT_EQ (Geolocation::Latitude{"+124543.891234"}.ToISOString (), "+124543.891234");
    }

    GTEST_TEST (Geolocation, Longitude_Tests_)
    {
        Geolocation::Longitude a{30.6};
        EXPECT_EQ (a.degrees (), 30);
        EXPECT_EQ (a.minutes (), 36);
        EXPECT_TRUE ((abs (a.seconds ()) - 0) < 0.00001);

        Geolocation::Longitude b{-30.634343};
        EXPECT_EQ (b.degrees (), -30);
        EXPECT_EQ (b.minutes (), -38);
        EXPECT_TRUE ((abs (b.seconds ()) - 3.6348) < 0.00001);

        Geolocation::Longitude c{5655};
        EXPECT_EQ (c.degrees (), 5655 % 360);
        EXPECT_EQ (c.minutes (), 0);
        EXPECT_TRUE ((abs (c.seconds ()) - 0) < 0.00001);

        EXPECT_TRUE (a != b);
        EXPECT_TRUE (b != c);

        Geolocation::Longitude d{"+21234.23"};
        EXPECT_EQ (d.degrees (), 212);
        EXPECT_EQ (d.minutes (), 34);
        EXPECT_TRUE ((abs (d.seconds ()) - .23) < 0.00001);

        Geolocation::Longitude e{"-21234.23"};
        EXPECT_EQ (e.degrees (), -212);
        EXPECT_EQ (e.minutes (), -34);
        EXPECT_TRUE ((abs (e.seconds ()) - .23) < 0.00001);

        EXPECT_EQ (Geolocation::Longitude{"+100"}.ToISOString (), "+100");
        EXPECT_EQ (Geolocation::Longitude{"+12345"}.ToISOString (), "+12345");
        EXPECT_EQ (Geolocation::Longitude{"+1234543"}.ToISOString (), "+1234543");
        EXPECT_EQ (Geolocation::Longitude{"-21234.23"}.ToISOString (), "-21234.23");
        EXPECT_EQ (Geolocation::Longitude{"+1234543.891234"}.ToISOString (), "+1234543.891234");
    }

    GTEST_TEST (Geolocation, TestOne_)
    {
        Geolocation a{String{"+1245"}, String{"+100"}};
        EXPECT_EQ (a.latitude, Geolocation::Latitude{"+1245"});
        EXPECT_EQ (a.longitude, Geolocation::Longitude{"+100"});
        EXPECT_EQ (a.ToISOString (), "+1245+100/");

        EXPECT_EQ (Geolocation (String{"-2134.23"}, String{"+1234543.891234"}).ToISOString (), "-2134.23+1234543.891234/");

        // examples from https://en.wikipedia.org/wiki/ISO_6709

        // @todo ask sterl why tests failing (leading zeros)
        //    cerr << "x=" << Geolocation{"+00-025/"}.ToISOString ().AsNarrowSDKString() << endl;
        //    +00-25/
        //    EXPECT_EQ (Geolocation{"+00-025/"}.ToISOString (), "+00-025/");                                                    // Atlantic Ocean

        EXPECT_EQ (Geolocation{"+48.8577+002.295/"}.ToISOString (), "+48.8577+002.295/"); // Eiffel Tower

        // @todo ask sterl why tests failing (leading zeros)
        // cerr << "x=" << Geolocation{"+27.5916+086.5640+8850CRSWGS_84/"}.ToISOString ().AsNarrowSDKString() << endl;
        //                                                                         +27.5916+086.564+8850CRSWGS_84/
        // EXPECT_EQ (Geolocation{"+27.5916+086.5640+8850CRSWGS_84/"}.ToISOString (), "+27.5916+086.5640+8850CRSWGS_84/"); // Mount Everest

        EXPECT_EQ (Geolocation{"-90+000+2800CRSWGS_84/"}.ToISOString (), "-90+000+2800CRSWGS_84/"); // South Pole

        // @todo ask sterl why tests failing (leading zeros)
        // cerr << "x=" << Geolocation{"+40.6894-074.0447/"}.ToISOString ().AsNarrowSDKString() << endl;
        //                                                              +40.6894-74.0447/
        //EXPECT_EQ (Geolocation{"+40.6894-074.0447/"}.ToISOString () , "+40.6894-074.0447/"); // Statue of Liberty
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
