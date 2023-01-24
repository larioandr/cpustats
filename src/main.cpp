#include "cpustats/managers/cpu_manager.hpp"
#include "cpustats/managers/pid_manager.hpp"
#include "cpustats/consumers/table.hpp"
#include "cpustats/consumers/csv_output.hpp"

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
    bool all_pids{false};
    bool normalize_cpu_utility{false};

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
            ("P,all-pids", "Track CPUs assigned to all processes or threads", cxxopts::value<bool>()->default_value("false"))
            ("f,file", "Base name for CSV files where to record results", cxxopts::value<std::string>()->default_value(""))
            ("cpu-file", "CSV file name to record CPU stats", cxxopts::value<std::string>()->default_value(""))
            ("pid-file", "CSV file name to record PID stats", cxxopts::value<std::string>()->default_value(""))
            ("ncu,normalize-cpu-utility", "Write CPU load in normal form, 0 <= utility <= 1, instead of percents",
                    cxxopts::value<bool>()->default_value("false"))
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
    if (args.count("all-pids")) {
        settings.all_pids = true;
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
    if (args.count("normalize-cpu-utility")) {
        settings.normalize_cpu_utility = true;
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
    int num_cpus = GetCpuCount();

    /*
     * Create managers
     */
    auto cpu_manager = std::make_shared<CpuManager>();
    managers.push_back(cpu_manager);

    std::shared_ptr<PidManager> pid_manager{};
    if (!settings.pids.empty() || settings.all_pids) {
        pid_manager = std::make_shared<PidManager>();
        for (auto pid: settings.pids) {
            pid_manager->add_pid(pid);
        }
        if (settings.all_pids) {
            pid_manager->set_track_all(true);
        }
        managers.push_back(pid_manager);
    }

    /* Create consumers */
    // 1) Table
    Table::Settings table_props{};
    table_props.show_cpu_stats = true;
    table_props.show_pid_stats = !settings.pids.empty() || settings.all_pids;
    table_props.num_cpus = num_cpus;
    table_props.show_outer_delims = true;
    table_props.show_heading = true;
    table_props.show_divider = false;
    table_props.normalize_cpu_utility = settings.normalize_cpu_utility;
    auto table = std::make_shared<Table>(table_props);
    consumers.push_back(table);

    // 2) CPU utility CSV
    std::shared_ptr<CpuUtilCsvWriter> cpu_util_csv{};
    if (!settings.cpu_stats_file_name.empty()) {
        cpu_util_csv = std::make_shared<CpuUtilCsvWriter>();
        cpu_util_csv->set_stream(std::ofstream(settings.cpu_stats_file_name, std::ios::out));
        cpu_util_csv->enable_header(true);
        cpu_util_csv->set_num_cpus(num_cpus);
        cpu_util_csv->set_normalize_cpu_utility(settings.normalize_cpu_utility);
        consumers.push_back(cpu_util_csv);
    }

    // 3) PID CPU CSV
    std::shared_ptr<PidCpuCsvWriter> pid_cpu_csv{};
    if (!settings.pid_stats_file_name.empty()) {
        pid_cpu_csv = std::make_shared<PidCpuCsvWriter>();
        pid_cpu_csv->set_stream(std::ofstream{settings.pid_stats_file_name, std::ios::out});
        pid_cpu_csv->enable_header(true);
        consumers.push_back(pid_cpu_csv);
    }

    /* Bind consumers to managers */
    cpu_manager->add_acceptor(dynamic_pointer_cast<CpuInfoAcceptor>(table));
    cpu_manager->add_acceptor(dynamic_pointer_cast<CpuUtilAcceptor>(table));
    if (cpu_util_csv) {
        cpu_manager->add_acceptor(cpu_util_csv);
    }
    if (pid_manager) {
        pid_manager->add_acceptor(table);
        if (pid_cpu_csv) {
            pid_manager->add_acceptor(pid_cpu_csv);
        }
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
}
