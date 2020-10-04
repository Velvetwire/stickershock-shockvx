//=============================================================================
// project: ShockVx
//  module: Stickershock firmware for cold chain tracking.
//  author: Velvetwire, llc
//    file: broadcast.h
//
// Bluetooth broadcast packet encoding.
//
// (c) Copyright 2016-2020 Velvetwire, LLC. All rights reserved.
//=============================================================================

#ifndef   __BROADCAST__
#define   __BROADCAST__

//=============================================================================
// SECTION : BROADCAST PACKET ENCODINGS
//=============================================================================

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define   BROADCAST_PACKET_CODE       0x5657                                    // Manufacturer code 'VW'

//-----------------------------------------------------------------------------
// Broadcast packet data is encoded within a service data record using the same
// byte packing as a Bluetooth advertisement record.
//-----------------------------------------------------------------------------

#define   BROADCAST_PACKET_SIZE       252                                       // Maximum data payload permitted

typedef   struct __attribute__ (( packed )) {                                   // Broadcast packet:

          unsigned short              code;                                     //  Packet code
          unsigned char               data [ BROADCAST_PACKET_SIZE ];           //  Packet data

          } broadcast_packet_t;

          broadcast_packet_t *        broadcast_packet ( unsigned short code );
          unsigned char               broadcast_length ( broadcast_packet_t * packet );
          void                        broadcast_append ( broadcast_packet_t * packet, void * data, unsigned char size, unsigned char type );

//-----------------------------------------------------------------------------
// Each manufacturer record starts with two bytes indicating size in bytes and
// record type. The high order bit of the record type indicates that the type
// is a secure variant.
//-----------------------------------------------------------------------------

typedef   struct __attribute__ (( packed )) {                                    // Broadcast record:

           unsigned char              size;                                     //  Record size in bytes
           unsigned char              type;                                     //  Record type code

           } broadcast_record_t;

#define   BROADCAST_TYPE_NORMAL(t)    (t)                                       // Normal broadcast record
#define   BROADCAST_TYPE_SECURE(t)    (0x80 | t)                                // Secure broadcast record


//=============================================================================
// SECTION : BROADCAST IDENTITY AND NETWORK ENCODINGS
//=============================================================================

//-----------------------------------------------------------------------------
// The broadcast identity uniquely identifies the owner of the broadcast and,
// if secure, includes a security hash derived from the time code nonce. The
// signal horizon (in dB) and battery (in percent) elements are not secured.
//-----------------------------------------------------------------------------

#define   BROADCAST_TYPE_IDENTITY     0x01                                      // Identity code
#define   BROADCAST_SIZE_IDENTITY     (sizeof(broadcast_identity_t) + 1)

typedef   struct __attribute__ (( packed )) {                                   // Broadcast identity record:

          unsigned                    timecode;                                 //  Unit secure time-code nonce

          hash_t                      identity;                                 //  Identity code (64-bit)
          hash_t                      security;                                 //  Security hash (64-bit ignored if not secure)

          signed char                 horizon;                                  //  Signal horizon (standard dB at 1 meter)
          signed char                 battery;                                  //  Battery level (negative = charging)

          } broadcast_identity_t;

//-----------------------------------------------------------------------------
// The broadcast network notifies observers of the sub-network of the device.
//-----------------------------------------------------------------------------

#define   BROADCAST_TYPE_NETWORK      0x03                                      // Network code
#define   BROADCAST_SIZE_NETWORK      (sizeof(broadcast_network_t) + 1)

typedef   struct __attribute__ (( packed )) {                                   // Broadcast network record:

          hash_t                      identity;                                 //  Identity code (64-bit)

          } broadcast_network_t;

//-----------------------------------------------------------------------------
// The broadcast time code notifies observers of the current UTC epoch time.
//-----------------------------------------------------------------------------

#define   BROADCAST_TYPE_TIMECODE     0x05
#define   BROADCAST_SIZE_TIMECODE     (sizeof(broadcast_timecode_t) + 1)

typedef   struct __attribute__ (( packed )) {                                   // Broadcast time record:

          unsigned                    code;                                     //  UTC epoch time code

          } broadcast_timecode_t;

//-----------------------------------------------------------------------------
// The broadcast time code notifies observers of the current UTC epoch time.
//-----------------------------------------------------------------------------

#define   BROADCAST_TYPE_VARIANT      0x07
#define   BROADCAST_SIZE_VARIANT      (sizeof(broadcast_variant_t) + 1)

typedef   struct __attribute__ (( packed )) {                                   // Broadcast variant record:

          unsigned short              type;                                     //  variant type code

          } broadcast_variant_t;


//=============================================================================
// SECTION : BROADCAST POSITION ENCODINGS
//=============================================================================

//-----------------------------------------------------------------------------
// The broadcast position comes in two variants, determined by size, one which
// only includes two-dimensional position and one three-dimensional position
//-----------------------------------------------------------------------------

