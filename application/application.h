//=============================================================================
// project: ShockVx
//  module: Stickershock firmware for cold chain tracking
//  author: Velvetwire, llc
//    file: application.h
//
// Application logic.
//
// (c) Copyright 2016-2020 Velvetwire, LLC. All rights reserved.
//=============================================================================

#ifndef   __APPLICATION__
#define   __APPLICATION__

#include  "settings.h"

//-----------------------------------------------------------------------------
// Declare the default name to use for the application.
//-----------------------------------------------------------------------------

#define   APPLICATION_NAME            "Shock Vx"
#define   APPLICATION_FILE            "ShockVx.set"
#define   APPLICATION_DATA            "Telemetry.rec"

//-----------------------------------------------------------------------------
// Platform defaults to assume during intialization.
//-----------------------------------------------------------------------------

#define   APPLICATION_PLATFORM        "Stickershock"
#define   APPLICATION_DEFAULTS        0

//-----------------------------------------------------------------------------
// Declare a telemetry compliance tracking structure.
//-----------------------------------------------------------------------------

typedef   struct {
          
          unsigned                    incursion;
          unsigned                    excursion;
          
          } application_compliance_t;

//-----------------------------------------------------------------------------
// Declare the application resource structure.
//-----------------------------------------------------------------------------

#define   APPLICATION_WATCH           3.0                                       // 3 second watchdog
#define   APPLICATION_STACK           768                                       // Application stack size in bytes

typedef   struct {

          CTL_EVENT_SET_t             status;                                   // Application status and event bits
          unsigned                    option;                                   // Application option bits

          struct {                                                              // Platform hardware description
            unsigned                  code;                                     //  platform 32-bit fingerprint code
            const char *              make;                                     //  platform make string
            const char *              model;                                    //  platform model string
            const char *              version;                                  //  platform version string
            unsigned                  revision;                                 //  platform revision index
            } hardware;

          struct {                                                              // Platform firmware description
            unsigned                  code;                                     //  firmware 32-bit fingerprint code
            unsigned char             major;                                    //  major version index
            unsigned char             minor;                                    //  minor version index
            unsigned short            build;                                    //  build number
            } firmware;

          struct {
            float                     percent;
            float                     voltage;
            } battery;

          application_settings_t      settings;

          struct {

            application_compliance_t  surface;                                  //  Surface temperature compliance
            application_compliance_t  ambient;                                  //  Ambient temperature compliance
            application_compliance_t  humidity;                                 //  Humidity compliance
            application_compliance_t  pressure;                                 //  Pressure compliance

            unsigned                  misorient;                                //  Misoriented according to preferences
            unsigned                  dropped;                                  //  Drops have been detected
            unsigned                  bumped;                                   //  Bumps have been detected
            unsigned                  tipped;                                   //  Tilt has been detected

            } compliance;

          } application_t;

          void                        main ( application_t * application );

//-----------------------------------------------------------------------------
// Application status and event bits
//-----------------------------------------------------------------------------

#define   APPLICATION_STATUS_EVENTS   (0xFFFFFF00)                              // Dedicate upper 24-bits to events
#define   APPLICATION_STATUS_STATES   (0x000000FF)                              // Reserve lower 8-bits for states

static    void                        application_raise ( application_t * application, unsigned state );
static    void                        application_lower ( application_t * application, unsigned state );

#define   APPLICATION_STATE_CHANGED   (1 << 0)                                  // Application settings have changed
#define   APPLICATION_STATE_PROBLEM   (1 << 1)                                  // Problem condition
#define   APPLICATION_STATE_BATTERY   (1 << 2)                                  // Battery is critical
#define   APPLICATION_STATE_CHARGED   (1 << 3)                                  // Charger is connected and charged
#define   APPLICATION_STATE_CHARGER   (1 << 4)                                  // Charger is connected and charging
#define   APPLICATION_STATE_BLINKER   (1 << 5)                                  // Identification blinker
#define   APPLICATION_STATE_CONNECT   (1 << 6)                                  // Connected to peer
#define   APPLICATION_STATE_NOMINAL   (1 << 7)                                  // Telemetry is nominal

//-----------------------------------------------------------------------------
// Application configuration options
//-----------------------------------------------------------------------------

#define   APPLICATION_OPTIONS_DEFAULT (APPLICATION_OPTION_NFC | APPLICATION_OPTION_BLE)

          CTL_EVENT_SET_t             application_configure ( application_t * application, unsigned options );
          unsigned                    application_bluetooth ( application_t * application );
          unsigned                    application_nearfield ( application_t * application );

#define   APPLICATION_OPTION_BLE      (1 << 31)
#define   APPLICATION_OPTION_NFC      (1 << 30)

