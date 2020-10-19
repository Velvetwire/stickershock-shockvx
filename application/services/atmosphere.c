//=============================================================================
// project: ShockVx
//  module: Stickershock Sensor for cold chain tracking.
//  author: Velvetwire, llc
//    file: atmosphere.c
//
// Atmospheric telemetry and settings service.
//
// (c) Copyright 2016-2020 Velvetwire, LLC. All rights reserved.
//=============================================================================

#include  <stickershock.h>

#include  "bluetooth.h"
#include  "atmosphere.h"

//=============================================================================
// SECTION : SERVICE RESOURCE
//=============================================================================

//-----------------------------------------------------------------------------
// Declare the service class resource area.
//-----------------------------------------------------------------------------

static    atmosphere_t       resource = { 0 };

//-----------------------------------------------------------------------------
//  function: atmosphere_uuid ( )
// arguments: none
//   returns: the 128-bit service UUID
//
// Retrieve the UUID for the service class.
//-----------------------------------------------------------------------------

const void *                  atmosphere_uuid ( void ) { return ( atmosphere_id ( ATMOSPHERE_SERVICES_UUID ) ); }
static const void *           atmosphere_id ( unsigned service ) { static uuid_t id; return ( uuid ( &(id), service ) ); }


//=============================================================================
// SECTION : SERVICE CLASS
//=============================================================================

//-----------------------------------------------------------------------------
//  function: atmosphere_register ( lower, upper )
// arguments: lower - lower limits
//            upper - upper limits
//   returns: NRF_ERROR_RESOURCES if no resources available
//            NRF_SUCCESS if registered
//
// Register the atmospheric telemetry GATT service with the Bluetooth stack.
//-----------------------------------------------------------------------------

