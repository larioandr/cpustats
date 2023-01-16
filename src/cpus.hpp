#ifndef CPUSTATS_CPUS_HPP
#define CPUSTATS_CPUS_HPP

#include <array>
#include <vector>
#include <string>


struct CpuStat {
    std::array<int, 10> values;

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


size_t GetCpusCount();
void ReadStats(std::vector<CpuStat>& cpus);
void Subtract(const CpuStat& curr, const CpuStat& prev, CpuStat& result);


#endif //CPUSTATS_CPUS_HPP
