#include "pid_manager.hpp"

void PidManager::Init() {
}

void PidManager::Update() {
    if (track_all_) {
        pids_list_ = ListPids();
    }
    for (auto pid: pids_list_) {
        PidStat stat{.pid = pid};
        ReadProcPidStat(pid, stat);
        for (size_t i{}; i < acceptors_.size(); i++) {
            acceptors_.at(i)->Accept(stat);
        }
    }
}

void PidManager::Finish() {
}
