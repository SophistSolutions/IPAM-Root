/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2023.  All rights reserved
 */

#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Debug/TimingTrace.h"
#include "Stroika/Foundation/Debug/Trace.h"

#include "LibIPAM/Common/Geolocation.h"

using namespace Stroika::Foundation;

using Characters::RegularExpression;
using Characters::String;

using namespace IPAM::LibIPAM::Common;

namespace {
    void Latitude_Tests()
    {
        Geolocation::Latitude a(30.6);
        Assert(a.degrees() == 30);
        Assert(a.minutes() == 36);
        Assert((abs(a.seconds()) - 0) < 0.00001);

        Geolocation::Latitude b(-30.634343);
        Assert(b.degrees() == -30);
        Assert(b.minutes() == -38);
        Assert((abs(b.seconds()) - 3.6348) < 0.00001);

        Geolocation::Latitude c(5655);
        Assert(c.degrees() == 5655 % 360);
        Assert(c.minutes() == 0);
        Assert((abs(c.seconds()) - 0) < 0.00001);

        Assert(a != b);
        Assert(b != c);

        Geolocation::Latitude d(L"+1234.23");
        Assert(d.degrees() == 12);
        Assert(d.minutes() == 34);
        Assert((abs(d.seconds()) - .23) < 0.00001);

        Geolocation::Latitude e(L"-1234.23");
        Assert(e.degrees() == -12);
        Assert(e.minutes() == -34);
        Assert((abs(e.seconds()) - .23) < 0.00001);

        Assert(Geolocation::Latitude(L"+10").ToISOString() == L"+10");
        Assert(Geolocation::Latitude(L"+1245").ToISOString() == L"+1245");
        Assert(Geolocation::Latitude(L"+124543").ToISOString() == L"+124543");
        Assert(Geolocation::Latitude(L"-2134.23").ToISOString() == L"-2134.23");
        Assert(Geolocation::Latitude(L"+124543.891234").ToISOString() == L"+124543.891234");
    }

    void Longitude_Tests()
    {
        Geolocation::Longitude a(30.6);
        Assert(a.degrees() == 30);
        Assert(a.minutes() == 36);
        Assert((abs(a.seconds()) - 0) < 0.00001);

        Geolocation::Longitude b(-30.634343);
        Assert(b.degrees() == -30);
        Assert(b.minutes() == -38);
        Assert((abs(b.seconds()) - 3.6348) < 0.00001);

        Geolocation::Longitude c(5655);
        Assert(c.degrees() == 5655 % 360);
        Assert(c.minutes() == 0);
        Assert((abs(c.seconds()) - 0) < 0.00001);

        Assert(a != b);
        Assert(b != c);

        Geolocation::Longitude d(L"+21234.23");
        Assert(d.degrees() == 212);
        Assert(d.minutes() == 34);
        Assert((abs(d.seconds()) - .23) < 0.00001);

        Geolocation::Longitude e(L"-21234.23");
        Assert(e.degrees() == -212);
        Assert(e.minutes() == -34);
        Assert((abs(e.seconds()) - .23) < 0.00001);

        Assert(Geolocation::Longitude(L"+100").ToISOString() == L"+100");
        Assert(Geolocation::Longitude(L"+12345").ToISOString() == L"+12345");
        Assert(Geolocation::Longitude(L"+1234543").ToISOString() == L"+1234543");
        Assert(Geolocation::Longitude(L"-21234.23").ToISOString() == L"-21234.23");
        Assert(Geolocation::Longitude(L"+1234543.891234").ToISOString() == L"+1234543.891234");
    }

    void TestOne()
    {
        Geolocation a(String(L"+1245"), String(L"+100"));
        Assert(a.latitude == Geolocation::Latitude(L"+1245"));
        Assert(a.longitude == Geolocation::Longitude(L"+100"));
        Assert(a.ToISOString() = L"+1245+100/");

        Assert(Geolocation(String(L"-2134.23"), String(L"+1234543.891234")).ToISOString() = L"-2134.23+1234543.891234/");

        // examples from https://en.wikipedia.org/wiki/ISO_6709
        Assert(Geolocation(L"+00-025/").ToISOString() = L"+00-025/");                                                 // Atlantic Ocean
        Assert(Geolocation(L"+48.8577+002.295/").ToISOString() = L"+48.8577+002.295/");                               // Eiffel Tower
        Assert(Geolocation(L"+27.5916+086.5640+8850CRSWGS_84/").ToISOString() = L"+27.5916+086.5640+8850CRSWGS_84/"); // Mount Everest
        Assert(Geolocation(L"-90+000+2800CRSWGS_84/").ToISOString() = L"-90+000+2800CRSWGS_84/");                     // South Pole
        Assert(Geolocation(L"+40.6894-074.0447/").ToISOString() = L"+40.6894-074.0447/");                             // Statue of Liberty
    }
}

int main ([[maybe_unused]] int argc, [[maybe_unused]] const char* argv[])
{
    Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"main", L"argv=%s", Characters::ToString (vector<const char*>{argv, argv + argc}).c_str ())};

    Latitude_Tests();
    Longitude_Tests();
    TestOne();

    return EXIT_SUCCESS;
}
