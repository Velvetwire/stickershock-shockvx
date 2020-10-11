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

            } handle;

          struct {                                                              // Characteristic values:

            float                     value;                                    //  Measured value
            float                     lower;                                    //  Lower limits
            float                     upper;                                    //  Upper limits

            } value;

          } surface_t;

static    unsigned                    surface_event ( surface_t * surface, ble_evt_t * event );
static    unsigned                    surface_write ( surface_t * surface, unsigned short connection, ble_gatts_evt_write_t * write );

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

//=============================================================================
#endif

