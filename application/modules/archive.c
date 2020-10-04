//=============================================================================
// project: ShockVx
//  module: Stickershock Sensor for cold chain tracking.
//  author: Velvetwire, llc
//    file: archive.c
//
//
// (c) Copyright 2016-2020 Velvetwire, LLC. All rights reserved.
//=============================================================================

#include  <stickershock.h>

#include  "bluetooth.h"
#include  "archive.h"

//=============================================================================
// SECTION : 
//=============================================================================

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static CTL_TASK_t *            thread = NULL;
static archive_t             resource = { 0 };

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned archive_start ( const char * filename ) {

  archive_t *                 archive = &(resource);
  unsigned                     result = NRF_SUCCESS;

  // The module cannot have already been started.

  if ( thread == NULL ) { ctl_mutex_init ( &(archive->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );

  ctl_events_init ( &(archive->status), ARCHIVE_EVENT_SETTINGS );

  if ( (thread = ctl_spawn ( "archive", (CTL_TASK_ENTRY_t) archive_manager, archive, ARCHIVE_MANAGER_STACK, ARCHIVE_MANAGER_PRIORITY )) ) {
    
    strncpy ( archive->filename, filename, ARCHIVE_FILENAME_LIMIT );
    
    } else { result = NRF_ERROR_NO_MEM; }

  // Return with result.

  return ( result );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned archive_begin ( float interval ) {

  archive_t *                 archive = &(resource);
  CTL_TIME_t                   period = (CTL_TIME_t) roundf ( interval * 1000.0 );
  unsigned                     result = NRF_SUCCESS;
  
  // Make sure that the module has been started.

  if ( thread ) { ctl_mutex_lock_uc ( &(archive->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );

  // Start or cancel the periodic archive timer.

  if ( period ) {
    
    ctl_timer_start ( CTL_TIMER_CYCLICAL, &(archive->status), ARCHIVE_EVENT_PERIODIC, period );
    ctl_events_set ( &(archive->status), ARCHIVE_EVENT_PERIODIC );
    
    } else { ctl_timer_clear ( &(archive->status), ARCHIVE_EVENT_PERIODIC ); }

  // Free the resource and return with the result.

  return ( ctl_mutex_unlock ( &(archive->mutex) ), result );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned archive_cease ( void ) {

  archive_t *                 archive = &(resource);
  unsigned                     result = NRF_SUCCESS;
  
  // Make sure that the module has been started.

  if ( thread ) { ctl_mutex_lock_uc ( &(archive->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );

  // Clear the periodic timer and any pending event.

  ctl_events_clear ( &(archive->status), ARCHIVE_EVENT_PERIODIC );
  ctl_timer_clear ( &(archive->status), ARCHIVE_EVENT_PERIODIC );

  // Free the resource and return with the result.

  return ( ctl_mutex_unlock ( &(archive->mutex) ), result );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned archive_close ( void ) {

  archive_t *                 archive = &(resource);

  // If the module thread has not been started, there is nothing to close.

  if ( thread ) { ctl_events_set ( &(archive->status), ARCHIVE_EVENT_SHUTDOWN ); }
  else return ( NRF_ERROR_INVALID_STATE );

  // Wait for the the module thread to acknowlege shutdown.

  if ( ctl_events_wait ( CTL_EVENT_WAIT_ALL_EVENTS, &(archive->status), ARCHIVE_STATE_CLOSED, CTL_TIMEOUT_DELAY, ARCHIVE_CLOSE_TIMEOUT ) ) { thread = NULL; }
  else return ( NRF_ERROR_TIMEOUT );

  // Wipe the module resource data.

  memset ( archive, 0, sizeof(archive_t) );

  // Module successfully shut down.

  return ( NRF_SUCCESS );

  }

#if 0
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned archive_telemetry ( telemetry_record_t * record, unsigned status ) {

  archive_t *                 archive = &(resource);

  if ( thread ) { ctl_mutex_lock_uc ( &(archive->mutex) ); }
  else return ( NRF_ERROR_INVALID_STATE );
  
  memset ( &(archive->record), 0, sizeof(archive_record_t) );

  archive->record.timecode            = ctl_time_get ( );

  if ( status & TELEMETRY_VALID_PRESSURE ) { archive->record.data |= ARCHIVE_DATA_PRESSURE; }
  if ( status & TELEMETRY_VALID_HUMIDITY ) { archive->record.data |= ARCHIVE_DATA_HUMIDITY; }  
  if ( status & TELEMETRY_VALID_AMBIENT ) { archive->record.data |= ARCHIVE_DATA_AMBIENT; }
  if ( status & TELEMETRY_VALID_SURFACE ) { archive->record.data |= ARCHIVE_DATA_SURFACE; }

  if ( archive->record.data & ARCHIVE_DATA_PRESSURE ) { archive->record.pressure = (short) roundf( record->pressure * (float) 1e3 ); }
  if ( archive->record.data & ARCHIVE_DATA_HUMIDITY ) { archive->record.humidity = (short) roundf( record->humidity * (float) 1e4 ); }
  if ( archive->record.data & ARCHIVE_DATA_AMBIENT ) { archive->record.ambient = (short) roundf( record->ambient * (float) 1e2 ); }
  if ( archive->record.data & ARCHIVE_DATA_SURFACE ) { archive->record.surface = (short) roundf( record->surface * (float) 1e2 ); }

  ctl_events_set ( &(archive->status), ARCHIVE_STATE_RECORD );

  return ( ctl_mutex_unlock ( &(archive->mutex) ), NRF_SUCCESS );

  }
#endif


//=============================================================================
// SECTION : BEACON MANAGER THREAD
//=============================================================================

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void archive_manager ( archive_t * archive ) {

  // Process thread events until a shutdown request has been issued
  // from outside the thread.

  forever {

    CTL_EVENT_SET_t            events = ARCHIVE_MANAGER_EVENTS;
    CTL_EVENT_SET_t            status = ctl_events_wait_uc ( CTL_EVENT_WAIT_ANY_EVENTS_WITH_AUTO_CLEAR, &(archive->status), events );

    // If shutdown is requested, perform shutdown requests and break out of the loop.

    if ( status & ARCHIVE_EVENT_SHUTDOWN ) { archive_shutdown ( archive ); break; }
    if ( status & ARCHIVE_EVENT_SETTINGS ) { archive_settings ( archive ); }

    // Respond to the periodic event.

    if ( status & ARCHIVE_EVENT_PERIODIC ) { archive_periodic ( archive ); }

    }

  // Indicate that the manager thread is closed.

  ctl_events_init ( &(archive->status), ARCHIVE_STATE_CLOSED ); 

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void archive_shutdown ( archive_t * archive ) {

  // Request that the storage flush any pending writes and return to sleep.

  storage_sleep ( );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void archive_settings ( archive_t * archive ) {

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void archive_periodic ( archive_t * archive ) {
  
  unsigned                     result = NRF_ERROR_NULL;

  // If there is a pending record, open the archive file and append the record
  // to the tail of the file.

  if ( archive->status & ARCHIVE_STATE_RECORD ) {

    file_handle_t                file = file_open ( archive->filename, FILE_MODE_CREATE | FILE_MODE_WRITE | FILE_MODE_READ );
    int                          size = file_tail ( file );

    if ( file > FILE_OK ) {
      
      if ( sizeof(archive_record_t) == file_write ( file, &(archive->record), sizeof(archive_record_t) ) ) { result = NRF_SUCCESS; }
      else { result = NRF_ERROR_INTERNAL; }      
      
      file_close ( file );

      }

    ctl_events_clear ( &(archive->status), ARCHIVE_STATE_RECORD );

    }

  }
