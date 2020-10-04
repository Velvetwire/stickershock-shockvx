//=============================================================================
// project: ShockVx
//  module: Stickershock Sensor for cold chain tracking.
//  author: Velvetwire, llc
//    file: archive.h
//
//
// (c) Copyright 2016-2020 Velvetwire, LLC. All rights reserved.
//=============================================================================

#include  "shockvx.h"

#ifndef   __ARCHIVE__
#define   __ARCHIVE__

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define   ARCHIVE_CLOSE_TIMEOUT     1000
#define   ARCHIVE_FILENAME_LIMIT      32

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

typedef   struct __attribute__ (( packed )) {
          
          unsigned                  timecode;

          unsigned char             data;                                       // Data flags
          signed char               tilt;                                       // Tilt angle (-90 to +90)

          signed short              pressure;                                   // Pressure in millibars
          signed short              humidity;                                   // Humidity in 1/100 percent
          signed short              ambient;                                    // Ambent temperature in degrees Celsius / 100
          signed short              surface;                                    // Surface temperature in degrees Celsius / 100

          } archive_record_t;

#define   ARCHIVE_DATA_PRESSURE       (1 << 7)                                  // Pressure data is valid
#define   ARCHIVE_DATA_HUMIDITY       (1 << 6)                                  // Humidity data is valid
#define   ARCHIVE_DATA_AMBIENT        (1 << 5)                                  // Ambient data is valid
#define   ARCHIVE_DATA_SURFACE        (1 << 4)                                  // Surface data is valid
#define   ARCHIVE_DATA_ANGLE          (1 << 3)                                  // Tilt angle is valid

#define   ARCHIVE_ORIENTATION(o)      (o & 7)                                   // Orientation bits (0 = unknown)

//-----------------------------------------------------------------------------
// Telemetry manager resource
//-----------------------------------------------------------------------------


#define   ARCHIVE_MANAGER_STACK       768                                       // Thread stack size in bytes
#define   ARCHIVE_MANAGER_PRIORITY    (CTL_TASK_PRIORITY_LOW + 3)               // Thread priority

typedef   struct {
          
          CTL_MUTEX_t                 mutex;                                    // Access mutex
          CTL_EVENT_SET_t             option;                                   // Option bits
          CTL_EVENT_SET_t             status;                                   // State and event bits

          char                        filename[ ARCHIVE_FILENAME_LIMIT ];       // Archive file name

          archive_record_t            record;                                   // Telmetry record

          } archive_t;

static    void                        archive_manager ( archive_t * archive );

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define   ARCHIVE_MANAGER_EVENTS      (0x6000ffff)
#define   ARCHIVE_MANAGER_STATES      (0x9fff0000)

#define   ARCHIVE_STATE_CLOSED        (1 << 31)                                 // Module has been closed
#define   ARCHIVE_EVENT_SHUTDOWN      (1 << 30)                                 // Request module shutdown
#define   ARCHIVE_EVENT_SETTINGS      (1 << 29)                                 // Configure settings

static    void                        archive_shutdown ( archive_t * archive );
static    void                        archive_settings ( archive_t * archive );

#define   ARCHIVE_STATE_RECORD        (1 << 28)                                 // Record is pending

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define   ARCHIVE_EVENT_PERIODIC      (1 << 15)                                 // Periodic recording

static    void                        archive_periodic ( archive_t * archive );

//=============================================================================
#endif
