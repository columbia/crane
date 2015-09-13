/*
 * =====================================================================================
 *
 *       Filename:  replica.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/05/2014 12:53:17 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Milannic (), milannic.cheng.liu@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef REPLICA_H
#define REPLICA_H

#include <sys/types.h>
#include <stdint.h>
#include "../db/db-interface.h"

// expose records and its key type to the user
typedef struct request_record_t{
    struct timeval created_time; // data created timestamp
    char is_closed;
    uint64_t bit_map; // now we assume the maximal replica group size is 64;
    size_t data_size; // data size
    char data[0];     // real data
}__attribute__((packed))request_record;
#define REQ_RECORD_SIZE(M) (sizeof(request_record)+(M->data_size))
typedef uint64_t db_key_type;

struct node_t;

struct node_t* system_initialize(int64_t node_id,const char* start_mode,const char* config_path,const char* log_path,int deliver_mode,void(*user_cb)(size_t data_size,void* data,void* arg),void* db_ptr,void* arg);
void system_run(struct node_t* replica);
void system_exit(struct node_t* replica);

#endif
