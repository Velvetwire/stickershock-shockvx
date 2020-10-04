//=============================================================================
// project: ShockVx
//  module: Stickershock firmware for cold chain tracking.
//  author: Velvetwire, llc
//    file: broadcast.c
//
// Bluetooth broadcast packet encoding.
//
// (c) Copyright 2016-2020 Velvetwire, LLC. All rights reserved.
//=============================================================================

#include  <stickershock.h>

#include  "broadcast.h"

//=============================================================================
// SECTION : BROADCAST PACKET ASSEMBLY
//=============================================================================

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

broadcast_packet_t * broadcast_packet ( unsigned short code ) {
  
  broadcast_packet_t *         packet = malloc ( sizeof(broadcast_packet_t ) );
  
  if ( packet ) { memset ( packet->data, 0, BROADCAST_PACKET_SIZE ); packet->code = code; }

  return ( packet );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned char broadcast_length ( broadcast_packet_t * packet ) {

  unsigned char                length = 0;

  if ( packet ) for ( broadcast_record_t * record = (broadcast_record_t *) packet->data; record->size; record = (broadcast_record_t *) (packet->data + (++ length)) ) { length += record->size; }

  return ( length );

  }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void broadcast_append ( broadcast_packet_t * packet, void * data, unsigned char size, unsigned char type ) {
  
  unsigned char                length = broadcast_length ( packet );
  broadcast_record_t *         record = (broadcast_record_t *) (packet->data + length);

  if ( packet && ((length + size + sizeof(short)) <= BROADCAST_PACKET_SIZE) ) { memcpy ( record + 1, data, size ); }
  else return;

  record->size                        = size + 1;
  record->type                        = type;

  }