//=============================================================================
// project: ShockVx
//  module: Stickershock Sensor for cold chain tracking.
//  author: Velvetwire, llc
//    file: telemetry.c
//
// Telemetry metrics and settings service.
//
// (c) Copyright 2016-2020 Velvetwire, LLC. All rights reserved.
//=============================================================================

#include  <stickershock.h>

#include  "bluetooth.h"
#include  "telemetry.h"

//=============================================================================
// SECTION : SERVICE RESOURCE
//=============================================================================

//-----------------------------------------------------------------------------
// Declare the service class resource area.
//-----------------------------------------------------------------------------

static    telemetry_t        resource = { 0 };

//-----------------------------------------------------------------------------
//  function: telemetry_uuid ( )
// arguments: none
//   returns: the 128-bit service UUID
//
// Retrieve the UUID for the service class.
//-----------------------------------------------------------------------------

const void *                  telemetry_uuid ( void ) { return ( telemetry_id ( TELEMETRY_SERVICES_UUID ) ); }
static const void *           telemetry_id ( unsigned service ) { static uuid_t id; return ( uuid ( &(id), service ) ); }


//=============================================================================
// SECTION : SERVICE CLASS
//=============================================================================

//-----------------------------------------------------------------------------
//  function: telemetry_register ( interval, lower, upper )
// arguments: interval - measurment interval (seconds)
//            lower - lower limits
//            upper - upper limits
//   returns: NRF_ERROR_RESOURCES if no resources available
//            NRF_SUCCESS if registered
//
// Register the telemetry metrics GATT service with the Bluetooth stack.
//-----------------------------------------------------------------------------

