/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2023.  All rights reserved
 */

#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Configuration/StroikaVersion.h"
#include "Stroika/Foundation/Debug/TimingTrace.h"
#include "Stroika/Foundation/Debug/Trace.h"

#include "LibIPAM/Common/Geolocation.h"

using namespace Stroika::Foundation;

using Characters::RegularExpression;
using Characters::String;

using namespace IPAM::LibIPAM::Common;


/// <summary>
#include <cstdlib>
#include <iostream>

#include "Stroika/Foundation/Debug/Assertions.h"
#include "Stroika/Foundation/Debug/Debugger.h"
#include "Stroika/Foundation/Debug/Fatal.h"
#include "Stroika/Foundation/Execution/Exceptions.h"
#include "Stroika/Foundation/Execution/SignalHandlers.h"

using namespace Stroika;
using namespace Stroika::Foundation;

namespace {
    void ASSERT_HANDLER_(const char* assertCategory, const char* assertionText, const char* fileName, int lineNum, const char* functionName) noexcept
    {
        if (assertCategory == nullptr) {
            assertCategory = "Unknown assertion";
        }
        if (assertionText == nullptr) {
            assertionText = "";
        }
        if (functionName == nullptr) {
            functionName = "";
        }
        cerr << "FAILED: " << assertCategory << "; " << assertionText << ";" << functionName << ";" << fileName << ": " << lineNum << endl;
        DbgTrace("FAILED: %s; %s; %s; %s; %d", assertCategory, assertionText, functionName, fileName, lineNum);

        Debug::DropIntoDebuggerIfPresent();

        _Exit(EXIT_FAILURE); // skip
    }
    void FatalErrorHandler_(const Characters::SDKChar* msg) noexcept
    {
#if kStroika_Version_FullVersion >= Stroika_Make_FULL_VERSION(3, 0, kStroika_Version_Stage_Dev, 1, 0)
        cerr << "FAILED: " << String::FromSDKString (msg).AsNarrowSDKString () << endl;
#else
#if qTargetPlatformSDKUseswchar_t
        cerr << "FAILED: " << Characters::WideStringToNarrowSDKString(msg) << endl;
#else
        cerr << "FAILED: " << msg << endl;
#endif
#endif
        Debug::DropIntoDebuggerIfPresent ();
        _Exit(EXIT_FAILURE); // skip
    }
    void FatalSignalHandler_(Execution::SignalID signal) noexcept
    {
        cerr << "FAILED: SIGNAL= " << Execution::SignalToName(signal).AsNarrowSDKString() << endl;
        DbgTrace(L"FAILED: SIGNAL= %s", Execution::SignalToName(signal).c_str());
        Debug::DropIntoDebuggerIfPresent();
        _Exit(EXIT_FAILURE); // skip
    }
}

namespace Stroika::TestHarness {

    void Setup();

    // print succeeded if it completes, and failed if exception caught
    int PrintPassOrFail(void (*regressionTest) ());

    // LIKE calling Assert but it will ALSO trigger a failure in NODEBUG builds
    void Test_(bool failIfFalse, bool isFailureElseWarning, const char* regressionTestText, const char* fileName, int lineNum);

#define VerifyTestResult(c) Stroika::TestHarness::Test_ (!!(c), true, #c, __FILE__, __LINE__)
#define VerifyTestResultWarning(c) Stroika::TestHarness::Test_ (!!(c), false, #c, __FILE__, __LINE__)

    void WarnTestIssue(const char* issue);
    void WarnTestIssue(const wchar_t* issue);
}
void TestHarness::Setup()
{
#if qDebug
    Stroika::Foundation::Debug::SetAssertionHandler(ASSERT_HANDLER_);
#endif
    Debug::RegisterDefaultFatalErrorHandlers(FatalErrorHandler_);
    using namespace Execution;
    SignalHandlerRegistry::Get().SetStandardCrashHandlerSignals(SignalHandler{ FatalSignalHandler_, SignalHandler::Type::eDirect });
}
int TestHarness::PrintPassOrFail(void (*regressionTest) ())
{
    try {
        (*regressionTest) ();
        cout << "Succeeded" << endl;
        DbgTrace(L"Succeeded");
    }
    catch (...) {
        auto exc = current_exception();
        cerr << "FAILED: REGRESSION TEST DUE TO EXCEPTION: '" << Characters::ToString(exc).AsNarrowSDKString() << "'" << endl;
        cout << "Failed" << endl;
        DbgTrace(L"FAILED: REGRESSION TEST (Exception): '%s", Characters::ToString(exc).c_str());
        Debug::DropIntoDebuggerIfPresent();
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void TestHarness::Test_(bool failIfFalse, bool isFailureElseWarning, const char* regressionTestText, const char* fileName, int lineNum)
{
    if (not failIfFalse) {
        if (isFailureElseWarning) {
            ASSERT_HANDLER_("RegressionTestFailure", regressionTestText, fileName, lineNum, "");
        }
        else {
            cerr << "WARNING: REGRESSION TEST ISSUE: " << regressionTestText << ";" << fileName << ": " << lineNum << endl;
            DbgTrace("WARNING: REGRESSION TEST ISSUE: ; %s; %s; %d", regressionTestText, fileName, lineNum);
            // OK to continue
        }
    }
}
void TestHarness::WarnTestIssue(const char* issue)
{
    cerr << "WARNING: REGRESSION TEST ISSUE: '" << issue << "'" << endl;
    DbgTrace("WARNING: REGRESSION TEST ISSUE: '%s", issue);
}

void TestHarness::WarnTestIssue(const wchar_t* issue)
{
    using namespace Characters;
    string r;
    WideStringToNarrow(issue, issue + wcslen(issue), GetDefaultSDKCodePage(), &r);
    WarnTestIssue(r.c_str());
}
/// </summary>

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

namespace {
    void RegressionTests()
    {
        Latitude_Tests();
        Longitude_Tests();
        TestOne();
    }
}

int main ([[maybe_unused]] int argc, [[maybe_unused]] const char* argv[])
{
    Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"main", L"argv=%s", Characters::ToString (vector<const char*>{argv, argv + argc}).c_str ())};

    Stroika::TestHarness::Setup();
    return Stroika::TestHarness::PrintPassOrFail(RegressionTests);
}
