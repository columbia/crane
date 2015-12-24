primary_ip=`cat $MSMR_ROOT/eval-container/head`

# Common default options.
xtern=0                                               # 1 use xtern, 0 otherwise.
proxy=0                                               # 1 use proxy, 0 otherwise
sch_paxos=0                                           # 1 xtern will schedule with paxos, 0 otherwise
sch_dmt=0                                             # 1 libevent_paxos will schedule with DMT, 0 otherwise
enable_lxc="no"
checkpoint=0
leader_elect=0

dmt_log_output=0

use_orig_plan() {
    xtern=0
    proxy=0
    sch_paxos=0
    sch_dmt=0
    checkpoint=0
    enable_lxc="no"
}

use_xtern_only_plan() {
    xtern=1
    proxy=0
    sch_paxos=0
    sch_dmt=0
    checkpoint=0
    enable_lxc="no"
}

use_proxy_only_plan() {
    xtern=0
    proxy=1
    sch_paxos=0
    sch_dmt=0
    checkpoint=0
    enable_lxc="no"
}

use_separate_scheduling_plan() {
    xtern=1
    proxy=1
    sch_paxos=0
    sch_dmt=0
    checkpoint=0
    enable_lxc="no"
}

use_joint_scheduling_plan() {
    xtern=1
    proxy=1
    sch_paxos=1
    sch_dmt=1
    checkpoint=0
    enable_lxc="no"
}

use_joint_scheduling_and_lxc_plan() {
    xtern=1
    proxy=1
    sch_paxos=1
    sch_dmt=1
    checkpoint=0                                    
    enable_lxc="yes"
}

use_worker_backup_plan() {
    xtern=1
    proxy=1
    sch_paxos=1
    sch_dmt=1
    checkpoint=1
    enable_lxc="yes"
}

