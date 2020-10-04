//=============================================================================
// project: ShockVx
//  module: Stickershock firmware for cold chain tracking.
//  author: Velvetwire, llc
//    file: peripheral.h
//
// Bluetooth peripheral for controlling and commisioning the sensor.
//
// (c) Copyright 2016-2020 Velvetwire, LLC. All rights reserved.
//=============================================================================

#ifndef   __PERIPHERAL__
#define   __PERIPHERAL__

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define   PERIPHERAL_CLOSE_TIMEOUT    1000

#define   PERIPHERAL_POWER_MAXIMUM    (4)                                       // Maximum power setting is 4db
#define   PERIPHERAL_INTERVAL_MINIMUM ((float) 20e-3)                           // Minimum interval is 20ms

//-----------------------------------------------------------------------------
// Central manager resource
//-----------------------------------------------------------------------------

#define   PERIPHERAL_MANAGER_STACK    512                                       // Thread stack size in bytes
#define   PERIPHERAL_MANAGER_PRIORITY (CTL_TASK_PRIORITY_STANDARD + 2)          // Thread priority

typedef   struct {

          CTL_MUTEX_t                 mutex;                                    // Access mutex
          CTL_EVENT_SET_t             status;                                   // State and event bits
          CTL_NOTICE_t                notice [ PERIPHERAL_NOTICES ];            // Module notices

          struct {                                                              // Broadcast settings:

            float                     interval;                                 //  Broadcast interval
            float                     period;                                   //  Advertisement period
            unsigned char             flags;                                    //  Advertisement flags
            signed char               power;                                    //  Power settings

            } broadcast;

          struct {                                                              // Peripheral advertisement:

            softble_advertisement_t * data;                                     //  Advertisement data packet
            softble_advertisement_t * scan;                                     //  Scan response data packet

            } advertisement;

          } peripheral_t;

static    void                        peripheral_manager ( peripheral_t * peripheral );

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define   PERIPHERAL_MANAGER_EVENTS   (0x4000ffff)
#define   PERIPHERAL_MANAGER_STATES   (0xbfff0000)

#define   PERIPHERAL_STATE_CLOSED     (1 << 31)                                 // Module has been closed
#define   PERIPHERAL_EVENT_SHUTDOWN   (1 << 30)                                 // Request module shutdown

static    void                        peripheral_shutdown ( peripheral_t * peripheral );

#define   PERIPHERAL_STATE_ACTIVE     (1 << 29)                                 // Actively advertising
#define   PERIPHERAL_STATE_PACKET     (1 << 28)                                 // Broadcast packet loaded
#define   PERIPHERAL_STATE_PERIOD     (1 << 27)                                 // Broadcast period defined
#define   PERIPHERAL_STATE_LINKED     (1 << 26)                                 // Currently linked to peer

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define   PERIPHERAL_EVENT_CONFIGURE  (1 << 15)                                 // Configure the broadcast
#define   PERIPHERAL_EVENT_CONSTRUCT  (1 << 14)                                 // Construct the broadcast
#define   PERIPHERAL_EVENT_BROADCAST  (1 << 13)                                 // Start the advertisement

static    void                        peripheral_configure ( peripheral_t * peripheral );
static    void                        peripheral_construct ( peripheral_t * peripheral );
static    void                        peripheral_broadcast ( peripheral_t * peripheral );

#define   PERIPHERAL_EVENT_ADVERTISE  (1 << 12)                                 // Started advertising
#define   PERIPHERAL_EVENT_TERMINATE  (1 << 11)                                 // Ceased advertising
#define   PERIPHERAL_EVENT_INSPECTED  (1 << 10)                                 // Packet inspected

static    void                        peripheral_advertise ( peripheral_t * peripheral );
static    void                        peripheral_terminate ( peripheral_t * peripheral );
static    void                        peripheral_inspected ( peripheral_t * peripheral );

#define   PERIPHERAL_EVENT_BEGIN      (PERIPHERAL_EVENT_CONFIGURE | PERIPHERAL_EVENT_CONSTRUCT | PERIPHERAL_EVENT_BROADCAST)
#define   PERIPHERAL_CLEAR_BEGIN      (PERIPHERAL_EVENT_ADVERTISE | PERIPHERAL_EVENT_TERMINATE | PERIPHERAL_STATE_PERIOD | PERIPHERAL_STATE_PACKET | PERIPHERAL_STATE_ACTIVE)
#define   PERIPHERAL_CLEAR_CEASE      (PERIPHERAL_STATE_PACKET)

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define   PERIPHERAL_EVENT_ATTACHED   (1 << 9)                                  // Construct the advertisement broadcast
#define   PERIPHERAL_EVENT_DETACHED   (1 << 8)                                  // Construct the advertisement broadcast

static    void                        peripheral_attached ( peripheral_t * peripheral );
static    void                        peripheral_detached ( peripheral_t * peripheral );

//=============================================================================
#endif
