/*
 * Copyright(c) Sophist Solutions, Inc. 2022.  All rights reserved
 */
#ifndef __IPAM_LibIPAM_Common_Geolocation_h__
#define __IPAM_LibIPAM_Common_Geolocation_h__ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/RegularExpression.h"

namespace IPAM::LibIPAM::Common {

    using namespace Stroika::Foundation;
    using Characters::String;

    /**
     */
    class Geolocation {
    public:
        class Coordinate {
        public:
            Coordinate () = delete;

            static double GPSCoordStringToValue (const String& coor);

        protected:
            Coordinate (double d);
            Coordinate (const String& s, const Characters::RegularExpression& regex);

        public:
            nonvirtual int    degrees () const;
            nonvirtual int    minutes () const;
            nonvirtual double seconds () const;

        public:
            nonvirtual bool operator== (const Coordinate& rhs) const;
            nonvirtual auto operator<=> (const Coordinate& rhs) const;

        protected:
            static constexpr double _kBase      = 60.0;
            static constexpr double _kPrecision = 100000.0;
            double                  _value;

            nonvirtual String ToISOString_ (const wchar_t* degreeSpecification);
        };

        /**
         */
        class Latitude : public Coordinate {
        public:
            Latitude (double d = 0);
            Latitude (const String& s);
            Latitude (const Latitude& rhs);

            nonvirtual void   operator= (const Latitude& rhs);
            nonvirtual String ToISOString ();

        private:
            // ISO 6709
            // sign followed by 2,4 or 6 digits
            // followed by optional decimal point and further digits
            // ^([+-])([0-9]{2})([0-9]{2})?([0-9]{2})?(\.[0-9]+)?
            static inline Characters::RegularExpression kLatitudeExp_{L"^([+-])([0-9]{2})([0-9]{2})?([0-9]{2})?(\\.[0-9]+)?"};

#if qDebug
        public:
            static void TestSuite ();
#endif
        };

        /**
         */
        class Longitude : public Coordinate {
        public:
            Longitude (double d = 0);
            Longitude (const String& s);
            Longitude (const Longitude& rhs);

            nonvirtual void operator= (const Longitude& rhs);

            nonvirtual String ToISOString ();

        private:
            // ISO 6709
            // sign followed by 3,5 or 7 digits
            // followed by optional decimal point and further digits
            // ^([+-])([0-9]{2})([0-9]{2})?([0-9]{2})?(\.[0-9]+)?
            static inline Characters::RegularExpression kLongitudeExp_{L"^([+-])([0-9]{3})([0-9]{2})?([0-9]{2})?(\\.[0-9]+)?"};
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

        Geolocation (wchar_t* _latitude, wchar_t* _longitude, wchar_t* _altitude = nullptr);

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
        static inline Characters::RegularExpression kExp_{L"^(?:([+-][0-9]{2,6}(?:\\.[0-9]+)?)([+-][0-9]{3,7}(?:\\.[0-9]+)?))(?:([+-][0-9]+(?:\\.[0-9]+)?)(?:CRSWGS_84))?\\/"};
    };

}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "Geolocation.inl"

#endif /*__IPAM_LibIPAM_Common_Geolocation_h__*/
