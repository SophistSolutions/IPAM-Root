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
            /**
             *  \note comparisons not exact - but with within _kPrecision
             */
            nonvirtual bool operator== (const Coordinate& rhs) const;
            nonvirtual auto operator<=> (const Coordinate& rhs) const;

        protected:
            static constexpr double _kBase      = 60.0;
            static constexpr double _kPrecision = 1000000.0;
            double                  _value;

            nonvirtual String ToISOString_ (const wchar_t* degreeSpecification);
        };

        /**
         */
        class Latitude : public Coordinate {
        public:
            Latitude (double d = 0);
            Latitude (const String& s);
            Latitude (const Latitude& rhs) = default;

            nonvirtual Latitude& operator= (const Latitude& rhs) = default;
            nonvirtual String    ToISOString ();

        private:
            // ISO 6709
            // sign followed by 2,4 or 6 digits
            // followed by optional decimal point and further digits
            // ^([+-])([0-9]{2})([0-9]{2})?([0-9]{2})?(\.[0-9]+)?
            static inline Characters::RegularExpression kLatitudeExp_{"^([+-])([0-9]{2})([0-9]{2})?([0-9]{2})?(\\.[0-9]+)?"sv};

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
            Longitude (const Longitude& rhs) = default;

            nonvirtual Longitude& operator= (const Longitude& rhs) = default;

            nonvirtual String ToISOString ();

        private:
            // ISO 6709
            // sign followed by 3,5 or 7 digits
            // followed by optional decimal point and further digits
            // ^([+-])([0-9]{2})([0-9]{2})?([0-9]{2})?(\.[0-9]+)?
            static inline Characters::RegularExpression kLongitudeExp_{"^([+-])([0-9]{3})([0-9]{2})?([0-9]{2})?(\\.[0-9]+)?"sv};
#if qDebug
        public:
            static void TestSuite ();
#endif
        };

    public:
        /**
         */
        Geolocation (Latitude lat, Longitude lon, optional<double> alt = nullopt);
        Geolocation (const String& lat, const String& lon, const optional<String>& alt = nullopt);
        Geolocation (const String& isoString);

    public:
        /**
         */
        nonvirtual String ToISOString ();

    public:
        Latitude         latitude;
        Longitude        longitude;
        optional<double> altitude;

#if qDebug
    public:
        static void TestSuite ();
#endif
    private:
        static inline Characters::RegularExpression kExp_{
            "^(?:([+-][0-9]{2,6}(?:\\.[0-9]+)?)([+-][0-9]{3,7}(?:\\.[0-9]+)?))(?:([+-][0-9]+(?:\\.[0-9]+)?)(?:CRSWGS_84))?\\/"sv};
    };

}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "Geolocation.inl"

#endif /*__IPAM_LibIPAM_Common_Geolocation_h__*/
