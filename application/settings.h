//=============================================================================
// project: ShockVx
//  module: Stickershock firmware for cold chain tracking
//  author: Velvetwire, llc
//    file: settings.h
//
// Application settings.
//
// (c) Copyright 2016-2020 Velvetwire, LLC. All rights reserved.
//=============================================================================

#ifndef   __SETTINGS__
#define   __SETTINGS__

#include  "bluetooth.h"

//=============================================================================
// SECTION : PERSISTENT APPLICATION SETTINGS
//=============================================================================

//-----------------------------------------------------------------------------
// Declare the application settings file structure.
//-----------------------------------------------------------------------------

typedef   struct {

          // Device broadcast label (currently unused)

          char                        label [ SOFTBLE_LABEL_LIMIT ];            // BLE device label

          // The device can belong to a network node with a 64-bit and an
          // optional 128-bit security key. This key will be used to hash
          // the beacon information for secure verification. The time and
          // 128-bit UUID of the creator and acceptor of the transaction
          // are also recorded.

          struct {                                                              // Tracking settings:

            unsigned char             lock [ SOFTDEVICE_KEY_LENGTH ];           //  Security lock (128-bit)
            hash_t                    node;                                     //  Tracking node (64-bit)

            struct {                                                            //  Tracking window time:
              unsigned                opened;                                   //   UTC epoch time when tracking opened
              unsigned                closed;                                   //   UTC epoch time when tracking closed
              } time;

            struct {                                                            //  Tracking signatures:
              unsigned char           opened [ SOFTDEVICE_KEY_LENGTH ];         //   create signature (opened)
              unsigned char           closed [ SOFTDEVICE_KEY_LENGTH ];         //   accept signature (closed)
              } signature;

            } tracking;

          // The telemetry settings control how often the device measures
          // telemetry and the upper and lower limits that trigger alarms.

          struct  {                                                             // Telemetry settings:

            float                     interval;                                 //  Telemetry measurement interval (0 = off)
            float                     archival;                                 //  Telemetry archive interval (0 = off)

            } telemetry;
 
          // The surface settings control the surface measurements that
          // are permitted.

          struct {
            
            float                     lower;                                    //  Lower surface limit
            float                     upper;                                    //  Upper surface limit

            } surface;

          // The atmospheric settings control the telemetry limits that
          // are permitted.
          
          struct {                                                              // Atmospheric settings:

            atmosphere_values_t       lower;                                    //  Lower telemetry limits
            atmosphere_values_t       upper;                                    //  Upper telemetry limits

            } atmosphere;

          // The handling settings control how the device responds to
          // g-forces and tilt angles.

          struct {                                                              // Handling settings:

            handling_values_t         limit;                                    //  Handling limits

            } handling;

          } application_settings_t;

//=============================================================================
#endif
