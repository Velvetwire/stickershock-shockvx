//=============================================================================
// project: ShockVx
//  module: Stickershock firmware for cold chain tracking.
//  author: Velvetwire, llc
//    file: beacon.c
//
// Bluetooth beacon for controlling and commisioning the sensor.
//
// (c) Copyright 2016-2020 Velvetwire, LLC. All rights reserved.
//=============================================================================

#include  <stickershock.h>

#include  "bluetooth.h"
#include  "broadcast.h"
#include  "beacon.h"

//=============================================================================
// SECTION :
//=============================================================================

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static CTL_TASK_t *            thread = NULL;
static beacon_t              resource = { 0 };

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned beacon_start ( unsigned short variant ) {

  beacon_t *                   beacon = &(resource);
  unsigned                     result = NRF_SUCCESS;

  // The thread cannot have already been started.

  if ( thread == NULL ) { ctl_mutex_init ( &(beacon->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );

  // Start the module thread and initialize the beacon profile.

  if ( (thread = ctl_spawn ( "beacon", (CTL_TASK_ENTRY_t) beacon_manager, beacon, BEACON_MANAGER_STACK, BEACON_MANAGER_PRIORITY )) ) {

    beacon->record.horizon            = BEACON_POWER_HORIZON;
    beacon->record.variant.type       = variant;

    } else { result = NRF_ERROR_NO_MEM; }

  // Return with result.

  return ( result );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned beacon_state ( bool * active ) {

  beacon_t *           beacon = &(resource);

  // If the thread is active, retrieve the broadcast state.

  if ( thread ) {

    if ( active ) { *(active) = (beacon->status & BEACON_STATE_ACTIVE) ? true : false; }

    } else return ( NRF_ERROR_INVALID_STATE );

  // State retrieved successfully.

  return ( NRF_SUCCESS );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned beacon_begin ( float interval, float period, signed char power ) {

  beacon_t *                   beacon = &(resource);
  unsigned                     result = NRF_SUCCESS;

  // Make sure that the module has been started.

  if ( thread ) { ctl_mutex_lock_uc ( &(beacon->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );

  if ( (interval >= BEACON_INTERVAL_MINIMUM) && (power <= BEACON_POWER_MAXIMUM) ) {

    beacon->broadcast.interval        = interval;
    beacon->broadcast.period          = period;
    beacon->broadcast.flags           = period
                                      ? BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE
                                      : BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    beacon->broadcast.power           = power;

    beacon->broadcast.code            = BROADCAST_PACKET_CODE;

    ctl_events_set_clear ( &(beacon->status), BEACON_EVENT_BEGIN, BEACON_CLEAR_BEGIN );

    } else { result = NRF_ERROR_INVALID_PARAM; }

  #ifdef DEBUG
  debug_printf ( "\r\nBeacon: (%f)", interval );
  #endif

  // Free the resource and return with the result.

  return ( ctl_mutex_unlock ( &(beacon->mutex) ), result );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned beacon_cease ( void ) {

  beacon_t *                   beacon = &(resource);
  bool                        enabled = false;

  // If the broadcast is currently active, issue a terminate request.

  if ( thread ) {

    if ( (NRF_SUCCESS == softble_advertisement_state ( &(enabled) )) && (enabled) ) { softble_advertisement_cease ( ); }
    else return ( NRF_ERROR_INVALID_STATE );

    } else return ( NRF_ERROR_INVALID_STATE );

  // Clear the state flags after ceasing.

  ctl_events_clear ( &(beacon->status), BEACON_CLEAR_CEASE );

  #ifdef DEBUG
  debug_printf ( "\r\nBeacon: off" );
  #endif

  // Return with success

  return ( NRF_SUCCESS );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned beacon_close ( void ) {

  beacon_t *                   beacon = &(resource);

  // If the module thread has not been started, there is nothing to close.

  if ( thread ) { ctl_events_set ( &(beacon->status), BEACON_EVENT_SHUTDOWN ); }
  else return ( NRF_ERROR_INVALID_STATE );

  // Wait for the the module thread to acknowlege shutdown.

  if ( ctl_events_wait ( CTL_EVENT_WAIT_ALL_EVENTS, &(beacon->status), BEACON_STATE_CLOSED, CTL_TIMEOUT_DELAY, BEACON_CLOSE_TIMEOUT ) ) { thread = NULL; }
  else return ( NRF_ERROR_TIMEOUT );

  // Wipe the module resource data.

  memset ( beacon, 0, sizeof(beacon_t) );

  // Module successfully shut down.

  return ( NRF_SUCCESS );

  }

//-----------------------------------------------------------------------------
//  function: beacon_notice ( notice, set, events )
// arguments: notice - the notice index to enable or disable
//            set - event set to trigger (NULL to disable)
//            events - event bits to set
//   returns: NRF_SUCCESS - if notice enabled
//            NRF_ERROR_INVALID_PARAM - if the notice index is not valid
//
// Register for notices with the beacon manager.
//-----------------------------------------------------------------------------

unsigned beacon_notice ( beacon_notice_t notice, CTL_EVENT_SET_t * set, CTL_EVENT_SET_t events ) {

  beacon_t *                   beacon = &(resource);

  // Make sure that the requested notice is valid and register the notice.

  if ( notice < BEACON_NOTICES ) { ctl_mutex_lock_uc ( &(beacon->mutex) ); }
  else return ( NRF_ERROR_INVALID_PARAM );

  beacon->notice[ notice ].set        = set;
  beacon->notice[ notice ].events     = events;

  // Notice registered.

  return ( ctl_mutex_unlock ( &(beacon->mutex) ), NRF_SUCCESS );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned beacon_network ( void * node ) {

  beacon_t *                   beacon = &(resource);
  unsigned                     result = NRF_SUCCESS;

  // Make sure that the module has started and lock the module resource.

  if ( thread ) { ctl_mutex_lock_uc ( &(beacon->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );

  // Release the resource and return with result.

  return ( ctl_mutex_unlock ( &(beacon->mutex) ), result );

  }

//-----------------------------------------------------------------------------
//  function: beacon_battery ( battery )
// arguments: battery - battery level (-100 to +100) negative indicates charging
//   returns: NRF_SUCCESS - if updated
//            NRF_ERROR_INVALID_STATE - if the beacon module has not started
//
// Update the battery level published by the beacon.
//-----------------------------------------------------------------------------

unsigned beacon_battery ( signed char battery ) {

  beacon_t *                   beacon = &(resource);
  unsigned                     result = NRF_SUCCESS;

  // Make sure that the module has started and lock the module resource.

  if ( thread ) { ctl_mutex_lock_uc ( &(beacon->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );

  beacon->record.battery              = battery;

  // Request construction of an updated broadcast packet.

  if ( beacon->status & BEACON_STATE_PERIOD ) { ctl_events_set ( &(beacon->status), BEACON_EVENT_CONSTRUCT ); }

  // Release the resource and return with result.

  return ( ctl_mutex_unlock ( &(beacon->mutex) ), result );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned beacon_surface ( float measurement, unsigned incursion, unsigned excursion ) {

  beacon_t *                   beacon = &(resource);
  unsigned                     result = NRF_SUCCESS;

  // Make sure that the module has started and lock the module resource.

  if ( thread ) { ctl_mutex_lock_uc ( &(beacon->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );
  
  beacon->record.temperature.measurement  = (short) roundf ( measurement * 1e2 );
  beacon->record.temperature.incursion    = (incursion < (65535 * 60) ) ? (unsigned short) (incursion / 60) : 65535;
  beacon->record.temperature.excursion    = (excursion < (65535 * 60) ) ? (unsigned short) (excursion / 60) : 65535;

  // Request construction of an updated broadcast packet.

  if ( beacon->status & BEACON_STATE_PERIOD ) { ctl_events_set ( &(beacon->status), BEACON_EVENT_CONSTRUCT ); }

  // Release the resource and return with result.

  return ( ctl_mutex_unlock ( &(beacon->mutex) ), result );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned beacon_ambient ( float measurement, unsigned incursion, unsigned excursion ) {

  beacon_t *                   beacon = &(resource);
  unsigned                     result = NRF_SUCCESS;

  // Make sure that the module has started and lock the module resource.

  if ( thread ) { ctl_mutex_lock_uc ( &(beacon->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );
  
  beacon->record.atmosphere.temperature.measurement   = (short) roundf ( measurement * 1e2 );
  beacon->record.atmosphere.temperature.incursion     = (incursion < (65535 * 60) ) ? (unsigned short) (incursion / 60) : 65535;
  beacon->record.atmosphere.temperature.excursion     = (excursion < (65535 * 60) ) ? (unsigned short) (excursion / 60) : 65535;

  // Request construction of an updated broadcast packet.

  if ( beacon->status & BEACON_STATE_PERIOD ) { ctl_events_set ( &(beacon->status), BEACON_EVENT_CONSTRUCT ); }

  // Release the resource and return with result.

  return ( ctl_mutex_unlock ( &(beacon->mutex) ), result );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned beacon_humidity ( float measurement, unsigned incursion, unsigned excursion ) {

  beacon_t *                   beacon = &(resource);
  unsigned                     result = NRF_SUCCESS;

  // Make sure that the module has started and lock the module resource.

  if ( thread ) { ctl_mutex_lock_uc ( &(beacon->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );
  
  beacon->record.atmosphere.humidity.measurement    = (short) roundf ( measurement * 1e4 );
  beacon->record.atmosphere.humidity.incursion      = (incursion < (65535 * 60) ) ? (unsigned short) (incursion / 60) : 65535;
  beacon->record.atmosphere.humidity.excursion      = (excursion < (65535 * 60) ) ? (unsigned short) (excursion / 60) : 65535;

  // Request construction of an updated broadcast packet.

  if ( beacon->status & BEACON_STATE_PERIOD ) { ctl_events_set ( &(beacon->status), BEACON_EVENT_CONSTRUCT ); }

  // Release the resource and return with result.

  return ( ctl_mutex_unlock ( &(beacon->mutex) ), result );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned beacon_pressure ( float measurement, unsigned incursion, unsigned excursion ) {

  beacon_t *                   beacon = &(resource);
  unsigned                     result = NRF_SUCCESS;

  // Make sure that the module has started and lock the module resource.

  if ( thread ) { ctl_mutex_lock_uc ( &(beacon->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );
  
  beacon->record.atmosphere.pressure.measurement    = (short) roundf ( measurement * 1e3 );
  beacon->record.atmosphere.pressure.incursion      = (incursion < (65535 * 60) ) ? (unsigned short) (incursion / 60) : 65535;
  beacon->record.atmosphere.pressure.excursion      = (excursion < (65535 * 60) ) ? (unsigned short) (excursion / 60) : 65535;

  // Request construction of an updated broadcast packet.

  if ( beacon->status & BEACON_STATE_PERIOD ) { ctl_events_set ( &(beacon->status), BEACON_EVENT_CONSTRUCT ); }

  // Release the resource and return with result.

  return ( ctl_mutex_unlock ( &(beacon->mutex) ), result );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned beacon_orientation ( float angle, unsigned char orientation ) {

  beacon_t *                   beacon = &(resource);
  unsigned                     result = NRF_SUCCESS;

  // Make sure that the module has started and lock the module resource.

  if ( thread ) { ctl_mutex_lock_uc ( &(beacon->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );
  
  beacon->record.handling.angle                     = (char) roundf ( angle );
  beacon->record.handling.orientation               = beacon->record.handling.orientation
                                                    | BROADCAST_HANLDING_FACE( orientation)
                                                    | BROADCAST_ORIENTATION_ANGLE;

  // Request construction of an updated broadcast packet.

  if ( beacon->status & BEACON_STATE_PERIOD ) { ctl_events_set ( &(beacon->status), BEACON_EVENT_CONSTRUCT ); }

  // Release the resource and return with result.

  return ( ctl_mutex_unlock ( &(beacon->mutex) ), result );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned beacon_misoriented ( void ) {

  beacon_t *                   beacon = &(resource);
  unsigned                     result = NRF_SUCCESS;

  // Make sure that the module has started and lock the module resource.

  if ( thread ) { ctl_mutex_lock_uc ( &(beacon->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );
  
  beacon->record.handling.orientation               = beacon->record.handling.orientation
                                                    | BROADCAST_ORIENTATION_FACE;

  // Release the resource and return with result.

  return ( ctl_mutex_unlock ( &(beacon->mutex) ), result );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned beacon_dropped ( void ) {

  beacon_t *                   beacon = &(resource);
  unsigned                     result = NRF_SUCCESS;

  // Make sure that the module has started and lock the module resource.

  if ( thread ) { ctl_mutex_lock_uc ( &(beacon->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );
  
  beacon->record.handling.orientation               = beacon->record.handling.orientation
                                                    | BROADCAST_ORIENTATION_DROP;

  // Release the resource and return with result.

  return ( ctl_mutex_unlock ( &(beacon->mutex) ), result );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned beacon_bumped ( void ) {

  beacon_t *                   beacon = &(resource);
  unsigned                     result = NRF_SUCCESS;

  // Make sure that the module has started and lock the module resource.

  if ( thread ) { ctl_mutex_lock_uc ( &(beacon->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );
  
  beacon->record.handling.orientation               = beacon->record.handling.orientation
                                                    | BROADCAST_ORIENTATION_BUMP;

  // Release the resource and return with result.

  return ( ctl_mutex_unlock ( &(beacon->mutex) ), result );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned beacon_tipped ( void ) {

  beacon_t *                   beacon = &(resource);
  unsigned                     result = NRF_SUCCESS;

  // Make sure that the module has started and lock the module resource.

  if ( thread ) { ctl_mutex_lock_uc ( &(beacon->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );
  
  beacon->record.handling.orientation               = beacon->record.handling.orientation
                                                    | BROADCAST_ORIENTATION_TILT;

  // Release the resource and return with result.

  return ( ctl_mutex_unlock ( &(beacon->mutex) ), result );

  }


//=============================================================================
// SECTION : BEACON MANAGER THREAD
//=============================================================================

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void beacon_manager ( beacon_t * beacon ) {

  // Process  thread events until a shutdown request has been issued
  // from outside the thread.

  forever {

    CTL_EVENT_SET_t            events = BEACON_MANAGER_EVENTS;
    CTL_EVENT_SET_t            status = ctl_events_wait_uc ( CTL_EVENT_WAIT_ANY_EVENTS_WITH_AUTO_CLEAR, &(beacon->status), events );

    // If shutdown is requested, perform shutdown requests and break out of the loop.

    if ( status & BEACON_EVENT_SHUTDOWN ) { beacon_shutdown ( beacon ); break; }

    // Handle broadcast construction and state change events.

    if ( status & BEACON_EVENT_CONFIGURE ) { beacon_configure ( beacon ); }
    if ( status & BEACON_EVENT_CONSTRUCT ) { beacon_construct ( beacon ); }
    if ( status & BEACON_EVENT_BROADCAST ) { beacon_broadcast ( beacon ); }

    if ( status & BEACON_EVENT_ADVERTISE ) { beacon_advertise ( beacon ); }
    if ( status & BEACON_EVENT_TERMINATE ) { beacon_terminate ( beacon ); }
    if ( status & BEACON_EVENT_INSPECTED ) { beacon_inspected ( beacon ); }

    }

  // Indicate that the manager thread is closed.

  ctl_events_init ( &(beacon->status), BEACON_STATE_CLOSED );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void beacon_shutdown ( beacon_t * beacon ) {

  bool                        enabled = false;

  // If the broadcast is currently active, cease the advertising broadcast.

  if ( NRF_SUCCESS == softble_advertisement_state ( &(enabled) ) ) { if ( enabled ) softble_advertisement_cease ( ); }

  // If an advertisement has been constructed, free the buffers.

  if ( beacon->advertisement.data ) { free ( beacon->advertisement.data ); }
  if ( beacon->advertisement.scan ) { free ( beacon->advertisement.scan ); }

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void beacon_configure ( beacon_t * beacon ) {

  // Disable any ongoing broadcast before contructing a new advertisement.

  softble_advertisement_cease ( );

  // Program the duration and broadcast rate of the advertisement period and
  // register to receive notices when it expires.

  if ( NRF_SUCCESS == softble_advertisement_period ( BLE_GAP_ADV_TYPE_EXTENDED_NONCONNECTABLE_SCANNABLE_UNDIRECTED, beacon->broadcast.interval, beacon->broadcast.period ) ) {

    // Set up notifications for advertisment start and stop.

    softdevice_notice ( SOFTBLE_NOTICE_ADVERTISE_START, &(beacon->status), BEACON_EVENT_ADVERTISE );
    softdevice_notice ( SOFTBLE_NOTICE_ADVERTISE_CEASE, &(beacon->status), BEACON_EVENT_TERMINATE );

    // Interested whenever the beacon broadcast is inspected.

    softdevice_notice ( SOFTBLE_NOTICE_INSPECTED, &(beacon->status), BEACON_EVENT_INSPECTED );

    }

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void beacon_construct ( beacon_t * beacon ) {

  softble_advertisement_t *      data = NULL;
  softble_advertisement_t *      scan = softble_advertisement_create ( );
  broadcast_packet_t *         packet = broadcast_packet ( beacon->broadcast.code );
  const void *               security = access_key ( );

  // Construct a broadcast data packet to contain either a secure identity
  // record or a standard identity record, depending on the presence of a key.

  if ( packet ) {

    broadcast_identity_t     identity = { .timecode = ctl_time_get ( ), .identity = *((hash_t *) NRF_FICR->DEVICEID) };
    identity.horizon                  = beacon->record.horizon;
    identity.battery                  = beacon->record.battery;

    if ( security ) { identity.security = hash ( security, &(identity), sizeof(unsigned) + sizeof(hash_t) ); }
    if ( security ) { broadcast_append ( packet, &(identity), sizeof(broadcast_identity_t), BROADCAST_TYPE_SECURE( BROADCAST_TYPE_IDENTITY ) ); }
    else { broadcast_append ( packet, &(identity), sizeof(broadcast_identity_t), BROADCAST_TYPE_NORMAL( BROADCAST_TYPE_IDENTITY ) ); }

    broadcast_append ( packet, &(beacon->record.variant), sizeof(broadcast_variant_t), BROADCAST_TYPE_NORMAL(BROADCAST_TYPE_VARIANT) );

    }

  // Append the telemetry information for the surface temperature, atmospherics
  // and handling.

  if ( packet ) {
    
    broadcast_append ( packet, &(beacon->record.temperature), sizeof(broadcast_temperature_t), BROADCAST_TYPE_NORMAL(BROADCAST_TYPE_TEMPERATURE) );
    broadcast_append ( packet, &(beacon->record.atmosphere), sizeof(broadcast_atmosphere_t), BROADCAST_TYPE_NORMAL(BROADCAST_TYPE_ATMOSPHERE) );
    broadcast_append ( packet, &(beacon->record.handling), sizeof(broadcast_handling_t), BROADCAST_TYPE_NORMAL(BROADCAST_TYPE_HANDLING) );

    }

  // Add the broadcast packet to the scan data.

  if ( scan && packet ) { softble_advertisement_append ( scan, BLE_GAP_AD_TYPE_SERVICE_DATA, packet, broadcast_length ( packet ) + sizeof(short) ); }

  // If there is new advertising broadcast data, update the bluetooth
  // stack with the new broadcast.

  if ( data || scan ) {

    unsigned result = NRF_SUCCESS;

    // Update the BLE stack with the new advertising packet.

    if ( NRF_SUCCESS == (result = softble_advertisement_packet ( data, scan ) )) { ctl_events_set ( &(beacon->status), BEACON_STATE_PACKET ); }
    else { ctl_events_clear ( &(beacon->status), BEACON_STATE_PACKET ); }

    // Free the the old advertising packet.

    free ( beacon->advertisement.data );
    free ( beacon->advertisement.scan );

    }

  // Update the resource with the new advertisement records.

  beacon->advertisement.data      = data;
  beacon->advertisement.scan      = scan;

  // Clean up the data packet.

  free ( packet );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void beacon_broadcast ( beacon_t * beacon ) {

  // Open the advertisement broadcast period.

  if ( NRF_SUCCESS == softble_advertisement_begin ( beacon->broadcast.power ) ) { ctl_events_set ( &(beacon->status), BEACON_STATE_PERIOD ); }
  else { ctl_events_clear ( &(beacon->status), BEACON_STATE_PERIOD ); }

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void beacon_advertise ( beacon_t * beacon ) {

  ctl_events_set ( &(beacon->status), BEACON_STATE_ACTIVE );
  ctl_notice ( beacon->notice + BEACON_NOTICE_ADVERTISE );

  #ifdef DEBUG
  debug_printf ( "\r\nBeacon: advertise" );
  #endif

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void beacon_terminate ( beacon_t * beacon ) {

  ctl_events_clear ( &(beacon->status), BEACON_STATE_ACTIVE );
  ctl_notice ( beacon->notice + BEACON_NOTICE_TERMINATE );

  #ifdef DEBUG
  debug_printf ( "\r\nBeacon: stopped" );
  #endif

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void beacon_inspected ( beacon_t * beacon ) {

  ctl_notice ( beacon->notice + BEACON_NOTICE_INSPECTED );

  }

