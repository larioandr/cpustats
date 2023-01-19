#include "cpustats/managers/cpu_manager.hpp"
#include "cpustats/managers/pid_manager.hpp"
#include "cpustats/consumers/table.hpp"

#include <cxxopts.hpp>
#include <date.h>

#include <chrono>
#include <condition_variable>
#include <csignal>
#include <iostream>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

// ---- Global state -----
std::mutex should_stop_m{};
std::condition_variable should_stop_cv{};
bool should_stop{};


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


void MainLoop(
        int interval_ms,
        std::vector<std::shared_ptr<Manager>> const& managers_list,
        std::vector<std::shared_ptr<Consumer>> const& consumers_list
) {
    for (auto const& consumer: consumers_list) consumer->Start();
    while (true) {
        {
            std::unique_lock lock{should_stop_m};
            should_stop_cv.wait_for(
                    lock, std::chrono::milliseconds(interval_ms),
                    []() { return should_stop; });
            if (should_stop) break;
        }
        for (auto const& consumer: consumers_list) consumer->BeginIter();
        for (auto const& manager: managers_list) manager->Update();
        for (auto const& consumer: consumers_list) consumer->EndIter();
    }
    for (auto const& consumer: consumers_list) consumer->Finish();
}


void HandleSignal(int signo) {
    if (signo == SIGINT || signo == SIGTERM) {
        std::lock_guard lock{should_stop_m};
        should_stop = true;
        should_stop_cv.notify_all();
    }
}

template <class Precision>
std::string GetISOCurrentTime()
{
    auto now = std::chrono::system_clock::now();
    return date::format("%T", date::floor<Precision>(now));
}

