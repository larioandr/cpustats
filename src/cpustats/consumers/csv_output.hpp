#ifndef CPUSTATS_CSV_OUTPUT_HPP
#define CPUSTATS_CSV_OUTPUT_HPP

#include "consumer_base.hpp"
#include "../managers/cpu_manager.hpp"
#include "../managers/pid_manager.hpp"

#include <chrono>
#include <fstream>
#include <optional>

class CpuUtilCsvWriter : public Consumer, public CpuUtilAcceptor {
public:
    struct Settings {
        std::string file_name;
        int num_cpus{1};
        char delim{';'};
        bool write_header{true};
    };

    explicit CpuUtilCsvWriter(Settings const& settings);

    bool Start() override;
    void BeginIter() override;
    void EndIter() override;
    void Finish() override;

    void Accept(CpuUtil const& value, bool last_in_cycle = false) override;
private:
    Settings settings_{};
    std::string iter_start_timestamp_{};
    std::ofstream stream_{};
    std::vector<std::optional<double>> cpu_list_{};
};


class PidCpuCsvWriter : public Consumer, public PidStatAcceptor {
public:
    struct Settings {
        std::string file_name;
        char delim{';'};
    };
};

#endif //CPUSTATS_CSV_OUTPUT_HPP
