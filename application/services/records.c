//=============================================================================
// project: ShockVx
//  module: Stickershock Sensor for cold chain tracking.
//  author: Velvetwire, llc
//    file: records.c
//
// Telemetry records service.
//
// (c) Copyright 2016-2020 Velvetwire, LLC. All rights reserved.
//=============================================================================

#include  <stickershock.h>

#include  "bluetooth.h"
#include  "records.h"

//=============================================================================
// SECTION : SERVICE RESOURCE
//=============================================================================

//-----------------------------------------------------------------------------
// Declare the service class resource area.
//-----------------------------------------------------------------------------

static    records_t          resource = { 0 };

//-----------------------------------------------------------------------------
//  function: records_uuid ( )
// arguments: none
//   returns: the 128-bit service UUID
//
// Retrieve the UUID for the service class.
//-----------------------------------------------------------------------------

const void *                  records_uuid ( void ) { return ( records_id ( RECORDS_SERVICES_UUID ) ); }
static const void *           records_id ( unsigned service ) { static uuid_t id; return ( uuid ( &(id), service ) ); }


//=============================================================================
// SECTION : SERVICE CLASS
//=============================================================================

//-----------------------------------------------------------------------------
//  function: records_register ( interval )
// arguments: interval - archival interval (seconds)
//   returns: NRF_ERROR_RESOURCES if no resources available
//            NRF_SUCCESS if registered
//
// Register the telemetry records GATT service with the Bluetooth stack.
//-----------------------------------------------------------------------------

