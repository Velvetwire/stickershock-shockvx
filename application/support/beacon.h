//=============================================================================
// project: ShockVx
//  module: Stickershock firmware for cold chain tracking.
//  author: Velvetwire, llc
//    file: beacon.h
//
// Bluetooth beacon for controlling and commisioning the sensor.
//
// (c) Copyright 2016-2020 Velvetwire, LLC. All rights reserved.
//=============================================================================

#ifndef   __BEACON__
#define   __BEACON__

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define   BEACON_CLOSE_TIMEOUT        1000

#define   BEACON_POWER_HORIZON        (-61)                                     // Horizon power estimate at 1m (assuming 0dB)
#define   BEACON_POWER_MAXIMUM        (4)                                       // Maximum power setting is 4db

#define   BEACON_INTERVAL_MINIMUM     ((float) 20e-3)                           // Minimum interval is 20ms

//-----------------------------------------------------------------------------
// Central manager resource
//-----------------------------------------------------------------------------

#define   BEACON_MANAGER_STACK        512                                       // Thread stack size in bytes
#define   BEACON_MANAGER_PRIORITY     (CTL_TASK_PRIORITY_STANDARD + 1)          // Thread priority

typedef   struct {

          CTL_MUTEX_t                 mutex;                                    // Access mutex
          CTL_EVENT_SET_t             status;                                   // State and event bits
          CTL_NOTICE_t                notice [ BEACON_NOTICES ];                // Module notices

          struct {                                                              // Broadcast settings:

            float                     interval;                                 //  Broadcast interval
            float                     period;                                   //  Advertisement period

            unsigned char             flags;                                    //  Advertisement flags
            signed char               power;                                    //  Power settings

            beacon_type_t             type;                                     //  Beacon type (4x or 5x)
            
            } broadcast;

          struct {                                                              // Peripheral advertisement:

            softble_advertisement_t * data;                                     //  Advertisement data packet
            softble_advertisement_t * scan;                                     //  Scan response data packet

            } advertisement;

          struct {

            signed char               horizon;                                  // Power horizon
            signed char               battery;                                  // Battery level            
            broadcast_variant_t       variant;

            broadcast_temperature_t   temperature;
            broadcast_atmosphere_t    atmosphere;
            broadcast_handling_t      handling;

            } record;

          } beacon_t;

static    void                        beacon_manager ( beacon_t * beacon );

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define   BEACON_MANAGER_EVENTS       (0x4000ffff)
#define   BEACON_MANAGER_STATES       (0xbfff0000)

#define   BEACON_STATE_CLOSED         (1 << 31)                                 // Module has been closed
#define   BEACON_EVENT_SHUTDOWN       (1 << 30)                                 // Request module shutdown

static    void                        beacon_shutdown ( beacon_t * beacon );

#define   BEACON_STATE_ACTIVE         (1 << 29)                                 // Actively advertising
#define   BEACON_STATE_PACKET         (1 << 28)                                 // Broadcast packet loaded
#define   BEACON_STATE_PERIOD         (1 << 27)                                 // Broadcast period defined

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define   BEACON_EVENT_CONFIGURE      (1 << 15)                                 // Configure the broadcast
#define   BEACON_EVENT_CONSTRUCT      (1 << 14)                                 // Construct the broadcast
#define   BEACON_EVENT_BROADCAST      (1 << 13)                                 // Start the advertisement

static    void                        beacon_configure ( beacon_t * beacon );
static    void                        beacon_construct ( beacon_t * beacon );
static    void                        beacon_broadcast ( beacon_t * beacon );

#define   BEACON_EVENT_ADVERTISE      (1 << 12)                                 // Started advertising
#define   BEACON_EVENT_TERMINATE      (1 << 11)                                 // Ceased advertising
#define   BEACON_EVENT_INSPECTED      (1 << 10)                                 // Packet inspected

static    void                        beacon_advertise ( beacon_t * beacon );
static    void                        beacon_terminate ( beacon_t * beacon );
static    void                        beacon_inspected ( beacon_t * beacon );

#define   BEACON_EVENT_BEGIN          (BEACON_EVENT_CONFIGURE | BEACON_EVENT_CONSTRUCT | BEACON_EVENT_BROADCAST)
#define   BEACON_CLEAR_BEGIN          (BEACON_EVENT_ADVERTISE | BEACON_EVENT_TERMINATE | BEACON_STATE_PERIOD | BEACON_STATE_PACKET | BEACON_STATE_ACTIVE)
#define   BEACON_CLEAR_CEASE          (BEACON_STATE_PACKET)

//-----------------------------------------------------------------------------
// Beacon support for 4.x or 5.x advertisement broadcast configuration.
//-----------------------------------------------------------------------------

static    void                        beacon_configure_ble_4 ( beacon_t * beacon );
static    void                        beacon_configure_ble_5 ( beacon_t * beacon );

//-----------------------------------------------------------------------------
// Beacon support for 4.x or 5.x advertisement packet construction.
//-----------------------------------------------------------------------------

static    void                        beacon_construct_ble_4 ( beacon_t * beacon );
static    void                        beacon_construct_ble_5 ( beacon_t * beacon );

//=============================================================================
#endif
