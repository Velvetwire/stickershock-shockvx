//=============================================================================
// project: ShockVx
//  module: Stickershock firmware for cold chain tracking.
//  author: Velvetwire, llc
//    file: bluetooth.h
//
// Bluetooth broadcast packet construction.
//
// (c) Copyright 2016-2020 Velvetwire, LLC. All rights reserved.
//=============================================================================

#ifndef   __BLUETOOTH__
#define   __BLUETOOTH__

//=============================================================================
// SECTION : BLUETOOTH LOW ENERGY CONFIGURATION
//=============================================================================

//-----------------------------------------------------------------------------
// BLE transmission settings.
//-----------------------------------------------------------------------------

#define   BLUETOOTH_EVENT_LENGTH      (float) 3.75e-3                           // Use the standard BLE event length (3.75ms)
#define   BLUETOOTH_MTU_LENGTH        (255 + 3)                                 // Extended MTU length in bytes (255 + 3)

//-----------------------------------------------------------------------------
// BLE stack configuration.
//-----------------------------------------------------------------------------

#define   BLUETOOTH_SERVER_LIMIT      1                                         // One peripheral server required
#define   BLUETOOTH_CLIENT_LIMIT      0                                         // No central clients required

#define   BLUETOOTH_QUEUE_SIZE        BLE_GATTS_HVN_TX_QUEUE_SIZE_DEFAULT       // Use the default queue size
#define   BLUETOOTH_TABLE_SIZE        (0x720)                                   // Use a larger table size to accomodate characteristics
#define   BLUETOOTH_VSID_COUNT        BLE_UUID_VS_COUNT_DEFAULT                 // Use the default vendor specific ID space count

//-----------------------------------------------------------------------------
// BLE connection preferences
//-----------------------------------------------------------------------------

#define   BLUETOOTH_MINIMUM_INTERVAL  (float) 50e-3                             // Minimum packet interval (should be >= 20ms)
#define   BLUETOOTH_MAXIMUM_INTERVAL  (float) 400e-3                            // Maximum packet interval (should be <= 400ms)
#define   BLUETOOTH_INTERVAL_TIMEOUT  (float) 6.0                               // Timeout period (should be <= 6s)
#define   BLUETOOTH_INTERVAL_LATENCY  4                                         // Event latency (# allowed to skip)

//-----------------------------------------------------------------------------
// Bluetooth low energy device.
//-----------------------------------------------------------------------------

          unsigned                    bluetooth_start ( const char * label );


//=============================================================================
// SECTION : BLUETOOTH BEACON
//=============================================================================

//-----------------------------------------------------------------------------
// Bluetooth beacon service
//
// Note: iOS recommends (20ms, 152.5ms, 211.25ms, 318.75ms, 417.5ms, 546.25ms,
//       760ms, 852.5ms, 1022.5ms, 1285ms)
//-----------------------------------------------------------------------------

#define   BEACON_BROADCAST_RATE       ((float) 1285e-3)                         // Broadcast at 1285ms rate
#define   BEACON_BROADCAST_POWER      (0)                                       // Broadcast at 0 db
#define   BEACON_BROADCAST_PERIOD     ((float) 0)                               // Broadcast indefinitely
#define   BEACON_BROADCAST_PROFILE    (0x5678)                                  // Broadcast profile code

          unsigned                    beacon_start ( unsigned short variant );
          unsigned                    beacon_state ( bool * active );
          unsigned                    beacon_begin ( float interval, float period, signed char power );
          unsigned                    beacon_cease ( void );
          unsigned                    beacon_close ( void );

typedef   enum {                                                                // Service notices:
          BEACON_NOTICE_ADVERTISE,                                              //  Advertising started
          BEACON_NOTICE_TERMINATE,                                              //  Advertising stopped
          BEACON_NOTICE_INSPECTED,                                              //  Scan packet inspected
          BEACON_NOTICES
          } beacon_notice_t;

          unsigned                    beacon_notice ( beacon_notice_t notice, CTL_EVENT_SET_t * set, CTL_EVENT_SET_t events );

          unsigned                    beacon_network ( void * node );
          unsigned                    beacon_battery ( signed char battery );

          unsigned                    beacon_surface ( float measurement, unsigned incursion, unsigned excursion );
          unsigned                    beacon_ambient ( float measurement, unsigned incursion, unsigned excursion );
          unsigned                    beacon_humidity ( float measurement, unsigned incursion, unsigned excursion );
          unsigned                    beacon_pressure ( float measurement, unsigned incursion, unsigned excursion );

          unsigned                    beacon_orientation ( float angle, unsigned char orientation );
          unsigned                    beacon_misoriented ( void );

          unsigned                    beacon_dropped ( void );
          unsigned                    beacon_bumped ( void );
          unsigned                    beacon_tipped ( void );


//=============================================================================
// SECTION : BLUETOOTH PERIPHERAL
//=============================================================================

