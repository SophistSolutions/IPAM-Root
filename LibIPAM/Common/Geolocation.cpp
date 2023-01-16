/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2022.  All rights reserved
 */

#include <math.h>

#include "Stroika/Foundation/Characters/FloatConversion.h"
#include "Stroika/Foundation/Characters/Format.h"
#include "Stroika/Foundation/Characters/RegularExpression.h"
#include "Stroika/Foundation/Characters/String2Int.h"
#include "Stroika/Foundation/DataExchange/BadFormatException.h"
#include "Stroika/Foundation/Debug/Trace.h"

#include "Geolocation.h"

using namespace Stroika::Foundation;

using Characters::RegularExpression;
using Characters::String;

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
                Execution::Throw (DataExchange::BadFormatException{L"Invalid coordinate specification (minutes > 60)"sv});
            }
            _value += minutes / _kBase;
        }
        if (matches[3].length () == 2) {
            int seconds = Characters::String2Int (matches[3]);
            if (abs (seconds) >= _kBase) {
                Execution::Throw (DataExchange::BadFormatException{L"Invalid coordinate specification (seconds > 60)"sv});
            }
            _value += seconds / (_kBase * _kBase);
        }
        if (matches[4].length () > 1) {
            _value += Characters::FloatConversion::ToFloat (matches[4]) / (_kBase * _kBase);
        }
        _value *= (matches[0] == L"-") ? -1.0 : 1.0;
    }
    else {
        Execution::Throw (DataExchange::BadFormatException{L"Invalid coordinate specification"sv});
    }
}

double Geolocation::Coordinate::GPSCoordStringToValue (const String& coor)
{
    static const RegularExpression kCoordinateExp_{L"^([0-9]*)?,([0-9]*)(\\.[0-9]*)([NWES]?)"};
    Containers::Sequence<String>   matches;
    if (coor.Matches (kCoordinateExp_, &matches)) {
        double degrees = std::stod (matches[0].c_str ());
        double minutes = std::stod (matches[1].c_str ());
        double seconds = std::stod (matches[2].c_str ());
        double sign    = (matches[3] == L"N" or matches[3] == L"E") ? 1 : -1;
        return sign * (degrees + (minutes + seconds) / 60);
    }
    Execution::Throw (DataExchange::BadFormatException{L"Invalid coordinate specification"sv});
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

String Geolocation::Coordinate::ToISOString_ (const wchar_t* degreeSpecification)
{
    String result;
    if (_value >= 0) {
        result += L'+';
    }
    result += Characters::Format (degreeSpecification, degrees ());
    if (minutes () != 0) {
        result += Characters::Format (L"%02d", abs (minutes ()));
    }
    if ((abs (seconds ()) - 0) > 0.00001) {
        double _integral;
        double fractional = std::modf (abs (seconds ()), &_integral);
        int    xxx        = int (abs (_integral));
        if (xxx != 0) {
            result += Characters::Format (L"%02d", xxx);
        }
        if (abs (fractional) > 0.000001) {
            result += Characters::Format (L"%.6g", abs (fractional)).SubString (1);
        }
    }
    return result;
}

/*
 ********************************************************************************
 ***************************** Geolocation::Latitude ****************************
 ********************************************************************************
 */
#if qDebug
void Geolocation::Latitude::TestSuite ()
{
    Latitude a (30.6);
    Assert (a.degrees () == 30);
    Assert (a.minutes () == 36);
    Assert ((abs (a.seconds ()) - 0) < 0.00001);

    Latitude b (-30.634343);
    Assert (b.degrees () == -30);
    Assert (b.minutes () == -38);
    Assert ((abs (b.seconds ()) - 3.6348) < 0.00001);

    Latitude c (5655);
    Assert (c.degrees () == 5655 % 360);
    Assert (c.minutes () == 0);
    Assert ((abs (c.seconds ()) - 0) < 0.00001);

    Assert (a != b);
    Assert (b != c);

    Latitude d (L"+1234.23");
    Assert (d.degrees () == 12);
    Assert (d.minutes () == 34);
    Assert ((abs (d.seconds ()) - .23) < 0.00001);

    Latitude e (L"-1234.23");
    Assert (e.degrees () == -12);
    Assert (e.minutes () == -34);
    Assert ((abs (e.seconds ()) - .23) < 0.00001);

    Assert (Latitude (L"+10").ToISOString () == L"+10");
    Assert (Latitude (L"+1245").ToISOString () == L"+1245");
    Assert (Latitude (L"+124543").ToISOString () == L"+124543");
    Assert (Latitude (L"-2134.23").ToISOString () == L"-2134.23");
    Assert (Latitude (L"+124543.891234").ToISOString () == L"+124543.891234");
}

void Geolocation::Longitude::TestSuite ()
{
    Longitude a (30.6);
    Assert (a.degrees () == 30);
    Assert (a.minutes () == 36);
    Assert ((abs (a.seconds ()) - 0) < 0.00001);

    Longitude b (-30.634343);
    Assert (b.degrees () == -30);
    Assert (b.minutes () == -38);
    Assert ((abs (b.seconds ()) - 3.6348) < 0.00001);

    Longitude c (5655);
    Assert (c.degrees () == 5655 % 360);
    Assert (c.minutes () == 0);
    Assert ((abs (c.seconds ()) - 0) < 0.00001);

    Assert (a != b);
    Assert (b != c);

    Longitude d (L"+21234.23");
    Assert (d.degrees () == 212);
    Assert (d.minutes () == 34);
    Assert ((abs (d.seconds ()) - .23) < 0.00001);

    Longitude e (L"-21234.23");
    Assert (e.degrees () == -212);
    Assert (e.minutes () == -34);
    Assert ((abs (e.seconds ()) - .23) < 0.00001);

    Assert (Longitude (L"+100").ToISOString () == L"+100");
    Assert (Longitude (L"+12345").ToISOString () == L"+12345");
    Assert (Longitude (L"+1234543").ToISOString () == L"+1234543");
    Assert (Longitude (L"-21234.23").ToISOString () == L"-21234.23");
    Assert (Longitude (L"+1234543.891234").ToISOString () == L"+1234543.891234");
}
#endif

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
        latitude  = Latitude (matches[0]);
        longitude = Longitude (matches[1]);
        if (matches[2].length () > 1) {
            altitude = Characters::FloatConversion::ToFloat (matches[2]);
        }
    }
    else {
        Execution::Throw (DataExchange::BadFormatException{L"Invalid Geolocation specification"sv});
    }
}

