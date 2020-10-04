//=============================================================================
// project: ShockVx
//  module: Stickershock firmware for cold chain tracking.
//  author: Velvetwire, llc
//    file: shockvx.c
//
//
// (c) Copyright 2016-2020 Velvetwire, LLC. All rights reserved.
//=============================================================================

#include  <stickershock.h>

#include  "settings.h"
#include  "bluetooth.h"
#include  "application.h"
#include  "shockvx.h"

//=============================================================================
// SECTION : APPLICATION INITIALIZATION AND STARTUP
//=============================================================================

//-----------------------------------------------------------------------------
// Declare the application data resource.
//-----------------------------------------------------------------------------

static    application_t               application = { 0 };

//-----------------------------------------------------------------------------
//  function: init ( area, size )
// arguments: type - restart type
//            code - restart code
//            area - pointer to scratch memory area
//            size - size of scratch area in bytes
//   returns: pointer to pass to idle ( )
//
// Initialize the system by starting the idle process. Spawn the main ( ) task
// and return.
//
// note: when init ( ) returns, it will immediately fall into idle ( ). It is
// possible to return a value which will be passed as the first argument to
// idle ( ).
//-----------------------------------------------------------------------------

void * init ( signed char type, unsigned char code, void * area, unsigned size ) {

  unsigned                   defaults = APPLICATION_DEFAULTS;
  const char *                  label = APPLICATION_PLATFORM;

  // Initialize the system hardware to its default state and re-boot if
  // required. Otherwise, record the boot type and code in the application
  // data and retrieve the hardware information.

  if ( ctl_defaults ( defaults ) <= CTL_SVC_OK ) {

    // Retrieve the hardware platform information.

    application.hardware.revision     = ctl_identity ( NULL, &(label) );
    application.hardware.code         = ctl_platform ( &(application.hardware.make), &(application.hardware.model), &(application.hardware.version) );
    application.firmware.code         = ctl_package ( &(application.firmware.major), &(application.firmware.minor), &(application.firmware.build) );

    } else { ctl_reboot ( CTL_REBOOT_TYPE_NORMAL, CTL_REBOOT_CODE_NONE ); }

  // Start the idle process, assuming the internal clock as the source,
  // and enable the system watchdog.

  ctl_start ( "idle", (CLOCK_LFCLKSRC_SRC_RC << CLOCK_LFCLKSRC_SRC_Pos) );
  ctl_watch ( APPLICATION_WATCH );

  if ( type == CTL_REBOOT_TYPE_FAULT ) { ctl_spawn ( "fail", (CTL_TASK_ENTRY_t) fail, FAULT_CONDITION( type,code ), APPLICATION_STACK, CTL_TASK_PRIORITY_STANDARD ); }
  else { ctl_spawn ( "main", (CTL_TASK_ENTRY_t) main, &(application), APPLICATION_STACK, CTL_TASK_PRIORITY_STANDARD ); }

  // If this was an NFC wake-up event ...

  #ifdef DEBUG
  debug_printf ( "\r\nType: %i Code: %i", type, code );
  #endif

  #ifdef DEBUG
  if ( type == CTL_REBOOT_TYPE_RESET ) {
    if ( code == CTL_RESET_CODE_DETECT ) { debug_printf ( "\r\nDetect: %08X", NRF_GPIO->LATCH ); }
    if ( code == CTL_RESET_CODE_NFC ) { }
    }
  #endif

  // Return with a pointer to the initialized application data area.

  return ( &(application) );

  }

//-----------------------------------------------------------------------------
//  function: idle ( application )
// arguments: application - application data structure
//   returns: nothing
//
// The idle loop is invoked after returning from init ( ). It cyclically puts
// the system to sleep for a duration determined by the system sleep delay,
// saving power by halting the CPU when no processing is needed.
//
// note: it is possible to declare idle ( ) with a single parameter which will
// receive the value returned by init ( ).
//
// note: returning from idle ( ) will force the system to re-boot.
//-----------------------------------------------------------------------------

void __attribute__ (( noreturn )) idle ( application_t * application ) {

  // Make sure that the idle priority is established before sleeping.

  ctl_task_set_priority ( ctl_task_executing, CTL_TASK_PRIORITY_IDLE );

  // Put the system to sleep for the computed delay period or until the
  // next task schedule is required.

  forever { ctl_sleep ( ); }

  }

//-----------------------------------------------------------------------------
//  function: tick ( time )
// arguments: time - system UTC time
//   returns: nothing
//
// One second tick timer called after the UTC time has been established.
//-----------------------------------------------------------------------------

void tick ( CTL_TIME_t time ) {

  // If settings have changed, request an update to the settings file.

  if ( application.status & APPLICATION_STATE_CHANGED ) { ctl_events_set ( &(application.status), APPLICATION_EVENT_SETTINGS ); }

  //ctl_events_set ( &(application.status), APPLICATION_EVENT_SCHEDULE );

  }

//-----------------------------------------------------------------------------
//  function: fail ( condition )
// arguments: condition - condition code for the failure
//   returns: nothing
//
// The fail ( ) process is invoked instead of main ( ) whenever the condition
// codes at initialization are set for hard failure.
//
//      note: handling of the condition, such as a visual indication, should be
//            followed by a request for normal reboot.
//-----------------------------------------------------------------------------

