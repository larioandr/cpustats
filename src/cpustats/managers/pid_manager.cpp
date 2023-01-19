#include "pid_manager.hpp"

void PidManager::Init() {}

void PidManager::Update() {
    if (track_all_) {
        pids_list_ = ListPids();
    }
    for (auto pid: pids_list_) {
        PidStat stat{.pid = pid};
        ReadProcPidStat(pid, stat);
        for (size_t i{}; i < acceptors_.size(); i++) {
            auto last_in_cycle = i +1 == acceptors_.size();
            acceptors_.at(i)->Accept(stat, last_in_cycle);
        }
    }
}

void PidManager::Finish() {}
