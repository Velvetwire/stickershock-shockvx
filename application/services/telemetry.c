//=============================================================================
// project: ShockVx
//  module: Stickershock Sensor for cold chain tracking.
//  author: Velvetwire, llc
//    file: telemetry.c
//
// Telemetry control and settings service.
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
//  function: telemetry_register ( interval, archival )
// arguments: interval - measurment interval (seconds)
//            archival - recording interval (seconds)
//   returns: NRF_ERROR_RESOURCES if no resources available
//            NRF_SUCCESS if registered
//
// Register the telemetry GATT service with the Bluetooth stack.
//-----------------------------------------------------------------------------

unsigned telemetry_register ( float interval, float archival ) {

  telemetry_t *             telemetry = &(resource);
  unsigned                     result = NRF_SUCCESS;

  // Initialize the service resource.

  if ( telemetry->service == BLE_GATT_HANDLE_INVALID ) { ctl_mutex_init ( &(telemetry->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );

  // Register the service with the soft device low energy stack and add the
  // service characteristics.

  if ( (telemetry->service = softble_server_register ( BLE_GATTS_SRVC_TYPE_PRIMARY, telemetry_uuid ( ) )) ) {

    if ( NRF_SUCCESS == result ) { result = telemetry_interval_characteristic ( telemetry, interval ); }
    if ( NRF_SUCCESS == result ) { result = telemetry_archival_characteristic ( telemetry, archival ); }

    } else return ( NRF_ERROR_RESOURCES );

  // Request a subcription to the soft device event publisher.

  if ( NRF_SUCCESS == result ) { result = softble_subscribe ( (softble_subscriber_t) telemetry_event, telemetry ); }
  
  // Return with registration result.

  return ( result );

  }

//-----------------------------------------------------------------------------
//  function: telemetry_settings ( interval, archival )
// arguments: interval - value to receive measurement interval
//            archival - structure to receive lower limit settings
//   returns: NRF_SUCCESS if retrieved
//
// Get the limit settings.
//-----------------------------------------------------------------------------

unsigned telemetry_settings ( float * interval, float * archival ) {

  telemetry_t *             telemetry = &(resource);
  
  if ( interval ) { *(interval) = telemetry->value.interval; }
  if ( archival ) { *(archival) = telemetry->value.archival; }

  return ( NRF_SUCCESS );

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
  if ( write->handle == telemetry->handle.archival.value_handle ) { memcpy ( (void *) &(telemetry->value.archival) + write->offset, write->data, write->len ); }

  // Write processed.

  return ( NRF_SUCCESS );

  }


//=============================================================================
// SECTION : SERVICE CHARACTERISITC DECLARATIONS
//=============================================================================

//-----------------------------------------------------------------------------
//  function: telemetry_interval_characteristic ( telemetry, period )
// arguments: telemetry - service resource
//            period - period in seconds
//   returns: NRF_ERROR_INVALID_PARAM - if parameters are invalid or missing
//            NRF_SUCCESS - if added
//
// Register the measurement interval characteristic with the GATT service. This
// is a read-write floating point value.
//-----------------------------------------------------------------------------

static unsigned telemetry_interval_characteristic ( telemetry_t * telemetry, float period ) {

  const void *                   uuid = telemetry_id ( TELEMETRY_INTERVAL_UUID );
  softble_characteristic_t       data = { .handles  = &(telemetry->handle.interval),
                                          .length   = sizeof(float),
                                          .limit    = sizeof(float),
                                          .value    = &(telemetry->value.interval) };
  
  telemetry->value.interval           = period;

  return ( softble_characteristic_declare ( telemetry->service, BLE_ATTR_PROTECTED | BLE_ATTR_WRITE | BLE_ATTR_READ, uuid, &(data) ) );

  }


//-----------------------------------------------------------------------------
//  function: telemetry_archival_characteristic ( telemetry, period )
// arguments: telemetry - service resource
//            period - period in seconds
//   returns: NRF_ERROR_INVALID_PARAM - if parameters are invalid or missing
//            NRF_SUCCESS - if added
//
// Register the measurement interval characteristic with the GATT service. This
// is a read-write floating point value.
//-----------------------------------------------------------------------------

static unsigned telemetry_archival_characteristic ( telemetry_t * telemetry, float period ) {

  const void *                   uuid = telemetry_id ( TELEMETRY_ARCHIVAL_UUID );
  softble_characteristic_t       data = { .handles  = &(telemetry->handle.archival),
                                          .length   = sizeof(float),
                                          .limit    = sizeof(float),
                                          .value    = &(telemetry->value.archival) };
  
  telemetry->value.archival           = period;

  return ( softble_characteristic_declare ( telemetry->service, BLE_ATTR_PROTECTED | BLE_ATTR_WRITE | BLE_ATTR_READ, uuid, &(data) ) );

  }