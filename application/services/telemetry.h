//=============================================================================
// project: ShockVx
//  module: Stickershock Sensor for cold chain tracking.
//  author: Velvetwire, llc
//    file: telemetry.h
//
// Telemetry control and settings service.
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
            ble_gatts_char_handles_t  archival;                                 //  Archival characteristic

            } handle;

          struct {                                                              // Characteristic values:

            float                     interval;                                 //  Measurement interval (seconds)
            float                     archival;                                 //  Archive interval (seconds)

            } value;

          } telemetry_t;

static    unsigned                    telemetry_event ( telemetry_t * telemetry, ble_evt_t * event );
static    unsigned                    telemetry_write ( telemetry_t * telemetry, unsigned short connection, ble_gatts_evt_write_t * write );

//-----------------------------------------------------------------------------
// Measurement interval characteristic
//-----------------------------------------------------------------------------

#define   TELEMETRY_INTERVAL_UUID     (0x54654D69)                              // 32-bit characteristic UUID component (TeMi)

static    unsigned                    telemetry_interval_characteristic ( telemetry_t * telemetry, float period );

//-----------------------------------------------------------------------------
// Archive interval characteristic
//-----------------------------------------------------------------------------

#define   TELEMETRY_ARCHIVAL_UUID     (0x54654169)                              // 32-bit characteristic UUID component (TeAi)

static    unsigned                    telemetry_archival_characteristic ( telemetry_t * telemetry, float period );

//=============================================================================
#endif

