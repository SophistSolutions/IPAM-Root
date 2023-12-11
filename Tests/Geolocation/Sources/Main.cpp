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

namespace {
    void Latitude_Tests_ ()
    {
        Geolocation::Latitude a{30.6};
        VerifyTestResult (a.degrees () == 30);
        VerifyTestResult (a.minutes () == 36);
        VerifyTestResult ((abs (a.seconds ()) - 0) < 0.00001);

        Geolocation::Latitude b{-30.634343};
        VerifyTestResult (b.degrees () == -30);
        VerifyTestResult (b.minutes () == -38);
        VerifyTestResult ((abs (b.seconds ()) - 3.6348) < 0.00001);

        Geolocation::Latitude c{5655};
        VerifyTestResult (c.degrees () == 5655 % 360);
        VerifyTestResult (c.minutes () == 0);
        VerifyTestResult ((abs (c.seconds ()) - 0) < 0.00001);

        VerifyTestResult (a != b);
        VerifyTestResult (b != c);

        Geolocation::Latitude d{"+1234.23"};
        VerifyTestResult (d.degrees () == 12);
        VerifyTestResult (d.minutes () == 34);
        VerifyTestResult ((abs (d.seconds ()) - .23) < 0.00001);

        Geolocation::Latitude e{"-1234.23"};
        VerifyTestResult (e.degrees () == -12);
        VerifyTestResult (e.minutes () == -34);
        VerifyTestResult ((abs (e.seconds ()) - .23) < 0.00001);

        VerifyTestResult (Geolocation::Latitude{"+10"}.ToISOString () == "+10");
        VerifyTestResult (Geolocation::Latitude{"+1245"}.ToISOString () == "+1245");
        VerifyTestResult (Geolocation::Latitude{"+124543"}.ToISOString () == "+124543");
        VerifyTestResult (Geolocation::Latitude{"-2134.23"}.ToISOString () == "-2134.23");
        VerifyTestResult (Geolocation::Latitude{"+124543.891234"}.ToISOString () == "+124543.891234");
    }

    void Longitude_Tests_ ()
    {
        Geolocation::Longitude a{30.6};
        VerifyTestResult (a.degrees () == 30);
        VerifyTestResult (a.minutes () == 36);
        VerifyTestResult ((abs (a.seconds ()) - 0) < 0.00001);

        Geolocation::Longitude b{-30.634343};
        VerifyTestResult (b.degrees () == -30);
        VerifyTestResult (b.minutes () == -38);
        VerifyTestResult ((abs (b.seconds ()) - 3.6348) < 0.00001);

        Geolocation::Longitude c{5655};
        VerifyTestResult (c.degrees () == 5655 % 360);
        VerifyTestResult (c.minutes () == 0);
        VerifyTestResult ((abs (c.seconds ()) - 0) < 0.00001);

        VerifyTestResult (a != b);
        VerifyTestResult (b != c);

        Geolocation::Longitude d{"+21234.23"};
        VerifyTestResult (d.degrees () == 212);
        VerifyTestResult (d.minutes () == 34);
        VerifyTestResult ((abs (d.seconds ()) - .23) < 0.00001);

        Geolocation::Longitude e{"-21234.23"};
        VerifyTestResult (e.degrees () == -212);
        VerifyTestResult (e.minutes () == -34);
        VerifyTestResult ((abs (e.seconds ()) - .23) < 0.00001);

        VerifyTestResult (Geolocation::Longitude{"+100"}.ToISOString () == L"+100");
        VerifyTestResult (Geolocation::Longitude{"+12345"}.ToISOString () == L"+12345");
        VerifyTestResult (Geolocation::Longitude{"+1234543"}.ToISOString () == L"+1234543");
        VerifyTestResult (Geolocation::Longitude{"-21234.23"}.ToISOString () == L"-21234.23");
        VerifyTestResult (Geolocation::Longitude{"+1234543.891234"}.ToISOString () == L"+1234543.891234");
    }

    void TestOne_ ()
    {
        Geolocation a{String{"+1245"}, String{"+100"}};
        VerifyTestResult (a.latitude == Geolocation::Latitude{"+1245"});
        VerifyTestResult (a.longitude == Geolocation::Longitude{"+100"});
        VerifyTestResult (a.ToISOString () = "+1245+100/");

        VerifyTestResult (Geolocation (String{"-2134.23"}, String{"+1234543.891234"}).ToISOString () = "-2134.23+1234543.891234/");

        // examples from https://en.wikipedia.org/wiki/ISO_6709
        VerifyTestResult (Geolocation{"+00-025/"}.ToISOString () = "+00-025/");                   // Atlantic Ocean
        VerifyTestResult (Geolocation{"+48.8577+002.295/"}.ToISOString () = "+48.8577+002.295/"); // Eiffel Tower
        VerifyTestResult (Geolocation{"+27.5916+086.5640+8850CRSWGS_84/"}.ToISOString () = "+27.5916+086.5640+8850CRSWGS_84/"); // Mount Everest
        VerifyTestResult (Geolocation{"-90+000+2800CRSWGS_84/"}.ToISOString () = "-90+000+2800CRSWGS_84/"); // South Pole
        VerifyTestResult (Geolocation{"+40.6894-074.0447/"}.ToISOString () = "+40.6894-074.0447/");         // Statue of Liberty
    }
}

namespace {
    void RegressionTests_ ()
    {
        Latitude_Tests_ ();
        Longitude_Tests_ ();
        TestOne_ ();
    }
}

int main (int argc, const char* argv[])
{
    Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (
        L"main", L"argv=%s", Characters::ToString (vector<const char*>{argv, argv + argc}).c_str ())};
    Test::Setup (argc, argv);
    return Test::PrintPassOrFail (RegressionTests_);
}
