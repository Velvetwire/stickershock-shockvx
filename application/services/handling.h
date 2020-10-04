//=============================================================================
// project: ShockVx
//  module: Stickershock Sensor for cold chain tracking.
//  author: Velvetwire, llc
//    file: handling.h
//
// Hanlding and abuse service.
//
// (c) Copyright 2016-2020 Velvetwire, LLC. All rights reserved.
//=============================================================================

#ifndef   __HANDLING__
#define   __HANDLING__

//-----------------------------------------------------------------------------
// Handling and abuse GATT service
//-----------------------------------------------------------------------------

#define   HANDLING_SERVICES_UUID      (0x48610000)                              // 32-bit service UUID component (Ha00)

static    const void *                handling_id ( unsigned service );

//-----------------------------------------------------------------------------
// Service resource
//-----------------------------------------------------------------------------

typedef   struct {
          
          CTL_MUTEX_t                 mutex;                                    // Access mutex
          unsigned short              service;                                  // Service handle

          struct {                                                              // Characteristic handles:

            ble_gatts_char_handles_t  value;                                    //  Handling values
            ble_gatts_char_handles_t  limit;                                    //  Handling limits

            } handle;

          struct {                                                              // Characteristic values:

            handling_values_t         value;                                    //  Handling values
            handling_values_t         limit;                                    //  Handling limits

            } value;

          } handling_t;

static    unsigned                    handling_event ( handling_t * handling, ble_evt_t * event );
static    unsigned                    handling_write ( handling_t * handling, unsigned short connection, ble_gatts_evt_write_t * write );

//-----------------------------------------------------------------------------
// Measurement value and limit characteristics
//-----------------------------------------------------------------------------

#define   HANDLING_VALUE_UUID         (0x48614D76)                              // 32-bit characteristic UUID component (HaMv)
#define   HANDLING_LIMIT_UUID         (0x48614C76)                              // 32-bit characteristic UUID component (HaLv)

static    unsigned                    handling_value_characteristic ( handling_t * handling );
static    unsigned                    handling_limit_characteristic ( handling_t * handling );

//=============================================================================
#endif

