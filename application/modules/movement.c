//=============================================================================
// project: ShockVx
//  module: Stickershock Sensor for cold chain tracking.
//  author: Velvetwire, llc
//    file: movement.c
//
//
// (c) Copyright 2016-2020 Velvetwire, LLC. All rights reserved.
//=============================================================================

#include  <stickershock.h>

#include  "bluetooth.h"
#include  "movement.h"

//=============================================================================
// SECTION :
//=============================================================================

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static CTL_TASK_t *            thread = NULL;
static movement_t            resource = { 0 };

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned movement_start ( unsigned option ) {

  movement_t *               movement = &(resource);

  // Make sure that the module has not already been started.

  if ( thread == NULL ) { ctl_mutex_init ( &(movement->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );

  // Spawn the module daemon.

  if ( (thread = ctl_spawn ( "movement", (CTL_TASK_ENTRY_t) movement_manager, movement, MOVEMENT_MANAGER_STACK, MOVEMENT_MANAGER_PRIORITY )) ) { movement->option = option; }
  else return ( NRF_ERROR_NO_MEM );

  // Module started.

  return ( NRF_SUCCESS );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned movement_begin ( float interval ) {

  movement_t *               movement = &(resource);
  CTL_TIME_t                   period = (CTL_TIME_t) (interval * 1000);
  unsigned                     result = NRF_SUCCESS;

  // Make sure that the module has been started.

  if ( thread ) { ctl_mutex_lock_uc ( &(movement->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );

  // Start or cancel periodic telemetry.

  if ( (movement->period = period) ) { ctl_events_set_clear ( &(movement->status), MOVEMENT_EVENT_SETTINGS, MOVEMENT_EVENT_PERIODIC ); }
  else { ctl_events_set_clear ( &(movement->status), MOVEMENT_EVENT_STANDBY, MOVEMENT_EVENT_PERIODIC ); }

  // Free the resource and return with the result.

  return ( ctl_mutex_unlock ( &(movement->mutex) ), result );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned movement_cease ( void ) {

  movement_t *               movement = &(resource);
  unsigned                     result = NRF_SUCCESS;

  // Make sure that the module has been started.

  if ( thread ) { ctl_mutex_lock_uc ( &(movement->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );

  // Cancel periodic telemetry.

  ctl_events_set_clear ( &(movement->status), MOVEMENT_EVENT_STANDBY, MOVEMENT_EVENT_PERIODIC );

  // Free the resource and return with the result.

  return ( ctl_mutex_unlock ( &(movement->mutex) ), result );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned movement_close ( void ) {

  movement_t *               movement = &(resource);

  // If the module thread has not been started, there is nothing to close.

  if ( thread ) { ctl_events_set ( &(movement->status), MOVEMENT_EVENT_SHUTDOWN ); }
  else return ( NRF_ERROR_INVALID_STATE );

  // Wait for the the module thread to acknowlege shutdown.

  if ( ctl_events_wait ( CTL_EVENT_WAIT_ALL_EVENTS, &(movement->status), MOVEMENT_STATE_CLOSED, CTL_TIMEOUT_DELAY, MOVEMENT_CLOSE_TIMEOUT ) ) { thread = NULL; }
  else return ( NRF_ERROR_TIMEOUT );

  // Wipe the module resource data.

  memset ( movement, 0, sizeof(movement_t) );

  // Module successfully shut down.

  return ( NRF_SUCCESS );

  }

//-----------------------------------------------------------------------------
//  function: movement_notice ( notice, set, events )
// arguments: notice - the notice index to enable or disable
//            set - event set to trigger (NULL to disable)
//            events - event bits to set
//   returns: NRF_SUCCESS - if notice enabled
//            NRF_ERROR_INVALID_PARAM - if the notice index is not valid
//
// Register for notices with the telemetry module.
//-----------------------------------------------------------------------------

unsigned movement_notice ( movement_notice_t notice, CTL_EVENT_SET_t * set, CTL_EVENT_SET_t events ) {

  movement_t *               movement = &(resource);

  // Make sure that the requested notice is valid and register the notice.

  if ( notice < MOVEMENT_NOTICES ) { ctl_mutex_lock_uc ( &(movement->mutex) ); }
  else return ( NRF_ERROR_INVALID_PARAM );

  movement->notice[ notice ].set      = set;
  movement->notice[ notice ].events   = events;

  // Notice registered.

  return ( ctl_mutex_unlock ( &(movement->mutex) ), NRF_SUCCESS );

  }

//-----------------------------------------------------------------------------
// Retrieve the temperature from the motion sensor.
//-----------------------------------------------------------------------------

unsigned movement_temperature ( float * temperature ) {

  movement_t *               movement = &(resource);
  unsigned                     result = NRF_SUCCESS;

  // Make sure that the module has been started.

  if ( thread ) { ctl_mutex_lock_uc ( &(movement->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );

  if ( temperature ) { *(temperature) = movement->temperature; }

  // Free the resource and return with result.

  return ( ctl_mutex_unlock ( &(movement->mutex) ), result );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned movement_forces ( float * force, float * x, float * y, float * z ) {

  movement_t *               movement = &(resource);
  unsigned                     result = NRF_SUCCESS;

  // Make sure that the module has been started.

  if ( thread ) { ctl_mutex_lock_uc ( &(movement->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );

  if ( movement->status & MOVEMENT_STATE_VECTORS ) {

    if ( force ) { *(force) = movement->force.value; }

    if ( x ) { *(x) = movement->vectors.linear.x; }
    if ( y ) { *(y) = movement->vectors.linear.y; }
    if ( z ) { *(z) = movement->vectors.linear.z; }

    } else return ( NRF_ERROR_NULL );

  // Free the resource and return with result.

  return ( ctl_mutex_unlock ( &(movement->mutex) ), result );

  }


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned movement_angles ( float * angle, char * orientation ) {

  movement_t *               movement = &(resource);
  unsigned                     result = NRF_SUCCESS;

  // Make sure that the module has been started.

  if ( thread ) { ctl_mutex_lock_uc ( &(movement->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );

  if ( movement->status & MOVEMENT_STATE_VECTORS ) {

    if ( orientation ) { *(orientation) = movement->orientation; }
    if ( angle ) { *(angle) = movement->angle.value; }

    } else return ( NRF_ERROR_NULL );

  // Free the resource and return with result.

  return ( ctl_mutex_unlock ( &(movement->mutex) ), result );

  }

//-----------------------------------------------------------------------------
// Set the alarm limits for movement
//-----------------------------------------------------------------------------

unsigned movement_limits ( float force, float angle ) {

  movement_t *               movement = &(resource);
  unsigned                     result = NRF_SUCCESS;

  // Make sure that the module has been started.

  if ( thread ) { ctl_mutex_lock_uc ( &(movement->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );

  movement->force.limit               = force;
  movement->angle.limit               = angle;

  // Free the resource and return with result.

  return ( ctl_mutex_unlock ( &(movement->mutex) ), result );

  }


//=============================================================================
// SECTION : MOVEMENT MANAGER THREAD
//=============================================================================

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void movement_manager ( movement_t * movement ) {

  // Process thread events until a shutdown request has been issued
  // from outside the thread.

  forever {

    CTL_EVENT_SET_t            events = MOVEMENT_MANAGER_EVENTS;
    CTL_EVENT_SET_t            status = ctl_events_wait_uc ( CTL_EVENT_WAIT_ANY_EVENTS_WITH_AUTO_CLEAR, &(movement->status), events );

    // If shutdown is requested, perform shutdown requests and break out of the loop.

    if ( status & MOVEMENT_EVENT_SHUTDOWN ) { movement_shutdown ( movement ); break; }
    if ( status & MOVEMENT_EVENT_SETTINGS ) { movement_settings ( movement ); }
    if ( status & MOVEMENT_EVENT_STANDBY ) { movement_standby ( movement ); }

    // Respond to the periodic event.

    if ( status & MOVEMENT_EVENT_PERIODIC ) { movement_periodic ( movement ); }

    // Respond to motion sensor events.

    if ( status & MOVEMENT_EVENT_ORIENTATION ) { movement_orientation ( movement ); }
    if ( status & MOVEMENT_EVENT_FREEFALL ) { movement_freefall ( movement ); }
    if ( status & MOVEMENT_EVENT_VECTORS ) { movement_vectors ( movement ); }
    if ( status & MOVEMENT_EVENT_ACTIVE ) { movement_active ( movement ); }
    if ( status & MOVEMENT_EVENT_ASLEEP ) { movement_asleep ( movement ); }

    }

  // Indicate that the manager thread is closed.

  ctl_events_init ( &(movement->status), MOVEMENT_STATE_CLOSED );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void movement_shutdown ( movement_t * movement ) {

  // If the motion unit is enabled, make sure it is disabled before shutting
  // down telemetry.

  if ( movement->option & PLATFORM_OPTION_MOTION ) { motion_disable ( ); }

  // Clear the vectors, free-fall state and periodic timer.

  ctl_events_clear ( &(movement->status), MOVEMENT_STATE_VECTORS | MOVEMENT_STATE_FREEFALL );
  ctl_timer_clear ( &(movement->status), MOVEMENT_EVENT_PERIODIC );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void movement_settings ( movement_t * movement ) {

  // If the motion sensor option is enabled, register for notices
  // from the sensor for orientation change, activity, free-fall and
  // updated vectors. Get the initial orientation.

  if ( movement->option & PLATFORM_OPTION_MOTION ) {

    motion_options ( MOTION_OPTION_TEMPERATURE | MOTION_OPTION_VECTORS | MOTION_OPTION_FREEFALL );
    //motion_linear ( MOTION_RATE_100HZ, MOTION_RANGE_16G );
    motion_linear ( MOTION_RATE_50HZ, MOTION_RANGE_16G );

    // Request wake-up on activity. This will put the motion unit into sleep
    // at its lowest frequency whenever inactivity is detected.

    if ( NRF_SUCCESS == motion_wakeup ( 0.25, 0.1, 0.0 ) ) { ctl_events_clear ( &(movement->status), MOVEMENT_STATE_ACTIVITY ); }

    // Set up the notices in which we are interested.

    motion_notice ( MOTION_NOTICE_ORIENTATION, &(movement->status), MOVEMENT_EVENT_ORIENTATION );
    motion_notice ( MOTION_NOTICE_FALLING, &(movement->status), MOVEMENT_EVENT_FREEFALL );
    motion_notice ( MOTION_NOTICE_VECTORS, &(movement->status), MOVEMENT_EVENT_VECTORS );
    motion_notice ( MOTION_NOTICE_ACTIVE, &(movement->status), MOVEMENT_EVENT_ACTIVE );
    motion_notice ( MOTION_NOTICE_ASLEEP, &(movement->status), MOVEMENT_EVENT_ASLEEP );

    // Get the initial orientation.

    motion_orientation ( &(movement->orientation) );

    } else return;

  // Retrieve the current atmospheric temperature settings and use these to
  // calibrate the offset of the surface temperature measurement.
  //
  // Note: this presumes that the surface temperature and the air temperature
  // of the device are essentially equivalent at start up.

  if ( movement->option & PLATFORM_OPTION_PRESSURE ) {

    float                     ambient = 0;

    if ( NRF_SUCCESS == pressure_measurement ( NULL, &(ambient) )  ) { motion_calibration ( ambient ); }

    } else if ( movement->option & PLATFORM_OPTION_HUMIDITY ) {

    float                     ambient = 0;

    if ( NRF_SUCCESS == humidity_measurement ( NULL, &(ambient) ) ) { motion_calibration ( ambient ); }

    }

  // Start the periodic timer.

  ctl_timer_start ( CTL_TIMER_CYCLICAL, &(movement->status), MOVEMENT_EVENT_PERIODIC, movement->period );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void movement_standby ( movement_t * movement ) {

  // Switch off the motion unit when going into standby.

  if ( movement->option & PLATFORM_OPTION_MOTION ) { motion_disable ( ); }

  // Clear the vectors, free-fall state and periodic timer.

  ctl_events_clear ( &(movement->status), MOVEMENT_STATE_VECTORS | MOVEMENT_STATE_FREEFALL );
  ctl_timer_clear ( &(movement->status), MOVEMENT_EVENT_PERIODIC );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void movement_periodic ( movement_t * movement ) {

  // Retrieve the latest temperature reading from the motion sensor.

  motion_temperature ( &(movement->temperature) );

  // Issue a movement update notice

  ctl_notice ( movement->notice + MOVEMENT_NOTICE_PERIODIC );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void movement_orientation ( movement_t * movement ) {

  if ( NRF_SUCCESS == motion_orientation ( &(movement->orientation) ) ) { ctl_notice ( movement->notice + MOVEMENT_NOTICE_ORIENTATION ); }

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void movement_freefall ( movement_t * movement ) {

  ctl_events_set ( &(movement->status), MOVEMENT_STATE_FREEFALL );
  ctl_notice ( movement->notice + MOVEMENT_NOTICE_FREEFALL );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void __attribute__ (( optimize(2) )) movement_vectors ( movement_t * movement ) {

  if ( NRF_SUCCESS == motion_vectors ( &(movement->vectors.angular), &(movement->vectors.linear) ) ) {

    float                      planar = (movement->vectors.linear.x * movement->vectors.linear.x) + (movement->vectors.linear.y * movement->vectors.linear.y);
    float                      vector = (movement->vectors.linear.z * movement->vectors.linear.z) + planar;
    float                      radius = sqrtf ( planar );
    float                       angle = (movement->vectors.linear.z > 0) ? (90.0) : (-90.0);

    // Compute the tilt angle of the force vector if a magnitude exists.

    if ( radius ) { angle = atan2f ( movement->vectors.linear.z, radius ) * 180.0 / M_PI; }

    // Compute the total force vector from the individual forces and, if it
    // exceeds the stress limit, trip the notice.

    if ( (movement->force.value = sqrtf ( vector )) > movement->force.limit ) {
      if ( movement->force.limit ) { ctl_notice ( movement->notice + MOVEMENT_NOTICE_STRESS ); }
      }

    // Adjust the neutral angle for the given orientation and make sure that it
    // does not exceed 90 degrees in either direction.

    if ( movement->orientation == MOTION_ORIENTATION_FACEUP ) { angle -= 90.0; }
    if ( movement->orientation == MOTION_ORIENTATION_FACEDOWN ) { angle += 90.0; }

    while ( angle > 90.0 ) { angle -= 90.0; }
    while ( angle < -90.0 ) { angle += 90.0; }

    // Get the absolute value of the angle offset from the neutral angle and,
    // if it exceeds the angle limit, trip the notice.

    if ( (movement->angle.value = fabsf ( angle )) > movement->angle.limit ) {
      if ( movement->angle.limit ) { ctl_notice ( movement->notice + MOVEMENT_NOTICE_TILT ); }
      }

    } else return;

  // We now have vectors.

  ctl_events_set ( &(movement->status), MOVEMENT_STATE_VECTORS );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void movement_active ( movement_t * movement ) {

  ctl_events_set ( &(movement->status), MOVEMENT_STATE_ACTIVITY );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void movement_asleep ( movement_t * movement ) {

  ctl_events_clear ( &(movement->status), MOVEMENT_STATE_ACTIVITY  );

  }