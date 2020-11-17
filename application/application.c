//=============================================================================
// project: ShockVx
//  module: Stickershock firmware for cold chain tracking
//  author: Velvetwire, llc
//    file: application.c
//
// Application logic.
//
// (c) Copyright 2016-2020 Velvetwire, LLC. All rights reserved.
//=============================================================================

#include  <stickershock.h>

#include  "shockvx.h"
#include  "settings.h"
#include  "application.h"

//=============================================================================
// SECTION : APPLICATION CONFIGURATION
//=============================================================================

//-----------------------------------------------------------------------------
//  function: application_configure ( application, options )
// arguments: application - application resource
//                options - configuration option bits
//   returns: initial application status
//
// Configure the platform for the given options. Load application settings from
// persistent storage. Return with the initial application status.
//-----------------------------------------------------------------------------

unsigned application_configure ( application_t * application, unsigned options ) {

  CTL_EVENT_SET_t              status = APPLICATION_EVENT_STARTING;
  application->option                 = application_defaults ( application, options );

  // If the bluetooth option is requested, start the bluetooth stack and
  // both the beacon broadcast and peripheral profile managers.

  if ( application->option & APPLICATION_OPTION_BLE ) {

    if ( NRF_SUCCESS == application_bluetooth ( application ) ) {

      if ( NRF_SUCCESS == beacon_start ( BEACON_BROADCAST_VARIANT ) ) {

        beacon_notice ( BEACON_NOTICE_INSPECTED, &(application->status), APPLICATION_EVENT_PROBED );

        }

      if ( NRF_SUCCESS == peripheral_start ( ) ) {

        peripheral_notice ( PERIPHERAL_NOTICE_TERMINATE, &(application->status), APPLICATION_EVENT_EXPIRE );
        peripheral_notice ( PERIPHERAL_NOTICE_ATTACHED, &(application->status), APPLICATION_EVENT_ATTACH );
        peripheral_notice ( PERIPHERAL_NOTICE_DETACHED, &(application->status), APPLICATION_EVENT_DETACH );

        }

      } else { application->option ^= APPLICATION_OPTION_BLE; }

    }

  // If the NFC option is requested, prepare the nearfield tag data
  // and request the device.

  if ( application->option & APPLICATION_OPTION_NFC ) {

    if ( NRF_SUCCESS == application_nearfield ( application ) ) {

      nfct_notice ( NFCT_NOTICE_SLEEP, &(application->status), APPLICATION_EVENT_TAGGED );
      nfct_request ( );

      } else { application->option ^= APPLICATION_OPTION_NFC; }

    }

  // If any of the environmental sensors is enabled, start the sensor module and
  // register to receive periodic telemetry updates.

  if ( application->option & (PLATFORM_OPTION_PRESSURE | PLATFORM_OPTION_HUMIDITY) ) {

    if ( NRF_SUCCESS == sensors_start ( application->option ) ) {

      sensors_notice ( SENSORS_NOTICE_TELEMETRY, &(application->status), APPLICATION_EVENT_TELEMETRY );
      sensors_notice ( SENSORS_NOTICE_ARCHIVE, &(application->status), APPLICATION_EVENT_ARCHIVE );

      sensors_begin ( application->settings.telemetry.interval, application->settings.telemetry.archival );

      }

    }

  // If the motion sensor is enabled, start the movement tracking module and
  // register to receive notices from the module for orientation change,
  // periodic updates and various alerts, such as freefall and tilt.

  if ( application->option & PLATFORM_OPTION_MOTION ) {

    if ( NRF_SUCCESS == movement_start ( application->option ) ) {

      movement_notice ( MOVEMENT_NOTICE_ORIENTATION, &(application->status), APPLICATION_EVENT_ORIENTED );
      movement_notice ( MOVEMENT_NOTICE_PERIODIC, &(application->status), APPLICATION_EVENT_HANDLING );

      movement_notice ( MOVEMENT_NOTICE_FREEFALL, &(application->status), APPLICATION_EVENT_DROPPED );
      movement_notice ( MOVEMENT_NOTICE_STRESS, &(application->status), APPLICATION_EVENT_STRESSED );
      movement_notice ( MOVEMENT_NOTICE_TILT, &(application->status), APPLICATION_EVENT_TILTED );

      movement_begin ( application->settings.telemetry.interval );

      }

    }

  // Return with the initial state and event bits.

  return ( status );

  }

