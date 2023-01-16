#include "pids.hpp"
#include <fstream>
#include <fmt/format.h>
#include <iostream>

static const char *ToNextWord(const char *s, int cnt = 1) {
    while (cnt > 0) {
        // Find word end
        while (*s && !std::isspace(*s)) {
            s++;
        }
        // Find next word start
        while (*s && std::isspace(*s)) {
            s++;
        }
        cnt--;
    }
    return s;
}

void ReadProcStats(int pid, ProcStats& stats) {
    std::ifstream ifs{};
    auto file_name{fmt::format("/proc/{}/stat", pid)};
    ifs.open(file_name, std::ios::in);
    if (ifs.fail()) {
        stats.state = ProcStats::State::not_found;
    } else if (std::string line; std::getline(ifs, line)) {
        const char *s = line.c_str();
        s = ToNextWord(s, 2);
        if (!*s) {
            std::cerr << "bad line in " << file_name
                << ": missing state column #2" << std::endl;
            stats.state = ProcStats::State::not_found;
            return;
        }
        stats.state = FromChar(*s);
        s = ToNextWord(s, 36);
        if (!*s) {
            std::cerr << "bad line in " << file_name
                << ": missing CPU column #39" << std::endl;
            return;
        }
        stats.cpu = std::atoi(s);
    }
}

const char *ToString(ProcStats::State state) {
    switch (state) {
        case ProcStats::State::not_found: return "not found";
        case ProcStats::State::running: return "running";
        case ProcStats::State::sleeping: return "sleeping";
        case ProcStats::State::waiting: return "waiting";
        case ProcStats::State::zombie: return "zombie";
        case ProcStats::State::stopped: return "stopped";
        case ProcStats::State::tracing_stop: return "tracing_stop";
        case ProcStats::State::dead: return "dead";
        case ProcStats::State::wakekill: return "wakekill";
        case ProcStats::State::waking_paging: return "waking or paging";
        case ProcStats::State::parked: return "parked";
        default: return "unknown";
    }
}

ProcStats::State FromChar(char c) {
    switch (c) {
        case 'R': return ProcStats::State::running;
        case 'S': return ProcStats::State::sleeping;
        case 'D': return ProcStats::State::waiting;
        case 'Z': return ProcStats::State::zombie;
        case 'T': return ProcStats::State::stopped;
        case 't': return ProcStats::State::tracing_stop;
        case 'W': return ProcStats::State::waking_paging;
        case 'X': case 'x': return ProcStats::State::dead;
        case 'K': return ProcStats::State::wakekill;
        case 'P': return ProcStats::State::parked;
        default: return ProcStats::State::unknown;
    }
}