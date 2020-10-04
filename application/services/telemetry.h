//=============================================================================
// project: ShockVx
//  module: Stickershock Sensor for cold chain tracking.
//  author: Velvetwire, llc
//    file: telemetry.h
//
// Telemetry metrics and settings service.
//
// (c) Copyright 2016-2020 Velvetwire, LLC. All rights reserved.
//=============================================================================

#ifndef   __TELEMETRY__
#define   __TELEMETRY__

//-----------------------------------------------------------------------------
// Surface GATT service
//-----------------------------------------------------------------------------

#define   TELEMETRY_SERVICES_UUID     (0x54650000)                              // 32-bit service UUID component (Te00)

static    const void *                telemetry_id ( unsigned service );

//-----------------------------------------------------------------------------
// Service resource
//-----------------------------------------------------------------------------

typedef   struct {
          
          CTL_MUTEX_t                 mutex;                                    // Access mutex
          unsigned short              service;                                  // Service handle

          struct {                                                              // Characteristic handles:

            ble_gatts_char_handles_t  interval;                                 //  Interval characteristic

            ble_gatts_char_handles_t  value;                                    //  Value characteristic
            ble_gatts_char_handles_t  lower;                                    //  Lower limit characteristic
            ble_gatts_char_handles_t  upper;                                    //  Upper limit characteristic

            } handle;

          struct {                                                              // Characteristic values:

            float                     interval;                                 //  Measurement interval (seconds)

            telemetry_values_t        value;                                    //  Measured values
            telemetry_values_t        lower;                                    //  Lower limits
            telemetry_values_t        upper;                                    //  Upper limits

            } value;

          } telemetry_t;

static    unsigned                    telemetry_event ( telemetry_t * telemetry, ble_evt_t * event );
static    unsigned                    telemetry_write ( telemetry_t * telemetry, unsigned short connection, ble_gatts_evt_write_t * write );

//-----------------------------------------------------------------------------
// Measurement interval characteristic
//-----------------------------------------------------------------------------

#define   TELEMETRY_INTERVAL_UUID     (0x54654D69)                              // 32-bit characteristic UUID component (TeMi)

static    unsigned                    telemetry_interval_characteristic ( telemetry_t * telemetry );

//-----------------------------------------------------------------------------
// Measurement value characteristic
//-----------------------------------------------------------------------------

#define   TELEMETRY_VALUE_UUID        (0x54654D76)                              // 32-bit characteristic UUID component (TeMv)

static    unsigned                    telemetry_value_characteristic ( telemetry_t * telemetry );

//-----------------------------------------------------------------------------
// Value limits characteristic
//-----------------------------------------------------------------------------

#define   TELEMETRY_LOWER_UUID        (0x54654C6C)                              // 32-bit characteristic UUID component (TeLl)
#define   TELEMETRY_UPPER_UUID        (0x5465556C)                              // 32-bit characteristic UUID component (TeUl)

static    unsigned                    telemetry_lower_characteristic ( telemetry_t * telemetry );
static    unsigned                    telemetry_upper_characteristic ( telemetry_t * telemetry );

//=============================================================================
#endif