//-----------------------------------------------------------------------------
//  function: application_bluetooth ( application )
// arguments: application - application resource
//   returns: NRF_SUCCESS if configured
//
// Configure the bluetooth stack for application use. Prepare the peripheral
// GATT services.
//-----------------------------------------------------------------------------

unsigned application_bluetooth ( application_t * application ) {

  unsigned                     result = bluetooth_start ( APPLICATION_NAME );

  // Add the device battery information service class and declare a fixed, rechargable battery type.

  if ( NRF_SUCCESS == result ) { result = battery_register ( BATTERY_TYPE_FIXED | BATTERY_TYPE_RECHARGEABLE ); }

  // Register the access control service qnd request notices for shutdown and time code updates.

  if ( NRF_SUCCESS == result ) { result = access_register ( ); }
  if ( NRF_SUCCESS == result ) {

    access_notice ( ACCESS_NOTICE_SHUTDOWN, &(application->status), APPLICATION_EVENT_SHUTDOWN );
    access_notice ( ACCESS_NOTICE_TIMECODE, &(application->status), APPLICATION_EVENT_TIMECODE );

    }

  // Register the device control service and record the current system
  // status in the control status flags.

  if ( NRF_SUCCESS == result ) { result = control_register ( &(application->settings.tracking.node),
                                                             application->settings.tracking.lock,
                                                             application->settings.tracking.signature.opened,
                                                             application->settings.tracking.signature.closed ); }

  // Add the device information service class and include the system firmware version.

  if ( NRF_SUCCESS == result ) { result = information_register ( application->hardware.make, application->hardware.model, application->hardware.version ); }
  if ( NRF_SUCCESS == result ) {

    char                   firmware [ INFORMATION_REVISION_LIMIT + 1 ];

    if ( (application->firmware.code != (unsigned)(-1)) ) {

      snprintf ( firmware, INFORMATION_REVISION_LIMIT + 1, "%s %u.%02u (%u)", APPLICATION_NAME,
                 application->firmware.major, application->firmware.minor, application->firmware.build );

      information_firmware ( firmware );

      }

    }

  // Add the telemetry data service and initialize the telemetry settings.

  if ( NRF_SUCCESS == result ) { result = surface_register ( application->settings.surface.lower, application->settings.surface.upper ); }
  if ( NRF_SUCCESS == result ) { result = telemetry_register ( application->settings.telemetry.interval, application->settings.telemetry.archival ); }
  if ( NRF_SUCCESS == result ) { result = atmosphere_register ( &(application->settings.atmosphere.lower), &(application->settings.atmosphere.upper) ); }

  // Add the orientation and handling service.

  if ( NRF_SUCCESS == result ) { result = handling_register ( &(application->settings.handling.limit) ); }

  // Return with result.

  return ( result );

  }

//-----------------------------------------------------------------------------
//  function: application_nearfield ( application )
// arguments: application - application resource
//   returns: NRF_SUCCESS if configured
//
// Configure the nearfield driver for application use. Prepare the application
// NFC tag content.
//-----------------------------------------------------------------------------

unsigned application_nearfield ( application_t * application ) {

  unsigned                     result = nfct_reserve ( );
  void *                         node = NULL;

  // Build the NDEF tag information XML to identifity the device and to
  // include the registered primary and access control services.

  if ( NRF_SUCCESS == result ) {

    const void *              primary = control_uuid ( );
    const void *              control = access_uuid ( );
    void *                       tags = malloc ( ndef_tags ( NULL, primary, control, node ) );
    unsigned                     size = ndef_tags ( tags, primary, control, node );

    if ( tags ) { result = nfct_data ( tags ); }
    else { result = NRF_ERROR_NO_MEM; }

    free ( tags ); 
    
    }

  // Return with result.

  return ( result );

  }


