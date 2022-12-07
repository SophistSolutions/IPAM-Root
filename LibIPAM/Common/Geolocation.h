/*
 * Copyright(c) Sophist Solutions, Inc. 2022.  All rights reserved
 */
#ifndef __IPAM_LibIPAM_Common_Geolocation_h__
#define __IPAM_LibIPAM_Common_Geolocation_h__ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/RegularExpression.h"
#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Debug/Trace.h"

using namespace Stroika::Foundation;
using Characters::String;

namespace IPAM::LibIPAM::Common {
    class Geolocation {
    public:
        class Coordinate {
        public:
            Coordinate () = delete;

            static double GPSCoordStringToValue (String coor);

        protected:
            Coordinate (double d);
            Coordinate (String s, const Characters::RegularExpression regex);

        public:
            nonvirtual int    degrees () const;
            nonvirtual int    minutes () const;
            nonvirtual double seconds () const;

            nonvirtual bool operator== (const Coordinate& rhs) const;
            nonvirtual bool operator!= (const Coordinate& rhs) const;
            nonvirtual bool operator>= (const Coordinate& rhs) const;
            nonvirtual bool operator> (const Coordinate& rhs) const;
            nonvirtual bool operator<= (const Coordinate& rhs) const;
            nonvirtual bool operator<(const Coordinate& rhs) const;

        protected:
            const double kBase      = 60.0;
            const double kPrecision = 100000.0;
            double       value;

            nonvirtual String ToISOString_ (const wchar_t* degreeSpecification);
        };

        class Latitude : public Coordinate {
        public:
            Latitude (double d = 0);
            Latitude (String s);
            Latitude (const Latitude& rhs);

            nonvirtual void   operator= (const Latitude& rhs);
            nonvirtual String ToISOString ();

        private:
            // ISO 6709
            // sign followed by 2,4 or 6 digits
            // followed by optional decimal point and further digits
            // ^([+-])([0-9]{2})([0-9]{2})?([0-9]{2})?(\.[0-9]+)?
            static inline Characters::RegularExpression kLatitudeExp{L"^([+-])([0-9]{2})([0-9]{2})?([0-9]{2})?(\\.[0-9]+)?"};

#if qDebug
        public:
            static void TestSuite ();
#endif
        };

        class Longitude : public Coordinate {
        public:
            Longitude (double d = 0);
            Longitude (String s);
            Longitude (const Longitude& rhs);

            nonvirtual void operator= (const Longitude& rhs);

            nonvirtual String ToISOString ();

        private:
            // ISO 6709
            // sign followed by 3,5 or 7 digits
            // followed by optional decimal point and further digits
            // ^([+-])([0-9]{2})([0-9]{2})?([0-9]{2})?(\.[0-9]+)?
            static inline Characters::RegularExpression kLongitudeExp{L"^([+-])([0-9]{3})([0-9]{2})?([0-9]{2})?(\\.[0-9]+)?"};
#if qDebug
        public:
            static void TestSuite ();
#endif
        };

    public:
        Geolocation (Latitude _latitude, Longitude _longitude, double _altitude);
        Geolocation (Latitude _latitude, Longitude _longitude);
        Geolocation (String _latitude, String _longitude);
        Geolocation (String _latitude, String _longitude, double _altitude);

        Geolocation (wchar_t* _latitude, wchar_t* _longitude, wchar_t* _altitude = NULL);

        Geolocation (String isoString);

        nonvirtual String ToISOString ();

        Latitude         latitude;
        Longitude        longitude;
        optional<double> altitude;

#if qDebug
    public:
        static void TestSuite ();
#endif
    private:
        static inline Characters::RegularExpression kExp{L"^(?:([+-][0-9]{2,6}(?:\\.[0-9]+)?)([+-][0-9]{3,7}(?:\\.[0-9]+)?))(?:([+-][0-9]+(?:\\.[0-9]+)?)(?:CRSWGS_84))?\\/"};
    };

    /*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
    inline bool Geolocation::Coordinate::operator== (const Geolocation::Coordinate& rhs) const
    {
        return (abs (rhs.value - value) <= .00001);
    }

    inline bool Geolocation::Coordinate::operator!= (const Geolocation::Coordinate& rhs) const
    {
        return (abs (rhs.value - value) > .00001);
    }
    inline bool Geolocation::Coordinate::operator>= (const Geolocation::Coordinate& rhs) const
    {
        return value >= rhs.value;
    }
    inline bool Geolocation::Coordinate::operator> (const Geolocation::Coordinate& rhs) const
    {
        return !operator== (rhs) and value > rhs.value;
    }
    inline bool Geolocation::Coordinate::operator<= (const Geolocation::Coordinate& rhs) const
    {
        return value <= rhs.value;
    }
    inline bool Geolocation::Coordinate::operator<(const Geolocation::Coordinate& rhs) const
    {
        return !operator== (rhs) and value < rhs.value;
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
        : Coordinate (s, kLongitudeExp)
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

} // namespace

#endif /*__IPAM_LibIPAM_Common_Geolocation_h__*/
