#ifndef DATAPATH_DB_H
#define DATAPATH_DB_H

#include "trema.h"


hash_table *create_datapath_db( void );
void insert_datapath_entry( hash_table *db, uint64_t datapath_id, void *user_data, void delete_user_data( void *user_data ) );
void *lookup_datapath_entry( hash_table *db, uint64_t datapath_id );
void delete_datapath_entry( hash_table *db, uint64_t datapath_id );
void delete_datapath_db( hash_table *db );
void foreach_datapath_db( hash_table *db, void function( void *user_data ) );

#endif // DATAPATH_DB_H

