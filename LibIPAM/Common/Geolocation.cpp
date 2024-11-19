/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2022.  All rights reserved
 */

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
        _value = std::fmod (Characters::FloatConversion::ToFloat (matches[1]), 360);
        if (matches[2].length () == 2) {
            int minutes = Characters::String2Int (matches[2]);
            if (abs (minutes) >= _kBase) {
                Execution::Throw (DataExchange::BadFormatException{"Invalid coordinate specification (minutes > 60)"sv});
            }
            _value += minutes / _kBase;
        }
        if (matches[3].length () == 2) {
            int seconds = Characters::String2Int (matches[3]);
            if (abs (seconds) >= _kBase) {
                Execution::Throw (DataExchange::BadFormatException{"Invalid coordinate specification (seconds > 60)"sv});
            }
            _value += seconds / (_kBase * _kBase);
        }
        if (matches[4].length () > 1) {
            _value += Characters::FloatConversion::ToFloat (matches[4]) / (_kBase * _kBase);
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
        double degrees = std::stod (matches[0].c_str ());
        double minutes = std::stod (matches[1].c_str ());
        double seconds = std::stod (matches[2].c_str ());
        double sign    = (matches[3] == "N" or matches[3] == "E") ? 1 : -1;
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
    StringBuilder result;
    if (_value >= 0) {
        result << '+';
    }
    result += Characters::Format (degreeSpecification, degrees ());
    if (minutes () != 0) {
        result << Characters::Format ("{:02}"_f, abs (minutes ()));
    }
    if ((abs (seconds ()) - 0) > 0.00001) {
        double _integral;
        double fractional = std::modf (abs (seconds ()), &_integral);
        int    xxx        = int (abs (_integral));
        if (xxx != 0) {
            result << Characters::Format ("{:02}"_f, xxx);
        }
        if (abs (fractional) > 0.000001) {
            result << Characters::Format ("{:.6g}"_f, abs (fractional)).SubString (1);
        }
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
