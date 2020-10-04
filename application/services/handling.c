//=============================================================================
// project: ShockVx
//  module: Stickershock Sensor for cold chain tracking.
//  author: Velvetwire, llc
//    file: handling.c
//
// Telemetry angles and settings service.
//
// (c) Copyright 2016-2020 Velvetwire, LLC. All rights reserved.
//=============================================================================

#include  <stickershock.h>

#include  "bluetooth.h"
#include  "handling.h"

//=============================================================================
// SECTION : SERVICE RESOURCE
//=============================================================================

//-----------------------------------------------------------------------------
// Declare the service class resource area.
//-----------------------------------------------------------------------------

static    handling_t         resource = { 0 };

//-----------------------------------------------------------------------------
//  function: handling_uuid ( )
// arguments: none
//   returns: the 128-bit service UUID
//
// Retrieve the UUID for the service class.
//-----------------------------------------------------------------------------

const void *                  handling_uuid ( void ) { return ( handling_id ( HANDLING_SERVICES_UUID ) ); }
static const void *           handling_id ( unsigned service ) { static uuid_t id; return ( uuid ( &(id), service ) ); }


//=============================================================================
// SECTION : SERVICE CLASS
//=============================================================================

//-----------------------------------------------------------------------------
//  function: handling_register ( limit )
// arguments: limit - tilt angle limit
//   returns: NRF_ERROR_RESOURCES if no resources available
//            NRF_SUCCESS if registered
//
// Register the telemetry angles GATT service with the Bluetooth stack.
//-----------------------------------------------------------------------------