//-----------------------------------------------------------------------------
// Application settings update event
//-----------------------------------------------------------------------------

#define   APPLICATION_EVENT_SETTINGS  (1 << 31)

          void                        application_settings ( application_t * application );

//-----------------------------------------------------------------------------
// Application shutdown and deep sleep
//-----------------------------------------------------------------------------

#define   APPLICATION_SHUTDOWN_DELAY  ((float) 2.5)
#define   APPLICATION_EVENT_SHUTDOWN  (1 << 30)

          void                        application_shutdown ( application_t * application );

//-----------------------------------------------------------------------------
// Scheduled events
//-----------------------------------------------------------------------------

#define   APPLICATION_EVENT_PERIODIC  (1 << 29)
#define   APPLICATION_EVENT_SCHEDULE  (1 << 28)
#define   APPLICATION_EVENT_TIMECODE  (1 << 27)

          void                        application_periodic ( application_t * application );
          void                        application_schedule ( application_t * application );
          void                        application_timecode ( application_t * application );

#define   APPLICATION_PERIOD          ((float) 60.0)

//-----------------------------------------------------------------------------
// Application indication modes
//-----------------------------------------------------------------------------

#define   APPLICATION_EVENT_INDICATE  (1 << 26)

typedef   enum {

          APPLICATION_INDICATE_NONE,                                            // Nothing to indicate (off)

          APPLICATION_INDICATE_PROBLEM,                                         // There is a problem condition
          APPLICATION_INDICATE_BATTERY,                                         // Battery is critically low
          APPLICATION_INDICATE_CHARGER,                                         // Charger is connected
          APPLICATION_INDICATE_CHARGED,                                         // Charger has charged battery
          APPLICATION_INDICATE_CONNECT,                                         // Connected to peer
          APPLICATION_INDICATE_BLINKER,                                         // Identity blinker

          } application_indicate_t;

          void                        application_indicate ( application_t * application );

//-----------------------------------------------------------------------------
// Battery level and charging status.
//-----------------------------------------------------------------------------

#define   APPLICATION_EVENT_CHARGER   (1 << 25)                                 // Charger connected or disconnected
#define   APPLICATION_EVENT_BATTERY   (1 << 24)                                 // Battery level has changed

          void                        application_charger ( application_t * application );
          void                        application_battery ( application_t * application );

#define   STARTING_BATTERY_THRESHOLD  ((float) 3.00)                            // Minimum battery power before startup
#define   INDICATE_BATTERY_THRESHOLD  ((float) 3.25)                            // Minimum battery to allow indication
#define   CRITICAL_BATTERY_THRESHOLD  ((float) 3.50)                            // Critical battery alert level

//-----------------------------------------------------------------------------
// Communication events.
//-----------------------------------------------------------------------------

#define   APPLICATION_EVENT_TAGGED    (1 << 23)                                 // Near field tag has been scanned
#define   APPLICATION_EVENT_ATTACH    (1 << 22)                                 // BLE peripheral has attached
#define   APPLICATION_EVENT_DETACH    (1 << 21)                                 // BLE connection has detached
#define   APPLICATION_EVENT_PROBED    (1 << 20)                                 // Scan response has been probed
#define   APPLICATION_EVENT_EXPIRE    (1 << 19)                                 // Advertisement expired

          void                        application_tagged ( application_t * application );
          void                        application_attach ( application_t * application );
          void                        application_detach ( application_t * application );
          void                        application_probed ( application_t * application );
          void                        application_expire ( application_t * application );

#define   APPLICATION_EVENT_STROBE    (1 << 18)
#define   APPLICATION_EVENT_CANCEL    (1 << 17)

          void                        application_strobe ( application_t * application );
          void                        application_cancel ( application_t * application );

//-----------------------------------------------------------------------------
// Periodic telemetry and handling updates
//-----------------------------------------------------------------------------

#define   APPLICATION_EVENT_TELEMETRY (1 << 16)
#define   APPLICATION_EVENT_HANDLING  (1 << 15)

          void                        application_telemetry ( application_t * application );
          void                        application_handling ( application_t * application );

//-----------------------------------------------------------------------------
// Movement related events
//-----------------------------------------------------------------------------

#define   APPLICATION_EVENT_ORIENTED  (1 << 14)
#define   APPLICATION_EVENT_STRESSED  (1 << 13)
#define   APPLICATION_EVENT_DROPPED   (1 << 12)
#define   APPLICATION_EVENT_TILTED    (1 << 11)

          void                        application_oriented ( application_t * application );
          void                        application_stressed ( application_t * application );
          void                        application_dropped ( application_t * application );
          void                        application_tilted ( application_t * application );

//=============================================================================
#endif
