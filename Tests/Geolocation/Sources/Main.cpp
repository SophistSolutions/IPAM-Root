/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2023.  All rights reserved
 */

#include <cstdlib>
#include <iostream>

#include "Stroika/Foundation/Characters/RegularExpression.h"
#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Configuration/StroikaVersion.h"
#include "Stroika/Foundation/Debug/TimingTrace.h"
#include "Stroika/Foundation/Debug/Trace.h"

#include "Stroika/Frameworks/Test/TestHarness.h"

#include "LibIPAM/Common/Geolocation.h"

using namespace Stroika::Foundation;
using namespace Stroika::Frameworks;

using Characters::RegularExpression;
using Characters::String;

using namespace IPAM::LibIPAM::Common;

#if qHasFeature_GoogleTest
namespace {
    GTEST_TEST (Geolocation, Latitude_Tests_)
    {
        Geolocation::Latitude a{30.6};
        EXPECT_TRUE (a.degrees () == 30);
        EXPECT_TRUE (a.minutes () == 36);
        EXPECT_TRUE ((abs (a.seconds ()) - 0) < 0.00001);

        Geolocation::Latitude b{-30.634343};
        EXPECT_TRUE (b.degrees () == -30);
        EXPECT_TRUE (b.minutes () == -38);
        EXPECT_TRUE ((abs (b.seconds ()) - 3.6348) < 0.00001);

        Geolocation::Latitude c{5655};
        EXPECT_TRUE (c.degrees () == 5655 % 360);
        EXPECT_TRUE (c.minutes () == 0);
        EXPECT_TRUE ((abs (c.seconds ()) - 0) < 0.00001);

        EXPECT_TRUE (a != b);
        EXPECT_TRUE (b != c);

        Geolocation::Latitude d{"+1234.23"};
        EXPECT_TRUE (d.degrees () == 12);
        EXPECT_TRUE (d.minutes () == 34);
        EXPECT_TRUE ((abs (d.seconds ()) - .23) < 0.00001);

        Geolocation::Latitude e{"-1234.23"};
        EXPECT_TRUE (e.degrees () == -12);
        EXPECT_TRUE (e.minutes () == -34);
        EXPECT_TRUE ((abs (e.seconds ()) - .23) < 0.00001);

        EXPECT_TRUE (Geolocation::Latitude{"+10"}.ToISOString () == "+10");
        EXPECT_TRUE (Geolocation::Latitude{"+1245"}.ToISOString () == "+1245");
        EXPECT_TRUE (Geolocation::Latitude{"+124543"}.ToISOString () == "+124543");
        EXPECT_TRUE (Geolocation::Latitude{"-2134.23"}.ToISOString () == "-2134.23");
        EXPECT_TRUE (Geolocation::Latitude{"+124543.891234"}.ToISOString () == "+124543.891234");
    }

    GTEST_TEST (Geolocation, Longitude_Tests_)
    {
        Geolocation::Longitude a{30.6};
        EXPECT_TRUE (a.degrees () == 30);
        EXPECT_TRUE (a.minutes () == 36);
        EXPECT_TRUE ((abs (a.seconds ()) - 0) < 0.00001);

        Geolocation::Longitude b{-30.634343};
        EXPECT_TRUE (b.degrees () == -30);
        EXPECT_TRUE (b.minutes () == -38);
        EXPECT_TRUE ((abs (b.seconds ()) - 3.6348) < 0.00001);

        Geolocation::Longitude c{5655};
        EXPECT_TRUE (c.degrees () == 5655 % 360);
        EXPECT_TRUE (c.minutes () == 0);
        EXPECT_TRUE ((abs (c.seconds ()) - 0) < 0.00001);

        EXPECT_TRUE (a != b);
        EXPECT_TRUE (b != c);

        Geolocation::Longitude d{"+21234.23"};
        EXPECT_TRUE (d.degrees () == 212);
        EXPECT_TRUE (d.minutes () == 34);
        EXPECT_TRUE ((abs (d.seconds ()) - .23) < 0.00001);

        Geolocation::Longitude e{"-21234.23"};
        EXPECT_TRUE (e.degrees () == -212);
        EXPECT_TRUE (e.minutes () == -34);
        EXPECT_TRUE ((abs (e.seconds ()) - .23) < 0.00001);

        EXPECT_TRUE (Geolocation::Longitude{"+100"}.ToISOString () == L"+100");
        EXPECT_TRUE (Geolocation::Longitude{"+12345"}.ToISOString () == L"+12345");
        EXPECT_TRUE (Geolocation::Longitude{"+1234543"}.ToISOString () == L"+1234543");
        EXPECT_TRUE (Geolocation::Longitude{"-21234.23"}.ToISOString () == L"-21234.23");
        EXPECT_TRUE (Geolocation::Longitude{"+1234543.891234"}.ToISOString () == L"+1234543.891234");
    }

    GTEST_TEST (Geolocation, TestOne_)
    {
        Geolocation a{String{"+1245"}, String{"+100"}};
        EXPECT_TRUE (a.latitude == Geolocation::Latitude{"+1245"});
        EXPECT_TRUE (a.longitude == Geolocation::Longitude{"+100"});
        EXPECT_TRUE (a.ToISOString () = "+1245+100/");

        EXPECT_TRUE (Geolocation (String{"-2134.23"}, String{"+1234543.891234"}).ToISOString () = "-2134.23+1234543.891234/");

        // examples from https://en.wikipedia.org/wiki/ISO_6709
        EXPECT_TRUE (Geolocation{"+00-025/"}.ToISOString () = "+00-025/");                   // Atlantic Ocean
        EXPECT_TRUE (Geolocation{"+48.8577+002.295/"}.ToISOString () = "+48.8577+002.295/"); // Eiffel Tower
        EXPECT_TRUE (Geolocation{"+27.5916+086.5640+8850CRSWGS_84/"}.ToISOString () = "+27.5916+086.5640+8850CRSWGS_84/"); // Mount Everest
        EXPECT_TRUE (Geolocation{"-90+000+2800CRSWGS_84/"}.ToISOString () = "-90+000+2800CRSWGS_84/"); // South Pole
        EXPECT_TRUE (Geolocation{"+40.6894-074.0447/"}.ToISOString () = "+40.6894-074.0447/");         // Statue of Liberty
    }
}
#endif

int main (int argc, const char* argv[])
{
    Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (
        L"main", L"argv=%s", Characters::ToString (vector<const char*>{argv, argv + argc}).c_str ())};
    Test::Setup (argc, argv);
#if qHasFeature_GoogleTest
    return RUN_ALL_TESTS ();
#else
    cerr << "IPAM regression tests require building with google test feature" << endl;
#endif
}
