/*
 * Copyright(c) Sophist Solutions, Inc. 2022.  All rights reserved
 */

#include "Stroika/Foundation/Math/Common.h"

namespace IPAM::LibIPAM::Common {

    /*
     ********************************************************************************
     ***************************** Geolocation::Coordinate **************************
     ********************************************************************************
     */
    inline bool Geolocation::Coordinate::operator== (const Geolocation::Coordinate& rhs) const
    {
        return Math::NearlyEquals (rhs._value, _value, 1 / _kPrecision);
    }
    inline auto Geolocation::Coordinate::operator<=> (const Geolocation::Coordinate& rhs) const
    {
        // @todo - this is not clearly right - and probably wrong... at least document why its true/correct
        return abs (rhs._value - _value) <=> 1 / _kPrecision;
    }

    /*
     ********************************************************************************
     ***************************** Geolocation::Latitude ****************************
     ********************************************************************************
     */
    inline Geolocation::Latitude::Latitude (double d)
        : Coordinate{d}
    {
    }
    inline Geolocation::Latitude::Latitude (const String& s)
        : Coordinate{s, kLatitudeExp_}
    {
    }
    inline String Geolocation::Latitude::ToISOString ()
    {
        using namespace Characters::Literals;
        return ToISOString_ ("{:02d}"_f);
    }

    /*
     ********************************************************************************
     **************************** Geolocation::Longitude ****************************
     ********************************************************************************
     */
    inline Geolocation::Longitude::Longitude (double d)
        : Coordinate{d}
    {
    }
    inline Geolocation::Longitude::Longitude (const String& s)
        : Coordinate{s, kLongitudeExp_}
    {
    }
    inline String Geolocation::Longitude::ToISOString ()
    {
        using namespace Characters::Literals;
        return ToISOString_ ("{:03}"_f);
    }

    /*
     ********************************************************************************
     ********************************* Geolocation **********************************
     ********************************************************************************
     */
    inline Geolocation::Geolocation (Latitude lat, Longitude lon, optional<double> alt)
        : latitude{lat}
        , longitude{lon}
        , altitude{alt}
    {
    }

}