unsigned atmosphere_register ( atmosphere_values_t * lower, atmosphere_values_t * upper ) {

  atmosphere_t *           atmosphere = &(resource);
  unsigned                     result = NRF_SUCCESS;

  // Initialize the service resource.

  if ( atmosphere->service == BLE_GATT_HANDLE_INVALID ) { ctl_mutex_init ( &(atmosphere->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );

  if ( lower ) { memcpy ( &(atmosphere->value.lower), lower, sizeof(atmosphere_values_t) ); }
  if ( upper ) { memcpy ( &(atmosphere->value.upper), upper, sizeof(atmosphere_values_t) ); }

  // Register the service with the soft device low energy stack and add the
  // service characteristics.

  if ( (atmosphere->service = softble_server_register ( BLE_GATTS_SRVC_TYPE_PRIMARY, atmosphere_uuid ( ) )) ) {

    if ( NRF_SUCCESS == result ) { result = atmosphere_value_characteristic ( atmosphere ); }
    if ( NRF_SUCCESS == result ) { result = atmosphere_lower_characteristic ( atmosphere ); }
    if ( NRF_SUCCESS == result ) { result = atmosphere_upper_characteristic ( atmosphere ); }

    if ( NRF_SUCCESS == result ) { result = atmosphere_event_characteristic ( atmosphere ); }
    if ( NRF_SUCCESS == result ) { result = atmosphere_count_characteristic ( atmosphere ); }

    } else return ( NRF_ERROR_RESOURCES );

  // Request a subcription to the soft device event publisher.

  if ( NRF_SUCCESS == result ) { result = softble_subscribe ( (softble_subscriber_t) atmosphere_event, atmosphere ); }
  
  // Return with registration result.

  return ( result );

  }

//-----------------------------------------------------------------------------
//  function: atmosphere_settings ( lower, upper )
// arguments: lower - structure to receive lower limit settings
//            upper - structure to receive upper limit settings
//   returns: NRF_SUCCESS if retrieved
//
// Get the limit settings.
//-----------------------------------------------------------------------------

unsigned atmosphere_settings ( atmosphere_values_t * lower, atmosphere_values_t * upper ) {

  atmosphere_t *           atmosphere = &(resource);
  
  if ( lower ) { memcpy ( lower, &(atmosphere->value.lower), sizeof(atmosphere_values_t) ); }
  if ( upper ) { memcpy ( upper, &(atmosphere->value.upper), sizeof(atmosphere_values_t) ); }

  return ( NRF_SUCCESS );

  }

//-----------------------------------------------------------------------------
//  function: atmosphere_measured ( values )
// arguments: values - pointer to measured values structure
//   returns: NRF_SUCCESS - if update issued
//            NRF_ERROR_INVALID_STATE - if service is not registered
//
// Update the atmosphere values characteristic.
//-----------------------------------------------------------------------------

unsigned atmosphere_measured ( atmosphere_values_t * values ) {

  atmosphere_t *           atmosphere = &(resource);

  if ( ! values ) return ( NRF_ERROR_NULL );

  // Make sure that the service has been registered with the stack.

  if ( atmosphere->service != BLE_GATT_HANDLE_INVALID ) { ctl_mutex_lock_uc ( &(atmosphere->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );

  // Update the metrics values structure and issue a notify to any connected peers.

  unsigned short               handle = atmosphere->handle.value.value_handle;
  unsigned                     result = softble_characteristic_update ( handle, values, 0, sizeof(atmosphere_values_t) );
    
  if ( NRF_SUCCESS == result ) { softble_characteristic_notify ( handle, BLE_CONN_HANDLE_ALL ); }

  // Return with the result.

  return ( ctl_mutex_unlock ( &(atmosphere->mutex) ), result );

  }

//-----------------------------------------------------------------------------
//  function: atmosphere_archive ( )
// arguments: none
//   returns: NRF_SUCCESS - if update issued
//            NRF_ERROR_INVALID_STATE - if service is not registered
//
// Request that the current values be recorded as an event in the archive.
//-----------------------------------------------------------------------------

unsigned atmosphere_archive ( void ) {

  atmosphere_t *           atmosphere = &(resource);
  unsigned                     result = NRF_SUCCESS;
  
  // Make sure that the service has been registered with the stack.

  if ( atmosphere->service != BLE_GATT_HANDLE_INVALID ) { ctl_mutex_lock_uc ( &(atmosphere->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );

  // Open the archive and append an event record based on the values in
  // measurement characteristic.

  file_handle_t               archive = file_open ( ATMOSPHERE_ARCHIVE, FILE_MODE_CREATE | FILE_MODE_WRITE | FILE_MODE_READ );

  if ( archive > FILE_OK ) {
    
    atmosphere_record_t        record = { .time = ctl_time_get ( ) };
    unsigned short             handle = atmosphere->handle.count.value_handle;
    unsigned short              count = (unsigned short) (file_tail ( archive ) / sizeof(atmosphere_record_t));

    record.data.temperature           = (short) roundf ( atmosphere->value.value.temperature * 1e2 );
    record.data.humidity              = (short) roundf ( atmosphere->value.value.humidity * 1e4 );
    record.data.pressure              = (short) roundf ( atmosphere->value.value.pressure * 1e3 );

    if ( sizeof(atmosphere_record_t) == file_write ( archive, &(record), sizeof(atmosphere_record_t) ) ) { ++ count; }
    else { result = NRF_ERROR_NO_MEM; }

    if ( NRF_SUCCESS == (result = softble_characteristic_update ( handle, &(count), 0, sizeof(short) )) ) {
      softble_characteristic_notify ( handle, BLE_CONN_HANDLE_ALL );
      }
 
    file_close ( archive );

    } else { result = NRF_ERROR_INTERNAL; }

  // Return with the result.

  return ( ctl_mutex_unlock ( &(atmosphere->mutex) ), result );

  }


//=============================================================================
// SECTION : SERVICE RESPONDER
//=============================================================================

//-----------------------------------------------------------------------------
//  callback: atmosphere_event ( atmosphere, event )
// arguments: atmosphere - service resource
//            event - BLE event structure
//   returns: NRF_SUCCESS if event processed
//
// Bluetooth service responder callback handler.
//-----------------------------------------------------------------------------

static unsigned atmosphere_event ( atmosphere_t * atmosphere, ble_evt_t * event ) {
  
  switch ( event->header.evt_id ) {
    
    case BLE_GAP_EVT_CONNECTED:   return atmosphere_start ( atmosphere, event->evt.gap_evt.conn_handle, &(event->evt.gap_evt.params.connected) );
    case BLE_GATTS_EVT_WRITE:     return atmosphere_write ( atmosphere, event->evt.gatts_evt.conn_handle, &(event->evt.gatts_evt.params.write) );

    default:                      return ( NRF_SUCCESS );

    }

  }

//-----------------------------------------------------------------------------
//  function: atmosphere_start ( atmosphere, connection, connected )
// arguments: atmosphere - service resource
//            connection - connection handle
//            connected - connected information structure
//   returns: NRF_SUCCESS if processed
//
// Connection to a peer has been established. Re-load the event count and reset
// the event record characteristic.
//-----------------------------------------------------------------------------

static unsigned atmosphere_start ( atmosphere_t * atmosphere, unsigned short connection, ble_gap_evt_connected_t * connected ) {

  file_handle_t               archive = file_open ( ATMOSPHERE_ARCHIVE, FILE_MODE_READ );
  atmosphere_record_t          record = { 0 };
  unsigned short                count = 0;

  if ( archive > FILE_OK ) { count = file_size ( archive, NULL ) / sizeof(atmosphere_record_t); }

  softble_characteristic_update ( atmosphere->handle.count.value_handle, &(count), 0, sizeof(short) );
  softble_characteristic_update ( atmosphere->handle.event.value_handle, &(record), 0, 0 );

  file_close ( archive );

  }

//-----------------------------------------------------------------------------
//  function: atmosphere_write ( atmosphere, connection, write )
// arguments: atmosphere - service resource
//            connection - connection handle
//            write - write information structure
//   returns: NRF_SUCCESS if processed
//
// Handle service characteristic write reqests.
//-----------------------------------------------------------------------------

static unsigned atmosphere_write ( atmosphere_t * atmosphere, unsigned short connection, ble_gatts_evt_write_t * write ) {

  // If this is a request to fetch an event record, process the request.

  if ( (write->handle == atmosphere->handle.event.value_handle) && (write->len == sizeof(short)) ) { atmosphere_fetch ( atmosphere, *((unsigned short *) write->data) ); }

  // For protected characteristics, the write data needs to be transferred
  // directly to the value data.

  if ( write->handle == atmosphere->handle.upper.value_handle ) { memcpy ( (void *) &(atmosphere->value.upper) + write->offset, write->data, write->len ); }
  if ( write->handle == atmosphere->handle.lower.value_handle ) { memcpy ( (void *) &(atmosphere->value.lower) + write->offset, write->data, write->len ); }

  // Write processed.

  return ( NRF_SUCCESS );

  }

//-----------------------------------------------------------------------------
//  function: atmosphere_fetch ( atmosphere, index )
// arguments: atmosphere - service resource
//            inbdex - event record index
//   returns: NRF_SUCCESS if successful
//
// Retrieve the event record from the archive and post it to the event
// characteristic with notification.
//-----------------------------------------------------------------------------

static unsigned atmosphere_fetch ( atmosphere_t * atmosphere, unsigned short index ) {

  file_handle_t               archive = file_open ( ATMOSPHERE_ARCHIVE, FILE_MODE_READ );
  unsigned short               handle = atmosphere->handle.event.value_handle;
  unsigned                     result = NRF_ERROR_NULL;

  if ( archive > FILE_OK ) {
   
    atmosphere_record_t        record = { 0 };
    int                        offset = sizeof(atmosphere_record_t) * index;

    if ( (offset == file_seek ( archive, FILE_SEEK_POSITION, offset ))
      && (sizeof(atmosphere_record_t) == file_read ( archive, &(record), sizeof(atmosphere_record_t) )) ) { result = NRF_SUCCESS; }

    if ( (NRF_SUCCESS == result) && (NRF_SUCCESS == (result = softble_characteristic_update ( handle, &(record), 0, sizeof(atmosphere_record_t) ))) ) {
      softble_characteristic_notify ( handle, BLE_CONN_HANDLE_ALL );
      }

    file_close ( archive );

    }

  return ( result );

  }


//=============================================================================
// SECTION : SERVICE CHARACTERISITC DECLARATIONS
//=============================================================================

//-----------------------------------------------------------------------------
//  function: atmosphere_value_characteristic ( atmosphere )
// arguments: atmosphere - service resource
//   returns: NRF_ERROR_INVALID_PARAM - if parameters are invalid or missing
//            NRF_SUCCESS - if added
//
// Register the measured values characteristic with the GATT service. This is
// a read-only structure with values expressed in floating point.
//-----------------------------------------------------------------------------

static unsigned atmosphere_value_characteristic ( atmosphere_t * atmosphere ) {

  const void *                   uuid = atmosphere_id ( ATMOSPHERE_VALUE_UUID );
  softble_characteristic_t       data = { .handles  = &(atmosphere->handle.value),
                                          .length   = sizeof(atmosphere_values_t),
                                          .limit    = sizeof(atmosphere_values_t),
                                          .value    = &(atmosphere->value.value) };

  return ( softble_characteristic_declare ( atmosphere->service, BLE_ATTR_NOTIFY | BLE_ATTR_READ, uuid, &(data) ) );
  
  }

//-----------------------------------------------------------------------------
//  function: atmosphere_upper_characteristic ( atmosphere )
// arguments: atmosphere - service resource
//   returns: NRF_ERROR_INVALID_PARAM - if parameters are invalid or missing
//            NRF_SUCCESS - if added
//
// Register the upper limit settings characteristic with the GATT service. This
// is a read-write structure with limits expressed in floating point.
//-----------------------------------------------------------------------------

static unsigned atmosphere_upper_characteristic ( atmosphere_t * atmosphere ) {

  const void *                   uuid = atmosphere_id ( ATMOSPHERE_UPPER_UUID );
  softble_characteristic_t       data = { .handles  = &(atmosphere->handle.upper),
                                          .length   = sizeof(atmosphere_values_t),
                                          .limit    = sizeof(atmosphere_values_t),
                                          .value    = &(atmosphere->value.upper) };

  return ( softble_characteristic_declare ( atmosphere->service, BLE_ATTR_PROTECTED | BLE_ATTR_WRITE | BLE_ATTR_READ, uuid, &(data) ) );

  }

//-----------------------------------------------------------------------------
//  function: atmosphere_lower_characteristic ( atmosphere )
// arguments: atmosphere - service resource
//   returns: NRF_ERROR_INVALID_PARAM - if parameters are invalid or missing
//            NRF_SUCCESS - if added
//
// Register the lower limit settings characteristic with the GATT service. This
// is a read-write structure with limits expressed in floating point.
//-----------------------------------------------------------------------------

static unsigned atmosphere_lower_characteristic ( atmosphere_t * atmosphere ) {

  const void *                   uuid = atmosphere_id ( ATMOSPHERE_LOWER_UUID );
  softble_characteristic_t       data = { .handles  = &(atmosphere->handle.lower),
                                          .length   = sizeof(atmosphere_values_t),
                                          .limit    = sizeof(atmosphere_values_t),
                                          .value    = &(atmosphere->value.lower) };

  return ( softble_characteristic_declare ( atmosphere->service, BLE_ATTR_PROTECTED | BLE_ATTR_WRITE | BLE_ATTR_READ, uuid, &(data) ) );

  }

//-----------------------------------------------------------------------------
//  function: atmosphere_event_characteristic ( atmosphere )
// arguments: atmosphere - service resource
//   returns: NRF_ERROR_INVALID_PARAM - if parameters are invalid or missing
//            NRF_SUCCESS - if added
//
// Register the archived event record characteristic. This is a read-write
// value which represents a single archived record when read, and which will
// fetch the given event record when written.
//-----------------------------------------------------------------------------

static unsigned atmosphere_event_characteristic ( atmosphere_t * atmosphere ) {

  const void *                   uuid = atmosphere_id ( ATMOSPHERE_EVENT_UUID );
  softble_characteristic_t       data = { .handles  = &(atmosphere->handle.event),
                                          .limit    = sizeof(atmosphere_record_t),
                                          .value    = &(atmosphere->value.event) };

  return ( softble_characteristic_declare ( atmosphere->service, BLE_ATTR_PROTECTED | BLE_ATTR_VARIABLE | BLE_ATTR_NOTIFY | BLE_ATTR_WRITE | BLE_ATTR_READ, uuid, &(data) ) );

  }

//-----------------------------------------------------------------------------
//  function: atmosphere_count_characteristic ( atmosphere )
// arguments: atmosphere - service resource
//   returns: NRF_ERROR_INVALID_PARAM - if parameters are invalid or missing
//            NRF_SUCCESS - if added
//
// Register the archived event count characteristic. This is a read-only value
// which indicates the count of archived events.
//-----------------------------------------------------------------------------

static unsigned atmosphere_count_characteristic ( atmosphere_t * atmosphere ) {

  const void *                   uuid = atmosphere_id ( ATMOSPHERE_COUNT_UUID );
  softble_characteristic_t       data = { .handles  = &(atmosphere->handle.count),
                                          .length   = sizeof(short),
                                          .limit    = sizeof(short),
                                          .value    = &(atmosphere->value.count) };

  return ( softble_characteristic_declare ( atmosphere->service, BLE_ATTR_PROTECTED | BLE_ATTR_NOTIFY | BLE_ATTR_READ, uuid, &(data) ) );

  }