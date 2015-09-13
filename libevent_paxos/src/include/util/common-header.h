/*
 * =====================================================================================
 *
 *       Filename:  common-header.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/23/2014 03:56:00 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef COMMON_HEADER_H
#define COMMON_HEADER_H
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>
#include <getopt.h>
#include <unistd.h>
#include <pthread.h>
#include <error.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "common-structure.h"
#include "net-communication.h"
#include "debug.h"

#endif

#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE
#endif
