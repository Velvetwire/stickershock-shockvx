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

#define   BEACON_BROADCAST_VARIANT    (0x5678)                                  // Default Variant "Vx"

//-----------------------------------------------------------------------------
// Beacon broadcast can be either BLE 4.x or BLE 5.x compliant.
//-----------------------------------------------------------------------------

typedef   enum {

          BEACON_TYPE_BLE_4,                                                    // Use BLE 4.x compliant beacon broadcast (31 byte data and 31 byte scan packets)
          BEACON_TYPE_BLE_5,                                                    // Use BLE 5.x compliant beacon broadcast (255 byte scan packet)

          } beacon_type_t;

          unsigned                    beacon_start ( unsigned short variant );
          unsigned                    beacon_state ( bool * active );
          unsigned                    beacon_close ( void );

//-----------------------------------------------------------------------------
// Beacon broadcast settings.
//
// Note: iOS recommends (20ms, 152.5ms, 211.25ms, 318.75ms, 417.5ms, 546.25ms,
//       760ms, 852.5ms, 1022.5ms, 1285ms)
//-----------------------------------------------------------------------------

#define   BEACON_BROADCAST_RATE       ((float) 1285e-3)                         // Broadcast at 1.285 second rate
#define   BEACON_BROADCAST_POWER      (0)                                       // Broadcast at 0 db
#define   BEACON_BROADCAST_PERIOD     ((float) 0)                               // Broadcast indefinitely

          unsigned                    beacon_begin ( float interval, float period, signed char power, beacon_type_t type );
          unsigned                    beacon_cease ( void );

//-----------------------------------------------------------------------------
// Beacon event notices
//-----------------------------------------------------------------------------

typedef   enum {                                                                // Service notices:
          BEACON_NOTICE_ADVERTISE,                                              //  Advertising started
          BEACON_NOTICE_TERMINATE,                                              //  Advertising stopped
          BEACON_NOTICE_INSPECTED,                                              //  Scan packet inspected
          BEACON_NOTICES
          } beacon_notice_t;

          unsigned                    beacon_notice ( beacon_notice_t notice, CTL_EVENT_SET_t * set, CTL_EVENT_SET_t events );

//-----------------------------------------------------------------------------
// Beacon device information
//-----------------------------------------------------------------------------

          unsigned                    beacon_network ( void * node );
          unsigned                    beacon_battery ( signed char battery );

//-----------------------------------------------------------------------------
// Beacon atmospheric telemetry
//-----------------------------------------------------------------------------

          unsigned                    beacon_ambient ( float measurement, unsigned incursion, unsigned excursion );
          unsigned                    beacon_humidity ( float measurement, unsigned incursion, unsigned excursion );
          unsigned                    beacon_pressure ( float measurement, unsigned incursion, unsigned excursion );

          unsigned                    beacon_temperature ( float measurement, unsigned incursion, unsigned excursion );

//-----------------------------------------------------------------------------
// Beacon orientation and handling
//-----------------------------------------------------------------------------

          unsigned                    beacon_orientation ( float angle, unsigned char orientation );
          unsigned                    beacon_misoriented ( void );

          unsigned                    beacon_dropped ( void );
          unsigned                    beacon_bumped ( void );
          unsigned                    beacon_tipped ( void );


//=============================================================================
// SECTION : BLUETOOTH PERIPHERAL
//=============================================================================

//-----------------------------------------------------------------------------
// Connectable bluetooth peripheral
//-----------------------------------------------------------------------------

          unsigned                    peripheral_start ( void );
          unsigned                    peripheral_state ( bool * active, bool * linked );
          unsigned                    peripheral_close ( void );

//-----------------------------------------------------------------------------
// Bluetooth peripheral settings
//
// Note: iOS recommends (20ms, 152.5ms, 211.25ms, 318.75ms, 417.5ms, 546.25ms,
//       760ms, 852.5ms, 1022.5ms, 1285ms)
//-----------------------------------------------------------------------------

