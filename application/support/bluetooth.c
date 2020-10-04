//=============================================================================
// project: ShockVx
//  module: Stickershock firmware for cold chain tracking.
//  author: Velvetwire, llc
//    file: bluetooth.c
//
// Bluetooth broadcast packet construction.
//
// (c) Copyright 2016-2020 Velvetwire, LLC. All rights reserved.
//=============================================================================

#include  <stickershock.h>

#include  "bluetooth.h"

//=============================================================================
// SECTION : APPLICATION CONFIGURATION
//=============================================================================

//-----------------------------------------------------------------------------
// note: the system does not use a low frequency clock
//-----------------------------------------------------------------------------

unsigned bluetooth_start ( const char * label ) {

  // Configure the BLE device settings and limits.

  unsigned                     result = softdevice_reserve ( NULL, NULL );
  const softble_settings_t   settings = { .limits = { .servers  = BLUETOOTH_SERVER_LIMIT,
                                                      .clients  = BLUETOOTH_CLIENT_LIMIT,
                                                      .notices  = BLUETOOTH_QUEUE_SIZE,
                                                      .uuids    = BLUETOOTH_VSID_COUNT,
                                                      .mtu      = BLUETOOTH_MTU_LENGTH },
                                          .event  = BLUETOOTH_EVENT_LENGTH,
                                          .space  = BLUETOOTH_TABLE_SIZE };

  // Request the BLE device and establish the default communication parameters.

  if ( NRF_SUCCESS == result ) { result = softble_request ( label, &(settings) ); }
  if ( NRF_SUCCESS == result ) { result = softble_parameters ( BLUETOOTH_MINIMUM_INTERVAL, BLUETOOTH_MAXIMUM_INTERVAL, BLUETOOTH_INTERVAL_TIMEOUT, BLUETOOTH_INTERVAL_LATENCY ); }

  return ( result );

  }
