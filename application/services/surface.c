//=============================================================================
// project: ShockVx
//  module: Stickershock Sensor for cold chain tracking.
//  author: Velvetwire, llc
//    file: surface.c
//
// Surface temperature telemetry and settings service.
//
// (c) Copyright 2016-2020 Velvetwire, LLC. All rights reserved.
//=============================================================================

#include  <stickershock.h>

#include  "bluetooth.h"
#include  "surface.h"

//=============================================================================
// SECTION : SERVICE RESOURCE
//=============================================================================

//-----------------------------------------------------------------------------
// Declare the service class resource area.
//-----------------------------------------------------------------------------

static    surface_t          resource = { 0 };

//-----------------------------------------------------------------------------
//  function: surface_uuid ( )
// arguments: none
//   returns: the 128-bit service UUID
//
// Retrieve the UUID for the service class.
//-----------------------------------------------------------------------------

const void *                  surface_uuid ( void ) { return ( surface_id ( SURFACE_SERVICES_UUID ) ); }
static const void *           surface_id ( unsigned service ) { static uuid_t id; return ( uuid ( &(id), service ) ); }


//=============================================================================
// SECTION : SERVICE CLASS
//=============================================================================

//-----------------------------------------------------------------------------
//  function: surface_register ( lower, upper )
// arguments: lower - lower limit
//            upper - upper limit
//   returns: NRF_ERROR_RESOURCES if no resources available
//            NRF_SUCCESS if registered
//
// Register the atmospheric telemetry GATT service with the Bluetooth stack.
//-----------------------------------------------------------------------------

