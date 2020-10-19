//=============================================================================
// project: ShockVx
//  module: Stickershock Sensor for cold chain tracking.
//  author: Velvetwire, llc
//    file: movement.h
//
//
// (c) Copyright 2016-2020 Velvetwire, LLC. All rights reserved.
//=============================================================================

#include  "shockvx.h"

#ifndef   __MOVEMENT__
#define   __MOVEMENT__

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define   MOVEMENT_CLOSE_TIMEOUT       1000

//-----------------------------------------------------------------------------
// Telemetry manager resource
//-----------------------------------------------------------------------------

#define   MOVEMENT_MANAGER_STACK      768                                       // Thread stack size in bytes
#define   MOVEMENT_MANAGER_PRIORITY   (CTL_TASK_PRIORITY_STANDARD + 7)          // Thread priority

typedef   struct {
          
          CTL_MUTEX_t                 mutex;                                    // Access mutex
          CTL_EVENT_SET_t             option;                                   // Option bits
          CTL_EVENT_SET_t             status;                                   // State and event bits
          CTL_NOTICE_t                notice [ MOVEMENT_NOTICES ];              // Module notices

          CTL_TIME_t                  period;                                   // Measurment period (milliseconds)
          float                       temperature;                              // Surface temperature

          struct {                                                              // Motion sensor vectors:
            motion_angular_vectors_t  angular;                                  //  Angular rotation vector
            motion_linear_vectors_t   linear;                                   //  Linear acceleration vector
            } vectors;

          struct {                                                              // Motion sensor computations:
            float                     value;                                    //  Force experienced
            float                     limit;                                    //  Force limit experienced
            } force;

          struct {                                                              // Motion sensor angles:
            float                     value;                                    //  Angle computation
            float                     limit;                                    //  Angle tilt limit
            } angle;

          unsigned char               orientation;                              // Orientation face

          } movement_t;

static    void                        movement_manager ( movement_t * movement );

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define   MOVEMENT_MANAGER_EVENTS     (0x7000ffff)
#define   MOVEMENT_MANAGER_STATES     (0x8fff0000)

#define   MOVEMENT_STATE_CLOSED       (1 << 31)                                 // Module has been closed
#define   MOVEMENT_EVENT_SHUTDOWN     (1 << 30)                                 // Request module shutdown
#define   MOVEMENT_EVENT_SETTINGS     (1 << 29)                                 // Configure settings
#define   MOVEMENT_EVENT_STANDBY      (1 << 28)                                 // Switch to standby

static    void                        movement_shutdown ( movement_t * movement );
static    void                        movement_settings ( movement_t * movement );
static    void                        movement_standby ( movement_t * movement );

#define   MOVEMENT_STATE_ACTIVITY     (1 << 27)                                 // Movement has occurred
#define   MOVEMENT_STATE_FREEFALL     (1 << 26)                                 // Free fall detected
#define   MOVEMENT_STATE_VECTORS      (1 << 25)                                 // Valid vectors available

//-----------------------------------------------------------------------------
// Periodic movement and orientation updates
//-----------------------------------------------------------------------------

#define   MOVEMENT_EVENT_PERIODIC     (1 << 15)

static    void                        movement_periodic ( movement_t * movement );

//-----------------------------------------------------------------------------
// Movement events events
//-----------------------------------------------------------------------------

#define   MOVEMENT_EVENT_ORIENTATION  (1 << 14)
#define   MOVEMENT_EVENT_FREEFALL     (1 << 13)
#define   MOVEMENT_EVENT_VECTORS      (1 << 12)

static    void                        movement_orientation ( movement_t * movement );
static    void                        movement_freefall ( movement_t * movement );
static    void                        movement_vectors ( movement_t * movement );

#define   MOVEMENT_EVENT_ACTIVE       (1 << 11)
#define   MOVEMENT_EVENT_ASLEEP       (1 << 10)

static    void                        movement_active ( movement_t * movement );
static    void                        movement_asleep ( movement_t * movement );

//=============================================================================
#endif
