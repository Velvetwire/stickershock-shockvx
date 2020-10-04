//=============================================================================
// project: ShockVx
//  module: Stickershock Sensor for cold chain tracking.
//  author: Velvetwire, llc
//    file: sensors.h
//
//
// (c) Copyright 2016-2020 Velvetwire, LLC. All rights reserved.
//=============================================================================

#include  "shockvx.h"

#ifndef   __SENSORS__
#define   __SENSORS__

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define   SENSORS_CLOSE_TIMEOUT       1000

//-----------------------------------------------------------------------------
// Telemetry manager resource
//-----------------------------------------------------------------------------

#define   SENSORS_MANAGER_STACK       768                                       // Thread stack size in bytes
#define   SENSORS_MANAGER_PRIORITY    (CTL_TASK_PRIORITY_STANDARD + 5)          // Thread priority

typedef   struct {
          
          CTL_MUTEX_t                 mutex;                                    // Access mutex
          CTL_EVENT_SET_t             option;                                   // Option bits
          CTL_EVENT_SET_t             status;                                   // State and event bits
          CTL_NOTICE_t                notice [ SENSORS_NOTICES ];               // Module notices

          struct {                                                              // Humidity sensor readings:
            float                     temperature;                              //  Ambient temperature
            float                     measurement;                              //  Humidity reading
            } humidity;

          struct {                                                              // Pressure sensor readings:
            float                     temperature;                              //  Standby temperature
            float                     measurement;                              //  Pressure reading
            } pressure;

          struct {                                                              // Motion sensor readings:
            float                     temperature;                              //  Surface temperature
            } surface;

          struct {                                                              // CPU readings:
            float                     temperature;                              //  Die temperature
            } core;

          struct {                                                              // Motion sensor vectors:
            motion_angular_vectors_t  angular;                                  //  Angular rotation vector
            motion_linear_vectors_t   linear;                                   //  Linear acceleration vector
            } vectors;

          struct {                                                              // Motion sensor computations:
            float                     force;                                    //  Force experienced
            float                     limit;                                    //  Force limit experienced
            } motion;

          struct {                                                              // Motion sensor angles:
            float                     angle;                                    //  Angle computation
            float                     limit;                                    //  Angle tilt limit
            unsigned char             orientation;                              //  Orientation face
            } tilt;

          } sensors_t;

static    void                        sensors_manager ( sensors_t * sensors );

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define   SENSORS_MANAGER_EVENTS      (0x6000ffff)
#define   SENSORS_MANAGER_STATES      (0x9fff0000)

#define   SENSORS_STATE_CLOSED        (1 << 31)                                 // Module has been closed
#define   SENSORS_EVENT_SHUTDOWN      (1 << 30)                                 // Request module shutdown
#define   SENSORS_EVENT_SETTINGS      (1 << 29)                                 // Configure settings

static    void                        sensors_shutdown ( sensors_t * sensors );
static    void                        sensors_settings ( sensors_t * sensors );

#define   SENSORS_STATE_MOVEMENT      (1 << 27)                                 // Movement has occurred
#define   SENSORS_STATE_FREEFALL      (1 << 26)                                 // Free fall detected
#define   SENSORS_STATE_VECTORS       (1 << 25)                                 // Valid vectors available

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define   SENSORS_VALUE_HUMIDITY      (1 << 23)                                 // Humidity reading available
#define   SENSORS_VALUE_PRESSURE      (1 << 22)                                 // Pressure reading available
#define   SENSORS_VALUE_AMBIENT       (1 << 21)                                 // Ambient temperature available
#define   SENSORS_VALUE_STANDBY       (1 << 20)                                 // Standby temperature available
#define   SENSORS_VALUE_SURFACE       (1 << 19)                                 // Surface temperature available
#define   SENSORS_VALUE_CORE          (1 << 18)                                 // Core temperature available

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define   SENSORS_EVENT_PERIODIC      (1 << 15)

static    void                        sensors_periodic ( sensors_t * sensors );

//-----------------------------------------------------------------------------
// Motion sensor events
//-----------------------------------------------------------------------------

#define   SENSORS_EVENT_ORIENTATION   (1 << 14)
#define   SENSORS_EVENT_FREEFALL      (1 << 13)
#define   SENSORS_EVENT_VECTORS       (1 << 12)

static    void                        sensors_orientation ( sensors_t * sensors );
static    void                        sensors_freefall ( sensors_t * sensors );
static    void                        sensors_vectors ( sensors_t * sensors );

#define   SENSORS_EVENT_ACTIVE        (1 << 11)
#define   SENSORS_EVENT_ASLEEP        (1 << 10)

static    void                        sensors_active ( sensors_t * sensors );
static    void                        sensors_asleep ( sensors_t * sensors );

//=============================================================================
#endif