//=============================================================================
// SECTION : STARTUP AND SHUTDOWN PREPARATION
//=============================================================================

//-----------------------------------------------------------------------------
//  function: application_starting ( application )
// arguments: application - application resource
//
// The application is starting after configuration.
//-----------------------------------------------------------------------------

void application_starting ( application_t * application ) {

  // Start the status module.

  status_start ( STATUS_UPDATE_INTERVAL );

  // If the tracking window is open, start the beacon broadcast.

  if ( (application->settings.tracking.time.opened) && !(application->settings.tracking.time.closed) ) {

    beacon_begin ( BEACON_BROADCAST_RATE, BEACON_BROADCAST_PERIOD, BEACON_BROADCAST_POWER, BEACON_TYPE_BLE_4 );

    }

  // Start the periodic event timer.

  ctl_timer_start ( CTL_TIMER_CYCLICAL, &(application->status), APPLICATION_EVENT_PERIODIC, (CTL_TIME_t) roundf( APPLICATION_PERIOD  * 1000.0 ) );

  #ifdef DEBUG
  debug_printf ( "\r\nStarting ..." );
  #endif

  }

//-----------------------------------------------------------------------------
//  function: application_shutdown ( application )
// arguments: application - application resource
//
// Shut down the various modules and services before stopping the application.
//-----------------------------------------------------------------------------

void application_shutdown ( application_t * application ) {

  bool                         active = false;

  #ifdef DEBUG
  debug_printf ( "\r\nShutting down." );
  #endif

  // Make sure that the indicator is off before shutting down.

  if ( application->option & PLATFORM_OPTION_INDICATOR ) { indicator_off ( ); }

  // If there are any pending settings changes, make sure that they are written out.

  if ( application->status & APPLICATION_STATE_SETTINGS ) { application_settings ( application ); }

  // Shutdown the various modules to make sure things are cleaned up.

  if ( NRF_SUCCESS == beacon_state ( &(active) ) && active ) { beacon_close ( ); }
  if ( NRF_SUCCESS == peripheral_state ( &(active), NULL ) && active ) { peripheral_close ( ); }

  // Shut down the sensor telemetry and movement monitoring modules.

  if ( application->option & (PLATFORM_OPTION_PRESSURE | PLATFORM_OPTION_HUMIDITY) ) { sensors_close ( ); }
  if ( application->option & (PLATFORM_OPTION_MOTION) ) { movement_close ( ); }

  // Pause for a short delay before releasing for shutdown.

  ctl_delay ( APPLICATION_SHUTDOWN_DELAY );

  }


//=============================================================================
// SECTION : SCHEDULED AND PERIODIC EVENTS
//=============================================================================

//-----------------------------------------------------------------------------
//  function: application_schedule ( application )
// arguments: application - application resource
//
// Scheduled check-in.
//-----------------------------------------------------------------------------

void application_schedule ( application_t * application ) {

  // NOTE: this is triggered by the tick ( ) callback

  }

//-----------------------------------------------------------------------------
//  function: application_periodic ( application )
// arguments: application - application resource
//
// Periodic application check-in.
//-----------------------------------------------------------------------------

void application_periodic ( application_t * application ) {

  bool                         active = false;
  bool                         linked = false;

  // Request that the storage flush any pending writes and return to sleep.

  if ( application->option & PLATFORM_STORAGE_OPTIONS ) { storage_sleep ( ); }

  // If the peripheral is advertising or linked to a peer, there is system
  // activity. If the beacon is advertising, there is also system activity.

  if ( NRF_SUCCESS == peripheral_state ( &(active), &(linked) ) ) { if ( active || linked ) return; }
  if ( NRF_SUCCESS == beacon_state ( &(active) ) ) { if ( active ) return; }

  // As long as the system is not currently charging or charged, it is
  // safe to shut down.

  if ( ! status_check ( STATUS_CHARGER | STATUS_CHARGED ) ) { ctl_events_set ( &(application->status), APPLICATION_EVENT_SHUTDOWN ); }

  }

