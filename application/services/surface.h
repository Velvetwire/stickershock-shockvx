//=============================================================================
// project: Shockvx
//  module: Stickershock Sensor for cold chain tracking.
//  author: Velvetwire, llc
//    file: surface.h
//
// Atmospheric telemetry and settings service.
//
// (c) Copyright 2016-2020 Velvetwire, LLC. All rights reserved.
//=============================================================================

#ifndef   __SURFACE__
#define   __SURFACE__

//-----------------------------------------------------------------------------
// Surface temperature event archvie
//-----------------------------------------------------------------------------

#define   SURFACE_ARCHIVE             "internal:archive/surface.rec"            // Surface temperature archive file

typedef   struct __attribute__ (( packed )) {                                   // Surface temperature archive record
          
          unsigned                    time;                                     //  UTC time stamp
          
          struct __attribute__ (( packed )) {                                   //  Event data:
            signed short              temperature;                              //   Temperature value (1/100 Degree Celsius)
            } data;

          } surface_record_t;

//-----------------------------------------------------------------------------
// Surface GATT service
//-----------------------------------------------------------------------------

#define   SURFACE_SERVICES_UUID       (0x53740000)                              // 32-bit service UUID component (St00)

static    const void *                surface_id ( unsigned service );

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

            float                     value;                                    //  Measured value
            float                     lower;                                    //  Lower limits
            float                     upper;                                    //  Upper limits

            surface_record_t          event;                                    //  Archived event data (or index)
            unsigned short            count;                                    //  Record count

            } value;

          struct {                                                              // Compliance values:

            surface_compliance_t      incursion;                                //  Time within compliance
            surface_compliance_t      excursion;                                //  Time outside compliance

            } compliance;

          } surface_t;

static    unsigned                    surface_event ( surface_t * surface, ble_evt_t * event );

static    unsigned                    surface_start ( surface_t * surface, unsigned short connection, ble_gap_evt_connected_t * connected );
static    unsigned                    surface_write ( surface_t * surface, unsigned short connection, ble_gatts_evt_write_t * write );
static    unsigned                    surface_fetch ( surface_t * surface, unsigned short index );

//-----------------------------------------------------------------------------
// Measurement value characteristic
//-----------------------------------------------------------------------------

#define   SURFACE_VALUE_UUID          (0x53744D76)                              // 32-bit characteristic UUID component (StMv)

static    unsigned                    surface_value_characteristic ( surface_t * surface );

//-----------------------------------------------------------------------------
// Value limits characteristic
//-----------------------------------------------------------------------------

#define   SURFACE_LOWER_UUID          (0x53744C6C)                              // 32-bit characteristic UUID component (StLl)
#define   SURFACE_UPPER_UUID          (0x5374556C)                              // 32-bit characteristic UUID component (StUl)

static    unsigned                    surface_lower_characteristic ( surface_t * surface, float value );
static    unsigned                    surface_upper_characteristic ( surface_t * surface, float value );


//-----------------------------------------------------------------------------
// Archived event record characteristics
//-----------------------------------------------------------------------------

#define   SURFACE_COUNT_UUID        (0x53745263)                                // 32-bit characteristic UUID component (AtRc)
#define   SURFACE_EVENT_UUID        (0x53745265)                                // 32-bit characteristic UUID component (AtRe)

static    unsigned                    surface_count_characteristic ( surface_t * surface );
static    unsigned                    surface_event_characteristic ( surface_t * surface );

//=============================================================================
#endif

