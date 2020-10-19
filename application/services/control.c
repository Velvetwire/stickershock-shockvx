//=============================================================================
// project: ShockVx
//  module: Stickershock firmware for cold chain tracking.
//  author: Velvetwire, llc
//    file: control.c
//
// (c) Copyright 2016-2020 Velvetwire, LLC. All rights reserved.
//=============================================================================

#include  <stickershock.h>

#include  "bluetooth.h"
#include  "control.h"

//=============================================================================
// SECTION : SERVICE RESOURCE
//=============================================================================

//-----------------------------------------------------------------------------
// Declare the service class resource area.
//-----------------------------------------------------------------------------

static    control_t           resource = { 0 };

//-----------------------------------------------------------------------------
//  function: control_uuid ( )
// arguments: none
//   returns: the 128-bit service UUID
//
// Retrieve the UUID for the service class.
//-----------------------------------------------------------------------------

const void *                  control_uuid ( void ) { return ( control_id ( CONTROL_SERVICES_UUID ) ); }
static const void *           control_id ( unsigned service ) { static uuid_t id; return ( uuid ( &(id), service ) ); }


//=============================================================================
// SECTION : SERVICE CLASS
//=============================================================================

//-----------------------------------------------------------------------------
//  function: control_register ( node, lock, create, accept )
// arguments: node - tracking node (64-bit)
//            lock - tracking lock (128-bit)
//            opened - UUID used to open tracking (128-bit)
//            closed - UUID used to close tracking (128-bit)
//
//   returns: NRF_ERROR_RESOURCES if no resources available
//            NRF_SUCCESS if registered
//
// Register the simple GATT service with the Bluetooth stack.
//-----------------------------------------------------------------------------