String Geolocation::ToISOString ()
{
    String result = latitude.ToISOString () + longitude.ToISOString ();
    if (altitude.has_value ()) {
        if (altitude.value () >= 0) {
            result += L'+';
        }
        result += Characters::Format (L"%.6g", altitude.value ());
        result += L"CRSWGS_84"; // need a CRS identifier, this is what is used in example (probably need to always require specification of one if height is specified)
    }
    return result + L"/";
}

#if qDebug
void Geolocation::TestSuite ()
{
    Latitude::TestSuite ();
    Longitude::TestSuite ();

    Geolocation a (String (L"+1245"), String (L"+100"));
    Assert (a.latitude == Latitude (L"+1245"));
    Assert (a.longitude == Longitude (L"+100"));
    Assert (a.ToISOString () = L"+1245+100/");

    Assert (Geolocation (String (L"-2134.23"), String (L"+1234543.891234")).ToISOString () = L"-2134.23+1234543.891234/");

    // examples from https://en.wikipedia.org/wiki/ISO_6709
    Assert (Geolocation (L"+00-025/").ToISOString () = L"+00-025/");                                                 // Atlantic Ocean
    Assert (Geolocation (L"+48.8577+002.295/").ToISOString () = L"+48.8577+002.295/");                               // Eiffel Tower
    Assert (Geolocation (L"+27.5916+086.5640+8850CRSWGS_84/").ToISOString () = L"+27.5916+086.5640+8850CRSWGS_84/"); // Mount Everest
    Assert (Geolocation (L"-90+000+2800CRSWGS_84/").ToISOString () = L"-90+000+2800CRSWGS_84/");                     // South Pole
    Assert (Geolocation (L"+40.6894-074.0447/").ToISOString () = L"+40.6894-074.0447/");                             // Statue of Liberty
}
#endif
