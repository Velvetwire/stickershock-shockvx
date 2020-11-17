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
// Sensor module timing
//-----------------------------------------------------------------------------

#define   SENSORS_CLOSE_TIMEOUT       (1024)
#define   SENSORS_YIELD_TIMEOUT       (256)

//-----------------------------------------------------------------------------
// Telemetry manager resource
//-----------------------------------------------------------------------------

#define   SENSORS_MANAGER_STACK       (512)                                     // Thread stack size in bytes
#define   SENSORS_MANAGER_PRIORITY    (CTL_TASK_PRIORITY_STANDARD + 5)          // Thread priority

typedef   struct {

          CTL_MUTEX_t                 mutex;                                    // Access mutex
          CTL_EVENT_SET_t             option;                                   // Option bits
          CTL_EVENT_SET_t             status;                                   // State and event bits
          CTL_NOTICE_t                notice [ SENSORS_NOTICES ];               // Module notices

          CTL_TIME_t                  period;                                   // Measurment period (milliseconds)

          struct {                                                              // Archival settings:
            CTL_TIME_t                window;                                   //  Archive window (milliseconds)
            CTL_TIME_t                elapse;                                   //  Elapsed time since last event
            } archive;

          struct {                                                              // Humidity sensor readings:
            float                     temperature;                              //  Ambient temperature
            float                     measurement;                              //  Humidity reading
            } humidity;

          struct {                                                              // Pressure sensor readings:
            float                     temperature;                              //  Standby temperature
            float                     measurement;                              //  Pressure reading
            } pressure;

          struct {                                                              // Internal readings:
            float                     temperature;                              //  Die temperature
            } internal;

          } sensors_t;

static    void                        sensors_manager ( sensors_t * sensors );

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define   SENSORS_MANAGER_EVENTS      (0x7000ffff)
#define   SENSORS_MANAGER_STATES      (0x8fff0000)

#define   SENSORS_STATE_CLOSED        (1 << 31)                                 // Module has been closed
#define   SENSORS_EVENT_SHUTDOWN      (1 << 30)                                 // Request module shutdown
#define   SENSORS_EVENT_SETTINGS      (1 << 29)                                 // Configure settings
#define   SENSORS_EVENT_STANDBY       (1 << 28)                                 // Switch to standby

static    void                        sensors_shutdown ( sensors_t * sensors );
static    void                        sensors_settings ( sensors_t * sensors );
static    void                        sensors_standby ( sensors_t * sensors );

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define   SENSORS_VALUES_MEASURED     (SENSOR_VALUE_INTERNAL | SENSORS_VALUE_PRESSURE | SENSORS_VALUE_HUMIDITY | SENSORS_VALUE_AMBIENT | SENSORS_VALUE_STANDBY)

#define   SENSORS_VALUE_INTERNAL      (1 << 23)                                 // Internal temperature available
#define   SENSORS_VALUE_PRESSURE      (1 << 22)                                 // Pressure reading available
#define   SENSORS_VALUE_HUMIDITY      (1 << 21)                                 // Humidity reading available
#define   SENSORS_VALUE_AMBIENT       (1 << 20)                                 // Ambient temperature available
#define   SENSORS_VALUE_STANDBY       (1 << 19)                                 // Standby temperature available

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define   SENSORS_PERIOD_MINIMUM      ((float) 1.0)                             // Minimum period is 1 second
#define   SENSORS_EVENT_PERIODIC      (1 << 15)

static    void                        sensors_periodic ( sensors_t * sensors );

//=============================================================================
#endif
