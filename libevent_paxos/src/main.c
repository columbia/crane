/*
 * =====================================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/06/2014 04:31:43 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Milannic (), milannic.cheng.liu@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "include/rsm-interface.h"
#include "include/replica-sys/replica.h"



static void usage(){
    fprintf(stderr,"Usage : -n NODE_ID\n");
    fprintf(stderr,"        -m [sr] Start Mode seed|recovery\n");
    fprintf(stderr,"        -c path path to configuration file\n");
    fprintf(stderr,"        -l path dir to store the log file of the proxy\n");
    fprintf(stderr,"        -f let the proxy start in fake mode to do debug\n");
    fprintf(stderr,"        -d enable system internal debug msg.\n");
}

static int number = 1;

static void pseudo_cb(int data_size,void* data){
    fprintf(stdout,"%d : %s\n",number,(char*)data);
    number++;
    fflush(stdout);
    return; }

int main(int argc,char** argv){
    char* start_mode= NULL;
    char* config_path = NULL;
    char* log_dir = NULL;
    int node_id = -1;
    int fake_mode = 1;
    int c;

    while((c = getopt (argc,argv,"drl:c:n:m:")) != -1){
        switch(c){
            case 'n':
                node_id = atoi(optarg);
                break;
            case 'c':
                config_path = optarg;
                break;
            case 'l':
                log_dir = optarg;
                break;
            case 'm':
                start_mode= optarg;
                if(*start_mode!='s' && *start_mode!='r'){
                    fprintf(stderr,"Unknown Start Mode.\n");
                    usage();
                    exit(1);
                }
                break;
            case 'r':
                fprintf(stderr,"real mode is opened.\n");
                fake_mode = 0;
            case '?':
                if(optopt == 'n'){
                    fprintf(stderr,"Option -n requires an argument.\n");
                    usage();
                    exit(1);
                }
                else if(optopt == 'm'){
                    fprintf(stderr,"Option -m requires an argument.\n");
                    usage();
                    exit(1);
                }
                else if(optopt == 'c'){
                    fprintf(stderr,"Option -c requires an argument.\n");
                    usage();
                    exit(1);
                }
                break;
            default:
                fprintf(stderr,"Unknown Option %d \n",c);
                exit(1);
        }
    }
    if(config_path==NULL||node_id==-1||start_mode==NULL){
        usage();
        exit(1);
    }

    struct proxy_node_t* proxy = proxy_init(node_id,start_mode,config_path,log_dir,fake_mode);
    if(NULL==proxy){
        return 1;
    }else{
        proxy_run(proxy);
    }
    fprintf(stdout,"Program Finishes.\n");
    return 0;
}
