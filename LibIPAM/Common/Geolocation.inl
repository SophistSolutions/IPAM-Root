/*
 * Copyright(c) Sophist Solutions, Inc. 2022.  All rights reserved
 */
#ifndef __IPAM_LibIPAM_Common_Geolocation_inl__
#define __IPAM_LibIPAM_Common_Geolocation_inl__ 1

namespace IPAM::LibIPAM::Common {

    /*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
    inline bool Geolocation::Coordinate::operator== (const Geolocation::Coordinate& rhs) const
    {
        return (abs (rhs.value - value) <= .00001);
    }
    inline auto Geolocation::Coordinate::operator<=> (const Geolocation::Coordinate& rhs) const
    {
        return (abs (rhs.value - value) <=> .00001);
    }

    inline Geolocation::Latitude::Latitude (double d)
        : Coordinate (d)
    {
    }

    inline Geolocation::Latitude::Latitude (String s)
        : Coordinate (s, kLatitudeExp)
    {
    }

    inline Geolocation::Latitude::Latitude (const Latitude& rhs)
        : Coordinate (rhs.value)
    {
    }
    inline void Geolocation::Latitude::operator= (const Latitude& rhs)
    {
        value = rhs.value;
    }

    inline String Geolocation::Latitude::ToISOString ()
    {
        return ToISOString_ (L"%02d");
    }

    inline Geolocation::Longitude::Longitude (double d)
        : Coordinate (d)
    {
    }
    inline Geolocation::Longitude::Longitude (String s)
        : Coordinate (s, kLongitudeExp_)
    {
    }

    inline Geolocation::Longitude::Longitude (const Longitude& rhs)
        : Coordinate (rhs.value)
    {
    }

    inline void Geolocation::Longitude::operator= (const Longitude& rhs)
    {
        value = rhs.value;
    }

    inline String Geolocation::Longitude::ToISOString ()
    {
        return ToISOString_ (L"%03d");
    }

    inline Geolocation::Geolocation (Latitude _latitude, Longitude _longitude, double _altitude)
        : latitude (_latitude)
        , longitude (_longitude)
        , altitude (_altitude)
    {
    }

    inline Geolocation::Geolocation (Latitude _latitude, Geolocation::Longitude _longitude)
        : latitude (_latitude)
        , longitude (_longitude)
    {
    }

    inline Geolocation::Geolocation (String _latitude, String _longitude)
        : latitude (_latitude)
        , longitude (_longitude)
    {
    }

    inline Geolocation::Geolocation (String _latitude, String _longitude, double _altitude)
        : latitude (_latitude)
        , longitude (_longitude)
        , altitude (_altitude)
    {
    }

}

#endif /*__IPAM_LibIPAM_Common_Geolocation_inl__*/
