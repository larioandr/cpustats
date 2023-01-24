#ifndef CPUSTATS_CSV_OUTPUT_HPP
#define CPUSTATS_CSV_OUTPUT_HPP

#include "consumer_base.hpp"
#include "../managers/cpu_manager.hpp"
#include "../managers/pid_manager.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <optional>

class CsvWriterBase {
public:
    void set_stream(std::ofstream&& stream);
    void set_stream(std::ostream& stream);
    void set_delim(char delim);
    void enable_header(bool enabled);

    std::ostream& stream() const { return *stream_; }
    char delim() const { return delim_; }
    bool is_header_enabled() const { return header_enabled_; }

private:
    std::optional<std::ofstream> stored_ofstream_{};
    std::ostream *stream_{&std::cout};
    char delim_{';'};
    bool header_enabled_{true};
};

class CpuUtilCsvWriter : public CsvWriterBase, public Consumer, public CpuUtilAcceptor {
public:
    void set_num_cpus(int num_cpus);
    void set_normalize_cpu_utility(bool enabled) { normalize_cpu_utility_ = enabled; }

    bool Start() override;
    void BeginIter() override;
    void EndIter() override;
    void Finish() override;

    void Accept(CpuUtil const& value, bool last_in_cycle = false) override;
private:
    bool normalize_cpu_utility_{false};
    std::string iter_start_timestamp_{};
    std::vector<std::optional<double>> cpu_list_{};
};


class PidCpuCsvWriter : public CsvWriterBase, public Consumer, public PidStatAcceptor {
public:
    bool Start() override;
    void BeginIter() override;
    void EndIter() override;
    void Finish() override;

    void Accept(PidStat const& value, bool last_in_cycle = false) override;
private:
    std::string iter_start_timestamp_{};
};

#endif //CPUSTATS_CSV_OUTPUT_HPP