//-----------------------------------------------------------------------------
//  function: application_timecode ( application )
// arguments: application - application resource
//
// The application UTC time code has been updated via the access service.
//-----------------------------------------------------------------------------

void application_timecode ( application_t * application ) {

  // NOTE: this is triggered when the UTC time code is changed via bluetooth

  #ifdef DEBUG
  debug_printf ( "\r\nTimecode: %u", ctl_time_get ( ) );
  #endif

  }


//=============================================================================
// SECTION : INTERACTION EVENT PROCESSING
//=============================================================================

//-----------------------------------------------------------------------------
//  function: application_tagged ( application )
// arguments: application - application resource
//
// Respond to an NFC tagging event.
//-----------------------------------------------------------------------------

void application_tagged ( application_t * application ) {

  bool                         active = false;
  bool                         linked = false;

  // Whenever the NFC is tagged, switch the bluetooth profile from beacon to
  // peripheral.

  if ( application->option & APPLICATION_OPTION_BLE ) {

    if ( NRF_SUCCESS == beacon_state ( &(active) ) && (active) ) { beacon_cease ( ); }
    if ( NRF_SUCCESS == peripheral_state ( &(active), &(linked) ) && !(active) && !(linked) ) { ctl_yield ( APPLICATION_TAG_DELAY ); }
    else return;

    if ( linked ) { peripheral_cease ( ); }

    peripheral_begin ( PERIPHERAL_BROADCAST_RATE, PERIPHERAL_BROADCAST_PERIOD, PERIPHERAL_BROADCAST_POWER );

    }

  }


//=============================================================================
// SECTION : BLUETOOTH CONNECTION EVENT PROCESSING
//=============================================================================

//-----------------------------------------------------------------------------
//  function: application_attach ( application )
// arguments: application - application resource
//
// Respond to a BLE peripheral peer attachment.
//-----------------------------------------------------------------------------

void application_attach ( application_t * application ) {

  // Release the NFC device while connected and clear the problem
  // state.

  if ( application->option & APPLICATION_OPTION_NFC ) { nfct_release ( ); }

  status_raise ( STATUS_CONNECT );
  status_lower ( STATUS_PROBLEM );

  // Update the tracking window in the control service and switch the telemetry
  // rate to the connected interval setting.

  control_window ( application->settings.tracking.time.opened, application->settings.tracking.time.closed );

  sensors_begin ( TELEMETRY_SERVICE_INTERVAL, application->settings.telemetry.archival );
  movement_begin ( TELEMETRY_SERVICE_INTERVAL );

  }

//-----------------------------------------------------------------------------
//  function: application_closed ( application )
// arguments: application - application resource
//
// Respond to a BLE peripheral peer detachment.
//-----------------------------------------------------------------------------