//-----------------------------------------------------------------------------
// Bluetooth peripheral service
//
// Note: iOS recommends (20ms, 152.5ms, 211.25ms, 318.75ms, 417.5ms, 546.25ms,
//       760ms, 852.5ms, 1022.5ms, 1285ms)
//-----------------------------------------------------------------------------

#define   PERIPHERAL_BROADCAST_RATE   (float) 152.5e-3                          // Broadcast at 152.5ms rate
#define   PERIPHERAL_BROADCAST_POWER  (0)                                       // Broadcast at 0 db
#define   PERIPHERAL_BROADCAST_PERIOD (float) 60                                // Broadcast for one minute

          unsigned                    peripheral_start ( void );
          unsigned                    peripheral_state ( bool * active, bool * linked );
          unsigned                    peripheral_begin ( float interval, float period, signed char power );
          unsigned                    peripheral_cease ( void );
          unsigned                    peripheral_close ( void );

typedef   enum {                                                                // Service notices:
          PERIPHERAL_NOTICE_ADVERTISE,                                          //  Advertising started
          PERIPHERAL_NOTICE_TERMINATE,                                          //  Advertising stopped
          PERIPHERAL_NOTICE_INSPECTED,                                          //  Scan packet inspected
          PERIPHERAL_NOTICE_ATTACHED,                                           //  Peer attached
          PERIPHERAL_NOTICE_DETACHED,                                           //  Peer detached
          PERIPHERAL_NOTICES
          } peripheral_notice_t;

          unsigned                    peripheral_notice ( peripheral_notice_t notice, CTL_EVENT_SET_t * set, CTL_EVENT_SET_t events );

//-----------------------------------------------------------------------------
// Device control GATT service
//-----------------------------------------------------------------------------

          const void *                control_uuid ( void );
          unsigned                    control_register ( void * node, void * lock, void * create, void * accept );

typedef   enum {                                                                // Service notices:
          CONTROL_NOTICE_IDENTIFY,                                              //  request to identify
          CONTROL_NOTICES
          } control_notice_t;

          unsigned                    control_notice ( control_notice_t notice, CTL_EVENT_SET_t * set, CTL_EVENT_SET_t events );

          unsigned                    control_window ( unsigned opened, unsigned closed );
          unsigned                    control_tracking ( void * node, void * lock, void * create, void * accept );
          unsigned                    control_identify ( unsigned * duration );

//-----------------------------------------------------------------------------
// Sensor telemetry settings GATT service
//-----------------------------------------------------------------------------

#define   TELEMETRY_DEFAULT_INTERVAL  ((float) 15.0)                            // Collect telemetry every 15 seconds
#define   TELEMETRY_SERVICE_INTERVAL  ((float) 2.5)                             // Every 2.5 seconds when connected


          const void *                telemetry_uuid ( void );
          unsigned                    telemetry_register ( float interval, float archival );
          unsigned                    telemetry_settings ( float * interval, float * archival );

//-----------------------------------------------------------------------------
// Surface temperature telemetry GATT service
//-----------------------------------------------------------------------------

          unsigned                    surface_register ( float lower, float upper );
          unsigned                    surface_settings ( float * lower, float * upper );
          unsigned                    surface_measured ( float value );

//-----------------------------------------------------------------------------
// Atmospheric telemetry GATT service
//-----------------------------------------------------------------------------

typedef   struct {

          float                       temperature;                              // Air temperature (deg C)
          float                       humidity;                                 // Humidity (saturation)
          float                       pressure;                                 // Air pressure (bars)

          } atmosphere_values_t;

          unsigned                    atmosphere_register ( atmosphere_values_t * lower, atmosphere_values_t * upper );
          unsigned                    atmosphere_settings ( atmosphere_values_t * lower, atmosphere_values_t * upper );
          unsigned                    atmosphere_measured ( atmosphere_values_t * values );

//-----------------------------------------------------------------------------
// Orientation and handling GATT service
//-----------------------------------------------------------------------------

typedef   struct {

          float                       force;                                    // Force (in gravs)
          float                       angle;                                    // Angle (in degrees)
          
          unsigned char               face;                                     // Orientation code (0 = unknown, don't care)

          } handling_values_t;

          const void *                handling_uuid ( void );
          unsigned                    handling_register ( handling_values_t * limits );
          unsigned                    handling_settings ( handling_values_t * limits );
          unsigned                    handling_observed ( handling_values_t * values );

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define   RECORD_DATA_LIMIT           (255)                                     // Maximum record data size

          const void *                records_uuid ( void );
          unsigned                    records_register ( float interval );
          unsigned                    records_settings ( float * interval );

typedef   enum {                                                                // Service notices:
          RECORDS_NOTICE_REQUEST,                                               //  record data requested
          RECORDS_NOTICES
          } records_notice_t;

          unsigned                    records_notice ( records_notice_t notice, CTL_EVENT_SET_t * set, CTL_EVENT_SET_t events );

typedef   struct __attribute__ (( packed )) {                                   // Records database cursor:

          unsigned short              index;                                    //  record index
          unsigned short              count;                                    //  record count

          } records_cursor_t;

//=============================================================================
#endif
