#ifndef CPUSTATS_LINUX_PROC_HPP
#define CPUSTATS_LINUX_PROC_HPP

#include <array>
#include <string>
#include <vector>


struct CpuStat {
    static constexpr int kNumValues = 10;
    int cpu;
    std::array<int, kNumValues> values;

    int* const user() { return values.data(); }
    int* const nice() { return values.data() + 1; }
    int* const system() { return values.data() + 2; }
    int* const idle() { return values.data() + 3; }
    int* const iowait() { return values.data() + 4; }
    int* const irq() { return values.data() + 5; }
    int* const softirq() { return values.data() + 6; }
    int* const steal() { return values.data() + 7; }
    int* const guest() { return values.data() + 8; }
    int* const guest_nice() { return values.data() + 9; }
};


struct CpuInfo {
    int cpu{};
    std::string model_name{};
};


struct PidStat {
    enum class State {
        running,
        sleeping,
        waiting,
        zombie,
        stopped,
        tracing_stop,
        dead,
        wakekill,
        waking_paging,
        parked,
        not_found,
        unknown
    };

    int pid{};
    State state{State::unknown};
    int cpu{};
};


int GetCpuCount();
std::vector<CpuInfo> LoadProcCpuInfo();
void ReadProcStat(std::vector<CpuStat>& cpus);
void ReadProcPidStat(int pid, PidStat& stat);

PidStat::State PidStateFromChar(char c);
const char *ToString(PidStat::State state);

std::vector<int> ListPids();

#endif //CPUSTATS_LINUX_PROC_HPP
