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

#include  "application.h"
#include  "shockvx.h"

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

  application->option                 = platform_options ( options );
  CTL_EVENT_SET_t              status = APPLICATION_EVENT_CHARGER | APPLICATION_EVENT_BATTERY;

  // Configure the settings default to reasonable values.

  application_settings_t *   defaults = memset ( &(application->settings), 0, sizeof(application_settings_t) );

  defaults->telemetry.interval        = TELEMETRY_DEFAULT_INTERVAL;
  defaults->telemetry.archival        = ARCHIVE_DEFAULT_INTERVAL;

  defaults->handling.limits.face      = MOTION_ORIENTATION_FACEUP;

  strncpy ( defaults->label, APPLICATION_NAME, sizeof(defaults->label) );

  // Load the application settings from persistent storage. If no settings file exists,
  // create the file using the already configured defaults.

  file_handle_t              settings = file_open ( APPLICATION_FILE, FILE_MODE_CREATE | FILE_MODE_WRITE | FILE_MODE_READ );

  if ( settings > FILE_OK ) {

    if ( sizeof(application_settings_t) != file_read ( settings, &(application->settings), sizeof(application_settings_t) ) ) {

      file_seek ( settings, FILE_SEEK_POSITION, 0 );
      file_write ( settings, &(application->settings), sizeof(application_settings_t) );
      file_clamp ( settings );

      }

    file_close ( settings );

    }

  // If the bluetooth option is requested, start the bluetooth stack and
  // both the beacon broadcast and peripheral profile managers.

  if ( application->option & APPLICATION_OPTION_BLE ) {

    if ( NRF_SUCCESS == application_bluetooth ( application ) ) {

      if ( NRF_SUCCESS == beacon_start ( BEACON_BROADCAST_PROFILE ) ) {

        beacon_notice ( BEACON_NOTICE_INSPECTED, &(application->status), APPLICATION_EVENT_PROBED );

        }

      if ( NRF_SUCCESS == peripheral_start ( ) ) {

        peripheral_notice ( PERIPHERAL_NOTICE_TERMINATE, &(application->status), APPLICATION_EVENT_EXPIRE );
        peripheral_notice ( PERIPHERAL_NOTICE_ATTACHED, &(application->status), APPLICATION_EVENT_ATTACH );
        peripheral_notice ( PERIPHERAL_NOTICE_DETACHED, &(application->status), APPLICATION_EVENT_DETACH );

        peripheral_begin ( PERIPHERAL_BROADCAST_RATE, PERIPHERAL_BROADCAST_PERIOD, PERIPHERAL_BROADCAST_POWER );

        }

      } else { application->option ^= APPLICATION_OPTION_BLE; }

    }

  // If the NFC option is requested, start the nearfield device.

  if ( application->option & APPLICATION_OPTION_NFC ) {

    if ( NRF_SUCCESS != application_nearfield ( application ) ) { application->option ^= APPLICATION_OPTION_NFC; }

    }

  // If the power monitor is enabled, make sure that the battery has
  // reached minimum warm-up power and then register to receive notices.

  if ( application->option & PLATFORM_OPTION_POWER ) {

    power_notice ( POWER_NOTICE_CHARGER, &(application->status), APPLICATION_EVENT_CHARGER );
    power_notice ( POWER_NOTICE_BATTERY, &(application->status), APPLICATION_EVENT_BATTERY );

    #ifndef DEBUG
    power_warmup ( STARTING_BATTERY_THRESHOLD );
    #endif

    }

  // If any of the telemetry sensors is enabled, start the telemetry
  // capture module and enable the archive.

  if ( application->option & (PLATFORM_OPTION_MOTION | PLATFORM_OPTION_PRESSURE | PLATFORM_OPTION_HUMIDITY) ) {

    if ( NRF_SUCCESS == sensors_start ( application->option ) ) {

      sensors_notice ( SENSORS_NOTICE_ORIENTATION, &(application->status), APPLICATION_EVENT_ORIENT );
      sensors_notice ( SENSORS_NOTICE_TELEMETRY, &(application->status), APPLICATION_EVENT_SENSOR | APPLICATION_EVENT_ANGLES );
      sensors_notice ( SENSORS_NOTICE_FREEFALL, &(application->status), APPLICATION_EVENT_FALLEN );
      sensors_begin ( application->settings.telemetry.interval );

      //archive_start ( APPLICATION_DATA );

      }

    }

  // If the indicator is enabled, request an update.

  if ( application->option & PLATFORM_OPTION_INDICATE ) { status |= APPLICATION_EVENT_INDICATE; }

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

  // Register the device control service and request identification notices.

  if ( NRF_SUCCESS == result ) { result = control_register ( &(application->settings.tracking.node),
                                                             application->settings.tracking.lock,
                                                             application->settings.tracking.signature.opened,
                                                             application->settings.tracking.signature.closed ); }

  if ( NRF_SUCCESS == result ) { control_notice ( CONTROL_NOTICE_IDENTIFY, &(application->status), APPLICATION_EVENT_STROBE ); }

  // Add the device information service class and include the system firmware version.

  if ( NRF_SUCCESS == result ) { result = information_register ( application->hardware.make, application->hardware.model, application->hardware.version ); }
  if ( NRF_SUCCESS == result ) {

    char *                   firmware = malloc ( INFORMATION_REVISION_LIMIT + 1 );

    if ( firmware ) {

      if ( application->firmware.code < (unsigned)(-1) ) { snprintf ( firmware, INFORMATION_REVISION_LIMIT, "%s %u.%u", APPLICATION_NAME, application->firmware.major, application->firmware.minor ); }
      else { strncpy ( firmware, APPLICATION_NAME, INFORMATION_REVISION_LIMIT ); }

      information_firmware ( firmware );

      }

    free ( firmware );

    }

  // Add the telemetry data service, the handling data service, the record data
  // service and initialize the service settings.

  if ( NRF_SUCCESS == result ) { result = telemetry_register ( application->settings.telemetry.interval, &(application->settings.telemetry.limits.lower), &(application->settings.telemetry.limits.upper) ); }
  if ( NRF_SUCCESS == result ) { result = handling_register ( &(application->settings.handling.limits) ); }
  //if ( NRF_SUCCESS == result ) { result = records_register ( application->settings.telemetry.archival ); }

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

    if ( tags ) { result = nfct_data ( tags ); free ( tags ); }
    else { result = NRF_ERROR_NO_MEM; }

    }

  // Request the NFC device and register to receive tag scan notices.

  if ( NRF_SUCCESS == result ) { result = nfct_request ( ); }
  if ( NRF_SUCCESS == result ) { nfct_notice ( NFCT_NOTICE_READY, &(application->status), APPLICATION_EVENT_TAGGED ); }

  // Return with result.

  return ( result );

  }


