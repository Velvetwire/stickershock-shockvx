//=============================================================================
// project: ShockVx
//  module: Stickershock firmware for cold chain tracking.
//  author: Velvetwire, llc
//    file: peripheral.c
//
// Bluetooth peripheral for controlling and commisioning the sensor.
//
// (c) Copyright 2016-2020 Velvetwire, LLC. All rights reserved.
//=============================================================================

#include  <stickershock.h>

#include  "bluetooth.h"
#include  "peripheral.h"

//=============================================================================
// SECTION : CONNECTABLE PERIPHERAL MANAGER
//=============================================================================

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static CTL_TASK_t *            thread = NULL;
static peripheral_t          resource = { 0 };

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned peripheral_start ( void ) {

  peripheral_t *           peripheral = &(resource);
  unsigned                     result = NRF_SUCCESS;

  if ( thread == NULL ) { ctl_mutex_init ( &(peripheral->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );

  if ( (thread = ctl_spawn ( "peripheral", (CTL_TASK_ENTRY_t) peripheral_manager, peripheral, PERIPHERAL_MANAGER_STACK, PERIPHERAL_MANAGER_PRIORITY )) ) {

    } else { result = NRF_ERROR_NO_MEM; }

  return ( result );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned peripheral_state ( bool * active, bool * linked ) {

  peripheral_t *           peripheral = &(resource);

  if ( thread ) {

    if ( active ) { *(active) = (peripheral->status & PERIPHERAL_STATE_ACTIVE) ? true : false; }
    if ( linked ) { *(linked) = (peripheral->status & PERIPHERAL_STATE_LINKED) ? true : false; }

    } else return ( NRF_ERROR_INVALID_STATE );

  return ( NRF_SUCCESS );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned peripheral_begin ( float interval, float period, signed char power ) {

  peripheral_t *           peripheral = &(resource);
  unsigned                     result = NRF_SUCCESS;

  // Make sure that the module has been started.

  if ( thread ) { ctl_mutex_lock_uc ( &(peripheral->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );

  if ( (interval >= PERIPHERAL_INTERVAL_MINIMUM) && (power <= PERIPHERAL_POWER_MAXIMUM) ) {

    peripheral->broadcast.interval    = interval;
    peripheral->broadcast.period      = period;
    peripheral->broadcast.flags       = period
                                      ? BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE
                                      : BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

    peripheral->broadcast.power       = power;

    ctl_events_set_clear ( &(peripheral->status), PERIPHERAL_EVENT_BEGIN, PERIPHERAL_CLEAR_BEGIN );

    } else { result = NRF_ERROR_INVALID_PARAM; }

  // Free the resource and return with the result.

  return ( ctl_mutex_unlock ( &(peripheral->mutex) ), result );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned peripheral_cease ( void ) {

  peripheral_t *           peripheral = &(resource);
  bool                        enabled = false;

  // If the broadcast is currently active, issue a terminate request.

  if ( thread ) { softble_advertisement_cease ( ); }
  else return ( NRF_ERROR_INVALID_STATE );

  // Clear the state flags after ceasing.

  ctl_events_clear ( &(peripheral->status), PERIPHERAL_CLEAR_CEASE );
  ctl_yield ( 128 );

  // Return with success

  return ( NRF_SUCCESS );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned peripheral_close ( void ) {

  peripheral_t *           peripheral = &(resource);

  // If the module thread has not been started, there is nothing to close.

  if ( thread ) { ctl_events_set ( &(peripheral->status), PERIPHERAL_EVENT_SHUTDOWN ); }
  else return ( NRF_ERROR_INVALID_STATE );

  // Wait for the the module thread to acknowlege shutdown.

  if ( ctl_events_wait ( CTL_EVENT_WAIT_ALL_EVENTS, &(peripheral->status), PERIPHERAL_STATE_CLOSED, CTL_TIMEOUT_DELAY, PERIPHERAL_CLOSE_TIMEOUT ) ) { thread = NULL; }
  else return ( NRF_ERROR_TIMEOUT );

  // Wipe the module resource data.

  memset ( peripheral, 0, sizeof(peripheral_t) );

  // Module successfully shut down.

  return ( NRF_SUCCESS );

  }

//-----------------------------------------------------------------------------
//  function: peripheral_notice ( notice, set, events )
// arguments: notice - the notice index to enable or disable
//            set - event set to trigger (NULL to disable)
//            events - event bits to set
//   returns: NRF_SUCCESS - if notice enabled
//            NRF_ERROR_INVALID_PARAM - if the notice index is not valid
//
// Register for notices with the peripheral manager.
//-----------------------------------------------------------------------------

unsigned peripheral_notice ( peripheral_notice_t notice, CTL_EVENT_SET_t * set, CTL_EVENT_SET_t events ) {

  peripheral_t *           peripheral = &(resource);

  // Make sure that the requested notice is valid and register the notice.

  if ( notice < PERIPHERAL_NOTICES ) { ctl_mutex_lock_uc ( &(peripheral->mutex) ); }
  else return ( NRF_ERROR_INVALID_PARAM );

  peripheral->notice[ notice ].set    = set;
  peripheral->notice[ notice ].events = events;

  // Notice registered.

  return ( ctl_mutex_unlock ( &(peripheral->mutex) ), NRF_SUCCESS );

  }


//=============================================================================
// SECTION : PERIPHERAL MANAGER THREAD
//=============================================================================

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void peripheral_manager ( peripheral_t * peripheral ) {

  // Process  thread events until a shutdown request has been issued
  // from outside the thread.

  forever {

    CTL_EVENT_SET_t            events = PERIPHERAL_MANAGER_EVENTS;
    CTL_EVENT_SET_t            status = ctl_events_wait_uc ( CTL_EVENT_WAIT_ANY_EVENTS_WITH_AUTO_CLEAR, &(peripheral->status), events );

    // If shutdown is requested, perform shutdown requests and break out of the loop.

    if ( status & PERIPHERAL_EVENT_SHUTDOWN ) { peripheral_shutdown ( peripheral ); break; }

    // Handle broadcast construction and state change events.

    if ( status & PERIPHERAL_EVENT_CONFIGURE ) { peripheral_configure ( peripheral ); }
    if ( status & PERIPHERAL_EVENT_CONSTRUCT ) { peripheral_construct ( peripheral ); }
    if ( status & PERIPHERAL_EVENT_BROADCAST ) { peripheral_broadcast ( peripheral ); }

    if ( status & PERIPHERAL_EVENT_ADVERTISE ) { peripheral_advertise ( peripheral ); }
    if ( status & PERIPHERAL_EVENT_TERMINATE ) { peripheral_terminate ( peripheral ); }
    if ( status & PERIPHERAL_EVENT_INSPECTED ) { peripheral_inspected ( peripheral ); }

    // Peripheral server attachment and detachment.

    if ( status & PERIPHERAL_EVENT_ATTACHED ) { peripheral_attached ( peripheral ); }
    if ( status & PERIPHERAL_EVENT_DETACHED ) { peripheral_detached ( peripheral ); }

    }

  // Indicate that the manager thread is closed.

  ctl_events_init ( &(peripheral->status), PERIPHERAL_STATE_CLOSED );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void peripheral_shutdown ( peripheral_t * peripheral ) {

  bool                        enabled = false;

  // If the broadcast is currently active, cease the advertising broadcast.

  if ( NRF_SUCCESS == softble_advertisement_state ( &(enabled) ) ) { if ( enabled ) softble_advertisement_cease ( ); }

  // If an advertisement has been constructed, free the buffers.

  if ( peripheral->advertisement.data ) { free ( peripheral->advertisement.data ); }
  if ( peripheral->advertisement.scan ) { free ( peripheral->advertisement.scan ); }

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void peripheral_configure ( peripheral_t * peripheral ) {

  // Disable any ongoing broadcast before contructing a new advertisement.

  softble_advertisement_cease ( );

  // Program the duration and broadcast rate of the advertisement period and
  // register to receive notices when it expires.

  if ( NRF_SUCCESS == softble_advertisement_period ( BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED, peripheral->broadcast.interval, peripheral->broadcast.period ) ) {

    // Set up notifications for advertisment start and stop.

    softdevice_notice ( SOFTBLE_NOTICE_ADVERTISE_START, &(peripheral->status), PERIPHERAL_EVENT_ADVERTISE );
    softdevice_notice ( SOFTBLE_NOTICE_ADVERTISE_CEASE, &(peripheral->status), PERIPHERAL_EVENT_TERMINATE );

    // Set up notifications for peripheral server attachment and detachment.

    softdevice_notice ( SOFTBLE_NOTICE_SERVER_ATTACH, &(peripheral->status), PERIPHERAL_EVENT_ATTACHED );
    softdevice_notice ( SOFTBLE_NOTICE_SERVER_DETACH, &(peripheral->status), PERIPHERAL_EVENT_DETACHED );

    // Interested whenever the beacon broadcast is inspected.

    softdevice_notice ( SOFTBLE_NOTICE_INSPECTED, &(peripheral->status), PERIPHERAL_EVENT_INSPECTED );

    }

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void peripheral_construct ( peripheral_t * peripheral ) {

  softble_advertisement_t *      data = softble_advertisement_create ( );
  softble_advertisement_t *      scan = softble_advertisement_create ( );

  // Construct the advertisement data packet.

  if ( data ) {

    softble_advertisement_append ( (softble_advertisement_t *) data, BLE_GAP_AD_TYPE_FLAGS, &(peripheral->broadcast.flags), sizeof(char) );
    softble_advertisement_append ( (softble_advertisement_t *) data, BLE_GAP_AD_TYPE_SERVICE_DATA, information_identity ( ), sizeof(information_identity_t) );

    }

  // Construct the scan response data packet.

  if ( scan ) {

    softble_advertisement_append ( (softble_advertisement_t *) scan, BLE_GAP_AD_TYPE_128BIT_SERVICE_UUID_COMPLETE, control_uuid ( ), sizeof(ble_uuid128_t) );

    }

  // If there is new advertising broadcast data, update the bluetooth
  // stack with the new broadcast.

  if ( data || scan ) {

    // Update the BLE stack with the new advertising packet.

    if ( NRF_SUCCESS == softble_advertisement_packet ( data, scan ) ) { ctl_events_set ( &(peripheral->status), PERIPHERAL_STATE_PACKET ); }
    else { ctl_events_clear ( &(peripheral->status), PERIPHERAL_STATE_PACKET ); }

    // Free the the old advertising data packets.

    free ( peripheral->advertisement.data );
    free ( peripheral->advertisement.scan );

    }

  // Update the resource with the new advertisement records.

  peripheral->advertisement.data      = data;
  peripheral->advertisement.scan      = scan;

  #ifdef DEBUG
  debug_printf ( "\r\nPeripheral: advertising" );
  #endif

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void peripheral_broadcast ( peripheral_t * peripheral ) {

  // Open the advertisement broadcast period.

  if ( NRF_SUCCESS == softble_advertisement_begin ( peripheral->broadcast.power ) ) { ctl_events_set ( &(peripheral->status), PERIPHERAL_STATE_PERIOD ); }
  else { ctl_events_clear ( &(peripheral->status), PERIPHERAL_STATE_PERIOD ); }

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void peripheral_advertise ( peripheral_t * peripheral ) {

  ctl_events_set ( &(peripheral->status), PERIPHERAL_STATE_ACTIVE );
  ctl_notice ( peripheral->notice + PERIPHERAL_NOTICE_ADVERTISE );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void peripheral_terminate ( peripheral_t * peripheral ) {

  ctl_events_clear ( &(peripheral->status), PERIPHERAL_STATE_ACTIVE );
  ctl_notice ( peripheral->notice + PERIPHERAL_NOTICE_TERMINATE );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void peripheral_inspected ( peripheral_t * peripheral ) {

  ctl_notice ( peripheral->notice + PERIPHERAL_NOTICE_INSPECTED );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void peripheral_attached ( peripheral_t * peripheral ) {

  ctl_events_set_clear ( &(peripheral->status), PERIPHERAL_STATE_LINKED, PERIPHERAL_STATE_ACTIVE );
  ctl_notice ( peripheral->notice + PERIPHERAL_NOTICE_ATTACHED );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void peripheral_detached ( peripheral_t * peripheral ) {

  ctl_events_clear ( &(peripheral->status), PERIPHERAL_STATE_LINKED );
  ctl_notice ( peripheral->notice + PERIPHERAL_NOTICE_DETACHED );

  }