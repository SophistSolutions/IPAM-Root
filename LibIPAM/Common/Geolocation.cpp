/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2022.  All rights reserved
 */

#include "Stroika/Frameworks/StroikaPreComp.h"

#include <math.h>

#include "Stroika/Foundation/Characters/FloatConversion.h"
#include "Stroika/Foundation/Characters/Format.h"
#include "Stroika/Foundation/Characters/RegularExpression.h"
#include "Stroika/Foundation/Characters/String2Int.h"
#include "Stroika/Foundation/Characters/StringBuilder.h"
#include "Stroika/Foundation/DataExchange/BadFormatException.h"
#include "Stroika/Foundation/Debug/Trace.h"

#include "Geolocation.h"

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;

using namespace IPAM::LibIPAM::Common;

/*
 ********************************************************************************
 **************************** Geolocation::Coordinate ***************************
 ********************************************************************************
 */
Geolocation::Coordinate::Coordinate (double d)
{
    _value = std::fmod (d, 360);
}

Geolocation::Coordinate::Coordinate (const String& s, const RegularExpression& regex)
    : Coordinate{0}
{
    Containers::Sequence<String> matches;
    if (s.Matches (regex, &matches)) {
        _value                   = std::fmod (Characters::FloatConversion::ToFloat (matches[1]), 360);
        fLastUnit_               = Unit_::eDegrees;
        double lastUnitInDegrees = 1;
        if (matches[2].length () == 2) {
            int minutes = Characters::String2Int (matches[2]);
            if (abs (minutes) >= _kBase) {
                Execution::Throw (DataExchange::BadFormatException{"Invalid coordinate specification (minutes > 60)"sv});
            }
            _value += minutes / _kBase;
            fLastUnit_        = Unit_::eMinutes;
            lastUnitInDegrees = 1 / _kBase;
        }
        if (matches[3].length () == 2) {
            int seconds = Characters::String2Int (matches[3]);
            if (abs (seconds) >= _kBase) {
                Execution::Throw (DataExchange::BadFormatException{"Invalid coordinate specification (seconds > 60)"sv});
            }
            _value += seconds / (_kBase * _kBase);
            fLastUnit_        = Unit_::eSeconds;
            lastUnitInDegrees = 1 / (_kBase * _kBase);
        }
        if (matches[4].length () > 1) {
            _value += Characters::FloatConversion::ToFloat (matches[4]) * lastUnitInDegrees;
            fFractionDigits_ = matches[4].length () - 1;
        }
        _value *= (matches[0] == "-") ? -1.0 : 1.0;
    }
    else {
        Execution::Throw (DataExchange::BadFormatException{"Invalid coordinate specification"sv});
    }
}

double Geolocation::Coordinate::GPSCoordStringToValue (const String& coor)
{
    static const RegularExpression kCoordinateExp_{"^([0-9]*)?,([0-9]*)(\\.[0-9]*)([NWES]?)"};
    Containers::Sequence<String>   matches;
    if (coor.Matches (kCoordinateExp_, &matches)) {
        double degrees = std::stod (matches[0].As<wstring> ().c_str ());
        double minutes = std::stod (matches[1].As<wstring> ().c_str ());
        double seconds = std::stod (matches[2].As<wstring> ().c_str ());
        double sign    = (matches[3] == "N"sv or matches[3] == "E"sv) ? 1 : -1;
        return sign * (degrees + (minutes + seconds) / 60);
    }
    Execution::Throw (DataExchange::BadFormatException{"Invalid coordinate specification"sv});
}

int Geolocation::Coordinate::degrees () const
{
    double intPart;
    modf (_value, &intPart);
    return int (intPart);
}

int Geolocation::Coordinate::minutes () const
{
    double intPart;
    double fractional = modf (_value, &intPart);
    modf (fractional * _kBase, &intPart);
    return int (intPart);
}

double Geolocation::Coordinate::seconds () const
{
    double intPart;
    double fractional = modf (_value, &intPart);
    double result     = modf (fractional * _kBase, &intPart);
    return std::round (result * _kBase * _kPrecision) / _kPrecision;
}

String Geolocation::Coordinate::ToISOString_ (const Characters::FormatString<char> degreeSpecification)
{
    // Count the whole value in its last unit, rounded to the digits printed, and split that: splitting the double unit by
    // unit truncates 42.99999 seconds to 42
    Unit_        unit      = fLastUnit_.value_or (Unit_::eSeconds);
    unsigned int digits    = static_cast<unsigned int> (fFractionDigits_.value_or (6));
    double       perDegree = unit == Unit_::eDegrees ? 1 : unit == Unit_::eMinutes ? _kBase : _kBase * _kBase;
    double       scale     = std::pow (10.0, digits);
    double       total     = std::round (abs (_value) * perDegree * scale) / scale;
    double       fraction  = total - std::floor (total);
    long long    whole     = static_cast<long long> (std::floor (total));
    long long    degrees   = whole;
    long long    minutes   = 0;
    long long    seconds   = 0;
    if (unit == Unit_::eMinutes) {
        degrees = whole / 60;
        minutes = whole % 60;
    }
    else if (unit == Unit_::eSeconds) {
        degrees = whole / 3600;
        minutes = whole / 60 % 60;
        seconds = whole % 60;
    }
    bool hasFraction = fraction >= 0.5 / scale;
    if (not fLastUnit_.has_value () and seconds == 0 and not hasFraction) {
        unit = minutes == 0 ? Unit_::eDegrees : Unit_::eMinutes;
    }

    // the sign goes first, then the zero-padded magnitude: formatted with the number, a '-' takes a digit's place in the width
    StringBuilder result;
    result << (_value < 0 ? '-' : '+');
    result << Characters::Format (degreeSpecification, degrees);
    if (unit != Unit_::eDegrees) {
        result << Characters::Format ("{:02}"_f, minutes);
    }
    if (unit == Unit_::eSeconds) {
        result << Characters::Format ("{:02}"_f, seconds);
    }
    if (fFractionDigits_.has_value ()) {
        result << Characters::Format ("{:.{}f}"_f, fraction, digits).SubString (1);
    }
    else if (hasFraction) {
        result << Characters::Format ("{:.6f}"_f, fraction).SubString (1).RTrim ([] (Character c) { return c == '0'; });
    }
    return result;
}

/*
 ********************************************************************************
 *********************************** Geolocation ********************************
 ********************************************************************************
 */
Geolocation::Geolocation (const String& lat, const String& lon, const optional<String>& alt)
    : latitude{lat}
    , longitude{lon}
{
    if (alt != std::nullopt) {
        altitude = std::stod (alt->As<wstring> ().c_str ());
    }
}

Geolocation::Geolocation (const String& isoString)
{
    Containers::Sequence<String> matches;
    if (isoString.Matches (kExp_, &matches)) {
        latitude  = Latitude{matches[0]};
        longitude = Longitude{matches[1]};
        if (matches[2].length () > 1) {
            altitude = Characters::FloatConversion::ToFloat (matches[2]);
        }
    }
    else {
        Execution::Throw (DataExchange::BadFormatException{"Invalid Geolocation specification"sv});
    }
}

String Geolocation::ToISOString ()
{
    StringBuilder result = latitude.ToISOString ();
    result << longitude.ToISOString ();
    if (altitude.has_value ()) {
        if (altitude.value () >= 0) {
            result << '+';
        }
        result << Characters::Format ("{:.6g}"_f, altitude.value ());
        result << "CRSWGS_84"; // need a CRS identifier, this is what is used in example (probably need to always require specification of one if height is specified)
    }
    result << "/";
    return result;
}