//=============================================================================
// SECTION : PERSISTENT SETTINGS
//=============================================================================

//-----------------------------------------------------------------------------
//  function: application_settings ( application )
// arguments: application - application resource
//
// The settings have changed so update the settings file.
//-----------------------------------------------------------------------------

void application_settings ( application_t * application ) {

  file_handle_t              settings = file_open ( APPLICATION_FILE, FILE_MODE_WRITE | FILE_MODE_READ );

  // Update the persistent settings.

  if ( settings > 0 ) { ctl_events_clear ( &(application->status), APPLICATION_STATE_CHANGED ); }
  else return;

  file_write ( settings, &(application->settings), sizeof(application_settings_t) );
  file_close ( settings );

  #ifdef DEBUG
  debug_printf ( "\r\nSettings: update." );
  #endif

  }


//=============================================================================
// SECTION : SHUTDOWN PREPARATION
//=============================================================================

//-----------------------------------------------------------------------------
//  function: application_shutdown ( application )
// arguments: application - application resource
//
// Shut down the various modules and services before stopping the application.
//-----------------------------------------------------------------------------

void application_shutdown ( application_t * application ) {

  bool                         active = false;

  // Make sure that the indicator is off before shutting down.

  if ( application->option & PLATFORM_OPTION_INDICATE ) { indicator_off ( ); }

  // Shutdown the various modules to make sure things are cleaned up.

  if ( NRF_SUCCESS == beacon_state ( &(active) ) && active ) { beacon_close ( ); }
  if ( NRF_SUCCESS == peripheral_state ( &(active), NULL ) && active ) { peripheral_close ( ); }

  if ( application->option & (PLATFORM_OPTION_MOTION | PLATFORM_OPTION_PRESSURE | PLATFORM_OPTION_HUMIDITY) ) {

    sensors_close ( );
    //archive_close ( );

    }

  // If there are any pending status changes, make sure that they are written out.

  if ( application->status & APPLICATION_STATE_CHANGED ) { application_settings ( application ); }

  // Pause for a short delay before releasing for shutdown.

  ctl_delay ( APPLICATION_SHUTDOWN_DELAY );

  #ifdef DEBUG
  debug_printf ( "\r\nShutting down." );
  #endif

  }


