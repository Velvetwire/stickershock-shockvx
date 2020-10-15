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
// SECTION : TELEMETRY SENSORS SERVICE
//=============================================================================

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

          unsigned                    sensors_start ( unsigned option );
          unsigned                    sensors_begin ( float interval, float archival );
          unsigned                    sensors_cease ( void );
          unsigned                    sensors_close ( void );

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

typedef   enum {                                                                // Module notices:
          SENSORS_NOTICE_TELEMETRY,                                             //  telemetry updated
          SENSORS_NOTICE_ARCHIVE,                                               //  archive requested
          SENSORS_NOTICES
          } sensors_notice_t;

          unsigned                    sensors_notice ( sensors_notice_t notice, CTL_EVENT_SET_t * set, CTL_EVENT_SET_t events );

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

          unsigned                    sensors_temperature ( float * temperature );
          unsigned                    sensors_atmosphere ( float * temperature, float * humidity, float * pressure );


//=============================================================================
// SECTION : MOVEMENT AND ORIENTATION DETECTION SERVICE
//=============================================================================

          unsigned                    movement_start ( unsigned option );
          unsigned                    movement_begin ( float interval );
          unsigned                    movement_cease ( void );
          unsigned                    movement_close ( void );

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

typedef   enum {                                                                // Module notices:
          MOVEMENT_NOTICE_ORIENTATION,                                          //  orientation change
          MOVEMENT_NOTICE_MOVEMENT,                                             //  periodic movement update
          MOVEMENT_NOTICE_FREEFALL,                                             //  freefall detected
          MOVEMENT_NOTICE_STARTED,                                              //  movement activity started
          MOVEMENT_NOTICE_STOPPED,                                              //  movement activity stopped
          MOVEMENT_NOTICE_STRESS,                                               //  excessive force detected
          MOVEMENT_NOTICE_TILT,                                                 //  excessive tilt detected
          MOVEMENT_NOTICES
          } movement_notice_t;

          unsigned                    movement_notice ( movement_notice_t notice, CTL_EVENT_SET_t * set, CTL_EVENT_SET_t events );

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

          unsigned                    movement_temperature ( float * temperature );
          unsigned                    movement_forces ( float * force, float * x, float * y, float * z );
          unsigned                    movement_angles ( float * angle, char * orientation );
          unsigned                    movement_limits ( float force, float angle );


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
