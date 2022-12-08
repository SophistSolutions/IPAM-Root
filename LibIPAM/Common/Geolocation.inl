/*
 * Copyright(c) Sophist Solutions, Inc. 2022.  All rights reserved
 */
#ifndef __IPAM_LibIPAM_Common_Geolocation_inl__
#define __IPAM_LibIPAM_Common_Geolocation_inl__ 1

namespace IPAM::LibIPAM::Common {

    /*
     ********************************************************************************
     ***************************** Geolocation::Coordinate **************************
     ********************************************************************************
     */
    inline bool Geolocation::Coordinate::operator== (const Geolocation::Coordinate& rhs) const
    {
        return abs (rhs._value - _value) <= .00001; // @todo REWRITE using kPrecision
    }
    inline auto Geolocation::Coordinate::operator<=> (const Geolocation::Coordinate& rhs) const
    {
        return abs (rhs._value - _value) <=> .00001;
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
    inline Geolocation::Latitude::Latitude (const Latitude& rhs)
        : Coordinate{rhs._value}
    {
    }
    inline void Geolocation::Latitude::operator= (const Latitude& rhs)
    {
        _value = rhs._value;
    }
    inline String Geolocation::Latitude::ToISOString ()
    {
        return ToISOString_ (L"%02d");
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
    inline Geolocation::Longitude::Longitude (const Longitude& rhs)
        : Coordinate{rhs._value}
    {
    }
    inline void Geolocation::Longitude::operator= (const Longitude& rhs)
    {
        _value = rhs._value;
    }
    inline String Geolocation::Longitude::ToISOString ()
    {
        return ToISOString_ (L"%03d");
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

#endif /*__IPAM_LibIPAM_Common_Geolocation_inl__*/