void application_detach ( application_t * application ) {

  // Clear the connected status.

  status_lower ( STATUS_CONNECT );

  // Request the NFC device after detaching.

  if ( application->option & APPLICATION_OPTION_NFC ) { nfct_request ( ); }

  // Retreive the control settings for tracking from the tracking service. If the
  // tracking window has been opened, record the open time. If the tracking window
  // has been closed, record the close time.

  if ( NRF_SUCCESS == control_tracking ( &(application->settings.tracking.node),
                                         application->settings.tracking.lock,
                                         application->settings.tracking.signature.opened,
                                         application->settings.tracking.signature.closed ) ) {

    // If the tracking window has been opened, mark the UTC time.

    if ( ! application->settings.tracking.time.opened ) for ( unsigned n = 0; n < SOFTDEVICE_KEY_LENGTH; ++ n ) {
      if ( application->settings.tracking.signature.opened[ n ] ) { application->settings.tracking.time.opened = ctl_time_get ( ); break; }
      }

    // If the tracking window has been closed, mark the UTC time.

    if ( ! application->settings.tracking.time.closed ) for ( unsigned n = 0; n < SOFTDEVICE_KEY_LENGTH; ++ n ) {
      if ( application->settings.tracking.signature.closed[ n ] ) { application->settings.tracking.time.closed = ctl_time_get ( ); break; }
      }

    }

  // Retrieve the settings values from the various telemetry services.

  surface_settings ( &(application->settings.surface.lower), &(application->settings.surface.upper) );
  handling_settings ( &(application->settings.handling.limit) );
  telemetry_settings ( &(application->settings.telemetry.interval), &(application->settings.telemetry.archival) );
  atmosphere_settings ( &(application->settings.atmosphere.lower), &(application->settings.atmosphere.upper) );

  // Update the sensor telemetry intervals.

  sensors_begin ( application->settings.telemetry.interval, application->settings.telemetry.archival );

  // Update the movement limits and intervals.

  movement_limits ( application->settings.handling.limit.force, application->settings.handling.limit.angle );
  movement_begin ( application->settings.telemetry.interval );

  // If the tracking window is open, activate the telemetry beacon.

  if ( application->settings.tracking.time.opened && !(application->settings.tracking.time.closed) ) {

    beacon_begin ( BEACON_BROADCAST_RATE, BEACON_BROADCAST_PERIOD, BEACON_BROADCAST_POWER, BEACON_TYPE_BLE_4 );

    } else { beacon_cease ( ); }

  // Persistent settings need saving.

  ctl_events_set ( &(application->status), APPLICATION_STATE_SETTINGS );

  }

//-----------------------------------------------------------------------------
//  function: application_probed ( application )
// arguments: application - application resource
//
// Respond to a BLE beacon scan response probe.
//-----------------------------------------------------------------------------

void application_probed ( application_t * application ) {

  // NOTE: this is triggered whenever the beacon scan packet is inspected

  }

//-----------------------------------------------------------------------------
//  function: application_expire ( application )
// arguments: application - application resource
//
// Peripheral advertising period has expired.
//-----------------------------------------------------------------------------

void application_expire ( application_t * application ) {

  // Request the NFC device after the advertisement expires.

  if ( application->option & APPLICATION_OPTION_NFC ) { nfct_request ( ); }

  }


//=============================================================================
// SECTION : PERIODIC TELEMETRY AND HANDLING EVENTS
//=============================================================================

//-----------------------------------------------------------------------------
//  function: application_telemetry ( application )
// arguments: application - application resource
//
// Handle a notice that the sensor telemetry has been updated.
//-----------------------------------------------------------------------------

void application_telemetry ( application_t * application ) {

  atmosphere_values_t      atmosphere = { 0 };
  float                      interval = 0;

  // If the tracking window is open and the peripheral is not connected,
  // get the telemetry interval.

  if ( (application->settings.tracking.time.opened) && !(application->settings.tracking.time.closed) ) {

    if ( ! status_check ( STATUS_CONNECT ) ) { interval = application->settings.telemetry.interval; }

    }

  // Capture the telemetry values, update the telemetry service characteristics
  // and check for compliance. Raise the problem state if non-compliant.

  if ( NRF_SUCCESS == sensors_atmosphere ( &(atmosphere.temperature), &(atmosphere.humidity), &(atmosphere.pressure) ) ) {

    if ( NRF_SUCCESS == atmosphere_measured ( &(atmosphere), interval ) ) {

      atmosphere_compliance_t  inside = { 0 };
      atmosphere_compliance_t outside = { 0 };

      if ( NRF_SUCCESS == atmosphere_compliance ( &(inside), &(outside) ) ) {

        if ( inside.temperature && outside.temperature ) { status_raise ( STATUS_PROBLEM ); }
        if ( inside.humidity && outside.humidity ) { status_raise ( STATUS_PROBLEM ); }
        if ( inside.pressure && outside.pressure ) { status_raise ( STATUS_PROBLEM ); }

        }

      beacon_ambient ( atmosphere.temperature, inside.temperature, outside.temperature );
      beacon_humidity ( atmosphere.humidity, inside.humidity, outside.humidity );
      beacon_pressure ( atmosphere.pressure, inside.pressure, outside.pressure );

      }

    #ifdef DEBUG
    debug_printf ( "\r\nTelemetry: %1.2fC %1.1f%% %1.3f bar", atmosphere.temperature, atmosphere.humidity * 100.0, atmosphere.pressure );
    #endif

    }

  }

