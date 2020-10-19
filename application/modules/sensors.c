//=============================================================================
// project: ShockVx
//  module: Stickershock Sensor for cold chain tracking.
//  author: Velvetwire, llc
//    file: sensors.c
//
//
// (c) Copyright 2016-2020 Velvetwire, LLC. All rights reserved.
//=============================================================================

#include  <stickershock.h>

#include  "bluetooth.h"
#include  "sensors.h"

//=============================================================================
// SECTION : 
//=============================================================================

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static CTL_TASK_t *            thread = NULL;
static sensors_t             resource = { 0 };

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned sensors_start ( unsigned option ) {

  sensors_t *                 sensors = &(resource);

  // Make sure that the module has not already been started.

  if ( thread == NULL ) { ctl_mutex_init ( &(sensors->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );

  // Spawn the module daemon.

  if ( (thread = ctl_spawn ( "sensors", (CTL_TASK_ENTRY_t) sensors_manager, sensors, SENSORS_MANAGER_STACK, SENSORS_MANAGER_PRIORITY )) ) { sensors->option = option; }
  else return ( NRF_ERROR_NO_MEM );

  // Module started.

  return ( NRF_SUCCESS );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned sensors_begin ( float interval, float archival ) {

  sensors_t *                 sensors = &(resource);
  CTL_TIME_t                   period = (CTL_TIME_t) roundf ( interval * 1000.0 );
  unsigned                     result = NRF_SUCCESS;
  
  // Make sure that the module has been started.

  if ( thread ) { ctl_mutex_lock_uc ( &(sensors->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );

  // Clear any pending telemetry, update the period and request a settings
  // refresh to re-start telemetry with the new period.
  //
  // Note: we also reset the archive window and elapsed time counter.

  if ( (sensors->period = period) > SENSORS_PERIOD_MINIMUM ) {
    
    sensors->archive.window           = (CTL_TIME_t) roundf ( archival * 1000.0 );
    sensors->archive.elapse           = (CTL_TIME_t) 0;

    ctl_events_set_clear ( &(sensors->status), SENSORS_EVENT_SETTINGS, SENSORS_EVENT_PERIODIC );
    
    } else { result = NRF_ERROR_INVALID_PARAM; }

  // Free the resource and return with the result.

  return ( ctl_mutex_unlock ( &(sensors->mutex) ), result );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned sensors_cease ( void ) {

  sensors_t *                 sensors = &(resource);
  unsigned                     result = NRF_SUCCESS;
  
  // Make sure that the module has been started.

  if ( thread ) { ctl_mutex_lock_uc ( &(sensors->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );

  // Cancel periodic telemetry.

  ctl_events_set_clear ( &(sensors->status), SENSORS_EVENT_STANDBY, SENSORS_EVENT_PERIODIC );

  // Free the resource and return with the result.

  return ( ctl_mutex_unlock ( &(sensors->mutex) ), result );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned sensors_close ( void ) {

  sensors_t *                 sensors = &(resource);

  // If the module thread has not been started, there is nothing to close.

  if ( thread ) { ctl_events_set ( &(sensors->status), SENSORS_EVENT_SHUTDOWN ); }
  else return ( NRF_ERROR_INVALID_STATE );

  // Wait for the the module thread to acknowlege shutdown.

  if ( ctl_events_wait ( CTL_EVENT_WAIT_ALL_EVENTS, &(sensors->status), SENSORS_STATE_CLOSED, CTL_TIMEOUT_DELAY, SENSORS_CLOSE_TIMEOUT ) ) { thread = NULL; }
  else return ( NRF_ERROR_TIMEOUT );

  // Wipe the module resource data.

  memset ( sensors, 0, sizeof(sensors_t) );

  // Module successfully shut down.

  return ( NRF_SUCCESS );

  }

//-----------------------------------------------------------------------------
//  function: sensors_notice ( notice, set, events )
// arguments: notice - the notice index to enable or disable
//            set - event set to trigger (NULL to disable)
//            events - event bits to set
//   returns: NRF_SUCCESS - if notice enabled
//            NRF_ERROR_INVALID_PARAM - if the notice index is not valid
//
// Register for notices with the telemetry module.
//-----------------------------------------------------------------------------

unsigned sensors_notice ( sensors_notice_t notice, CTL_EVENT_SET_t * set, CTL_EVENT_SET_t events ) {
  
  sensors_t *                 sensors = &(resource);

  // Make sure that the requested notice is valid and register the notice.

  if ( notice < SENSORS_NOTICES ) { ctl_mutex_lock_uc ( &(sensors->mutex) ); }
  else return ( NRF_ERROR_INVALID_PARAM );
  
  sensors->notice[ notice ].set       = set;
  sensors->notice[ notice ].events    = events;
  
  // Notice registered.

  return ( ctl_mutex_unlock ( &(sensors->mutex) ), NRF_SUCCESS );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned sensors_temperature ( float * temperature ) {

  sensors_t *                 sensors = &(resource);
  unsigned                     result = NRF_SUCCESS;

  // Make sure that the module has been started.

  if ( thread ) { ctl_mutex_lock_uc ( &(sensors->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );
  
  if ( temperature ) {
    
    if ( sensors->status & SENSORS_VALUE_SURFACE ) { *(temperature) = sensors->surface.temperature; }
    else { result = NRF_ERROR_NOT_FOUND; }

    }

  // Free the resource and return with result.

  return ( ctl_mutex_unlock ( &(sensors->mutex) ), result );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned sensors_atmosphere ( float * temperature, float * humidity, float * pressure ) {

  sensors_t *                 sensors = &(resource);
  unsigned                     result = NRF_SUCCESS;

  // Make sure that the module has been started.

  if ( thread ) { ctl_mutex_lock_uc ( &(sensors->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );
  
  if ( temperature ) {

    if ( sensors->status & SENSORS_VALUE_AMBIENT ) { *(temperature) = sensors->humidity.temperature; }
    else { result = NRF_ERROR_NOT_FOUND; }

    }

  if ( humidity ) {

    if ( sensors->status & SENSORS_VALUE_HUMIDITY ) { *(humidity) = sensors->humidity.measurement; }
    else { result = NRF_ERROR_NOT_FOUND; }

    }

  if ( pressure ) {

    if ( sensors->status & SENSORS_VALUE_PRESSURE ) { *(pressure) = sensors->pressure.measurement; }
    else { result = NRF_ERROR_NOT_FOUND; }

    }

  // Free the resource and return with result.

  return ( ctl_mutex_unlock ( &(sensors->mutex) ), result );
  
  }


//=============================================================================
// SECTION : BEACON MANAGER THREAD
//=============================================================================

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void sensors_manager ( sensors_t * sensors ) {

  // Process thread events until a shutdown request has been issued
  // from outside the thread.

  forever {

    CTL_EVENT_SET_t            events = SENSORS_MANAGER_EVENTS;
    CTL_EVENT_SET_t            status = ctl_events_wait_uc ( CTL_EVENT_WAIT_ANY_EVENTS_WITH_AUTO_CLEAR, &(sensors->status), events );

    // If shutdown is requested, perform shutdown requests and break out of the loop.

    if ( status & SENSORS_EVENT_SHUTDOWN ) { sensors_shutdown ( sensors ); break; }
    if ( status & SENSORS_EVENT_SETTINGS ) { sensors_settings ( sensors ); }
    if ( status & SENSORS_EVENT_STANDBY ) { sensors_standby ( sensors ); }

    // Respond to the periodic event.

    if ( status & SENSORS_EVENT_PERIODIC ) { sensors_periodic ( sensors ); }

    }

  // Indicate that the manager thread is closed.

  ctl_events_init ( &(sensors->status), SENSORS_STATE_CLOSED ); 

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void sensors_shutdown ( sensors_t * sensors ) {

  // Clear the validity of the telemetry measurements and stop the periodic timer.

  ctl_events_clear ( &(sensors->status), SENSORS_VALUE_HUMIDITY | SENSORS_VALUE_PRESSURE | SENSORS_VALUE_AMBIENT | SENSORS_VALUE_STANDBY | SENSORS_VALUE_SURFACE );
  ctl_timer_clear ( &(sensors->status), SENSORS_EVENT_PERIODIC );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void sensors_settings ( sensors_t * sensors ) {

  // Start the periodic telemetry timer.

  ctl_timer_start ( CTL_TIMER_CYCLICAL, &(sensors->status), SENSORS_EVENT_PERIODIC, sensors->period );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void sensors_standby ( sensors_t * sensors ) {

  // Clear the validity of the telemetry measurements and stop the periodic timer.

  ctl_events_clear ( &(sensors->status), SENSORS_VALUE_HUMIDITY | SENSORS_VALUE_PRESSURE | SENSORS_VALUE_AMBIENT | SENSORS_VALUE_STANDBY | SENSORS_VALUE_SURFACE );
  ctl_timer_clear ( &(sensors->status), SENSORS_EVENT_PERIODIC );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void sensors_periodic ( sensors_t * sensors ) {

  // Start by reading the CPU core die temperature as the surface temperature.

  if ( NRF_SUCCESS == softdevice_temperature ( &(sensors->surface.temperature) ) ) { sensors->status |= (SENSORS_VALUE_SURFACE); }
  else { sensors->status &= ~(SENSORS_VALUE_SURFACE); }

  // Read the ambient temperature and humidity from the humidity sensor. If the
  // core temperature is low and the air temperature is high, assume that the
  // air temperature sensor has wrapped and adjust.

  if ( sensors->option & PLATFORM_OPTION_HUMIDITY ) {
    
    if ( NRF_SUCCESS == humidity_measurement ( &(sensors->humidity.measurement), &(sensors->humidity.temperature) ) ) { sensors->status |= (SENSORS_VALUE_HUMIDITY | SENSORS_VALUE_AMBIENT); }
    else { sensors->status &= ~(SENSORS_VALUE_HUMIDITY | SENSORS_VALUE_AMBIENT); }

    if ( (sensors->surface.temperature < ((float) - 20.0) && (sensors->humidity.temperature > ((float) 60.0))) ) {

      sensors->humidity.temperature = sensors->humidity.temperature - ((float) 175.72);
      sensors->humidity.measurement = sensors->humidity.measurement * ((float) 0.5);

      }

    }

  // Read the standby temperature and pressure from the pressure sensor.

  if ( sensors->option & PLATFORM_OPTION_PRESSURE ) {
    
    if ( NRF_SUCCESS == pressure_measurement ( &(sensors->pressure.measurement), &(sensors->pressure.temperature) ) ) { sensors->status |= (SENSORS_VALUE_PRESSURE | SENSORS_VALUE_STANDBY); }
    else { sensors->status &= ~(SENSORS_VALUE_PRESSURE | SENSORS_VALUE_STANDBY); }

    }

  // Issue a telemetry update notice

  ctl_notice ( sensors->notice + SENSORS_NOTICE_TELEMETRY );

  // If there is an established archive window and the window has elapsed,
  // issue an archival event.

  if ( sensors->archive.window ) {

    sensors->archive.elapse           = sensors->archive.elapse + sensors->period;
    if ( sensors->archive.elapse >= sensors->archive.window ) { ctl_notice ( sensors->notice + SENSORS_NOTICE_ARCHIVE ); }
    sensors->archive.elapse           = sensors->archive.elapse % sensors->archive.window;
    
    }

  }
