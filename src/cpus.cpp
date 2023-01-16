#include "cpus.hpp"
#include <fstream>
#include <cstring>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err34-c"
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
#pragma clang diagnostic pop

size_t GetCpusCount() {
    std::ifstream ifs;
    ifs.open("/proc/stat", std::ios::in);
    std::string line{};
    size_t n_cpus{};
    while (std::getline(ifs, line)) {
        auto cs = line.c_str();
        // If the line looks like "cpu2", then it is a particular core. Count them.
        // No matter, if it is "cpu2" or "cpu22" - each core appears only once.
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
