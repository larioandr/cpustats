#ifndef CPUSTATS_PIDS_HPP
#define CPUSTATS_PIDS_HPP

#include <optional>


struct ProcStats {
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

void ReadProcStats(int pid, ProcStats& stats);

ProcStats::State FromChar(char c);
const char *ToString(ProcStats::State state);

#endif //CPUSTATS_PIDS_HPP
