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
// Surface GATT service
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

            } handle;

          struct {                                                              // Characteristic values:

            atmosphere_values_t       value;                                    //  Measured values
            atmosphere_values_t       lower;                                    //  Lower limits
            atmosphere_values_t       upper;                                    //  Upper limits

            } value;

          } atmosphere_t;

static    unsigned                    atmosphere_event ( atmosphere_t * atmosphere, ble_evt_t * event );
static    unsigned                    atmosphere_write ( atmosphere_t * atmosphere, unsigned short connection, ble_gatts_evt_write_t * write );

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

//=============================================================================
#endif

