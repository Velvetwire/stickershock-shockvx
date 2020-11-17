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

//-----------------------------------------------------------------------------
// Declare the default name to use for the application and the path to the
// application settings file.
//-----------------------------------------------------------------------------

#define   APPLICATION_NAME            "ShockVx"                                 // Application name
#define   APPLICATION_FILE            "internal:settings/shockvx.set"           // Application settings file

//-----------------------------------------------------------------------------
// Platform defaults to assume during intialization.
//-----------------------------------------------------------------------------

#define   APPLICATION_PLATFORM        "Stickershock"
#define   APPLICATION_DEFAULTS        0

//-----------------------------------------------------------------------------
// Declare the application resource structure.
//-----------------------------------------------------------------------------

#define   APPLICATION_WATCH           ((float) 3.0)                             // 3 second watchdog
#define   APPLICATION_STACK           (768)                                     // Application stack size in bytes

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

          application_settings_t      settings;                                 // Persistent settings

          struct {

            unsigned                  misorient;                                //  Misoriented according to preferences
            unsigned                  dropped;                                  //  Drops have been detected
            unsigned                  bumped;                                   //  Bumps have been detected
            unsigned                  tipped;                                   //  Tilt has been detected

            } incident;

          } application_t;

          void                        main ( application_t * application );

//-----------------------------------------------------------------------------
// Application status bits
//-----------------------------------------------------------------------------

#define   APPLICATION_STATUS_EVENTS   (0x7FFFFFFF)
#define   APPLICATION_STATUS_STATES   (0x80000000)

//-----------------------------------------------------------------------------
// Persistent settings change
//-----------------------------------------------------------------------------

#define   APPLICATION_STATE_SETTINGS  (1 << 31)

          void                        application_settings ( application_t * application );
          unsigned                    application_defaults ( application_t * application, unsigned options );

//-----------------------------------------------------------------------------
// Application shutdown and deep sleep
//-----------------------------------------------------------------------------

#define   APPLICATION_SHUTDOWN_DELAY  ((float) 2.5)
#define   APPLICATION_STARTING_DELAY  ((float) 1.0)

#define   APPLICATION_EVENT_SHUTDOWN  (1 << 30)
#define   APPLICATION_EVENT_STARTING  (1 << 29)

          void                        application_shutdown ( application_t * application );
          void                        application_starting ( application_t * application );

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define   APPLICATION_EVENT_SCHEDULE  (1 << 28)                                 // UTC scheduled events
#define   APPLICATION_EVENT_PERIODIC  (1 << 27)                                 // Periodic timed events
#define   APPLICATION_EVENT_TIMECODE  (1 << 26)                                 // UTC timecode set

          void                        application_schedule ( application_t * application );
          void                        application_periodic ( application_t * application );
          void                        application_timecode ( application_t * application );

#define   APPLICATION_PERIOD          ((float) 90.0)                            // Periodic check-in every minute and one half

//-----------------------------------------------------------------------------
// Application configuration options
//-----------------------------------------------------------------------------

#define   APPLICATION_OPTIONS_DEFAULT (APPLICATION_OPTION_NFC | APPLICATION_OPTION_BLE)

          CTL_EVENT_SET_t             application_configure ( application_t * application, unsigned options );
          unsigned                    application_bluetooth ( application_t * application );
          unsigned                    application_nearfield ( application_t * application );

#define   APPLICATION_OPTION_BLE      (1 << 31)                                 // Bluetooth low energy radio
#define   APPLICATION_OPTION_NFC      (1 << 30)                                 // Near field radio

//-----------------------------------------------------------------------------
// Interactive events.
//-----------------------------------------------------------------------------

#define   APPLICATION_TAG_DELAY       (256)                                     // Short 256ms delay after NFC tagged
#define   APPLICATION_EVENT_TAGGED    (1 << 24)                                 // Near field tag has been scanned

          void                        application_tagged ( application_t * application );

//-----------------------------------------------------------------------------
// Bluetooth communication events.
//-----------------------------------------------------------------------------

          void                        application_advertise ( application_t * application );

#define   APPLICATION_EVENT_ATTACH    (1 << 23)                                 // BLE peripheral has attached
#define   APPLICATION_EVENT_DETACH    (1 << 22)                                 // BLE connection has detached
#define   APPLICATION_EVENT_PROBED    (1 << 21)                                 // Scan response has been probed
#define   APPLICATION_EVENT_EXPIRE    (1 << 20)                                 // Advertisement expired

          void                        application_attach ( application_t * application );
          void                        application_detach ( application_t * application );
          void                        application_probed ( application_t * application );
          void                        application_expire ( application_t * application );

//-----------------------------------------------------------------------------
// Periodic telemetry updates
//-----------------------------------------------------------------------------

#define   APPLICATION_EVENT_TELEMETRY (1 << 17)
#define   APPLICATION_EVENT_ARCHIVE   (1 << 16)

          void                        application_telemetry ( application_t * application );
          void                        application_archive ( application_t * application );

//-----------------------------------------------------------------------------
// Movement related events
//-----------------------------------------------------------------------------

#define   APPLICATION_EVENT_HANDLING  (1 << 15)
#define   APPLICATION_EVENT_ORIENTED  (1 << 14)

          void                        application_handling ( application_t * application );
          void                        application_oriented ( application_t * application );

//-----------------------------------------------------------------------------
// Incident handling
//-----------------------------------------------------------------------------

#define   APPLICATION_EVENT_STRESSED  (1 << 13)
#define   APPLICATION_EVENT_DROPPED   (1 << 12)
#define   APPLICATION_EVENT_TILTED    (1 << 11)

          void                        application_stressed ( application_t * application );
          void                        application_dropped ( application_t * application );
          void                        application_tilted ( application_t * application );

//=============================================================================
#endif