//-----------------------------------------------------------------------------
//  function: application_archive ( application )
// arguments: application - application resource
//
// Handle a notice that the an update to the telemetry archives is needed
//-----------------------------------------------------------------------------

void application_archive ( application_t * application ) {

  // Archiving only occurs while the tracking window is open.

  if ( ! application->settings.tracking.time.opened ) return;
  if ( application->settings.tracking.time.closed ) return;

  // If a non-zero UTC time has been established, record the telemetry
  // in the archives of the atmospheric and surface temperature services.

  if ( ctl_time_get ( ) ) {

    if ( NRF_SUCCESS == atmosphere_archive ( ) ) {
      #ifdef DEBUG
      debug_printf ( "\r\nArchive: atmosphere" );
      #endif
      }

    if ( NRF_SUCCESS == surface_archive ( ) ) {
      #ifdef DEBUG
      debug_printf ( "\r\nArchive: surface" );
      #endif
      }
    
    }

  }


//=============================================================================
// SECTION : MOVEMENT RELATED EVENTS
//=============================================================================

//-----------------------------------------------------------------------------
//  function: application_handling ( application )
// arguments: application - application resource
//
// Handle a notice that the movement and handling values need updating.
//-----------------------------------------------------------------------------

void application_handling ( application_t * application ) {

  handling_values_t          handling = { 0 };
  float                   temperature = 0;
  float                      interval = 0;

  // If the tracking window is open and the peripheral is not connected,
  // get the telemetry interval. 

  if ( (application->settings.tracking.time.opened) && !(application->settings.tracking.time.closed) ) {

    if ( ! status_check ( STATUS_CONNECT ) ) { interval = application->settings.telemetry.interval; }

    }

  // Capture the motion values and update the handling service characteristics.

  if ( (NRF_SUCCESS == movement_angles ( &(handling.angle), &(handling.face) ))
    && (NRF_SUCCESS == movement_forces ( &(handling.force), NULL, NULL, NULL )) ) {
      
    if ( NRF_SUCCESS == handling_observed ( &(handling) ) ) {

      beacon_orientation ( handling.angle, handling.face );

      }
    
    }

  // Capture the surface temperature using the motion unit, update the
  // service value and check for compliance. Raise the problem state
  // if non-compliant.

  if ( NRF_SUCCESS == motion_temperature ( &(temperature) ) ) {

    if ( NRF_SUCCESS == surface_measured ( temperature, interval ) ) {
      
      surface_compliance_t     inside = 0;
      surface_compliance_t    outside = 0;

      if ( NRF_SUCCESS == surface_compliance ( &(inside), &(outside) ) ) {

        if ( inside && outside ) { status_raise ( STATUS_PROBLEM ); }

        }
      
      beacon_temperature ( temperature, inside, outside );
      
      }

    #ifdef DEBUG
    debug_printf ( "\r\n  Surface: %1.2fC", temperature );
    #endif

    }

  }

//-----------------------------------------------------------------------------
//  function: application_oriented ( application )
// arguments: application - application resource
//
// Handle a notice that the orientation has changed.
//-----------------------------------------------------------------------------

void application_oriented ( application_t * application ) {

  unsigned char           orientation = MOTION_ORIENTATION_UNKNOWN;

  if ( (NRF_SUCCESS == movement_angles ( NULL, &(orientation) )) && orientation ) {

    if ( (application->settings.handling.limit.face != MOTION_ORIENTATION_UNKNOWN)
      && (application->settings.handling.limit.face != orientation) ) { /* */ }

    }

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void application_stressed ( application_t * application ) {

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void application_dropped ( application_t * application ) {

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void application_tilted ( application_t * application ) {

  }