unsigned records_register ( float interval ) {

  records_t *                 records = &(resource);
  unsigned                     result = NRF_SUCCESS;

  // Initialize the service resource.

  if ( records->service == BLE_GATT_HANDLE_INVALID ) { ctl_mutex_init ( &(records->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );

  records->value.interval             = interval;

  // Register the service with the soft device low energy stack and add the
  // service characteristics.

  if ( (records->service = softble_server_register ( BLE_GATTS_SRVC_TYPE_PRIMARY, records_uuid ( ) )) ) {

    if ( NRF_SUCCESS == result ) { result = records_interval_characteristic ( records ); }
    if ( NRF_SUCCESS == result ) { result = records_cursor_characteristic ( records ); }
    if ( NRF_SUCCESS == result ) { result = records_data_characteristic ( records ); }

    } else return ( NRF_ERROR_RESOURCES );

  // Request a subcription to the soft device event publisher.

  if ( NRF_SUCCESS == result ) { result = softble_subscribe ( (softble_subscriber_t) records_event, records ); }

  // Return with registration result.

  return ( result );

  }

//-----------------------------------------------------------------------------
//  function: records_settings ( limits )
// arguments: interval - value to receive archival interval
//   returns: identification interval in milliseconds
//
// Get the limit settings.
//-----------------------------------------------------------------------------

unsigned records_settings ( float * interval ) {

  records_t *                 records = &(resource);

  if ( interval ) { *(interval) = records->value.interval; }

  return ( NRF_SUCCESS );

  }

//-----------------------------------------------------------------------------
//  function: records_notice ( notice, set, events )
// arguments: notice - the notice index to enable or disable
//            set - event set to trigger (NULL to disable)
//            events - event bits to set
//   returns: NRF_SUCCESS - if notice enabled
//            NRF_ERROR_INVALID_PARAM - if the notice index is not valid
//
// Register for notices with the records service.
//-----------------------------------------------------------------------------

unsigned records_notice ( records_notice_t notice, CTL_EVENT_SET_t * set, CTL_EVENT_SET_t events ) {
  
  records_t *                 records = &(resource);

  // Make sure that the requested notice is valid and register the notice.

  if ( notice < RECORDS_NOTICES ) { ctl_mutex_lock_uc ( &(records->mutex) ); }
  else return ( NRF_ERROR_INVALID_PARAM );
  
  records->notice[ notice ].set       = set;
  records->notice[ notice ].events    = events;
  
  // Notice registered.

  return ( ctl_mutex_unlock ( &(records->mutex) ), NRF_SUCCESS );

  }


//=============================================================================
// SECTION : SERVICE RESPONDER
//=============================================================================

//-----------------------------------------------------------------------------
//  callback: records_event ( records, event )
// arguments: records - service resource
//            event - BLE event structure
//   returns: NRF_SUCCESS if event processed
//
// Bluetooth service responder callback handler.
//-----------------------------------------------------------------------------

static unsigned records_event ( records_t * records, ble_evt_t * event ) {
  
  switch ( event->header.evt_id ) {
    
    case BLE_GATTS_EVT_WRITE:     return records_write ( records, event->evt.gatts_evt.conn_handle, &(event->evt.gatts_evt.params.write) );

    default:                      return ( NRF_SUCCESS );

    }

  }

//-----------------------------------------------------------------------------
//  function: records_write ( records, connection, write )
// arguments: records - service resource
//            connection - connection handle
//            write - write information structure
//   returns: NRF_SUCCESS if processed
//
// Handle service characteristic write reqests.
//-----------------------------------------------------------------------------

static unsigned records_write ( records_t * records, unsigned short connection, ble_gatts_evt_write_t * write ) {

  // For protected characteristics, the write data needs to be transferred
  // directly to the value data.

  if ( write->handle == records->handle.interval.value_handle ) { memcpy ( (void *) &(records->value.interval) + write->offset, write->data, write->len ); }

  // Write processed.

  return ( NRF_SUCCESS );

  }


//=============================================================================
// SECTION : SERVICE CHARACTERISITC DECLARATIONS
//=============================================================================

//-----------------------------------------------------------------------------
//  function: records_interval_characteristic ( records )
// arguments: records - service resource
//   returns: NRF_ERROR_INVALID_PARAM - if parameters are invalid or missing
//            NRF_SUCCESS - if added
//
// Register the measurement interval characteristic with the GATT service. This
// is a read-write floating point value.
//-----------------------------------------------------------------------------

static unsigned records_interval_characteristic ( records_t * records ) {

  const void *                   uuid = records_id ( RECORDS_INTERVAL_UUID );
  softble_characteristic_t       data = { .handles  = &(records->handle.interval),
                                          .length   = sizeof(float),
                                          .limit    = sizeof(float),
                                          .value    = &(records->value.interval) };

  return ( softble_characteristic_declare ( records->service, BLE_ATTR_PROTECTED | BLE_ATTR_WRITE | BLE_ATTR_READ, uuid, &(data) ) );

  }

//-----------------------------------------------------------------------------
//  function: records_cursor_characteristic ( records )
// arguments: records - service resource
//   returns: NRF_ERROR_INVALID_PARAM - if parameters are invalid or missing
//            NRF_SUCCESS - if added
//
// Register the record cursor characteristic with the GATT service. This is a
// read-write structure with position and count elements.
//-----------------------------------------------------------------------------

static unsigned records_cursor_characteristic ( records_t * records ) {

  const void *                   uuid = records_id ( RECORDS_CURSOR_UUID );
  softble_characteristic_t       data = { .handles  = &(records->handle.cursor),
                                          .length   = sizeof(records_cursor_t),
                                          .limit    = sizeof(records_cursor_t),
                                          .value    = &(records->value.cursor) };

  return ( softble_characteristic_declare ( records->service, BLE_ATTR_PROTECTED | BLE_ATTR_WRITE | BLE_ATTR_READ, uuid, &(data) ) );

  }

//-----------------------------------------------------------------------------
//  function: records_data_characteristic ( records )
// arguments: records - service resource
//   returns: NRF_ERROR_INVALID_PARAM - if parameters are invalid or missing
//            NRF_SUCCESS - if added
//
// Register the record payload characteristic with the GATT service. This is a
// read-only buffer which will hold the record data.
//-----------------------------------------------------------------------------

static unsigned records_data_characteristic ( records_t * records ) {

  const void *                   uuid = records_id ( RECORDS_DATA_UUID );
  softble_characteristic_t       data = { .handles  = &(records->handle.record),
                                          .limit    = RECORD_DATA_LIMIT,
                                          .value    = &(records->value.record) };

  return ( softble_characteristic_declare ( records->service, BLE_ATTR_VARIABLE | BLE_ATTR_NOTIFY | BLE_ATTR_READ, uuid, &(data) ) );

  }
