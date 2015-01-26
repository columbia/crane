/* Copyright (c) 2014,  Regents of the Columbia University 
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other 
 * materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// alpha 0.1 version we implement socket -> connect -> send -> recv -> close 5 functions.
// And we haven't handle the corresponding errno

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif


#ifndef _LARGEFILE_SOURCE     /* ----- #if 0 : If0Label_2 ----- */
#define _LARGEFILE_SOURCE
#endif     /* ----- #if 0 : If0Label_2 ----- */

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <execinfo.h> 
#include "../src/include/rsm-interface.h"

#include "enter_sys.h"

#ifndef PROJECT_TAG
#define PROJECT_TAG "CLIB"
#endif

#define RESOLVE(x)	if (!fp_##x && !(fp_##x = dlsym(RTLD_NEXT, #x))) { fprintf(stderr, #x"() not found!\n"); exit(-1); }

#ifndef MAXSIZE
#define MAXSIZE 1024
#endif

#ifndef LD_DEBUG
#define LD_DEBUG 1
#endif

static ssize_t (*fp_write)(int fd, const void *buf, size_t count);

static ssize_t (*fp_send)(int socket, const void *buffer, size_t length, int flags);

ssize_t send(int socket, const void *buffer, size_t length, int flags){
    ssize_t ret =-1;
    if(!check_sys()){
#if LD_DEBUG
        fprintf(stderr,"now I am calling the fake %s function\n",__FUNCTION__);
#endif
        enter_sys();
        RESOLVE(send);
        client_msg *request = NULL;
        request = (client_msg*)malloc(CLIENT_MSG_HEADER_SIZE+length);
        if(request==NULL){
            leave_sys();
            return ret;
        }
        request->header.type = C_SEND_WR;
        request->header.data_size = length;
        memcpy(request->data,buffer,length);
        ret = fp_send(socket,request,CLIENT_MSG_HEADER_SIZE+length,flags);
        if(request!=NULL){free(request);};
        if(ret>0){
            ret = ret-CLIENT_MSG_HEADER_SIZE;
        }
        leave_sys();
#if LD_DEBUG
        fprintf(stderr,"now I am finished the fake %s function\n",__FUNCTION__);
#endif
    }else{
#if LD_DEBUG
        fprintf(stderr,"now I am calling the real %s function\n",__FUNCTION__);
#endif
        RESOLVE(send);
        ret = fp_send(socket,buffer,length,flags);
#if LD_DEBUG
        fprintf(stderr,"now I am finished the real %s function\n",__FUNCTION__);
#endif
    }
    return ret;
};

ssize_t write(int fd, const void *buf, size_t count){
    ssize_t ret =-1;
    struct stat st;
    int is_socket=0;
    fstat(fd,&st);
    is_socket = (__S_IFSOCK==(st.st_mode&__S_IFMT));
    if(!check_sys()&&is_socket){
#if LD_DEBUG
        fprintf(stderr,"now I am calling the fake %s function\n",__FUNCTION__);
#endif
        enter_sys();
        RESOLVE(write);
        client_msg *request = NULL;
        request = (client_msg*)malloc(CLIENT_MSG_HEADER_SIZE+count);
        if(request==NULL){
            leave_sys();
            return ret;
        }
        request->header.type = C_SEND_WR;
        request->header.data_size = count;
        memcpy(request->data,buf,count);
        ret = fp_write(fd,request,CLIENT_MSG_HEADER_SIZE+count);
        if(request!=NULL){free(request);};
        if(ret>0){
            ret = ret-CLIENT_MSG_HEADER_SIZE;
        }
        leave_sys();

#if LD_DEBUG
        fprintf(stderr,"now I am finished the fake %s function\n",__FUNCTION__);
#endif
    }else{
#if LD_DEBUG
        fprintf(stderr,"now I am calling the real %s function\n",__FUNCTION__);
#endif
        RESOLVE(write);
        ret = fp_write(fd,buf,count);
#if LD_DEBUG
        fprintf(stderr,"now I am finished the real %s function\n",__FUNCTION__);
#endif
    }
    return ret;
}

