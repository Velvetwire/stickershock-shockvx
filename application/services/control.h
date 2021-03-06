//=============================================================================
// project: ShockVx
//  module: Stickershock firmware for cold chain tracking.
//  author: Velvetwire, llc
//    file: control.h
//
// Device control GATT service.
//
// (c) Copyright 2016-2020 Velvetwire, LLC. All rights reserved.
//=============================================================================

#ifndef   __CONTROL__
#define   __CONTROL__

//-----------------------------------------------------------------------------
// Simple GATT service
//-----------------------------------------------------------------------------

#define   CONTROL_SERVICES_UUID       (0x56780000)                              // 32-bit service UUID component (Vx--)

static    const void *                control_id ( unsigned service );

//-----------------------------------------------------------------------------
// A tracking window consists of an open and close time, expressed in UTC.
//-----------------------------------------------------------------------------

typedef   struct __attribute__ (( packed )) {                                   // Tracking window:

          unsigned                    opened;                                   //  UTC time opened
          unsigned                    closed;                                   //  UTC time closed

          } control_window_t;

//-----------------------------------------------------------------------------
// Summarize memory conditions and operational status.
//-----------------------------------------------------------------------------

typedef   struct __attribute__ (( packed )) {

          control_status_t            status;                                   //  Status flags
          unsigned char               memory;                                   //  Operating memory available (0 - 100) percent
          unsigned char               storage;                                  //  Available storage memory (0 - 100) percent

          } control_summary_t;

//-----------------------------------------------------------------------------
// Service resource
//-----------------------------------------------------------------------------

typedef   struct {

          CTL_MUTEX_t                 mutex;                                    // Access mutex
          unsigned short              service;                                  // Service handle

          struct {                                                              // Characteristic handles:

            ble_gatts_char_handles_t  node;                                     //  Tracking node characteristic
            ble_gatts_char_handles_t  lock;                                     //  Tracking lock characteristic

            ble_gatts_char_handles_t  opened;                                   //  Tracking opened characteristic
            ble_gatts_char_handles_t  closed;                                   //  Tracking closed characteristic
            ble_gatts_char_handles_t  window;                                   //  Tracking time window

            ble_gatts_char_handles_t  summary;                                  //  Summary characteristic

            } handle;

          struct {                                                              // Characteristic values:

            hash_t                    node;                                     //  Identity (64-bit network node)
            unsigned char             lock [ SOFTDEVICE_KEY_LENGTH ];

            unsigned char             opened [ SOFTDEVICE_KEY_LENGTH ];         // UUID used to open the window
            unsigned char             closed [ SOFTDEVICE_KEY_LENGTH ];         // UUID used to close the window
            control_window_t          window;                                   // Tracking window

            control_summary_t         summary;                                  // Summary status

            } value;

          } control_t;

static    unsigned                    control_event ( control_t * control, ble_evt_t * event );
static    unsigned                    control_write ( control_t * control, unsigned short connection, ble_gatts_evt_write_t * write );

//-----------------------------------------------------------------------------
// The node and lock can be used to secure the tracking beacon.
//-----------------------------------------------------------------------------

#define   CONTROL_NODE_UUID           (0x5678546e)                              // 32-bit characteristic UUID component (VxTn)
#define   CONTROL_LOCK_UUID           (0x5678546c)                              // 32-bit characteristic UUID component (VxTl)

static    unsigned                    control_node_characteristic ( control_t * control, void * node );
static    unsigned                    control_lock_characteristic ( control_t * control, void * lock );

//-----------------------------------------------------------------------------
// The opened and closed signatures are used to open and close the tracking
// window. They can only be written once.
//-----------------------------------------------------------------------------

#define   CONTROL_OPENED_UUID         (0x5678546f)                              // 32-bit characteristic UUID component (VxTo)
#define   CONTROL_CLOSED_UUID         (0x56785463)                              // 32-bit characteristic UUID component (VxTc)
#define   CONTROL_WINDOW_UUID         (0x56785477)                              // 32-bit characteristic UUID component (VxTw)

static    unsigned                    control_opened_characteristic ( control_t * control, void * opened );
static    unsigned                    control_closed_characteristic ( control_t * control, void * closed );
static    unsigned                    control_window_characteristic ( control_t * control );

//-----------------------------------------------------------------------------
// The summary characteristic is a read-only value used to report basic status.
//-----------------------------------------------------------------------------

#define   CONTROL_SUMMARY_UUID        (0x56784975)                              // 32-bit characteristic UUID component (VxSu)

static    unsigned                    control_summary_characteristic ( control_t * control );

//=============================================================================
#endif