void fail ( unsigned condition ) {

  unsigned                     option = platform_options ( PLATFORM_OPTIONS_FAILURE );
  unsigned char                  code = FAULT_CODE(condition);
  signed char                    type = FAULT_TYPE(condition);

  // If the indicator is available, flash red fault condition.

  if ( option & PLATFORM_OPTION_INDICATE ) { indicator_blink ( 1.0, 0.0, 0.0, 0.125, 0.125 ); }

  // In debug mode, publish the fault code and stop.

  #ifdef DEBUG
  debug_printf ( "FAIL: %i (%i)!\r\n", type, code );
  debug_break ( );
  #endif

  // In release mode, delay for the fault delay and then reboot.

  #ifndef DEBUG
  ctl_delay ( FAULT_DELAY );
  ctl_reboot ( CTL_REBOOT_TYPE_NORMAL, CTL_REBOOT_CODE_NONE );
  #endif

  }


//=============================================================================
// SECTION : APPLICATION MAIN LOGIC PROCESS
//=============================================================================

//-----------------------------------------------------------------------------
//  function: main ( application )
// arguments: application - application resource data
//   returns: nothing
//
// Main application entry point. Load settings drivers and modules then start
// the logic thread.
//
//      note: Because this is a spawned task, returning will remove this task
//            from the system.
//-----------------------------------------------------------------------------

void main ( application_t * application ) {

  // Configure the application options and set the default application status.

  ctl_events_init ( &(application->status), application_configure ( application, APPLICATION_OPTIONS_DEFAULT | PLATFORM_OPTIONS_DEFAULT ) );
  ctl_timer_start ( CTL_TIMER_CYCLICAL, &(application->status), APPLICATION_EVENT_PERIODIC, (CTL_TIME_t) roundf( APPLICATION_PERIOD  * 1000.0 ) );

  // Wait for application events and pass each event to the appropriate
  // application logic handler.

  forever {

    CTL_EVENT_SET_t            events = APPLICATION_STATUS_EVENTS;
    CTL_EVENT_SET_t            status = ctl_events_wait_uc ( CTL_EVENT_WAIT_ANY_EVENTS_WITH_AUTO_CLEAR, &(application->status), events );

    // Persistent settings changes.

    if ( status & APPLICATION_EVENT_SETTINGS ) { application_settings ( application ); }

    // If a shutdown request is issued, prepare for shutdown and exit.

    if ( status & APPLICATION_EVENT_SHUTDOWN ) { application_shutdown ( application ); break; }

    // Periodic events.

    if ( status & APPLICATION_EVENT_PERIODIC ) { application_periodic ( application ); }
    if ( status & APPLICATION_EVENT_SCHEDULE ) { application_schedule ( application ); }
    if ( status & APPLICATION_EVENT_TIMECODE ) { application_timecode ( application ); }

    // Indicator state update requested.

    if ( status & APPLICATION_EVENT_INDICATE ) { application_indicate ( application ); }

    // Battery level and charging events.

    if ( status & APPLICATION_EVENT_CHARGER ) { application_charger ( application ); }
    if ( status & APPLICATION_EVENT_BATTERY ) { application_battery ( application ); }

    // NFC scanning and peripheral connection events.

    if ( status & APPLICATION_EVENT_TAGGED ) { application_tagged ( application ); }
    if ( status & APPLICATION_EVENT_ATTACH ) { application_attach ( application ); }
    if ( status & APPLICATION_EVENT_DETACH ) { application_detach ( application ); }
    if ( status & APPLICATION_EVENT_PROBED ) { application_probed ( application ); }
    if ( status & APPLICATION_EVENT_EXPIRE ) { application_expire ( application ); }

    // Indication strobe and cancellation events.

    if ( status & APPLICATION_EVENT_STROBE ) { application_strobe ( application ); }
    if ( status & APPLICATION_EVENT_CANCEL ) { application_cancel ( application ); }

    // Sensor and telemetry events.

    if ( status & APPLICATION_EVENT_SENSOR ) { application_sensor ( application ); }
    if ( status & APPLICATION_EVENT_ANGLES ) { application_angles ( application ); }
    if ( status & APPLICATION_EVENT_ORIENT ) { application_orient ( application ); }
    if ( status & APPLICATION_EVENT_FALLEN ) { application_fallen ( application ); }
    if ( status & APPLICATION_EVENT_LIMITS ) { application_limits ( application ); }

    }

  // Make sure that the storage volumes have all been flushed and put to sleep.

  if ( application->option & (PLATFORM_OPTION_INTERNAL | PLATFORM_OPTION_EXTERNAL) ) { storage_sleep ( ); }

  // Application logic has been stopped so prepare the system for shutdown and then
  // halt the processor.

  if ( NRF_SUCCESS == softdevice_disable ( ) ) {

    ctl_shutdown ( );
    ctl_halt ( );

    }

  }

#ifdef DEBUG
void ctl_handle_error ( CTL_ERROR_CODE_t error ) {

  CTL_TASK_t *  task = ctl_task_executing;

  debug_break ( );

  }
#endif