//=============================================================================
// SECTION : PERIODIC EVENTS
//=============================================================================

//-----------------------------------------------------------------------------
//  function: application_periodic ( application )
// arguments: application - application resource
//
// Periodic application check-in.
//-----------------------------------------------------------------------------

void application_periodic ( application_t * application ) {

  bool                         active = false;
  bool                         linked = false;

  // Perform a periodic check of the battery, in case some status has changed.

  if ( application->option & PLATFORM_OPTION_POWER ) { power_check ( ); }

  // Request that the storage flush any pending writes and return to sleep.

  if ( application->option & (PLATFORM_OPTION_INTERNAL | PLATFORM_OPTION_EXTERNAL) ) { storage_sleep ( ); }

  // If the peripheral is advertising or linked to a peer, there is system
  // activity. If the beacon is advertising, there is also system activity.

  if ( NRF_SUCCESS == peripheral_state ( &(active), &(linked) ) ) { if ( active || linked ) return; }
  if ( NRF_SUCCESS == beacon_state ( &(active) ) ) { if ( active ) return; }

  // As long as the system is not currently charging or charged, it is
  // safe to shut down.

  if ( !(application->status & (APPLICATION_STATE_CHARGED | APPLICATION_STATE_CHARGER)) ) { ctl_events_set ( &(application->status), APPLICATION_EVENT_SHUTDOWN ); }

  }

//-----------------------------------------------------------------------------
//  function: application_schedule ( application )
// arguments: application - application resource
//
// Scheduled check-in.
//-----------------------------------------------------------------------------

