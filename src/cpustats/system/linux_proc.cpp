#include "linux_proc.hpp"
#include "../utility/strings.hpp"

#include <fmt/format.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

int GetCpuCount() {
    return static_cast<int>(LoadProcCpuInfo().size());
}

std::vector<CpuInfo> LoadProcCpuInfo() {
    std::ifstream ifs;
    ifs.open("/proc/cpuinfo", std::ios::in);
    if (ifs.fail()) {
        std::cerr << "Error opening /proc/cpuinfo\n";
        return {};
    }
    std::string line{};
    std::vector<CpuInfo> result;
    while (std::getline(ifs, line)) {
        auto separator_pos = line.find(':');
        if (separator_pos == std::string::npos) {
            continue;
        }
        // Strip trailing whitespaces from the keyword
        auto key_end_pos = separator_pos;
        while (key_end_pos > 0 && std::isspace(line[key_end_pos - 1])) {
            key_end_pos--;
        }
        auto key = line.substr(0, key_end_pos);
        // Strip leading whitespaces from the value
        auto value_start_pos = separator_pos + 1;
        while (value_start_pos < line.size() && std::isspace(line[value_start_pos])) {
            value_start_pos++;
        }
        auto value = line.substr(value_start_pos);
        if (key == "processor") {
            result.emplace_back();
            result.back().cpu = std::stoi(value);
        } else if (key == "model name") {
            result.back().model_name = value;
        }
    }
    return result;
}

void ReadProcStat(std::vector<CpuStat>& cpus) {
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

void ReadProcPidStat(int pid, PidStat& stat) {
    std::ifstream ifs{};
    auto file_name{fmt::format("/proc/{}/stat", pid)};
    ifs.open(file_name, std::ios::in);
    if (ifs.fail()) {
        stat.state = PidStat::State::not_found;
    } else if (std::string line; std::getline(ifs, line)) {
        const char *s = line.c_str();
        s = ToNextWord(s, 2);
        if (!*s) {
            std::cerr << "bad line in " << file_name
                      << ": missing state column #2" << std::endl;
            stat.state = PidStat::State::not_found;
            return;
        }
        stat.state = PidStateFromChar(*s);
        s = ToNextWord(s, 36);
        if (!*s) {
            std::cerr << "bad line in " << file_name
                      << ": missing CPU column #39" << std::endl;
            return;
        }
        stat.cpu = std::atoi(s);
    }
}

PidStat::State PidStateFromChar(char c) {
    switch (c) {
        case 'R': return PidStat::State::running;
        case 'S': return PidStat::State::sleeping;
        case 'D': return PidStat::State::waiting;
        case 'Z': return PidStat::State::zombie;
        case 'T': return PidStat::State::stopped;
        case 't': return PidStat::State::tracing_stop;
        case 'W': return PidStat::State::waking_paging;
        case 'X': case 'x': return PidStat::State::dead;
        case 'K': return PidStat::State::wakekill;
        case 'P': return PidStat::State::parked;
        default: return PidStat::State::unknown;
    }
}

const char *ToString(PidStat::State state) {
    switch (state) {
        case PidStat::State::not_found: return "not found";
        case PidStat::State::running: return "running";
        case PidStat::State::sleeping: return "sleeping";
        case PidStat::State::waiting: return "waiting";
        case PidStat::State::zombie: return "zombie";
        case PidStat::State::stopped: return "stopped";
        case PidStat::State::tracing_stop: return "tracing_stop";
        case PidStat::State::dead: return "dead";
        case PidStat::State::wakekill: return "wakekill";
        case PidStat::State::waking_paging: return "waking or paging";
        case PidStat::State::parked: return "parked";
        default: return "unknown";
    }
}

std::vector<int> ListPids() {
    std::vector<int> pids{};
    for (auto const& entry: fs::directory_iterator("/proc")) {
        auto path = entry.path();
        // Check that this is a PID:
        // 1) it is a directory,
        if (!entry.is_directory()) {
            continue;
        }
        // 2) filename is built from digits, and:
        bool not_pid{false};
        auto filename = path.filename().string();
        for (auto c: filename) {
            if (!std::isdigit(c)) {
                not_pid = true;
                break;
            }
        }
        if (not_pid) {
            continue;
        }
        // 3) it is a directory containing 'stat' file
        if (!fs::exists(path.append("stat"))) {
            continue;
        }
        pids.push_back(std::stoi(filename));
    }
    return pids;
}
