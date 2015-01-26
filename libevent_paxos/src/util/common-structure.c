/*
 * =====================================================================================
 *
 *       Filename:  commnon-structure.c
 *
 *    Description:  j
 *
 *        Version:  1.0
 *        Created:  08/06/2014 04:54:08 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Milannic (), milannic.cheng.liu@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */

#include "../include/util/common-structure.h"

int view_stamp_comp(view_stamp* op1,view_stamp* op2){
    if(op1->view_id<op2->view_id){
        return -1;
    }else if(op1->view_id>op2->view_id){
        return 1;
    }else{
        if(op1->req_id>op2->req_id){
            return 1;
        }else if(op1->req_id<op2->req_id){
            return -1;
        }else{
            return 0;
        }
    }
}

uint64_t vstol(view_stamp* vs){
    uint64_t result = ((uint64_t)vs->req_id)&0xFFFFFFFFl;
    uint64_t temp = (uint64_t)vs->view_id&0xFFFFFFFFl;
    result += temp<<32;
    return result;
};

view_stamp ltovs(uint64_t record_no){
    view_stamp vs;
    uint64_t temp;
    temp = record_no&0x00000000FFFFFFFFl;
    vs.req_id = temp;
    vs.view_id = (record_no>>32)&0x00000000FFFFFFFFl;
    return vs;
}


int timeval_comp(struct timeval *op1,struct timeval *op2){
    if(op1->tv_sec>op2->tv_sec){
        return 1;
    }
    if(op1->tv_sec==op2->tv_sec){
        if(op1->tv_usec>op2->tv_usec){
            return 1;
        }else if(op1->tv_usec == op2->tv_usec){
            return 0;
        }
    }
    return -1;
}

void timeval_add(struct timeval*op1,struct timeval* op2,struct timeval* res){
    res->tv_sec = op1->tv_sec + op2->tv_sec;
    res->tv_usec = op1->tv_usec + op2->tv_usec;
    return;
}