#define   PERIPHERAL_BROADCAST_RATE   ((float) 152.5e-3)                        // Broadcast at 152.5 millisecond rate
#define   PERIPHERAL_BROADCAST_POWER  (0)                                       // Broadcast at 0 db
#define   PERIPHERAL_BROADCAST_PERIOD ((float) 20)                              // Broadcast for 20 seconds

          unsigned                    peripheral_begin ( float interval, float period, signed char power );
          unsigned                    peripheral_cease ( void );

//-----------------------------------------------------------------------------
// Peripheral event notices
//-----------------------------------------------------------------------------

typedef   enum {                                                                // Service notices:
          PERIPHERAL_NOTICE_ADVERTISE,                                          //  Advertising started
          PERIPHERAL_NOTICE_TERMINATE,                                          //  Advertising stopped
          PERIPHERAL_NOTICE_INSPECTED,                                          //  Scan packet inspected
          PERIPHERAL_NOTICE_ATTACHED,                                           //  Peer attached
          PERIPHERAL_NOTICE_DETACHED,                                           //  Peer detached
          PERIPHERAL_NOTICES
          } peripheral_notice_t;

          unsigned                    peripheral_notice ( peripheral_notice_t notice, CTL_EVENT_SET_t * set, CTL_EVENT_SET_t events );


//=============================================================================
// SECTION : BLUETOOTH GATT SERVICES
//=============================================================================

//-----------------------------------------------------------------------------
// Device control GATT service
//-----------------------------------------------------------------------------

          const void *                control_uuid ( void );
          unsigned                    control_register ( void * node, void * lock, void * create, void * accept );
          unsigned                    control_tracking ( void * node, void * lock, void * create, void * accept );

//-----------------------------------------------------------------------------
// Control tracking window and status
//-----------------------------------------------------------------------------

typedef   unsigned short              control_status_t;

          unsigned                    control_status ( control_status_t status, float memory, float storage );
          unsigned                    control_window ( unsigned opened, unsigned closed );

#define   CONTROL_STATUS_SURFACE      (1 << 0)                                  // Surface temperature sensor OK
#define   CONTROL_STATUS_AMBIENT      (1 << 1)                                  // Ambient temperature sensor OK
#define   CONTROL_STATUS_HUMIDITY     (1 << 2)                                  // Humidity sensor OK
#define   CONTROL_STATUS_PRESSURE     (1 << 3)                                  // Pressure sensor OK
#define   CONTROL_STATUS_MOVEMENT     (1 << 4)                                  // Movement sensor OK

//-----------------------------------------------------------------------------
// Sensor telemetry settings GATT service
//-----------------------------------------------------------------------------

#define   TELEMETRY_ARCHIVE_INTERVAL  ((float) 15.0 * 60.0)                     // Archive telemetry every 15 minutes
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
          unsigned                    surface_measured ( float value, float interval );

//-----------------------------------------------------------------------------
// Surface compliance data
//-----------------------------------------------------------------------------

typedef   float                       surface_compliance_t;                     // Seconds inside or outside of compliance

          unsigned                    surface_compliance ( surface_compliance_t * incursion, surface_compliance_t * excursion );
          unsigned                    surface_archive ( void );

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
          unsigned                    atmosphere_measured ( atmosphere_values_t * values, float interval );

//-----------------------------------------------------------------------------
// Atmospheric compliance data
//-----------------------------------------------------------------------------

typedef   struct {                                                              // Atmospheric compliance:

          float                       temperature;                              //  Seconds inside or outside of temperature compliance
          float                       humidity;                                 //  Seconds inside or outside of humidity compliance
          float                       pressure;                                 //  Seconds inside or outside of pressure compliance

          } atmosphere_compliance_t;

          unsigned                    atmosphere_compliance ( atmosphere_compliance_t * incursion, atmosphere_compliance_t * excursion );
          unsigned                    atmosphere_archive ( void );

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

//=============================================================================
#endif