unsigned handling_register ( handling_values_t * limit ) {

  handling_t *               handling = &(resource);
  unsigned                     result = NRF_SUCCESS;

  // Initialize the service resource.

  if ( handling->service == BLE_GATT_HANDLE_INVALID ) { ctl_mutex_init ( &(handling->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );

  if ( limit ) { memcpy ( &(handling->value.limit), limit, sizeof(handling_values_t) ); }

  // Register the service with the soft device low energy stack and add the
  // service characteristics.

  if ( (handling->service = softble_server_register ( BLE_GATTS_SRVC_TYPE_PRIMARY, handling_uuid ( ) )) ) {

    if ( NRF_SUCCESS == result) { result = handling_value_characteristic ( handling ); }
    if ( NRF_SUCCESS == result) { result = handling_limit_characteristic ( handling ); }

    } else return ( NRF_ERROR_RESOURCES );

  // Request a subcription to the soft device event publisher.

  if ( NRF_SUCCESS == result ) { result = softble_subscribe ( (softble_subscriber_t) handling_event, handling ); }
  
  // Return with registration result.

  return ( result );

  }

//-----------------------------------------------------------------------------
//  function: handling_settings ( limit )
// arguments: limit - limits to use
//   returns: NRF_SUCCESS - if retrieved
//
// Get the limit settings.
//-----------------------------------------------------------------------------

unsigned handling_settings ( handling_values_t * limit ) {

  handling_t *               handling = &(resource);
  
  if ( limit ) { memcpy ( limit, &(handling->value.limit), sizeof(handling_values_t) ); }

  return ( NRF_SUCCESS );

  }

//-----------------------------------------------------------------------------
//  function: handling_values ( values )
// arguments: values - values to update
//   returns: NRF_SUCCESS - if update issued
//            NRF_ERROR_INVALID_STATE - if service is not registered
//
// Update the tilt angle characteristic.
//-----------------------------------------------------------------------------

unsigned handling_values ( handling_values_t * values ) {

  handling_t *               handling = &(resource);
  unsigned                     result = NRF_SUCCESS;

  // Make sure that the requested notice is valid and register the notice.

  if ( handling->service != BLE_GATT_HANDLE_INVALID ) { ctl_mutex_lock_uc ( &(handling->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );

  // Update the angle value and issue a notify to any connected peers.

  if ( values ) {

    unsigned short            handle = handling->handle.value.value_handle;

    if ( NRF_SUCCESS == (result = softble_characteristic_update ( handle, values, 0, sizeof(handling_values_t) )) ) { result = softble_characteristic_notify ( handle, BLE_CONN_HANDLE_ALL ); }
    
    } else { result = NRF_ERROR_NULL; }

  // Return with the result.

  return ( ctl_mutex_unlock ( &(handling->mutex) ), result );

  }


//=============================================================================
// SECTION : SERVICE RESPONDER
//=============================================================================

//-----------------------------------------------------------------------------
//  callback: handling_event ( handling, event )
// arguments: handling - service resource
//            event - BLE event structure
//   returns: NRF_SUCCESS if event processed
//
// Bluetooth service responder callback handler.
//-----------------------------------------------------------------------------

static unsigned handling_event ( handling_t * handling, ble_evt_t * event ) {
  
  switch ( event->header.evt_id ) {
    
    case BLE_GATTS_EVT_WRITE:     return handling_write ( handling, event->evt.gatts_evt.conn_handle, &(event->evt.gatts_evt.params.write) );

    default:                      return ( NRF_SUCCESS );

    }

  }

//-----------------------------------------------------------------------------
//  function: handling_write ( handling, connection, write )
// arguments: handling - service resource
//            connection - connection handle
//            write - write information structure
//   returns: NRF_SUCCESS if processed
//
// Handle service characteristic write reqests.
//-----------------------------------------------------------------------------

static unsigned handling_write ( handling_t * handling, unsigned short connection, ble_gatts_evt_write_t * write ) {

  // For protected characteristics, the write data needs to be transferred
  // directly to the value data.

  if ( write->handle == handling->handle.limit.value_handle ) { memcpy ( (void *) &(handling->value.limit) + write->offset, write->data, write->len ); }

  // Write processed.

  return ( NRF_SUCCESS );

  }


//=============================================================================
// SECTION : SERVICE CHARACTERISITC DECLARATIONS
//=============================================================================

//-----------------------------------------------------------------------------
//  function: handling_value_characteristic ( handling )
// arguments: handling - service resource
//   returns: NRF_ERROR_INVALID_PARAM - if parameters are invalid or missing
//            NRF_SUCCESS - if added
//
// Register the value characteristic with the GATT service. This is a read-only
// structure with values for force, angle and current orientation.
//-----------------------------------------------------------------------------

static unsigned handling_value_characteristic ( handling_t * handling ) {

  const void *                   uuid = handling_id ( HANDLING_VALUE_UUID );
  softble_characteristic_t       data = { .handles  = &(handling->handle.value),
                                          .length   = sizeof(handling_values_t),
                                          .limit    = sizeof(handling_values_t),
                                          .value    = &(handling->value.value) };

  return ( softble_characteristic_declare ( handling->service, BLE_ATTR_NOTIFY | BLE_ATTR_READ, uuid, &(data) ) );

  }

//-----------------------------------------------------------------------------
//  function: handling_limit_characteristic ( handling )
// arguments: handling - service resource
//   returns: NRF_ERROR_INVALID_PARAM - if parameters are invalid or missing
//            NRF_SUCCESS - if added
//
// Register the value characteristic with the GATT service. This is a read-write
// structure with values for force, angle and preferred orientation.
//-----------------------------------------------------------------------------

static unsigned handling_limit_characteristic ( handling_t * handling ) {

  const void *                   uuid = handling_id ( HANDLING_LIMIT_UUID );
  softble_characteristic_t       data = { .handles  = &(handling->handle.limit),
                                          .length   = sizeof(handling_values_t),
                                          .limit    = sizeof(handling_values_t),
                                          .value    = &(handling->value.limit) };

  return ( softble_characteristic_declare ( handling->service, BLE_ATTR_PROTECTED | BLE_ATTR_WRITE | BLE_ATTR_READ, uuid, &(data) ) );
  
  }