unsigned telemetry_register ( float interval, telemetry_values_t * lower, telemetry_values_t * upper ) {

  telemetry_t *             telemetry = &(resource);
  unsigned                     result = NRF_SUCCESS;

  // Initialize the service resource.

  if ( telemetry->service == BLE_GATT_HANDLE_INVALID ) { ctl_mutex_init ( &(telemetry->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );

  telemetry->value.interval           = interval;

  if ( lower ) { memcpy ( &(telemetry->value.lower), lower, sizeof(telemetry_values_t) ); }
  if ( upper ) { memcpy ( &(telemetry->value.upper), upper, sizeof(telemetry_values_t) ); }

  // Register the service with the soft device low energy stack and add the
  // service characteristics.

  if ( (telemetry->service = softble_server_register ( BLE_GATTS_SRVC_TYPE_PRIMARY, telemetry_uuid ( ) )) ) {

    if ( NRF_SUCCESS == result ) { result = telemetry_interval_characteristic ( telemetry ); }
    if ( NRF_SUCCESS == result ) { result = telemetry_value_characteristic ( telemetry ); }
    if ( NRF_SUCCESS == result ) { result = telemetry_lower_characteristic ( telemetry ); }
    if ( NRF_SUCCESS == result ) { result = telemetry_upper_characteristic ( telemetry ); }

    } else return ( NRF_ERROR_RESOURCES );

  // Request a subcription to the soft device event publisher.

  if ( NRF_SUCCESS == result ) { result = softble_subscribe ( (softble_subscriber_t) telemetry_event, telemetry ); }
  
  // Return with registration result.

  return ( result );

  }

//-----------------------------------------------------------------------------
//  function: telemetry_settings ( interval, lower, upper )
// arguments: interval - value to receive measurement interval
//            lower - structure to receive lower limit settings
//            upper - structure to receive upper limit settings
//   returns: NRF_SUCCESS if retrieved
//
// Get the limit settings.
//-----------------------------------------------------------------------------

unsigned telemetry_settings ( float * interval, telemetry_values_t * lower, telemetry_values_t * upper ) {

  telemetry_t *             telemetry = &(resource);
  
  if ( interval ) { *(interval) = telemetry->value.interval; }

  if ( lower ) { memcpy ( lower, &(telemetry->value.lower), sizeof(telemetry_values_t) ); }
  if ( upper ) { memcpy ( upper, &(telemetry->value.upper), sizeof(telemetry_values_t) ); }

  return ( NRF_SUCCESS );

  }

//-----------------------------------------------------------------------------
//  function: telemetry_values ( values )
// arguments: values - pointer to telemetry values structure
//   returns: NRF_SUCCESS - if update issued
//            NRF_ERROR_INVALID_STATE - if service is not registered
//
// Update the telemetry values characteristic.
//-----------------------------------------------------------------------------

unsigned telemetry_values ( telemetry_values_t * values ) {

  telemetry_t *             telemetry = &(resource);
  unsigned                     result = NRF_SUCCESS;

  // Make sure that the requested notice is valid and register the notice.

  if ( telemetry->service != BLE_GATT_HANDLE_INVALID ) { ctl_mutex_lock_uc ( &(telemetry->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );

  // Update the metrics values structure and issue a notify to any connected peers.

  if ( values ) {

    unsigned short             handle = telemetry->handle.value.value_handle;

    if ( NRF_SUCCESS == (result = softble_characteristic_update ( handle, values, 0, sizeof(telemetry_values_t) )) ) { result = softble_characteristic_notify ( handle, BLE_CONN_HANDLE_ALL ); }
    
    } else { result = NRF_ERROR_NULL; }

  // Return with the result.

  return ( ctl_mutex_unlock ( &(telemetry->mutex) ), result );

  }


//=============================================================================
// SECTION : SERVICE RESPONDER
//=============================================================================

//-----------------------------------------------------------------------------
//  callback: telemetry_event ( telemetry, event )
// arguments: telemetry - service resource
//            event - BLE event structure
//   returns: NRF_SUCCESS if event processed
//
// Bluetooth service responder callback handler.
//-----------------------------------------------------------------------------

static unsigned telemetry_event ( telemetry_t * telemetry, ble_evt_t * event ) {
  
  switch ( event->header.evt_id ) {
    
    case BLE_GATTS_EVT_WRITE:     return telemetry_write ( telemetry, event->evt.gatts_evt.conn_handle, &(event->evt.gatts_evt.params.write) );

    default:                      return ( NRF_SUCCESS );

    }

  }

//-----------------------------------------------------------------------------
//  function: telemetry_write ( telemetry, connection, write )
// arguments: telemetry - service resource
//            connection - connection handle
//            write - write information structure
//   returns: NRF_SUCCESS if processed
//
// Handle service characteristic write reqests.
//-----------------------------------------------------------------------------

static unsigned telemetry_write ( telemetry_t * telemetry, unsigned short connection, ble_gatts_evt_write_t * write ) {

  // For protected characteristics, the write data needs to be transferred
  // directly to the value data.

  if ( write->handle == telemetry->handle.interval.value_handle ) { memcpy ( (void *) &(telemetry->value.interval) + write->offset, write->data, write->len ); }
  
  if ( write->handle == telemetry->handle.upper.value_handle ) { memcpy ( (void *) &(telemetry->value.upper) + write->offset, write->data, write->len ); }
  if ( write->handle == telemetry->handle.lower.value_handle ) { memcpy ( (void *) &(telemetry->value.lower) + write->offset, write->data, write->len ); }

  // Write processed.

  return ( NRF_SUCCESS );

  }


//=============================================================================
// SECTION : SERVICE CHARACTERISITC DECLARATIONS
//=============================================================================

//-----------------------------------------------------------------------------
//  function: telemetry_interval_characteristic ( telemetry )
// arguments: telemetry - service resource
//   returns: NRF_ERROR_INVALID_PARAM - if parameters are invalid or missing
//            NRF_SUCCESS - if added
//
// Register the measurement interval characteristic with the GATT service. This
// is a read-write floating point value.
//-----------------------------------------------------------------------------

static unsigned telemetry_interval_characteristic ( telemetry_t * telemetry ) {

  const void *                   uuid = telemetry_id ( TELEMETRY_INTERVAL_UUID );
  softble_characteristic_t       data = { .handles  = &(telemetry->handle.interval),
                                          .length   = sizeof(float),
                                          .limit    = sizeof(float),
                                          .value    = &(telemetry->value.interval) };

  return ( softble_characteristic_declare ( telemetry->service, BLE_ATTR_PROTECTED | BLE_ATTR_WRITE | BLE_ATTR_READ, uuid, &(data) ) );

  }

//-----------------------------------------------------------------------------
//  function: telemetry_value_characteristic ( telemetry )
// arguments: telemetry - service resource
//   returns: NRF_ERROR_INVALID_PARAM - if parameters are invalid or missing
//            NRF_SUCCESS - if added
//
// Register the measured values characteristic with the GATT service. This is
// a read-only structure with values expressed in floating point.
//-----------------------------------------------------------------------------

static unsigned telemetry_value_characteristic ( telemetry_t * telemetry ) {

  const void *                   uuid = telemetry_id ( TELEMETRY_VALUE_UUID );
  softble_characteristic_t       data = { .handles  = &(telemetry->handle.value),
                                          .length   = sizeof(telemetry_values_t),
                                          .limit    = sizeof(telemetry_values_t),
                                          .value    = &(telemetry->value.value) };

  return ( softble_characteristic_declare ( telemetry->service, BLE_ATTR_NOTIFY | BLE_ATTR_READ, uuid, &(data) ) );
  
  }

//-----------------------------------------------------------------------------
//  function: telemetry_upper_characteristic ( telemetry )
// arguments: telemetry - service resource
//   returns: NRF_ERROR_INVALID_PARAM - if parameters are invalid or missing
//            NRF_SUCCESS - if added
//
// Register the upper limit settings characteristic with the GATT service. This
// is a read-write structure with limits expressed in floating point.
//-----------------------------------------------------------------------------

static unsigned telemetry_upper_characteristic ( telemetry_t * telemetry ) {

  const void *                   uuid = telemetry_id ( TELEMETRY_UPPER_UUID );
  softble_characteristic_t       data = { .handles  = &(telemetry->handle.upper),
                                          .length   = sizeof(telemetry_values_t),
                                          .limit    = sizeof(telemetry_values_t),
                                          .value    = &(telemetry->value.upper) };

  return ( softble_characteristic_declare ( telemetry->service, BLE_ATTR_PROTECTED | BLE_ATTR_WRITE | BLE_ATTR_READ, uuid, &(data) ) );

  }

//-----------------------------------------------------------------------------
//  function: telemetry_lower_characteristic ( telemetry )
// arguments: telemetry - service resource
//   returns: NRF_ERROR_INVALID_PARAM - if parameters are invalid or missing
//            NRF_SUCCESS - if added
//
// Register the lower limit settings characteristic with the GATT service. This
// is a read-write structure with limits expressed in floating point.
//-----------------------------------------------------------------------------

static unsigned telemetry_lower_characteristic ( telemetry_t * telemetry ) {

  const void *                   uuid = telemetry_id ( TELEMETRY_LOWER_UUID );
  softble_characteristic_t       data = { .handles  = &(telemetry->handle.lower),
                                          .length   = sizeof(telemetry_values_t),
                                          .limit    = sizeof(telemetry_values_t),
                                          .value    = &(telemetry->value.lower) };

  return ( softble_characteristic_declare ( telemetry->service, BLE_ATTR_PROTECTED | BLE_ATTR_WRITE | BLE_ATTR_READ, uuid, &(data) ) );

  }