int main(int argc, char **argv) {
    auto options = BuildOptions();
    auto parsed = options.parse(argc, argv);
    if (parsed.count("help")) {
        PrintHelp(options);
        std::exit(0);
    }
    auto settings = BuildSettings(parsed);
    std::cout << settings.String();

    std::vector<std::shared_ptr<Manager>> managers{};
    std::vector<std::shared_ptr<Consumer>> consumers{};

    /* Create managers, consumers and bind them */
    Table::Settings table_props{};
    table_props.show_cpu_stats = true;
    table_props.show_pid_stats = !settings.pids.empty();
    table_props.num_cpus = GetCpuCount();
    table_props.show_outer_delims = true;
    table_props.show_heading = true;
    auto table = std::make_shared<Table>(table_props);
    consumers.push_back(table);

    auto cpu_manager = std::make_shared<CpuManager>();
    cpu_manager->add_acceptor(dynamic_pointer_cast<CpuInfoAcceptor>(table));
    cpu_manager->add_acceptor(dynamic_pointer_cast<CpuUtilAcceptor>(table));
    managers.push_back(cpu_manager);

    std::shared_ptr<PidManager> pid_manager{};
    if (!settings.pids.empty()) {
        pid_manager = std::make_shared<PidManager>();
        pid_manager->add_acceptor(table);
        for (auto pid: settings.pids) {
            pid_manager->add_pid(pid);
        }
        managers.push_back(pid_manager);
    }

    /* Initialize managers */
    for (auto const& manager: managers) {
        manager->Init();
    }

    /* Setup signal handlers */
    std::signal(SIGINT, HandleSignal);
    std::signal(SIGTERM, HandleSignal);

    /* Start a worker thread */
    std::thread worker{[settings, &managers, &consumers](){
        MainLoop(settings.interval_ms, managers, consumers);
    }};

    /* Wait for worker to finish */
    worker.join();

    for (auto const& manager: managers) {
        manager->Finish();
    }



    //    size_t n_cpus = GetCpusCount();
//    std::vector<CpuStat> cpus1{};
//    std::vector<CpuStat> cpus2{};
//    std::vector<CpuStat> cpus_diff{};
//    std::vector<CpuStat> *curr_cpus = &cpus1;
//    std::vector<CpuStat> *prev_cpus = &cpus2;
//
//    {
//        std::cout << fmt::format("{:<13}|", "Time");
//        if (!settings.pids.empty()) {
//            std::cout << fmt::format("{:^12}|", "PID");
//        }
//        for (int i = 0; i < n_cpus; i++) {
//            std::cout << fmt::format("{:^8}|", fmt::format("cpu{}", i));
//        }
//        if (!settings.pids.empty()) {
//            std::cout << fmt::format("{:<12}", " Status");
//        }
//        std::cout << std::endl;
//    }
//
//
//    cpus1.resize(n_cpus);
//    cpus2.resize(n_cpus);
//    cpus_diff.resize(n_cpus);
//
//    std::vector<ProcStats> proc_stats_list{};
//    for (auto pid: settings.pids) {
//        proc_stats_list.push_back({.pid=pid});
//    }
//
//    ReadStats(*curr_cpus);
//    std::vector<std::vector<double>> util_list{};
//
//    while (true) {
//        std::this_thread::sleep_for(std::chrono::milliseconds(settings.interval_ms));
//
//        if (settings.use_cpu_stats) {
//            std::swap(curr_cpus, prev_cpus);
//            ReadStats(*curr_cpus);
//            util_list.emplace_back();
//            util_list.back().resize(n_cpus);
//            for (size_t i = 0; i < n_cpus; i++) {
//                CpuStat &cpu = cpus_diff[i];
//                Subtract((*curr_cpus)[i], (*prev_cpus)[i], cpu);
//                int total = std::accumulate(cpu.values.begin(), cpu.values.end(), 0);
//                int not_idle = total - *cpu.idle();
//                util_list.back()[i] = static_cast<double>(not_idle) / total;
//            }
//        }
//
//        for (auto& proc_stats: proc_stats_list) {
//            ReadProcStats(proc_stats.pid, proc_stats);
//        }
//
//        std::cout << fmt::format("{:<13}|", GetISOCurrentTime<std::chrono::milliseconds>());
//
//        bool first_line{true};
//        if (!settings.pids.empty()) {
//            std::cout << fmt::format("{:<12}|", " ");
//        }
//        if (settings.use_cpu_stats) {
//            for (size_t i = 0; i < n_cpus; i++) {
//                double val = (util_list.back()[i] * 100.0);
//                std::cout << fmt::format("{:>6.2f}% |", val);
//            }
//            first_line = false;
//            std::cout << std::endl;
//        }
//        for (const auto& proc_stats: proc_stats_list) {
//            if (!first_line) {
//                std::cout << fmt::format("{:<13}|", " ");  // Skip timestamp for 2nd line and all the rest
//            }
//            std::cout << fmt::format("{:^12}|", proc_stats.pid);
//            if (proc_stats.state != ProcStats::State::not_found) {
//                if (proc_stats.cpu >= n_cpus) {
//                    std::cerr << "Warning: bad CPU number " << proc_stats.cpu << std::endl;
//                } else {
//                    // Skip (I-1) cells
//                    for (int i = 0; i < proc_stats.cpu; i++) {
//                        std::cout << fmt::format("{:<8}|", " ");
//                    }
//                    std::cout << fmt::format("{:^8c}|", 'x');
//                    for (int i = proc_stats.cpu + 1; i < n_cpus; i++) {
//                        std::cout << fmt::format("{:<8}|", " ");
//                    }
//                    std::cout << " " << ToString(proc_stats.state) << std::endl;
//                }
//            } else {
//                // Skip all CPUs columns
//                for (int i = 0; i < n_cpus; i++) {
//                    std::cout << fmt::format("{:<8}|", " ");
//                }
//                std::cout << " " << ToString(proc_stats.state) << std::endl;
//            }
//            first_line = false;
//        }
//        if (first_line) {
//            std::cout << std::endl;
//        }
//    }
}
