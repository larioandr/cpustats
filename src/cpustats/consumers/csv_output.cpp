#include "csv_output.hpp"
#include "../utility/datetime.hpp"

#include <fmt/format.h>

#include <sstream>


// --------------------------------------------------------------------------
// CsvWriterBase
// --------------------------------------------------------------------------
void CsvWriterBase::set_stream(std::ostream &stream) {
    stream_ = &stream;
    stored_ofstream_ = std::nullopt;
}

void CsvWriterBase::set_stream(std::ofstream &&stream) {
    stored_ofstream_ = std::move(stream);
    stream_ = &stored_ofstream_.value();
}

void CsvWriterBase::set_delim(char delim) {
    delim_ = delim;
}

void CsvWriterBase::enable_header(bool enabled) {
    header_enabled_ = enabled;
}


// --------------------------------------------------------------------------
// CpuUtilCsvWriter
// --------------------------------------------------------------------------
void CpuUtilCsvWriter::set_num_cpus(int num_cpus) {
    cpu_list_.resize(num_cpus);
}

bool CpuUtilCsvWriter::Start() {
    if (is_header_enabled()) {
        stream() << "timestamp";
        for (int i{}; i < cpu_list_.size(); i++)
            stream() << delim() << fmt::format("cpu{}", i);
        stream() << std::endl;
    }
    return true;
}

void CpuUtilCsvWriter::BeginIter() {
    iter_start_timestamp_ = GetISOCurrentTime<std::chrono::milliseconds>();
    for (auto& cpu: cpu_list_) {
        cpu = std::nullopt;
    }
}

void CpuUtilCsvWriter::EndIter() {
    std::stringstream ss;
    ss << iter_start_timestamp_;
    for (auto const& cpu: cpu_list_) {
        ss << delim();
        if (cpu) {
            if (normalize_cpu_utility_) {
                ss << fmt::format("{:.5f}", *cpu);
            } else {
                ss << fmt::format("{:.2f}", *cpu * 100);
            }
        }
    }
    ss << std::endl;
    stream() << ss.str();
    stream().flush();
}

void CpuUtilCsvWriter::Finish() {
}

void CpuUtilCsvWriter::Accept(CpuUtil const& value, bool last_in_cycle) {
    if (value.cpu >= 0 && value.cpu < cpu_list_.size()) {
        cpu_list_[value.cpu] = value.busy_rate;
    }
}


// --------------------------------------------------------------------------
// PidCpuCsvWriter
// --------------------------------------------------------------------------
bool PidCpuCsvWriter::Start() {
    if (is_header_enabled()) {
        stream() << "timestamp"
            << delim() << "pid"
            << delim() << "cpu"
            << delim() << "state"
            << std::endl;
        stream().flush();
    }
    return true;
}

void PidCpuCsvWriter::BeginIter() {
    iter_start_timestamp_ = GetISOCurrentTime<std::chrono::milliseconds>();
}

void PidCpuCsvWriter::EndIter() {
    stream().flush();
}

void PidCpuCsvWriter::Finish() {}

void PidCpuCsvWriter::Accept(PidStat const& value, bool _) {
    stream() << iter_start_timestamp_
        << delim() << value.pid
        << delim() << value.cpu
        << delim() << ToString(value.state)
        << std::endl;
}
