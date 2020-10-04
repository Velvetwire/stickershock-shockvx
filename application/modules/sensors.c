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

  if ( thread == NULL ) { ctl_mutex_init ( &(sensors->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );

  sensors->option                     = option;

  if ( (thread = ctl_spawn ( "sensors", (CTL_TASK_ENTRY_t) sensors_manager, sensors, SENSORS_MANAGER_STACK, SENSORS_MANAGER_PRIORITY )) ) {
    
    ctl_events_init ( &(sensors->status), SENSORS_EVENT_SETTINGS );

    } else return ( NRF_ERROR_NO_MEM );

  return ( NRF_SUCCESS );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned sensors_begin ( float interval ) {

  sensors_t *                 sensors = &(resource);
  CTL_TIME_t                   period = (CTL_TIME_t) roundf ( interval * 1000.0 );
  unsigned                     result = NRF_SUCCESS;
  
  // Make sure that the module has been started.

  if ( thread ) { ctl_mutex_lock_uc ( &(sensors->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );

  // Start or cancel the periodic telemetry timer.

  if ( period ) {
    
    ctl_events_set_clear ( &(sensors->status), SENSORS_EVENT_PERIODIC, SENSORS_STATE_VECTORS | SENSORS_STATE_FREEFALL );
    ctl_timer_start ( CTL_TIMER_CYCLICAL, &(sensors->status), SENSORS_EVENT_PERIODIC, period );
    
    } else { ctl_timer_clear ( &(sensors->status), SENSORS_EVENT_PERIODIC ); }

  #ifdef DEBUG
  debug_printf ( "\r\nSensors: (%f)", interval );
  #endif

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

  // Clear the periodic timer and any pending event.

  ctl_events_clear ( &(sensors->status), SENSORS_EVENT_PERIODIC );
  ctl_timer_clear ( &(sensors->status), SENSORS_EVENT_PERIODIC );

  #ifdef DEBUG
  debug_printf ( "\r\nSensors: off" );
  #endif

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

unsigned sensors_telemetry ( telemetry_values_t * telemetry ) {

  sensors_t *                 sensors = &(resource);
  unsigned                     result = NRF_SUCCESS;

  // Make sure that the module has been started.

  if ( thread ) { ctl_mutex_lock_uc ( &(sensors->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );
  
  // Retrieve the telemetry measurements and populate the given structure.

  if ( sensors->status & (SENSORS_VALUE_SURFACE | SENSORS_VALUE_AMBIENT | SENSORS_VALUE_HUMIDITY | SENSORS_VALUE_PRESSURE) ) {

    if ( telemetry ) {

      telemetry->surface              = sensors->surface.temperature;
      telemetry->ambient              = sensors->humidity.temperature;
      telemetry->humidity             = sensors->humidity.measurement;
      telemetry->pressure             = sensors->pressure.measurement;

      }

    } else { result = NRF_ERROR_NULL; }

  // Free the resource and return with result.

  return ( ctl_mutex_unlock ( &(sensors->mutex) ), result );
  
  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned sensors_handling ( handling_values_t * handling ) {

  sensors_t *                 sensors = &(resource);
  unsigned                     result = NRF_SUCCESS;

  // Make sure that the module has been started.

  if ( thread ) { ctl_mutex_lock_uc ( &(sensors->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );
  
  // Retrieve the handling measurements and populate the given structure.

  if ( sensors->status & SENSORS_STATE_VECTORS ) {

    if ( handling ) {

      handling->force                 = sensors->motion.force;
      handling->angle                 = sensors->tilt.angle;
      handling->face                  = sensors->tilt.orientation;

      }

    } else { result = NRF_ERROR_NULL; }

  // Free the resource and return with result.

  return ( ctl_mutex_unlock ( &(sensors->mutex) ), result );
    
  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned sensors_angles ( float * limit ) {

  sensors_t *                 sensors = &(resource);
  unsigned                     result = NRF_SUCCESS;

  // Make sure that the module has been started.

  if ( thread ) { ctl_mutex_lock_uc ( &(sensors->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );

  // Retrieve the and reset tilt range.

  if ( sensors->status & SENSORS_STATE_VECTORS ) {

    if ( limit ) { *(limit) = sensors->tilt.limit; }

    ctl_events_clear ( &(sensors->status), SENSORS_STATE_VECTORS );

    sensors->tilt.limit               = sensors->tilt.angle;

    } else { result = NRF_ERROR_NULL; }

  // Free the resource and return with result.

  return ( ctl_mutex_unlock ( &(sensors->mutex) ), result );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned sensors_forces ( float * limit ) {

  sensors_t *                 sensors = &(resource);
  unsigned                     result = NRF_SUCCESS;

  // Make sure that the module has been started.

  if ( thread ) { ctl_mutex_lock_uc ( &(sensors->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );

  // Retrieve and reset the handling forces.

  if ( sensors->status & SENSORS_STATE_VECTORS ) {

    if ( limit ) { *(limit) = sensors->motion.limit; }

    ctl_events_clear ( &(sensors->status), SENSORS_STATE_FREEFALL );

    sensors->motion.limit             = sensors->motion.force;
    
    } else { result = NRF_ERROR_NULL; }

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

    // Respond to the periodic event.

    if ( status & SENSORS_EVENT_PERIODIC ) { sensors_periodic ( sensors ); }

    // Respond to motion sensor events.

    if ( status & SENSORS_EVENT_ORIENTATION ) { sensors_orientation ( sensors ); }
    if ( status & SENSORS_EVENT_FREEFALL ) { sensors_freefall ( sensors ); }
    if ( status & SENSORS_EVENT_VECTORS ) { sensors_vectors ( sensors ); }
    if ( status & SENSORS_EVENT_ACTIVE ) { sensors_active ( sensors ); }
    if ( status & SENSORS_EVENT_ASLEEP ) { sensors_asleep ( sensors ); }

    }

  // Indicate that the manager thread is closed.

  ctl_events_init ( &(sensors->status), SENSORS_STATE_CLOSED ); 

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void sensors_shutdown ( sensors_t * sensors ) {

  // If the motion unit is enabled, make sure it is disabled before shutting
  // down telemetry.

  if ( sensors->option & PLATFORM_OPTION_MOTION ) { motion_disable ( ); }

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void sensors_settings ( sensors_t * sensors ) {

  // If the motion sensor option is enabled, calibrate the surface sensor
  // using the on-chip die temperature as the reference. Enable notices
  // from the sensor for orientation change, activity, free-fall and
  // updated vectors.

  if ( sensors->option & PLATFORM_OPTION_MOTION ) {
  
    float                 temperature = 0;

    motion_linear ( MOTION_RATE_50HZ, MOTION_RANGE_16G );
    //motion_linear ( MOTION_RATE_100HZ, MOTION_RANGE_16G );
    motion_options ( MOTION_OPTION_TEMPERATURE | MOTION_OPTION_VECTORS | MOTION_OPTION_FREEFALL );

    if ( NRF_SUCCESS == softdevice_temperature ( &(temperature) ) ) { motion_calibration ( temperature ); }
    if ( NRF_SUCCESS == motion_wakeup ( 0.25, 0.1, 0.0 ) ) { ctl_events_clear ( &(sensors->status), SENSORS_STATE_MOVEMENT ); }

    motion_notice ( MOTION_NOTICE_ORIENTATION, &(sensors->status), SENSORS_EVENT_ORIENTATION );
    motion_notice ( MOTION_NOTICE_FALLING, &(sensors->status), SENSORS_EVENT_FREEFALL );
    motion_notice ( MOTION_NOTICE_VECTORS, &(sensors->status), SENSORS_EVENT_VECTORS );
    motion_notice ( MOTION_NOTICE_ACTIVE, &(sensors->status), SENSORS_EVENT_ACTIVE );
    motion_notice ( MOTION_NOTICE_ASLEEP, &(sensors->status), SENSORS_EVENT_ASLEEP );

    motion_orientation ( &(sensors->tilt.orientation) );

    }
  
  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void sensors_periodic ( sensors_t * sensors ) {

  // Start by reading the CPU core die temperature.

  if ( NRF_SUCCESS == softdevice_temperature ( &(sensors->core.temperature) ) ) { sensors->status |= (SENSORS_VALUE_CORE); }
  else { sensors->status &= ~(SENSORS_VALUE_CORE); }

  // Read the ambient temperature and humidity from the humidity sensor.

  if ( sensors->option & PLATFORM_OPTION_HUMIDITY ) {
    
    if ( NRF_SUCCESS == humidity_measurement ( &(sensors->humidity.measurement), &(sensors->humidity.temperature) ) ) { sensors->status |= (SENSORS_VALUE_HUMIDITY | SENSORS_VALUE_AMBIENT); }
    else { sensors->status &= ~(SENSORS_VALUE_HUMIDITY | SENSORS_VALUE_AMBIENT); }
      
    }

  // Read the standby temperature and pressure from the pressure sensor.

  if ( sensors->option & PLATFORM_OPTION_PRESSURE ) {
    
    if ( NRF_SUCCESS == pressure_measurement ( &(sensors->pressure.measurement), &(sensors->pressure.temperature) ) ) { sensors->status |= (SENSORS_VALUE_PRESSURE | SENSORS_VALUE_STANDBY); }
    else { sensors->status &= ~(SENSORS_VALUE_PRESSURE | SENSORS_VALUE_STANDBY); }

    }

  // Read the surface temperature from the motion sensor.

  if ( sensors->option & PLATFORM_OPTION_MOTION ) {

    if ( NRF_SUCCESS == motion_temperature ( &(sensors->surface.temperature) ) ) { sensors->status |= (SENSORS_VALUE_SURFACE); }
    else { sensors->status &= ~(SENSORS_VALUE_SURFACE); }

    }

  // If both ambient and standby temperatures are available, check whether there is a large disparity
  // between them and use this to cold-bias the ambient temperature (which register wraps).

  if ( (sensors->status & (SENSORS_VALUE_STANDBY | SENSORS_VALUE_AMBIENT)) == (SENSORS_VALUE_STANDBY | SENSORS_VALUE_AMBIENT) ) {
    
    if ( (sensors->pressure.temperature < 15.0) && (sensors->humidity.temperature > 15.0) ) { sensors->humidity.temperature -= 175.72; }
    
    }

  // Issue a telemetry update notice

  ctl_notice ( sensors->notice + SENSORS_NOTICE_TELEMETRY );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void sensors_orientation ( sensors_t * sensors ) {

  if ( NRF_SUCCESS == motion_orientation ( &(sensors->tilt.orientation) ) ) { ctl_notice ( sensors->notice + SENSORS_NOTICE_ORIENTATION ); }

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void sensors_freefall ( sensors_t * sensors ) {

  ctl_events_set ( &(sensors->status), SENSORS_STATE_FREEFALL );
  ctl_notice ( sensors->notice + SENSORS_NOTICE_FREEFALL );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void __attribute__ (( optimize(2) )) sensors_vectors ( sensors_t * sensors ) {

  if ( NRF_SUCCESS == motion_vectors ( &(sensors->vectors.angular), &(sensors->vectors.linear) ) ) {
    
    float                      planar = (sensors->vectors.linear.x * sensors->vectors.linear.x) + (sensors->vectors.linear.y * sensors->vectors.linear.y);
    float                      vector = (sensors->vectors.linear.z * sensors->vectors.linear.z) + planar;
    float                      radius = sqrtf ( planar );
    float                       angle = (sensors->vectors.linear.z > 0) ? (90.0) : (-90.0);

    // Compute the tilt angle of the force vector if a magnitude exists.

    if ( radius ) { angle = atan2f ( sensors->vectors.linear.z, radius ) * 180.0 / M_PI; }

    // Compute the total force vector from the individual forces.

    sensors->motion.force             = sqrtf ( vector );

    // Adjust the neutral angle for the given orientation and make sure that it
    // does not exceed 90 degrees in either direction.

    if ( sensors->tilt.orientation == MOTION_ORIENTATION_FACEUP ) { angle -= 90.0; }
    if ( sensors->tilt.orientation == MOTION_ORIENTATION_FACEDOWN ) { angle += 90.0; }

    while ( angle > 90.0 ) { angle -= 90.0; }
    while ( angle < -90.0 ) { angle += 90.0; }

    // Get the absolute value of the angle offset from the neutral angle.

    sensors->tilt.angle               = fabsf ( angle );

    } else return;

  // If we already have vectors from a previous sample, adjust the force
  // and tilt angle limits. Otherwise, initialize the limits from the
  // current force and angle computations.

  if ( sensors->status & SENSORS_STATE_VECTORS ) {

    if ( sensors->tilt.angle > sensors->tilt.limit ) { sensors->tilt.limit = sensors->tilt.angle; }
    if ( sensors->motion.force > sensors->motion.limit ) { sensors->motion.limit = sensors->motion.force; }

    } else {
    
    sensors->tilt.limit               = sensors->tilt.angle;
    sensors->motion.limit             = sensors->motion.force;

    }

  // We now have vectors.

  ctl_events_set ( &(sensors->status), SENSORS_STATE_VECTORS );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void sensors_active ( sensors_t * sensors ) {
  
  ctl_events_set ( &(sensors->status), SENSORS_STATE_MOVEMENT );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void sensors_asleep ( sensors_t * sensors ) {

  ctl_events_clear ( &(sensors->status), SENSORS_STATE_MOVEMENT  );

  }