
#ifndef DEBUG_H
#define DEBUG_H

#define debug_log(args...) do { \
    struct timeval tv; \
    gettimeofday(&tv,0); \
    fprintf(stderr,"%lu.%06lu:",tv.tv_sec,tv.tv_usec); \
    fprintf(stderr,args); \
}while(0);


#define paxos_log(args...) do { \
    struct timeval tv; \
    gettimeofday(&tv,0); \
    fprintf(stderr,"%lu.%06lu:",tv.tv_sec,tv.tv_usec); \
    fprintf(stderr,args); \
}while(0);

#define err_log(args...) do { \
    struct timeval tv; \
    gettimeofday(&tv,0); \
    fprintf(stderr,"%lu.%06lu:",tv.tv_sec,tv.tv_usec); \
    fprintf(stderr,args); \
}while(0);

#define rec_log(out,args...) do { \
    struct timeval tv; \
    gettimeofday(&tv,0); \
    fprintf((out),"%lu.%06lu:",tv.tv_sec,tv.tv_usec); \
    fprintf((out),args); \
    fflush(out); \
}while(0);

#define safe_rec_log(x,args...) {if(NULL!=(x)){rec_log((x),args);}}

#define SYS_LOG(x,args...) {if((x)->sys_log){safe_rec_log(((x)->sys_log_file),args)}}

#define STAT_LOG(x,args...) {if((x)->stat_log){safe_rec_log(((x)->sys_log_file),args)}}

#define REQ_LOG(x,args...) {if((x)->req_log){safe_rec_log(((x)->sys_log_file),args)}}

#define PROXY_ENTER(x) {SYS_LOG(x,"PROXY : Entering %s\n",__PRETTY_FUNCTION__)}

#define PROXY_LEAVE(x) {SYS_LOG(x,"PROXY : Leaving %s\n",__PRETTY_FUNCTION__)}

#define PROXY_ERR_LEAVE(x) {SYS_LOG(x,"PROXY : Error Occurred,Before Leaving %s\n",__PRETTY_FUNCTION__)}

#define CONSENSUS_ENTER(x) {if(x->sys_log_file){rec_log(((x)->sys_log_file),"CONSENSUS : Entering %s\n",__PRETTY_FUNCTION__)}}
#define CONSENSUS_LEAVE(x) {if(x->sys_log_file){rec_log(((x)->sys_log_file),"CONSENSUS : Leaving %s\n",__PRETTY_FUNCTION__)}}
#define CONSENSUS_ERR_LEAVE(x) {if(x->sys_log_file){rec_log(((x)->sys_log_file),"CONSENSUS : Error Occurred,Before Leaving %s\n",__PRETTY_FUNCTION__)}}

#define DEBUG_ENTER debug_log("Entering %s\n",__PRETTY_FUNCTION__)
#define DEBUG_LEAVE debug_log("Leaving %s\n",__PRETTY_FUNCTION__)

#define DEBUG_POINT(x,n) {if(x->sys_log_file){rec_log(((x)->sys_log_file),"Debug Point " #n ".\n")}}

// extra performance data collection
#define PERF_DEBUG 

#endif