unsigned surface_register ( float lower, float upper ) {

  surface_t *                 surface = &(resource);
  unsigned                     result = NRF_SUCCESS;

  // Initialize the service resource.

  if ( surface->service == BLE_GATT_HANDLE_INVALID ) { ctl_mutex_init ( &(surface->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );

  // Register the service with the soft device low energy stack and add the
  // service characteristics.

  if ( (surface->service = softble_server_register ( BLE_GATTS_SRVC_TYPE_PRIMARY, surface_uuid ( ) )) ) {

    if ( NRF_SUCCESS == result ) { result = surface_value_characteristic ( surface ); }
    if ( NRF_SUCCESS == result ) { result = surface_lower_characteristic ( surface, lower ); }
    if ( NRF_SUCCESS == result ) { result = surface_upper_characteristic ( surface, upper ); }

    } else return ( NRF_ERROR_RESOURCES );

  // Request a subcription to the soft device event publisher.

  if ( NRF_SUCCESS == result ) { result = softble_subscribe ( (softble_subscriber_t) surface_event, surface ); }
  
  // Return with registration result.

  return ( result );

  }

//-----------------------------------------------------------------------------
//  function: surface_settings ( lower, upper )
// arguments: lower - structure to receive lower limit settings
//            upper - structure to receive upper limit settings
//   returns: NRF_SUCCESS if retrieved
//
// Get the limit settings.
//-----------------------------------------------------------------------------

unsigned surface_settings ( float * lower, float * upper ) {

  surface_t *           surface = &(resource);
  
  if ( lower ) { *(lower) = surface->value.lower; }
  if ( upper ) { *(upper) = surface->value.upper; }

  return ( NRF_SUCCESS );

  }

//-----------------------------------------------------------------------------
//  function: surface_measured ( value )
// arguments: values - pointer to measured values structure
//   returns: NRF_SUCCESS - if update issued
//            NRF_ERROR_INVALID_STATE - if service is not registered
//
// Update the surface values characteristic.
//-----------------------------------------------------------------------------

unsigned surface_measured ( float value ) {

  surface_t *                 surface = &(resource);
  unsigned                     result = NRF_SUCCESS;

  // Make sure that the requested notice is valid and register the notice.

  if ( surface->service != BLE_GATT_HANDLE_INVALID ) { ctl_mutex_lock_uc ( &(surface->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );

  // Update the metrics values structure and issue a notify to any connected peers.

  unsigned short               handle = surface->handle.value.value_handle;

  if ( NRF_SUCCESS == (result = softble_characteristic_update ( handle, &(value), 0, sizeof(float) )) ) { result = softble_characteristic_notify ( handle, BLE_CONN_HANDLE_ALL ); }

  // Return with the result.

  return ( ctl_mutex_unlock ( &(surface->mutex) ), result );

  }


//=============================================================================
// SECTION : SERVICE RESPONDER
//=============================================================================

//-----------------------------------------------------------------------------
//  callback: surface_event ( surface, event )
// arguments: surface - service resource
//            event - BLE event structure
//   returns: NRF_SUCCESS if event processed
//
// Bluetooth service responder callback handler.
//-----------------------------------------------------------------------------

static unsigned surface_event ( surface_t * surface, ble_evt_t * event ) {
  
  switch ( event->header.evt_id ) {
    
    case BLE_GATTS_EVT_WRITE:     return surface_write ( surface, event->evt.gatts_evt.conn_handle, &(event->evt.gatts_evt.params.write) );

    default:                      return ( NRF_SUCCESS );

    }

  }

//-----------------------------------------------------------------------------
//  function: surface_write ( surface, connection, write )
// arguments: surface - service resource
//            connection - connection handle
//            write - write information structure
//   returns: NRF_SUCCESS if processed
//
// Handle service characteristic write reqests.
//-----------------------------------------------------------------------------

static unsigned surface_write ( surface_t * surface, unsigned short connection, ble_gatts_evt_write_t * write ) {

  // For protected characteristics, the write data needs to be transferred
  // directly to the value data.

  if ( write->handle == surface->handle.upper.value_handle ) { memcpy ( (void *) &(surface->value.upper) + write->offset, write->data, write->len ); }
  if ( write->handle == surface->handle.lower.value_handle ) { memcpy ( (void *) &(surface->value.lower) + write->offset, write->data, write->len ); }

  // Write processed.

  return ( NRF_SUCCESS );

  }


//=============================================================================
// SECTION : SERVICE CHARACTERISTIC DECLARATIONS
//=============================================================================

//-----------------------------------------------------------------------------
//  function: surface_value_characteristic ( surface )
// arguments: surface - service resource
//   returns: NRF_ERROR_INVALID_PARAM - if parameters are invalid or missing
//            NRF_SUCCESS - if added
//
// Register the measured values characteristic with the GATT service. This is
// a read-only structure with values expressed in floating point.
//-----------------------------------------------------------------------------

static unsigned surface_value_characteristic ( surface_t * surface ) {

  const void *                   uuid = surface_id ( SURFACE_VALUE_UUID );
  softble_characteristic_t       data = { .handles  = &(surface->handle.value),
                                          .length   = sizeof(float),
                                          .limit    = sizeof(float),
                                          .value    = &(surface->value.value) };

  return ( softble_characteristic_declare ( surface->service, BLE_ATTR_NOTIFY | BLE_ATTR_READ, uuid, &(data) ) );
  
  }

//-----------------------------------------------------------------------------
//  function: surface_upper_characteristic ( surface, value )
// arguments: surface - service resource
//            value - initial value
//   returns: NRF_ERROR_INVALID_PARAM - if parameters are invalid or missing
//            NRF_SUCCESS - if added
//
// Register the upper limit settings characteristic with the GATT service. This
// is a read-write structure with limits expressed in floating point.
//-----------------------------------------------------------------------------

static unsigned surface_upper_characteristic ( surface_t * surface, float value ) {

  const void *                   uuid = surface_id ( SURFACE_UPPER_UUID );
  softble_characteristic_t       data = { .handles  = &(surface->handle.upper),
                                          .length   = sizeof(float),
                                          .limit    = sizeof(float),
                                          .value    = &(surface->value.upper) };

  surface->value.upper                = value;

  return ( softble_characteristic_declare ( surface->service, BLE_ATTR_PROTECTED | BLE_ATTR_WRITE | BLE_ATTR_READ, uuid, &(data) ) );

  }

//-----------------------------------------------------------------------------
//  function: surface_lower_characteristic ( surface, value )
// arguments: surface - service resource
//            value - initial value
//   returns: NRF_ERROR_INVALID_PARAM - if parameters are invalid or missing
//            NRF_SUCCESS - if added
//
// Register the lower limit settings characteristic with the GATT service. This
// is a read-write structure with limits expressed in floating point.
//-----------------------------------------------------------------------------

static unsigned surface_lower_characteristic ( surface_t * surface, float value ) {

  const void *                   uuid = surface_id ( SURFACE_LOWER_UUID );
  softble_characteristic_t       data = { .handles  = &(surface->handle.lower),
                                          .length   = sizeof(float),
                                          .limit    = sizeof(float),
                                          .value    = &(surface->value.lower) };

  surface->value.lower                = value;

  return ( softble_characteristic_declare ( surface->service, BLE_ATTR_PROTECTED | BLE_ATTR_WRITE | BLE_ATTR_READ, uuid, &(data) ) );

  }