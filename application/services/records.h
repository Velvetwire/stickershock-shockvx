//=============================================================================
// project: ShockVx
//  module: Stickershock Sensor for cold chain tracking.
//  author: Velvetwire, llc
//    file: records.h
//
// Telemetry records service.
//
// (c) Copyright 2016-2020 Velvetwire, LLC. All rights reserved.
//=============================================================================

#ifndef   __RECORDS__
#define   __RECORDS__

//-----------------------------------------------------------------------------
// Surface GATT service
//-----------------------------------------------------------------------------

#define   RECORDS_SERVICES_UUID       (0x54720000)                              // 32-bit service UUID component (Tr00)

static    const void *                records_id ( unsigned service );

//-----------------------------------------------------------------------------
// Service resource
//-----------------------------------------------------------------------------

typedef   struct {
          
          CTL_MUTEX_t                 mutex;                                    // Access mutex
          unsigned short              service;                                  // Service handle

          CTL_NOTICE_t                notice [ RECORDS_NOTICES ];               // Service notices

          struct {                                                              // Characteristic handles:

            ble_gatts_char_handles_t  interval;                                 //  Record interval characteristic

            ble_gatts_char_handles_t  cursor;                                   //  Record cursor characteristic
            ble_gatts_char_handles_t  record;                                   //  Record payload characteristic

            } handle;

          struct {                                                              // Characteristic values:

            float                     interval;                                 //  Record interval (seconds)

            records_cursor_t          cursor;
            unsigned char *           record [ RECORD_DATA_LIMIT + 1 ];         //  Record data (data bytes)

            } value;

          } records_t;

static    unsigned                    records_event ( records_t * records, ble_evt_t * event );
static    unsigned                    records_write ( records_t * records, unsigned short connection, ble_gatts_evt_write_t * write );

//-----------------------------------------------------------------------------
// Recording interval characteristic
//-----------------------------------------------------------------------------

#define   RECORDS_INTERVAL_UUID       (0x54725269)                              // 32-bit characteristic UUID component (TrRi)

static    unsigned                    records_interval_characteristic ( records_t * records );

//-----------------------------------------------------------------------------
// Record cursor characteristic
//-----------------------------------------------------------------------------

#define   RECORDS_CURSOR_UUID         (0x54725263)                              // 32-bit characteristic UUID component (TrRc)

static    unsigned                    records_cursor_characteristic ( records_t * records );

//-----------------------------------------------------------------------------
// Record data characteristic
//-----------------------------------------------------------------------------

#define   RECORDS_DATA_UUID           (0x54725264)                              // 32-bit characteristic UUID component (TrRd)

static    unsigned                    records_data_characteristic ( records_t * records );

//=============================================================================
#endif

