//=============================================================================
// project: ShockVx
//  module: Stickershock firmware for cold chain tracking.
//  author: Velvetwire, llc
//    file: shockvx.h
//
//
// (c) Copyright 2016-2020 Velvetwire, LLC. All rights reserved.
//=============================================================================

#ifndef   __SHOCKVX__
#define   __SHOCKVX__

//=============================================================================
// SECTION : SENSOR SERVICE
//=============================================================================

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

          unsigned                    sensors_start ( unsigned option );
          unsigned                    sensors_begin ( float interval );
          unsigned                    sensors_cease ( void );
          unsigned                    sensors_close ( void );

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

typedef   enum {                                                                // Module notices:
          SENSORS_NOTICE_ORIENTATION,                                           //  orientation change
          SENSORS_NOTICE_TELEMETRY,                                             //  telemetry updated
          SENSORS_NOTICE_FREEFALL,                                              //  freefall detected
          SENSORS_NOTICES
          } sensors_notice_t;

          unsigned                    sensors_notice ( sensors_notice_t notice, CTL_EVENT_SET_t * set, CTL_EVENT_SET_t events );

          unsigned                    sensors_telemetry ( telemetry_values_t * telemetry );
          unsigned                    sensors_handling ( handling_values_t * handling );

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

          unsigned                    sensors_angles ( float * limit );
          unsigned                    sensors_forces ( float * limit );


//=============================================================================
// SECTION : ARCHIVE SERVICES
//=============================================================================

#define   ARCHIVE_DEFAULT_INTERVAL    ((float) (60*60))                         // Archive telemetry every 30 minutes

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

          unsigned                    archive_start ( const char * filename );
          unsigned                    archive_begin ( float interval );
          unsigned                    archive_cease ( void );
          unsigned                    archive_close ( void );

//          unsigned                    archive_telemetry ( telemetry_record_t * record, unsigned status );


//=============================================================================
// SECTION : FAULT HANDLING
//=============================================================================

//-----------------------------------------------------------------------------
// Hard fault code conditions.
//-----------------------------------------------------------------------------

#define   FAULT_CONDITION(t,c)        (void *)(((unsigned)t << 8) | ((unsigned)c << 0))
#define   FAULT_TYPE(f)               (signed char)((f >> 8) & 255)
#define   FAULT_CODE(f)               (unsigned char)(f & 255)

          void                        fail ( unsigned condition );

#define   FAULT_DELAY                 ((float) 3.0)

//=============================================================================
#endif
