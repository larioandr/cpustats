#include <numeric>
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <vector>
#include <set>
#include <cstring>
#include <unistd.h>
#include <iomanip>

#include <cxxopts.hpp>
#include <date.h>
#include <fmt/format.h>

#include "cpus.hpp"
#include "pids.hpp"

using namespace std::chrono_literals;


struct Settings {
    bool use_cpu_stats{true};
    std::vector<int> pids{};
    std::string cpu_stats_file_name{};
    std::string pid_stats_file_name{};
    int interval_ms{1'000};

    [[nodiscard]] std::string String() const {
        std::stringstream ss;
        ss << "use_cpu_stats: " << (use_cpu_stats ? "yes" : "no") << std::endl
            << "pids: [";
        for (int i{}; i < pids.size(); i++) {
            if (i > 0) ss << ", ";
            ss << pids[i];
        }
        ss << "]\n";
        ss << "cpu_stats_file_name: " << cpu_stats_file_name << std::endl;
        ss << "pid_stats_file_name: " << pid_stats_file_name << std::endl;
        ss << "interval_ms: " << interval_ms << std::endl;
        return ss.str();
    }
};


cxxopts::Options BuildOptions() {
    cxxopts::Options options(
            "cpu_stats",
            "Measure CPU utilization and track threads cores"
    );
    options.add_options()
            ("i,interval", "Interval between measurements in milliseconds", cxxopts::value<int>()->default_value("1000"))
            ("no-cpu", "Do not record CPU stats", cxxopts::value<bool>()->default_value("false"))
            ("p,pid", "Track CPUs assigned to process or thread with PID",cxxopts::value<std::vector<int>>())
            ("f,file", "Base name for CSV files where to record results", cxxopts::value<std::string>()->default_value(""))
            ("cpu-file", "CSV file name to record CPU stats", cxxopts::value<std::string>()->default_value(""))
            ("pid-file", "CSV file name to record PID stats", cxxopts::value<std::string>()->default_value(""))
            ("h,help", "Print usage")
            ;
    return options;
}


Settings BuildSettings(const cxxopts::ParseResult& args) {
    Settings settings{};
    if (args.count("interval")) {
        settings.interval_ms = args["interval"].as<int>();
        if (settings.interval_ms <= 0) {
            std::cerr << "Bad interval, must be non-negative\n";
            std::exit(1);
        }
    }
    settings.use_cpu_stats = !args.count("no-cpu");
    if (args.count("pid")) {
        for (int pid: args["pid"].as<std::vector<int>>()) {
            settings.pids.push_back(pid);
        }
    }
    if (args.count("file")) {
        auto file_name = args["file"].as<std::string>();
        if (!file_name.empty()) {
            settings.cpu_stats_file_name = file_name + "_cpus.csv";
            settings.pid_stats_file_name = file_name + "_pids.csv";
        }
    }
    if (args.count("cpu-file")) {
        settings.cpu_stats_file_name = args["cpu-file"].as<std::string>();
    }
    if (args.count("pid-file")) {
        settings.pid_stats_file_name = args["pid-file"].as<std::string>();
    }
    return settings;
}

void PrintHelp(const cxxopts::Options& options) {
    std::cout << options.help() << std::endl;
}

template <class Precision>
std::string GetISOCurrentTime()
{
    auto now = std::chrono::system_clock::now();
    return date::format("%T", date::floor<Precision>(now));
}

int main(int argc, char **argv) {
    // cxxopts::Options options(
    //     "cpu_stats", 
    //     "Measure CPU utilization and track CPU to thread assignments")
    auto options = BuildOptions();
    auto parsed = options.parse(argc, argv);
    if (parsed.count("help")) {
        PrintHelp(options);
        std::exit(0);
    }
    auto settings = BuildSettings(parsed);
    std::cout << settings.String();

    size_t n_cpus = GetCpusCount();
    std::vector<CpuStat> cpus1{};
    std::vector<CpuStat> cpus2{};
    std::vector<CpuStat> cpus_diff{};
    std::vector<CpuStat> *curr_cpus = &cpus1;
    std::vector<CpuStat> *prev_cpus = &cpus2;

    {
        std::cout << fmt::format("{:<13}|", "Time");
        if (!settings.pids.empty()) {
            std::cout << fmt::format("{:^12}|", "PID");
        }
        for (int i = 0; i < n_cpus; i++) {
            std::cout << fmt::format("{:^8}|", fmt::format("cpu{}", i));
        }
        if (!settings.pids.empty()) {
            std::cout << fmt::format("{:<12}", " Status");
        }
        std::cout << std::endl;
    }


    cpus1.resize(n_cpus);
    cpus2.resize(n_cpus);
    cpus_diff.resize(n_cpus);

    std::vector<ProcStats> proc_stats_list{};
    for (auto pid: settings.pids) {
        proc_stats_list.push_back({.pid=pid});
    }
    
    ReadStats(*curr_cpus);
    std::vector<std::vector<double>> util_list{};

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(settings.interval_ms));

        if (settings.use_cpu_stats) {
            std::swap(curr_cpus, prev_cpus);
            ReadStats(*curr_cpus);
            util_list.emplace_back();
            util_list.back().resize(n_cpus);
            for (size_t i = 0; i < n_cpus; i++) {
                CpuStat &cpu = cpus_diff[i];
                Subtract((*curr_cpus)[i], (*prev_cpus)[i], cpu);
                int total = std::accumulate(cpu.values.begin(), cpu.values.end(), 0);
                int not_idle = total - *cpu.idle();
                util_list.back()[i] = static_cast<double>(not_idle) / total;
            }
        }

        for (auto& proc_stats: proc_stats_list) {
            ReadProcStats(proc_stats.pid, proc_stats);
        }

        std::cout << fmt::format("{:<13}|", GetISOCurrentTime<std::chrono::milliseconds>());

        bool first_line{true};
        if (!settings.pids.empty()) {
            std::cout << fmt::format("{:<12}|", " ");
        }
        if (settings.use_cpu_stats) {
            for (size_t i = 0; i < n_cpus; i++) {
                double val = (util_list.back()[i] * 100.0);
                std::cout << fmt::format("{:>6.2f}% |", val);
            }
            first_line = false;
            std::cout << std::endl;
        }
        for (const auto& proc_stats: proc_stats_list) {
            if (!first_line) {
                std::cout << fmt::format("{:<13}|", " ");  // Skip timestamp for 2nd line and all the rest
            }
            std::cout << fmt::format("{:^12}|", proc_stats.pid);
            if (proc_stats.state != ProcStats::State::not_found) {
                if (proc_stats.cpu >= n_cpus) {
                    std::cerr << "Warning: bad CPU number " << proc_stats.cpu << std::endl;
                } else {
                    // Skip (I-1) cells
                    for (int i = 0; i < proc_stats.cpu; i++) {
                        std::cout << fmt::format("{:<8}|", " ");
                    }
                    std::cout << fmt::format("{:^8c}|", 'x');
                    for (int i = proc_stats.cpu + 1; i < n_cpus; i++) {
                        std::cout << fmt::format("{:<8}|", " ");
                    }
                    std::cout << " " << ToString(proc_stats.state) << std::endl;
                }
            } else {
                // Skip all CPUs columns
                for (int i = 0; i < n_cpus; i++) {
                    std::cout << fmt::format("{:<8}|", " ");
                }
                std::cout << " " << ToString(proc_stats.state) << std::endl;
            }
            first_line = false;
        }
        if (first_line) {
            std::cout << std::endl;
        }
    }
}