#define   BROADCAST_TYPE_POSITION     0x11
#define   BROADCAST_SIZE_POSITION_2D  ((2 * sizeof(float)) + 1)
#define   BROADCAST_SIZE_POSITION_3D  ((3 * sizeof(float)) + 1)

typedef   struct __attribute__ (( packed )) {                                   // Broadcast position record:

          float                       latitude;                                 //  Latitudinal position (in degrees)
          float                       longitude;                                //  Longitudinal position (in degrees)
          float                       altitude;                                 //  Altitude (in meters. 3D only)

          } broadcast_position_t;

//-----------------------------------------------------------------------------
// The broadcast location describes a fixed installation.
//-----------------------------------------------------------------------------

#define   BROADCAST_TYPE_LOCATION     0x13
#define   BROADCAST_SIZE_LOCATION     (sizeof(broadcast_location_t) + 1)

typedef   struct __attribute__ (( packed )) {                                   // Broadcast location record:

          unsigned char               campus;
          unsigned char               building;
          unsigned char               floor;
          unsigned char               zone;
  
          } broadcast_location_t;

//-----------------------------------------------------------------------------
// The broadcast coordinate record can be used as relative positioning from a
// known origin.
//-----------------------------------------------------------------------------

#define   BROADCAST_TYPE_COORDINATE   0x15
#define   BROADCAST_SIZE_COORDINATE   (sizeof(broadcast_coordinate_t) + 1)

typedef   struct __attribute__ (( packed )) {                                   // Broadcast coordinate record:

          float                       x;                                        //  x distance in meters
          float                       y;                                        //  y distance in meters
          float                       z;                                        //  z distance in meters

          } broadcast_coordinate_t;


//=============================================================================
// SECTION : BROADCAST TELEMETRY ENCODINGS
//=============================================================================

//-----------------------------------------------------------------------------
// Surface temperature record.
//-----------------------------------------------------------------------------

#define   BROADCAST_TYPE_TEMPERATURE  0x21
#define   BROADCAST_SIZE_TEMPERATURE  (sizeof(broadcast_temperature_t) + 1)

typedef   struct __attribute__ (( packed )) {                                   // Broadcast measurement record:

          signed short                measurement;                              // Recent measurement (degrees Celsius / 100)
          unsigned short              incursion;                                // Time inside limits (minutes)
          unsigned short              excursion;                                // Time outside limits (minutes)

          } broadcast_measurement_t;

typedef   broadcast_measurement_t     broadcast_temperature_t;                  // Broadcast temperature record

//-----------------------------------------------------------------------------
// Atmospherics records.
//-----------------------------------------------------------------------------

#define   BROADCAST_TYPE_ATMOSPHERE   0x23
#define   BROADCAST_SIZE_ATMOSPHERE   (sizeof(broadcast_atmosphere_t) + 1)

typedef   struct __attribute__ (( packed )) {                                   // Broadcast atmosphere record:

          broadcast_measurement_t     temperature;                              //  Air temperature
          broadcast_measurement_t     humidity;                                 //  Air humidity level
          broadcast_measurement_t     pressure;                                 //  Air pressure

          } broadcast_atmosphere_t;


//=============================================================================
// SECTION : BROADCAST HANDLING ENCODINGS
//=============================================================================

//-----------------------------------------------------------------------------
// Broadcast handling record summarizes product handling.
//-----------------------------------------------------------------------------

#define   BROADCAST_TYPE_HANDLING     0x31
#define   BROADCAST_SIZE_HANDLING     (sizeof(broadcast_handling_t) + 1)

typedef   struct __attribute__ (( packed )) {                                   // Broadcast handling record:

          unsigned char               orientation;                              //  Orientation code
          signed char                 angle;                                    //  Tilt angle (+/- 90)

          } broadcast_handling_t;

#define   BROADCAST_ORIENTATION_FACE  (1 << 7)                                  //  Orientation alert flag
#define   BROADCAST_ORIENTATION_DROP  (1 << 6)                                  //  Drop alert flag
#define   BROADCAST_ORIENTATION_BUMP  (1 << 5)                                  //  Bump alert flag
#define   BROADCAST_ORIENTATION_TILT  (1 << 4)                                  //  Tilt alert flag
#define   BROADCAST_ORIENTATION_ANGLE (1 << 3)                                  //  Valid angle flag

#define   BROADCAST_HANDLING_ORIENTED (o & BROADCAST_ORIENTATION_FACE)          //  Mis-orientation detected
#define   BROADCAST_HANDLING_DROPPED  (o & BROADCAST_ORIENTATION_DROP)          //  Drop detected
#define   BROADCAST_HANDLING_BUMPED   (o & BROADCAST_ORIENTATION_BUMP)          //  Excessive g-force detected
#define   BROADCAST_HANDLING_TIPPED   (o & BROADCAST_ORIENTATION_TILT)          //  Excessive tipping
#define   BROADCAST_HANDLING_ANGLE    (o & BROADCAST_ORIENTATION_ANGLE)         //  Angle is valid

#define   BROADCAST_HANLDING_FACE(o)  (o & 3)                                   //  Face ordinal (0 = unknown)

//=============================================================================
#endif
