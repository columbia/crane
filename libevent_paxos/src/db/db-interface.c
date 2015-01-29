/*
 * =====================================================================================
 *
 *       Filename:  db-interface.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/18/2014 02:14:41 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Milannic (), milannic.cheng.liu@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <db.h>
#include "../include/db/db-interface.h"
#include "../include/util/debug.h"

const char* db_dir="./.db";

struct db_t{
    DB* bdb_ptr;
};


void mk_path(char* dest,const char* prefix,const char* db_name){
    memcpy(dest,prefix,strlen(prefix));
    dest[strlen(prefix)] = '/';
    memcpy(dest+strlen(prefix)+1,db_name,strlen(db_name));
    dest[strlen(prefix)+strlen(db_name)+1] = '\0';
    return;
}

db* initialize_db(const char* db_name,uint32_t flag){
    db* db_ptr=NULL;
    DB* b_db;
    DB_ENV* dbenv;
    int ret;
    char* full_path = NULL;
    if((ret=mkdir(db_dir,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))!=0){
        if(errno!=EEXIST){
            err_log("DB : Dir Creation Failed\n");
            goto db_init_return;
        }
    }
    full_path = (char*)malloc(strlen(db_dir) + strlen(db_name)+2);
    mk_path(full_path,db_dir,db_name);
	if ((ret = db_env_create(&dbenv,0)) != 0) {
		dbenv->err(dbenv, ret, "Environment Created: %s", db_dir);
        goto db_init_return;
	}
	if ((ret = dbenv->open(dbenv,db_dir,DB_CREATE|DB_INIT_CDB|DB_INIT_MPOOL|DB_THREAD, 0)) != 0) {
		//dbenv->err(dbenv, ret, "Environment Open: %s", db_dir);
        goto db_init_return;
	}
    /* Initialize the DB handle */
    if((ret = db_create(&b_db,dbenv,flag))!=0){
        err_log("DB : %s.\n",db_strerror(ret));
        goto db_init_return;
    }

    if((ret = b_db->open(b_db,NULL,db_name,NULL,DB_BTREE,DB_THREAD|DB_CREATE,0))!=0){
        //b_db->err(b_db,ret,"%s","test.db");
        goto db_init_return;
    }
    db_ptr = (db*)(malloc(sizeof(db)));
    db_ptr->bdb_ptr = b_db;

db_init_return:
    if(full_path!=NULL){
        free(full_path);
    }
    if(db_ptr!=NULL){
        //debug_log("DB Initialization Finished\n");
        ;
    }
    return db_ptr;
}

void close_db(db* db_p,uint32_t mode){
    if(db_p!=NULL){
        if(db_p->bdb_ptr!=NULL){
            db_p->bdb_ptr->close(db_p->bdb_ptr,mode);
            db_p->bdb_ptr=NULL;
        }
        free(db_p);
        db_p = NULL;
    }
    return;
}

int store_record(db* db_p,size_t key_size,void* key_data,size_t data_size,void* data){
    // tom add 20150125
    struct timeval start_t;
    struct timeval end_t;
    long  difference;
    gettimeofday(&start_t,NULL);
    // end tom add

    int ret = 1;
    if((NULL==db_p)||(NULL==db_p->bdb_ptr)){
        goto db_store_return;
    }
    DB* b_db = db_p->bdb_ptr;
    DBT key,db_data;
    memset(&key,0,sizeof(key));
    memset(&db_data,0,sizeof(db_data));
    key.data = key_data;
    key.size = key_size;
    db_data.data = data;
    db_data.size = data_size;
    if ((ret=b_db->put(b_db,NULL,&key,&db_data,DB_AUTO_COMMIT))==0){
        //debug_log("db : %ld record stored. \n",*(uint64_t*)key_data);
        //b_db->sync(b_db,0);
    }
    else{
        //debug_log("db : can not save record %ld from database.\n",*(uint64_t*)key_data);
        //b_db->err(b_db,ret,"DB->Put");
    }
db_store_return:
        // tom add 20150125
        gettimeofday(&end_t,NULL);
        difference = (end_t.tv_sec*1000000+end_t.tv_usec ) - (start_t.tv_sec*1000000+start_t.tv_usec);
        if(difference > 10000) {
            printf("Warning store_record: %ld\n", difference);

        }
        // end tom add
    return ret;
}

int retrieve_record(db* db_p,size_t key_size,void* key_data,size_t* data_size,void** data){
    int ret=1;
    if(NULL==db_p || NULL==db_p->bdb_ptr){
        goto db_retrieve_return;
    }
    DB* b_db = db_p->bdb_ptr;
    DBT key,db_data;
    memset(&key,0,sizeof(key));
    memset(&db_data,0,sizeof(db_data));
    key.data = key_data;
    key.size = key_size;
    db_data.flags = DB_DBT_MALLOC;
    if((ret=b_db->get(b_db,NULL,&key,&db_data,0))==0){
        //debug_log("db : get record %ld from database.\n",*(uint64_t*)key_data);
    }else{
        //debug_log("db : can not get record %ld from database.\n",*(uint64_t*)key_data);
        //b_db->err(b_db,ret,"DB->Get");
        goto db_retrieve_return;
    }
    if(!db_data.size){
        goto db_retrieve_return;
    }
    *data = db_data.data;
    *data_size = db_data.size;
db_retrieve_return:
    return ret;
}
