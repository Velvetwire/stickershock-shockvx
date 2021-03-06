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
// SECTION : SENSOR TELEMETRY SERVICE
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
          unsigned                    sensors_alternate ( float * temperature );


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
          MOVEMENT_NOTICE_PERIODIC,                                             //  periodic movement update
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
// SECTION : SYSTEM STATUS MONITOR
//=============================================================================

//-----------------------------------------------------------------------------
// System status monitor
//-----------------------------------------------------------------------------

#define   STATUS_UPDATE_INTERVAL      ((float) 75.0)                            // Update interval 75 seconds

          unsigned                    status_start ( float interval );
          unsigned                    status_check ( unsigned states );

//-----------------------------------------------------------------------------
// Status state changes
//-----------------------------------------------------------------------------

          void                        status_raise ( unsigned states );
          void                        status_lower ( unsigned states );

//-----------------------------------------------------------------------------
// System status bits
//-----------------------------------------------------------------------------

#define   STATUS_SYSTEM               (0x00ff0000)

#define   STATUS_CONNECT              (1 << 16)                                 // Connected to peer
#define   STATUS_SCANNER              (1 << 17)                                 // Scanner is active
#define   STATUS_CHARGED              (1 << 18)                                 // Charger is connected and charged
#define   STATUS_CHARGER              (1 << 19)                                 // Charger is connected and charging
#define   STATUS_BATTERY              (1 << 20)                                 // Battery is critical
#define   STATUS_PROBLEM              (1 << 21)                                 // Problem condition

//-----------------------------------------------------------------------------
// Sensor status bits
//-----------------------------------------------------------------------------

#define   STATUS_SENSORS              (0x0000ffff)

#define   STATUS_SURFACE              (1 << 0)                                  // Surface temperature sensor OK
#define   STATUS_AMBIENT              (1 << 1)                                  // Ambient temperature sensor OK
#define   STATUS_HUMIDITY             (1 << 2)                                  // Humidity sensor OK
#define   STATUS_PRESSURE             (1 << 3)                                  // Pressure sensor OK
#define   STATUS_MOVEMENT             (1 << 4)                                  // Movement sensor OK

//-----------------------------------------------------------------------------
// Battery status
//-----------------------------------------------------------------------------

#define   STARTING_BATTERY_THRESHOLD  ((float) 3.00)                            // Minimum battery power before startup
#define   INDICATE_BATTERY_THRESHOLD  ((float) 3.25)                            // Minimum battery to allow indication
#define   CRITICAL_BATTERY_THRESHOLD  ((float) 3.00)                            // Critical battery alert level

          unsigned                    status_battery ( float * voltage, float * percent );


//=============================================================================
// SECTION : FAULT HANDLING
//=============================================================================

//-----------------------------------------------------------------------------
// Hard fault code conditions.
//-----------------------------------------------------------------------------

#define   FAULT_CONDITION(t,c)        (((unsigned)t << 8) | ((unsigned)c << 0))
#define   FAULT_TYPE(f)               (signed char)((f >> 8) & 255)
#define   FAULT_CODE(f)               (unsigned char)(f & 255)

          void                        fail ( unsigned condition );

#define   FAULT_DELAY                 ((float) 3.0)

//=============================================================================
#endif