void application_schedule ( application_t * application ) {

  // NOTE: this is triggered by the tick( ) callback

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
// SECTION : APPLICATION VISUAL INDICATION
//=============================================================================

//-----------------------------------------------------------------------------
//  function: application_indicate ( application )
// arguments: application - application resource
//
// Update the indicator status based on the current state flags.
//-----------------------------------------------------------------------------

void application_indicate ( application_t * application ) {

  static application_indicate_t state = APPLICATION_INDICATE_NONE;
  application_indicate_t         mode = APPLICATION_INDICATE_NONE;

  // Determine the appropriate indication mode from state bits.

  if ( application->status & APPLICATION_STATE_BLINKER ) { mode = APPLICATION_INDICATE_BLINKER; }
  else if ( application->status & APPLICATION_STATE_CONNECT ) { mode = APPLICATION_INDICATE_CONNECT; }
  else if ( application->status & APPLICATION_STATE_CHARGED ) { mode = APPLICATION_INDICATE_CHARGED; }
  else if ( application->status & APPLICATION_STATE_CHARGER ) { mode = APPLICATION_INDICATE_CHARGER; }
  else if ( application->status & APPLICATION_STATE_PROBLEM ) { mode = APPLICATION_INDICATE_PROBLEM; }
  else if ( application->status & APPLICATION_STATE_BATTERY ) { mode = APPLICATION_INDICATE_BATTERY; }

  // If the indicator state has changed, update the indicator.

  if ( state != mode ) switch ( (state = mode) ) {

    case APPLICATION_INDICATE_PROBLEM:  indicator_blink ( 1.0, 0.0, 0.0, 0.125, 4.825 ); break;
    case APPLICATION_INDICATE_BATTERY:  indicator_blink ( 0.5, 0.5, 0.0, 0.125, 4.825 ); break;
    case APPLICATION_INDICATE_CHARGER:  indicator_pulse ( 1.0, 1.0, 0.0, 1.0, 3.0 ); break;
    case APPLICATION_INDICATE_CHARGED:  indicator_color ( 0.0, 1.0, 0.0 ); break;
    case APPLICATION_INDICATE_CONNECT:  indicator_pulse ( 0.0, 0.0, 1.0, 1.0, 3.0 ); break;
    case APPLICATION_INDICATE_BLINKER:  indicator_blink ( 1.0, 1.0, 1.0, 0.250, 1.750 ); break;

    default:                            indicator_off ( ); break;

    }

  }


//=============================================================================
// SECTION : APPLICATION CONNECTION EVENT PROCESSING
//=============================================================================

//-----------------------------------------------------------------------------
//  function: application_tagged ( application )
// arguments: application - application resource
//
// Respond to an NFC tagging event.
//-----------------------------------------------------------------------------

void application_tagged ( application_t * application ) {

  // Whenever the NFC is tagged, switch the bluetooth profile from beacon to
  // peripheral.

  if ( application->option & APPLICATION_OPTION_BLE ) {

    bool                       active = false;
    bool                       linked = false;

    // If the beacon is active, cease broadcasting. If the peripheral is inactive
    // and not currently linked, start broadcasting.

    if ( NRF_SUCCESS == beacon_state ( &(active) ) && (active) ) { beacon_cease ( ); }
    if ( NRF_SUCCESS == peripheral_state ( &(active), &(linked) ) && !(active) && !(linked) ) {

      peripheral_begin ( PERIPHERAL_BROADCAST_RATE, PERIPHERAL_BROADCAST_PERIOD, PERIPHERAL_BROADCAST_POWER );

      }

    }

  }

//-----------------------------------------------------------------------------
//  function: application_attach ( application )
// arguments: application - application resource
//
// Respond to a BLE peripheral peer attachment.
//-----------------------------------------------------------------------------

void application_attach ( application_t * application ) {

  #ifdef DEBUG
  debug_printf ( "\r\nPeripheral: attach" );
  #endif

  // Update the tracking window in the control service and switch the telemetry
  // rate to the connected setting.

  control_window ( application->settings.tracking.time.opened, application->settings.tracking.time.closed );

  sensors_begin ( TELEMETRY_SERVICE_INTERVAL );

  // Raise the connected state and lower the nominal and problem states.

  application_raise ( application, APPLICATION_STATE_CONNECT );
  application_lower ( application, APPLICATION_STATE_NOMINAL | APPLICATION_STATE_PROBLEM );

  }

//-----------------------------------------------------------------------------
//  function: application_closed ( application )
// arguments: application - application resource
//
// Respond to a BLE peripheral peer detachment.
//-----------------------------------------------------------------------------

void application_detach ( application_t * application ) {

  #ifdef DEBUG
  debug_printf ( "\r\nPeripheral: detach" );
  #endif

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

  // Retrieve the settings values from the various connected services.

  //records_settings ( &(application->settings.telemetry.archival) );
  handling_settings ( &(application->settings.handling.limits) );
  telemetry_settings ( &(application->settings.telemetry.interval), &(application->settings.telemetry.limits.lower), &(application->settings.telemetry.limits.upper) );

  // Set the sensor telemetry interval to the new settings.

  sensors_begin ( application->settings.telemetry.interval );

  // If the tracking window is open, activate the archiver and start the
  // telemetry beacon.

  if ( application->settings.tracking.time.opened ) {

    //if ( ! application->settings.tracking.time.closed ) { archive_begin ( application->settings.telemetry.archival ); }
    //else { archive_cease ( ); }

    if ( ! application->settings.tracking.time.closed ) { beacon_begin ( BEACON_BROADCAST_RATE, BEACON_BROADCAST_PERIOD, BEACON_BROADCAST_POWER ); }
    else { beacon_cease ( ); }

    }

  // Lower the connected state and raise the changed status.

  application_raise ( application, APPLICATION_STATE_CHANGED );
  application_lower ( application, APPLICATION_STATE_CONNECT );

  }

//-----------------------------------------------------------------------------
//  function: application_probed ( application )
// arguments: application - application resource
//
// Respond to a BLE beacon scan response probe.
//-----------------------------------------------------------------------------

void application_probed ( application_t * application ) {

  }

//-----------------------------------------------------------------------------
//  function: application_expire ( application )
// arguments: application - application resource
//
// Peripheral advertising period has expired.
//-----------------------------------------------------------------------------

void application_expire ( application_t * application ) {

  #ifdef DEBUG
  debug_printf ( "\r\nAdvertisement: expired" );
  #endif

  }

//-----------------------------------------------------------------------------
//  function: application_strobe ( application )
// arguments: application - application resource
//
// Respond to an identity strobe request.
//-----------------------------------------------------------------------------

void application_strobe ( application_t * application ) {

   unsigned                  duration = 0;

  if ( NRF_SUCCESS == control_identify ( &(duration) ) && duration ) { ctl_timer_start ( CTL_TIMER_SINGULAR, &(application->status), APPLICATION_EVENT_CANCEL, duration ); }
  if ( duration ) { application_raise ( application, APPLICATION_STATE_BLINKER ); }

  }

//-----------------------------------------------------------------------------
//  function: application_cancel ( application )
// arguments: application - application resource
//
// Cancel any strobe requests.
//-----------------------------------------------------------------------------

void application_cancel ( application_t * application ) {

  application_lower ( application, APPLICATION_STATE_BLINKER );

  }


//=============================================================================
// SECTION : APPLICATION BATTERY AND CHARGING
//=============================================================================

//-----------------------------------------------------------------------------
//  function: application_charger ( application )
// arguments: application - application resource
//
// Respond to a charger status change event.
//-----------------------------------------------------------------------------

void application_charger ( application_t * application ) {

  power_status_t               status = POWER_STATUS_DISCHARGING;

  if ( NRF_SUCCESS == power_status ( &(status) ) ) switch ( status ) {

    case POWER_STATUS_CHARGING: application_lower ( application, APPLICATION_STATE_CHARGED );
                                application_raise ( application, APPLICATION_STATE_CHARGER );
                                battery_status ( BATTERY_STATUS_CONNECTED | BATTERY_STATUS_CHARGING );
                                break;

    case POWER_STATUS_CHARGED:  application_lower ( application, APPLICATION_STATE_CHARGER );
                                application_raise ( application, APPLICATION_STATE_CHARGED );
                                battery_status ( BATTERY_STATUS_CONNECTED | BATTERY_STATUS_CHARGED );
                                break;

    default:                    application_lower ( application, APPLICATION_STATE_CHARGER | APPLICATION_STATE_CHARGED );
                                battery_status ( BATTERY_STATUS_CONNECTED );
                                break;

    }

  }

//-----------------------------------------------------------------------------
//  function: application_battery ( application )
// arguments: application - application resource
//
// Respond to a battery status change event.
//-----------------------------------------------------------------------------

void application_battery ( application_t * application ) {

  float                       percent = 0;
  float                       voltage = 0;

  if ( NRF_SUCCESS == power_levels ( &(percent), &(voltage), NULL ) ) {

    if ( voltage <= CRITICAL_BATTERY_THRESHOLD ) { application_raise ( application, APPLICATION_STATE_BATTERY ); }
    else { application_lower ( application, APPLICATION_STATE_BATTERY ); }

    beacon_battery ( (char) roundf( percent ) );
    battery_charge ( percent );

    }

  }


//=============================================================================
// SECTION : SENSOR TELEMETRY EVENTS
//=============================================================================

//-----------------------------------------------------------------------------
//  function: application_sensor ( application )
// arguments: application - application resource
//
// Handle a notice that the environemental telemetry metrics have been updated.
//-----------------------------------------------------------------------------

void application_sensor ( application_t * application ) {

  telemetry_values_t        telemetry = { 0 };

  // Capture the telemetry values and update the telemetry service characteristics.

  if ( NRF_SUCCESS == sensors_telemetry ( &(telemetry) ) ) { telemetry_values ( &(telemetry) ); }
  else return;

  // If the tracking window is open, record compliance for the various
  // telemetry values.

  if ( application->settings.tracking.time.opened && !(application->settings.tracking.time.closed) ) {
  
    // Record surface temperature compliance.

    if ( application->settings.telemetry.limits.lower.surface < application->settings.telemetry.limits.upper.surface ) {
    
      if ( (telemetry.surface < application->settings.telemetry.limits.lower.surface)
        || (telemetry.surface > application->settings.telemetry.limits.upper.surface) ) { application->compliance.surface.excursion += application->settings.telemetry.interval; }
        else { application->compliance.surface.incursion += application->settings.telemetry.interval; }
      
      }

    // Record ambient temperature compliance.

    if ( application->settings.telemetry.limits.lower.ambient < application->settings.telemetry.limits.upper.ambient ) {

      if ( (telemetry.ambient < application->settings.telemetry.limits.lower.ambient)
        || (telemetry.ambient > application->settings.telemetry.limits.upper.ambient) ) { application->compliance.ambient.excursion += application->settings.telemetry.interval; }
        else { application->compliance.ambient.incursion += application->settings.telemetry.interval; }

      }

    // Record air humidity compliance.

    if ( application->settings.telemetry.limits.lower.humidity < application->settings.telemetry.limits.upper.humidity ) {

      if ( (telemetry.humidity < application->settings.telemetry.limits.lower.humidity)
        || (telemetry.humidity > application->settings.telemetry.limits.upper.humidity) ) { application->compliance.humidity.excursion += application->settings.telemetry.interval; }
        else { application->compliance.humidity.incursion += application->settings.telemetry.interval; }

      }

    // Record air pressure compliance.

    if ( application->settings.telemetry.limits.lower.pressure < application->settings.telemetry.limits.upper.pressure ) {

      if ( (telemetry.pressure < application->settings.telemetry.limits.lower.pressure)
        || (telemetry.pressure > application->settings.telemetry.limits.upper.pressure) ) { application->compliance.pressure.excursion += application->settings.telemetry.interval; }
        else { application->compliance.pressure.incursion += application->settings.telemetry.interval; }

      }

    // Check compliance against limits.

    ctl_events_set ( &(application->status), APPLICATION_EVENT_LIMITS );

    }

  // Update the beacon surface temperature and atmospherics.

  beacon_surface ( telemetry.surface, application->compliance.surface.incursion, application->compliance.surface.excursion );
  beacon_ambient ( telemetry.ambient, application->compliance.ambient.incursion, application->compliance.ambient.excursion );
  beacon_humidity ( telemetry.humidity, application->compliance.humidity.incursion, application->compliance.humidity.excursion );
  beacon_pressure ( telemetry.pressure, application->compliance.pressure.incursion, application->compliance.pressure.excursion );

  }

//-----------------------------------------------------------------------------
//  function: application_angles ( application )
// arguments: application - application resource
//
// Handle a notice that the angles and forces have updated.
//-----------------------------------------------------------------------------

void application_angles ( application_t * application ) {

  handling_values_t          handling = { 0 };

  // Capture the motion values and update the handling service characteristics.

  if ( (NRF_SUCCESS == sensors_handling ( &(handling) )) && (handling.face) ) { handling_values ( &(handling) ); }
  else return;

  // If the tracking window is open, check against limits.

  if ( application->settings.tracking.time.opened && !(application->settings.tracking.time.closed) ) {

    ctl_events_set ( &(application->status), APPLICATION_EVENT_LIMITS );

    }

  // Update the beacon angle and orientation face.

  beacon_orientation ( handling.angle, handling.face );

  }

//-----------------------------------------------------------------------------
//  function: application_orient ( application )
// arguments: application - application resource
//
// Handle a notice that the orientation has changed.
//-----------------------------------------------------------------------------

void application_orient ( application_t * application ) {

  handling_values_t          handling = { 0 };

  // Check the orientation change against the preferred orientation, if defined,
  // and if there is a mismatch, trip the misorientation compliance count.

  if ( (NRF_SUCCESS == sensors_handling ( &(handling) )) && (handling.face) ) {

    if ( application->settings.handling.limits.face && (handling.face != application->settings.handling.limits.face) ) { ++ application->compliance.misorient; }
    if ( application->compliance.misorient ) { beacon_misoriented ( ); }

    }

  }

//-----------------------------------------------------------------------------
//  function: application_limits ( application )
// arguments: application - application resource
//
// 
//-----------------------------------------------------------------------------

void application_limits ( application_t * application ) {

  // If an angle limit has been established, check against the limit and set
  // the tipping count if appropriate.

  if ( application->settings.handling.limits.angle ) {

    float                       limit = 0;

    if ( (NRF_SUCCESS == sensors_angles ( &(limit) )) &&  (limit > application->settings.handling.limits.angle) ) { ++ application->compliance.tipped; }
    if ( application->compliance.tipped ) { beacon_tipped ( ); }

    }

  // If an force limit has been established, check against the limit and set
  // the bump count if appropriate.

  if ( application->settings.handling.limits.force ) {

    float                       limit = 0;

    if ( (NRF_SUCCESS == sensors_forces ( &(limit) )) && (limit > application->settings.handling.limits.force) ) { ++ application->compliance.bumped; }
    if ( application->compliance.bumped ) { beacon_bumped ( ); }

    }
    
  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void application_fallen ( application_t * application ) {

  if ( ++ application->compliance.dropped ) { beacon_dropped ( ); }

  }


//=============================================================================
// SECTION : APPLICATION STATE UPDATES
//=============================================================================

//-----------------------------------------------------------------------------
//  function: application_raise ( application, state )
// arguments: application - application resource
//            state - state flags to raise
//
// Raise the given application state flags and update indication.
//-----------------------------------------------------------------------------

static void application_raise ( application_t * application, unsigned state ) {

  unsigned                     status = application->status & APPLICATION_STATUS_STATES;
  application->status                 = application->status | (state & APPLICATION_STATUS_STATES);

  if ( status ^ (application->status & APPLICATION_STATUS_STATES) ) {
    if ( application->option & PLATFORM_OPTION_INDICATE ) { ctl_events_set ( &(application->status), APPLICATION_EVENT_INDICATE ); }
    }

  }

//-----------------------------------------------------------------------------
//  function: application_lower ( application, state )
// arguments: application - application resource
//            state - state flags to raise
//
// Lower the given application state flags and update indication.
//-----------------------------------------------------------------------------

static void application_lower ( application_t * application, unsigned state ) {

  unsigned                     status = application->status & APPLICATION_STATUS_STATES;
  application->status                 = application->status & ~(state & APPLICATION_STATUS_STATES);

  if ( status ^ (application->status & APPLICATION_STATUS_STATES) ) {
    if ( application->option & PLATFORM_OPTION_INDICATE ) { ctl_events_set ( &(application->status), APPLICATION_EVENT_INDICATE ); }
    }

  }