unsigned control_register ( void * node, void * lock, void * opened, void * closed ) {

  control_t *                 control = &(resource);
  unsigned                     result = NRF_SUCCESS;

  // Initialize the service resource.

  if ( control->service == BLE_GATT_HANDLE_INVALID ) { ctl_mutex_init ( &(control->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );

  // Register the service with the soft device low energy stack and add the
  // service characteristics.

  if ( (control->service = softble_server_register ( BLE_GATTS_SRVC_TYPE_PRIMARY, control_uuid ( ) )) ) {

    if ( NRF_SUCCESS == result ) { result = control_node_characteristic ( control, node ); }
    if ( NRF_SUCCESS == result ) { result = control_lock_characteristic ( control, lock ); }

    if ( NRF_SUCCESS == result ) { result = control_opened_characteristic ( control, opened ); }
    if ( NRF_SUCCESS == result ) { result = control_closed_characteristic ( control, closed ); }
    if ( NRF_SUCCESS == result ) { result = control_window_characteristic ( control ); }

    if ( NRF_SUCCESS == result ) { result = control_identify_characteristic ( control ); }
    if ( NRF_SUCCESS == result ) { result = control_summary_characteristic ( control ); }

    } else return ( NRF_ERROR_RESOURCES );

  // Request a subcription to the soft device event publisher.

  if ( NRF_SUCCESS == result ) { result = softble_subscribe ( (softble_subscriber_t) control_event, control ); }

  // Return with registration result.

  return ( result );

  }

//-----------------------------------------------------------------------------
//  function: control_notice ( notice, set, events )
// arguments: notice - the notice index to enable or disable
//            set - event set to trigger (NULL to disable)
//            events - event bits to set
//   returns: NRF_SUCCESS - if notice enabled
//            NRF_ERROR_INVALID_PARAM - if the notice index is not valid
//
// Register for notices with the power monitor.
//-----------------------------------------------------------------------------

unsigned control_notice ( control_notice_t notice, CTL_EVENT_SET_t * set, CTL_EVENT_SET_t events ) {

  control_t *                 control = &(resource);

  // Make sure that the requested notice is valid and register the notice.

  if ( notice < CONTROL_NOTICES ) { ctl_mutex_lock_uc ( &(control->mutex) ); }
  else return ( NRF_ERROR_INVALID_PARAM );

  control->notice[ notice ].set       = set;
  control->notice[ notice ].events    = events;

  // Notice registered.

  return ( ctl_mutex_unlock ( &(control->mutex) ), NRF_SUCCESS );

  }

//-----------------------------------------------------------------------------
//  function: control_window ( opened, closed )
// arguments: opened - UTC time of tracking window open (0 = not opened)
//            closed - UTC time of tracking window close (0 = not closed)
//   returns: NRF_SUCCESS - if update issued
//            NRF_ERROR_INVALID_STATE - if service is not registered
//
// Set the tracking window opened and closed times (UTC).
//-----------------------------------------------------------------------------

unsigned control_window ( unsigned opened, unsigned closed ) {

  control_t *                 control = &(resource);
  control_window_t             window = { .opened = opened, .closed = closed };

  // Make sure that the requested notice is valid and register the notice.

  if ( control->service != BLE_GATT_HANDLE_INVALID ) { ctl_mutex_lock_uc ( &(control->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );

  // Update the window characteristic.

  unsigned short               handle = control->handle.window.value_handle;
  unsigned                     result = softble_characteristic_update ( handle, &(window), 0, sizeof(control_window_t) );

  // Return with the result.

  return ( ctl_mutex_unlock ( &(control->mutex) ), result );

  }

//-----------------------------------------------------------------------------
//  function: control_tracking ( node, lock, opened, closed )
// arguments: node - tracking node (64-bit)
//            lock - tracking lock (128-bit)
//            opened - UUID of tracking creator (128-bit)
//            closed - UUID of tracking acceptor (128-bit)
//   returns: NRF_SUCCESS
//
// Retrieve the tracking information. This includes the tracking node, the
// security lock, and the window opened and closed times.
//-----------------------------------------------------------------------------

unsigned control_tracking ( void * node, void * lock, void * opened, void * closed ) {

  control_t *                 control = &(resource);

  if ( node ) { memcpy ( node, &(control->value.node), sizeof(hash_t) ); }
  if ( lock ) { memcpy ( lock, control->value.lock, SOFTDEVICE_KEY_LENGTH ); }

  if ( opened ) { memcpy ( opened, control->value.opened, SOFTDEVICE_KEY_LENGTH ); }
  if ( closed ) { memcpy ( closed, control->value.closed, SOFTDEVICE_KEY_LENGTH ); }

  return ( NRF_SUCCESS );

  }

//-----------------------------------------------------------------------------
//  function: control_identify ( duration )
// arguments: duration - strobe duration in milliseconds
//   returns: NRF_SUCCESS
//
// Get the identification strobe duration in milliseconds.
//-----------------------------------------------------------------------------

unsigned control_identify ( unsigned * duration ) {

  control_t *                 control = &(resource);

  if ( duration ) { *(duration) = (unsigned) control->value.identify * 1000; }

  return ( NRF_SUCCESS );

  }

//-----------------------------------------------------------------------------
//  function: control_status ( status )
// arguments: status - status flags
//            memory - memory space available (0.0 - 1.0)
//            storage - storage space available (0.0 - 1.0)
//   returns: NRF_SUCCESS - if update issued
//            NRF_ERROR_INVALID_STATE - if service is not registered
//
// Update the status flags characteristic.
//-----------------------------------------------------------------------------

unsigned control_status ( control_status_t status, float memory, float storage ) {

  control_t *                 control = &(resource);
  control_summary_t           summary = { .status = status };

  // Compute the memory and storage percentages

  summary.memory                      = (unsigned char) roundf( memory * ((float) 100.0) );
  summary.storage                     = (unsigned char) roundf( storage * ((float) 100.0) );

  // Make sure that the requested notice is valid and register the notice.

  if ( control->service != BLE_GATT_HANDLE_INVALID ) { ctl_mutex_lock_uc ( &(control->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );

  // Update the metrics values structure and issue a notify to any connected peers.

  unsigned short               handle = control->handle.summary.value_handle;
  unsigned                     result = softble_characteristic_update ( handle, &(summary), 0, sizeof(control_summary_t) );

  if ( NRF_SUCCESS == result ) { softble_characteristic_notify ( handle, BLE_CONN_HANDLE_ALL ); }

  // Return with the result.

  return ( ctl_mutex_unlock ( &(control->mutex) ), result );

  }


//=============================================================================
// SECTION : SERVICE RESPONDER
//=============================================================================

//-----------------------------------------------------------------------------
//  callback: control_event ( control, event )
// arguments: control - service resource
//            event - BLE event structure
//   returns: NRF_SUCCESS if event processed
//
// Bluetooth service responder callback handler.
//-----------------------------------------------------------------------------

static unsigned control_event ( control_t * control, ble_evt_t * event ) {

  switch ( event->header.evt_id ) {

    case BLE_GATTS_EVT_WRITE:     return control_write ( control, event->evt.gatts_evt.conn_handle, &(event->evt.gatts_evt.params.write) );

    default:                      return ( NRF_SUCCESS );

    }

  }

//-----------------------------------------------------------------------------
//  function: control_write ( control, connection, write )
// arguments: control - service resource
//            connection - connection handle
//            write - write information structure
//   returns: NRF_SUCCESS if processed
//
// Handle service characteristic write reqests.
//-----------------------------------------------------------------------------

static unsigned control_write ( control_t * control, unsigned short connection, ble_gatts_evt_write_t * write ) {

  // For protected characteristics, the write data needs to be transferred
  // directly to the value data.

  if ( write->handle == control->handle.node.value_handle ) { memcpy ( (void *) &(control->value.node) + write->offset, write->data, write->len ); }
  if ( write->handle == control->handle.lock.value_handle ) { memcpy ( control->value.lock + write->offset, write->data, write->len ); }

  if ( write->handle == control->handle.opened.value_handle ) { memcpy ( control->value.opened + write->offset, write->data, write->len ); }
  if ( write->handle == control->handle.closed.value_handle ) { memcpy ( control->value.closed + write->offset, write->data, write->len ); }

  // If this is a write to the identify characteristic, issue an identify notice.

  if ( write->handle == control->handle.identify.value_handle ) { ctl_notice ( control->notice + CONTROL_NOTICE_IDENTIFY ); }

  // Write processed.

  return ( NRF_SUCCESS );

  }


//=============================================================================
// SECTION : SERVICE CHARACTERISTIC DECLARATIONS
//=============================================================================

//-----------------------------------------------------------------------------
//  function: control_node_characteristic ( control, node )
// arguments: control - service resource
//            node - initialization value
//   returns: NRF_ERROR_INVALID_PARAM - if parameters are invalid or missing
//            NRF_SUCCESS - if added
//
// Register the node identity characteristic with the GATT service.
//-----------------------------------------------------------------------------

static unsigned control_node_characteristic ( control_t * control, void * node ) {

  const void *                   uuid = control_id ( CONTROL_NODE_UUID );
  softble_characteristic_t       data = { .handles  = &(control->handle.node),
                                          .length   = sizeof(hash_t),
                                          .limit    = sizeof(hash_t),
                                          .value    = &(control->value.node) };

  if ( node ) { memcpy ( &(control->value.node), node, sizeof(hash_t) ); }

  return ( softble_characteristic_declare ( control->service, BLE_ATTR_PROTECTED | BLE_ATTR_WRITE | BLE_ATTR_READ, uuid, &(data) ) );

  }

//-----------------------------------------------------------------------------
//  function: control_lock_characteristic ( control, lock )
// arguments: control - service resource
//            lock - initialization value
//   returns: NRF_ERROR_INVALID_PARAM - if parameters are invalid or missing
//            NRF_ERROR_FORBIDDEN - if the lock is already established
//            NRF_SUCCESS - if added
//
// Register the node lock characteristic with the GATT service. It is only
// included if the lock is empty.
//-----------------------------------------------------------------------------

static unsigned control_lock_characteristic ( control_t * control, void * lock ) {

  const void *                   uuid = control_id ( CONTROL_LOCK_UUID );
  softble_characteristic_t       data = { .handles  = &(control->handle.lock),
                                          .length   = SOFTDEVICE_KEY_LENGTH,
                                          .limit    = SOFTDEVICE_KEY_LENGTH,
                                          .value    = &(control->value.lock) };

  if ( lock ) { memcpy ( &(control->value.lock), lock, SOFTDEVICE_KEY_LENGTH ); }
  if ( lock ) for ( unsigned char n = 0; n < SOFTDEVICE_KEY_LENGTH; ++ n ) { if ( control->value.lock[ n ] ) return ( NRF_ERROR_FORBIDDEN ); }

  return ( softble_characteristic_declare ( control->service, BLE_ATTR_PROTECTED | BLE_ATTR_WRITE, uuid, &(data) ) );

  }

//-----------------------------------------------------------------------------
//  function: control_opened_characteristic ( control, opened )
// arguments: control - service resource
//            opened - initialization value
//   returns: NRF_ERROR_INVALID_PARAM - if parameters are invalid or missing
//            NRF_SUCCESS - if added
//
// Register the UUID opened characteristic with the GATT service. This is only
// writable if the UUID is blank.
//-----------------------------------------------------------------------------

static unsigned control_opened_characteristic ( control_t * control, void * opened ) {

  const void *                   uuid = control_id ( CONTROL_OPENED_UUID );
  softble_characteristic_t       data = { .handles  = &(control->handle.opened),
                                          .length   = SOFTDEVICE_KEY_LENGTH,
                                          .limit    = SOFTDEVICE_KEY_LENGTH,
                                          .value    = &(control->value.opened) };
  unsigned char            attributes = BLE_ATTR_PROTECTED | BLE_ATTR_WRITE | BLE_ATTR_READ;

  if ( opened ) { memcpy ( &(control->value.opened), opened, SOFTDEVICE_KEY_LENGTH ); }
  if ( opened ) for ( unsigned char n = 0; n < SOFTDEVICE_KEY_LENGTH; ++ n ) { if ( control->value.opened[ n ] ) attributes &= ~(BLE_ATTR_WRITE); }

  return ( softble_characteristic_declare ( control->service, attributes, uuid, &(data) ) );

  }

//-----------------------------------------------------------------------------
//  function: control_closed_characteristic ( control, closed )
// arguments: control - service resource
//            closed - initialization value
//   returns: NRF_ERROR_INVALID_PARAM - if parameters are invalid or missing
//            NRF_SUCCESS - if added
//
// Register the UUID closed characteristic with the GATT service. This is only
// writable if the UUID is blank.
//-----------------------------------------------------------------------------

static unsigned control_closed_characteristic ( control_t * control, void * closed ) {

  const void *                   uuid = control_id ( CONTROL_CLOSED_UUID );
  softble_characteristic_t       data = { .handles  = &(control->handle.closed),
                                          .length   = SOFTDEVICE_KEY_LENGTH,
                                          .limit    = SOFTDEVICE_KEY_LENGTH,
                                          .value    = &(control->value.closed) };
  unsigned char            attributes = BLE_ATTR_PROTECTED | BLE_ATTR_WRITE | BLE_ATTR_READ;

  if ( closed ) { memcpy ( &(control->value.closed), closed, SOFTDEVICE_KEY_LENGTH ); }
  if ( closed ) for ( unsigned char n = 0; n < SOFTDEVICE_KEY_LENGTH; ++ n ) { if ( control->value.closed[ n ] ) attributes &= ~(BLE_ATTR_WRITE); }

  return ( softble_characteristic_declare ( control->service, attributes, uuid, &(data) ) );

  }

//-----------------------------------------------------------------------------
//  function: control_window_characteristic ( control )
// arguments: control - service resource
//   returns: NRF_ERROR_INVALID_PARAM - if parameters are invalid or missing
//            NRF_SUCCESS - if added
//
// Register the tracking window characteristic with the GATT service. This is
// a read-only characteristic containing the time stamps in UTC.
//-----------------------------------------------------------------------------

static unsigned control_window_characteristic ( control_t * control ) {

  const void *                   uuid = control_id ( CONTROL_WINDOW_UUID );
  softble_characteristic_t       data = { .handles  = &(control->handle.window),
                                          .length   = sizeof(control_window_t),
                                          .limit    = sizeof(control_window_t),
                                          .value    = &(control->value.window) };

  return ( softble_characteristic_declare ( control->service, BLE_ATTR_READ, uuid, &(data) ) );

  }

//-----------------------------------------------------------------------------
//  function: control_identify_characteristic ( control )
// arguments: control - service resource
//   returns: NRF_ERROR_INVALID_PARAM - if parameters are invalid or missing
//            NRF_SUCCESS - if added
//
// Register the identification strobe characteristic with the GATT service.
//-----------------------------------------------------------------------------

static unsigned control_identify_characteristic ( control_t * control ) {

  const void *                   uuid = control_id ( CONTROL_IDENTIFY_UUID );
  softble_characteristic_t       data = { .handles  = &(control->handle.identify),
                                          .length   = sizeof(char),
                                          .limit    = sizeof(char),
                                          .value    = &(control->value.identify) };

  return ( softble_characteristic_declare ( control->service, BLE_ATTR_WRITE, uuid, &(data) ) );

  }

//-----------------------------------------------------------------------------
//  function: control_summary_characteristic ( control )
// arguments: control - service resource
//   returns: NRF_ERROR_INVALID_PARAM - if parameters are invalid or missing
//            NRF_SUCCESS - if added
//
// Register the status flags characteristic with the GATT service.
//-----------------------------------------------------------------------------

static unsigned control_summary_characteristic ( control_t * control ) {

  const void *                   uuid = control_id ( CONTROL_SUMMARY_UUID );
  softble_characteristic_t       data = { .handles  = &(control->handle.summary),
                                          .length   = sizeof(control_summary_t),
                                          .limit    = sizeof(control_summary_t),
                                          .value    = &(control->value.summary) };

  return ( softble_characteristic_declare ( control->service, BLE_ATTR_NOTIFY | BLE_ATTR_READ, uuid, &(data) ) );

  }
