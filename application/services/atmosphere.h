//=============================================================================
// project: Shockvx
//  module: Stickershock Sensor for cold chain tracking.
//  author: Velvetwire, llc
//    file: atmosphere.h
//
// Atmospheric telemetry and settings service.
//
// (c) Copyright 2016-2020 Velvetwire, LLC. All rights reserved.
//=============================================================================

#ifndef   __ATMOSPHERE__
#define   __ATMOSPHERE__

//-----------------------------------------------------------------------------
// Atmospheric telemetry event archvie
//-----------------------------------------------------------------------------

#define   ATMOSPHERE_ARCHIVE          "internal:archive/atmosphere.rec"         // Atmospheric archive file

typedef   struct __attribute__ (( packed )) {                                   // Atmospheric archive record
          
          unsigned                    time;                                     //  UTC time stamp
          
          struct __attribute__ (( packed )) {                                   //  Event data:
            signed short              temperature;                              //   Temperature value (1/100 Degree Celsius)
            signed short              humidity;                                 //   Humidity value (1/100 percent)
            signed short              pressure;                                 //   Pressure value (millibars)
            } data;

          } atmosphere_record_t;

//-----------------------------------------------------------------------------
// Atmospheric telemetry GATT service
//-----------------------------------------------------------------------------

#define   ATMOSPHERE_SERVICES_UUID    (0x41740000)                              // 32-bit service UUID component (At00)

static    const void *                atmosphere_id ( unsigned service );

//-----------------------------------------------------------------------------
// Service resource
//-----------------------------------------------------------------------------

typedef   struct {
          
          CTL_MUTEX_t                 mutex;                                    // Access mutex
          unsigned short              service;                                  // Service handle

          struct {                                                              // Characteristic handles:

            ble_gatts_char_handles_t  value;                                    //  Value characteristic
            ble_gatts_char_handles_t  lower;                                    //  Lower limit characteristic
            ble_gatts_char_handles_t  upper;                                    //  Upper limit characteristic

            ble_gatts_char_handles_t  event;                                    //  Archived event data (or index)
            ble_gatts_char_handles_t  count;                                    //  Record count

            } handle;

          struct {                                                              // Characteristic values:

            atmosphere_values_t       value;                                    //  Measured values
            atmosphere_values_t       lower;                                    //  Lower limits
            atmosphere_values_t       upper;                                    //  Upper limits

            atmosphere_record_t       event;                                    //  Archived event data (or index)
            unsigned short            count;                                    //  Record count

            } value;

          } atmosphere_t;

static    unsigned                    atmosphere_event ( atmosphere_t * atmosphere, ble_evt_t * event );

static    unsigned                    atmosphere_start ( atmosphere_t * atmosphere, unsigned short connection, ble_gap_evt_connected_t * connected );
static    unsigned                    atmosphere_write ( atmosphere_t * atmosphere, unsigned short connection, ble_gatts_evt_write_t * write );
static    unsigned                    atmosphere_fetch ( atmosphere_t * atmosphere, unsigned short index );

//-----------------------------------------------------------------------------
// Measurement value characteristic
//-----------------------------------------------------------------------------

#define   ATMOSPHERE_VALUE_UUID       (0x41744D76)                              // 32-bit characteristic UUID component (AtMv)

static    unsigned                    atmosphere_value_characteristic ( atmosphere_t * atmosphere );

//-----------------------------------------------------------------------------
// Value limits characteristic
//-----------------------------------------------------------------------------

#define   ATMOSPHERE_LOWER_UUID       (0x41744C6C)                              // 32-bit characteristic UUID component (AtLl)
#define   ATMOSPHERE_UPPER_UUID       (0x4174556C)                              // 32-bit characteristic UUID component (AtUl)

static    unsigned                    atmosphere_lower_characteristic ( atmosphere_t * atmosphere );
static    unsigned                    atmosphere_upper_characteristic ( atmosphere_t * atmosphere );

//-----------------------------------------------------------------------------
// Archived event record characteristics
//-----------------------------------------------------------------------------

#define   ATMOSPHERE_COUNT_UUID       (0x41745263)                              // 32-bit characteristic UUID component (AtRc)
#define   ATMOSPHERE_EVENT_UUID       (0x41745265)                              // 32-bit characteristic UUID component (AtRe)

static    unsigned                    atmosphere_count_characteristic ( atmosphere_t * atmosphere );
static    unsigned                    atmosphere_event_characteristic ( atmosphere_t * atmosphere );

//=============================================================================
#endif

