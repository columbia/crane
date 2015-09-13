/*
 * =====================================================================================
 *
 *       Filename:  db_file.c
 *
 *    Description:  j
 *
 *        Version:  1.0
 *        Created:  01/27/2015 09:02:18 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */


#include <stdlib.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include "../../.local/include/db.h"
//#include "../include/db/db-interface.h"
//#include "../include/util/debug.h"

const char* db_dir="./.db";

struct db_t{
    DB* bdb_ptr;
};


typedef struct db_t db;

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
            //err_log("DB : Dir Creation Failed\n");
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
        //err_log("DB : %s.\n",db_strerror(ret));
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
        b_db->err(b_db,ret,"DB->Put");
    }
db_store_return:
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
        b_db->err(b_db,ret,"DB->Get");
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


int delete_record(db* db_p,size_t key_size,void* key_data){
    int ret=1;
    if(NULL==db_p || NULL==db_p->bdb_ptr){
        goto db_delete_return;
    }
    DB* b_db = db_p->bdb_ptr;
    DBT key;
    memset(&key,0,sizeof(key));
    key.data = key_data;
    key.size = key_size;
    ret=b_db->del(b_db,NULL,&key,0);
    if(ret!=0){
        b_db->err(b_db,ret,"DB->Delete");
    }
db_delete_return:
    return ret;
}

int main ( int argc, char *argv[] )
{
    db* my_db = initialize_db("test",0);
    uint64_t record_no = 1234567;
    char* data = (char*)(malloc)(6);
    int ret = delete_record(my_db,sizeof(record_no),&record_no);
    printf("1st Delete Return Value is %d.\n",ret);
    data[0] = 'a';
    data[1] = 'e';
    data[2] = 'd';
    data[3] = 'c';
    data[4] = 'b';
    data[5] = '\0';
    store_record(my_db,sizeof(record_no),&record_no,6,data);
    char* record_data = NULL;
    size_t data_size;
    retrieve_record(my_db,sizeof(record_no),&record_no,&data_size,(void**)&record_data);
    if(record_data!=NULL){
        printf("I've taken out the recorded data that is %s.\n",record_data);
        free(record_data);
    }
    ret = delete_record(my_db,sizeof(record_no),&record_no);
    printf("2nd Delete Return Value is %d.\n",ret);
    record_data = NULL;
    retrieve_record(my_db,sizeof(record_no),&record_no,&data_size,(void**)&record_data);
    if(record_data!=NULL){
        printf("I've taken out the recorded data that is %s.\n",record_data);
        free(record_data);
    }else{
        printf("the record has been deleted.\n");
    }
    return EXIT_SUCCESS;
}				/* ----------  end of function main  ---------- */
