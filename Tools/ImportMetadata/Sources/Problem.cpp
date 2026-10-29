/*
 * Copyright(c) Sophist Solutions, Inc. 2026.  All rights reserved
 */
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Debug/Trace.h"

#include "Problem.h"

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;

void Metadata::Report (const ProblemSink& problemSink, const Problem& problem)
{
    DbgTrace ("left out {} {}={}: {}"_f, problem.fFile, problem.fKey, problem.fValue, problem.fReason);
    if (problemSink) {
        problemSink (problem);
    }
}

void Metadata::ReadOrReport (const ProblemSink& problemSink, const String& file, const String& key, const String& value, const function<void ()>& read)
{
    try {
        read ();
    }
    catch (...) {
        Report (problemSink, Problem{.fFile = file, .fKey = key, .fValue = value, .fReason = Characters::ToString (current_exception ())});
    }
}
