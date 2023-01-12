#include <array>
#include <numeric>
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <vector>
#include <set>
#include <cstring>
#include <unistd.h>

#include "cxxopts.hpp"

using namespace std::chrono_literals;


struct CpuStat {
    std::array<int, 10> values;

    int * const user() { return values.data(); }
    int * const nice() { return values.data() + 1; }
    int * const system() { return values.data() + 2; }
    int * const idle() { return values.data() + 3; }
    int * const iowait() { return values.data() + 4; }
    int * const irq() { return values.data() + 5; }
    int * const softirq() { return values.data() + 6; }
    int * const steal() { return values.data() + 7; }
    int * const guest() { return values.data() + 8; }
    int * const guest_nice() { return values.data() + 9; }
};


void ReadStats(std::vector<CpuStat>& cpus) {
    std::ifstream ifs;
    ifs.open("/proc/stat", std::ios::in);
    std::string line{};
    int n_cpus{0};
    while (std::getline(ifs, line) && n_cpus < cpus.size()) {
        auto cs = line.c_str();
        /*
        * CPU core line format: 
        * cpu%n %user %nice %system %idle %iowait %irq %softirq %steal %guest %guest_nice
        */
        // Skip the line if it doesn't start with cpu%d
        if (line.size() < 4 || std::strncmp(cs, "cpu", 3) != 0 || !std::isdigit(cs[3])) {
            continue;
        }
        // Ok, the line is correct. Extract cpu index first.
        size_t index{};
        if (std::sscanf(cs, "cpu%lu", &index) != 1 || index >= cpus.size()) {
            continue;
        }
        CpuStat& cpu = cpus[index];
        int ret = std::sscanf(
            cs, "cpu%lu %d %d %d %d %d %d %d %d %d %d",
            &index, cpu.user(), cpu.nice(), cpu.system(), cpu.idle(),
            cpu.iowait(), cpu.irq(), cpu.softirq(), cpu.steal(),
            cpu.guest(), cpu.guest_nice());
        if (ret != 11) {
            continue;
        } else {
            n_cpus++;
        }
    }
}


size_t GetCpusCount() {
    std::ifstream ifs;
    ifs.open("/proc/stat", std::ios::in);
    std::string line{};
    size_t n_cpus{};
    while (std::getline(ifs, line)) {
        auto cs = line.c_str();
        if (line.size() >= 4 && std::strncmp(cs, "cpu", 3) == 0 && std::isdigit(cs[3])) {
            n_cpus++;
        }
    }
    return n_cpus;
}

void Subtract(const CpuStat& curr, const CpuStat& prev, CpuStat& result) {
    for (size_t i = 0; i < curr.values.size(); i++) {
        result.values[i] = curr.values[i] - prev.values[i];
    }
}

int main(int argc, char **argv) {
    // cxxopts::Options options(
    //     "cpu_stats", 
    //     "Measure CPU utilization and track CPU to thread assignments")

    size_t n_cpus = GetCpusCount();
    std::vector<CpuStat> cpus1{};
    std::vector<CpuStat> cpus2{};
    std::vector<CpuStat> cpus_diff{};
    std::vector<CpuStat> *curr_cpus = &cpus1;
    std::vector<CpuStat> *prev_cpus = &cpus2;

    cpus1.resize(n_cpus);
    cpus2.resize(n_cpus);
    cpus_diff.resize(n_cpus);
    
    ReadStats(*curr_cpus);
    std::vector<std::vector<double>> util_list{};

    while (true) {
        std::this_thread::sleep_for(500ms);
        std::swap(curr_cpus, prev_cpus);
        ReadStats(*curr_cpus);
        util_list.push_back({});
        util_list.back().resize(n_cpus);
        for (size_t i = 0; i < n_cpus; i++) {
            CpuStat& cpu = cpus_diff[i];
            Subtract((*curr_cpus)[i], (*prev_cpus)[i], cpu);
            int total = std::accumulate(cpu.values.begin(), cpu.values.end(), 0);
            int not_idle = total - *cpu.idle();
            util_list.back()[i] = static_cast<double>(not_idle) / total;
        }
        for (size_t i = 0; i < n_cpus; i++) {
            printf("%lu:%.2f%% ", i, util_list.back()[i] * 100.0);
        }
        std::cout << std::endl;
    }
}
