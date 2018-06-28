/*
 * Simple server load balancer application (NOT FULLY IMPLEMENTED YET)
 */

#include <assert.h>
#include <time.h>
#include "trema.h"


typedef struct {
  struct key {
    uint8_t mac[ OFP_ETH_ALEN ];
    uint64_t datapath_id;
  } key;
  uint16_t port_no;
  time_t last_update;
} forwarding_entry;


time_t
now() {
  return time( NULL );
}


/********************************************************************************
 * packet_in event handler
 ********************************************************************************/

static const int MAX_AGE = 300;


static bool
aged_out( forwarding_entry *entry ) {
  if ( entry->last_update + MAX_AGE < now() ) {
    return true;
  }
  else {
    return false;
  };
}


static void
age_forwarding_db( void *key, void *forwarding_entry, void *forwarding_db ) {
  if ( aged_out( forwarding_entry ) ) {
    delete_hash_entry( forwarding_db, key );
    xfree( forwarding_entry );
  }
}


static void
update_forwarding_db( void *forwarding_db ) {
  foreach_hash( forwarding_db, age_forwarding_db, forwarding_db );
}


static void
learn( hash_table *forwarding_db, struct key new_key, uint16_t port_no ) {
  forwarding_entry *entry = lookup_hash_entry( forwarding_db, &new_key );

  if ( entry == NULL ) {
    entry = xmalloc( sizeof( forwarding_entry ) );
    memcpy( entry->key.mac, new_key.mac, OFP_ETH_ALEN );
    entry->key.datapath_id = new_key.datapath_id;
    insert_hash_entry( forwarding_db, &entry->key, entry );
  }
  entry->port_no = port_no;
  entry->last_update = now();
}


static void
do_flooding( uint64_t datapath_id, uint32_t buffer_id, uint16_t in_port, const buffer *data ) {
  openflow_actions *actions = create_actions();
  append_action_output( actions, OFPP_FLOOD, UINT16_MAX );

  buffer *packet_out;
  if ( buffer_id == UINT32_MAX ) {
    buffer *frame = duplicate_buffer( data );
    fill_ether_padding( frame );
    packet_out = create_packet_out(
      get_transaction_id(),
      buffer_id,
      in_port,
      actions,
      frame
    );
    free_buffer( frame );
  }
  else {
    packet_out = create_packet_out(
      get_transaction_id(),
      buffer_id,
      in_port,
      actions,
      NULL
    );
  }
  send_openflow_message( datapath_id, packet_out );
  free_buffer( packet_out );
  delete_actions( actions );
}


static void
send_packet( uint16_t destination_port, uint64_t datapath_id, uint32_t buffer_id, uint16_t in_port, const buffer *data ) {
  openflow_actions *actions = create_actions();
  append_action_output( actions, destination_port, UINT16_MAX );

  struct ofp_match match;
  set_match_from_packet( &match, in_port, 0, data );

  buffer *flow_mod = create_flow_mod(
    get_transaction_id(),
    match,
    get_cookie(),
    OFPFC_ADD,
    60,
    0,
    UINT16_MAX,
    buffer_id,
    OFPP_NONE,
    OFPFF_SEND_FLOW_REM,
    actions
  );
  send_openflow_message( datapath_id, flow_mod );
  free_buffer( flow_mod );

  if ( buffer_id == UINT32_MAX ) {
    buffer *frame = duplicate_buffer( data );
    fill_ether_padding( frame );
    buffer *packet_out = create_packet_out(
      get_transaction_id(),
      buffer_id,
      in_port,
      actions,
      frame
    );
    send_openflow_message( datapath_id, packet_out );
    free_buffer( packet_out );
    free_buffer( frame );
  }

  delete_actions( actions );
}


static uint16_t output_port = 2;

static void
handle_packet_in( uint64_t datapath_id, uint32_t transaction_id,
                  uint32_t buffer_id, uint16_t total_len,
                  uint16_t in_port, uint8_t reason, const buffer *data,
                  void *user_data ) {
  UNUSED( transaction_id );
  UNUSED( total_len );
  UNUSED( reason );

  packet_info packet_info = get_packet_info( data );
  if ( packet_type_ipv4( data ) &&
       packet_info.ipv4_daddr == 0x01010101 ) {

    struct ofp_match match;
    set_match_from_packet( &match, in_port, 0, data );

    openflow_actions *actions = create_actions();
    append_action_output( actions, output_port, UINT16_MAX );

    buffer *flow_mod = create_flow_mod(
      get_transaction_id(),
      match,
      get_cookie(),
      OFPFC_ADD,
      60,
      0,
      UINT16_MAX,
      buffer_id,
      OFPP_NONE,
      OFPFF_SEND_FLOW_REM,
      actions
    );

    send_openflow_message( datapath_id, flow_mod );
    free_buffer( flow_mod );
    delete_actions( actions );

    struct ofp_match match2 = match;
    match2.in_port = output_port;
    memcpy( match2.dl_src, match.dl_dst, ETH_ADDRLEN );
    memcpy( match2.dl_dst, match.dl_src, ETH_ADDRLEN );
    match2.nw_src = match.nw_dst;
    match2.nw_dst = match.nw_src;
    match2.tp_src = match.tp_dst;
    match2.tp_dst = match.tp_src;

    actions = create_actions();
    append_action_output( actions, in_port, UINT16_MAX );

    flow_mod = create_flow_mod(
      get_transaction_id(),
      match2,
      get_cookie(),
      OFPFC_ADD,
      60,
      0,
      UINT16_MAX,
      UINT32_MAX,
      OFPP_NONE,
      OFPFF_SEND_FLOW_REM,
      actions
    );

    send_openflow_message( datapath_id, flow_mod );
    free_buffer( flow_mod );
    delete_actions( actions );

    if ( output_port == 2 ) {
      output_port = 3;
    }
    else {
      output_port = 2;
    }

    return;
  }

  struct key new_key;
  memcpy( new_key.mac, packet_info.eth_macsa, OFP_ETH_ALEN );
  new_key.datapath_id = datapath_id;
  hash_table *forwarding_db = user_data;
  learn( forwarding_db, new_key, in_port );

  struct key search_key;
  memcpy( search_key.mac, packet_info.eth_macda, OFP_ETH_ALEN );
  search_key.datapath_id = datapath_id;
  forwarding_entry *destination = lookup_hash_entry( forwarding_db, &search_key );

  if ( destination == NULL ) {
    do_flooding( datapath_id, buffer_id, in_port, data );
  }
  else {
    send_packet( destination->port_no, datapath_id, buffer_id, in_port, data );
  }
}


/********************************************************************************
 * Start learning_switch controller.
 ********************************************************************************/

static const int AGING_INTERVAL = 5;


unsigned int
hash_forwarding_entry( const void *key ) {
  return hash_mac( ( ( const struct key * ) key )->mac );
}


bool
compare_forwarding_entry( const void *x, const void *y ) {
  const forwarding_entry *ex = x;
  const forwarding_entry *ey = y;
  return ( memcmp( ex->key.mac, ey->key.mac, OFP_ETH_ALEN ) == 0 )
           && ( ex->key.datapath_id == ey->key.datapath_id );
}


int
main( int argc, char *argv[] ) {
  init_trema( &argc, &argv );

  hash_table *forwarding_db = create_hash( compare_forwarding_entry, hash_forwarding_entry );
  add_periodic_event_callback( AGING_INTERVAL, update_forwarding_db, forwarding_db );
  set_packet_in_handler( handle_packet_in, forwarding_db );

  start_trema();

  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
