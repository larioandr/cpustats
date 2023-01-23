#include "csv_output.hpp"
#include "../utility/datetime.hpp"

#include <fmt/format.h>

#include <sstream>


// --------------------------------------------------------------------------
// CpuUtilCsvWriter
// --------------------------------------------------------------------------
CpuUtilCsvWriter::CpuUtilCsvWriter(Settings const& settings)
: settings_{settings} {
    cpu_list_.resize(settings.num_cpus, std::nullopt);
}

bool CpuUtilCsvWriter::Start() {
    stream_.open(settings_.file_name, std::ios::out);
    if (stream_.fail()) {
        std::cerr << "Failed to open file \"" << settings_.file_name
                  << "\" for writing" << std::endl;
        return false;
    }
    if (settings_.write_header) {
        stream_ << "timestamp";
        for (int i{}; i < cpu_list_.size(); i++)
            stream_ << settings_.delim << fmt::format("cpu{}", i);
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
        ss << settings_.delim;
        if (cpu)
            ss << fmt::format("{:.2f}", *cpu * 100);
    }
    ss << std::endl;
    stream_ << ss.str();
    stream_.flush();
}

void CpuUtilCsvWriter::Finish() {
}

void CpuUtilCsvWriter::Accept(CpuUtil const& value, bool last_in_cycle) {
    if (value.cpu >= 0 && value.cpu < cpu_list_.size()) {
        cpu_list_[value.cpu] = value.busy_rate;
    }
}
