#include "cpu_manager.hpp"
#include "../utility/strings.hpp"
#include <numeric>
#include <cassert>
#include <cstring>


void CpuManager::Init() {
    cpu_info_list_ = LoadProcCpuInfo();
    auto n_cpus = cpu_info_list_.size();
    cpu_stat_list_1_.resize(n_cpus);
    cpu_stat_list_2_.resize(n_cpus);
    for (size_t i{}; i < n_cpus; i++) {
        int cpu = cpu_info_list_[i].cpu;
        cpu_stat_list_1_[i].cpu = cpu;
        cpu_stat_list_2_[i].cpu = cpu;
    }
    ReadProcStat(*curr_cpu_stat_list_);

    // Inform CPU-Info consumers about cpus (only here in Init())
    CallAcceptors(cpu_info_acceptors_, cpu_info_list_.begin(), cpu_info_list_.end());

    // Inform CPU-Stat consumers about current stats (also during updates)
    CallAcceptors(cpu_stat_acceptors_, curr_cpu_stat_list_->begin(), curr_cpu_stat_list_->end());
}


void CpuManager::Update() {
    std::swap(curr_cpu_stat_list_, prev_cpu_stat_list_);
    ReadProcStat(*curr_cpu_stat_list_);
    CallAcceptors(cpu_stat_acceptors_, curr_cpu_stat_list_->begin(), curr_cpu_stat_list_->end());

    std::vector<CpuUtil> cpu_util_list{};
    cpu_util_list.resize(cpu_info_list_.size());

    for (size_t i{}; i < cpu_info_list_.size(); i++) {
        auto const& curr = curr_cpu_stat_list_->at(i);
        auto const& prev = prev_cpu_stat_list_->at(i);
        assert(curr.cpu == prev.cpu);
        CpuStat diff{};
        Subtract(curr.values, prev.values, diff.values);

        auto& cpu_util = cpu_util_list[i];
        cpu_util.cpu = curr.cpu;
        int total = std::accumulate(diff.values.begin(), diff.values.end(), 0);
        int not_idle = total - *diff.idle();
        cpu_util.busy_rate = static_cast<double>(not_idle) / total;
        cpu_util.idle_rate = static_cast<double>(*diff.idle()) / total;
    }

    CallAcceptors(cpu_util_acceptors_, cpu_util_list.begin(), cpu_util_list.end());
}


void CpuManager::Finish() {
}
