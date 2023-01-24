#include "table.hpp"
#include "../utility/datetime.hpp"

#include <iostream>
#include <fmt/format.h>
#include <date.h>

Table::Table(const Settings &settings)
: settings_(settings) {
    // Build columns
    int index{};
    columns_.push_back({index++, "Timestamp", 14});
    if (settings.show_pid_stats) {
        columns_.push_back({index++, "PID", 8});
    }
    for (int i{}; i < settings.num_cpus; i++) {
        columns_.push_back({index++, fmt::format("cpu{}", i), 9});
    }
    if (settings.show_pid_stats) {
        columns_.push_back({index++, "Proc.status", 16});
    }
    // Build row
    for (auto const& col: columns_) {
        row_.push_back({col.index, std::nullopt});
    }
}

bool Table::Start() {
    if (settings_.show_heading) {
        for (int i{}; i < static_cast<int>(columns_.size()); i++) {
            auto const& col = columns_.at(i);
            row_[i].value = fmt::format("{:^{}s}", col.head, col.width);
        }
        empty_row_ = false;
        PrintRow();
        if (settings_.show_divider) {
            PrintDivider();
        }
    }
    return true;
}

void Table::BeginIter() {
    auto const& col = time_col();
    auto s_val = GetISOCurrentTime<std::chrono::milliseconds>();
    row_[col.index].value = fmt::format(" {:<{}s}", s_val, col.width-1);
    empty_row_ = false;
}

void Table::EndIter() {
    PrintRow();
    if (settings_.show_divider) {
        PrintDivider();
    }
}

void Table::Finish() {}


void Table::Accept(CpuInfo const& value, bool last_in_cycle) {
}

void Table::Accept(CpuUtil const& value, bool last_in_cycle) {
    if (!settings_.show_cpu_stats) return;
    auto const& col = cpu_col(value.cpu);
    std::string s_val;
    if (settings_.normalize_cpu_utility) {
        s_val = fmt::format("{:>7.5f}", value.busy_rate);
    } else {
        s_val = fmt::format("{:>6.2f}%", value.busy_rate * 100.0);
    }
    row_[col.index].value = fmt::format("{:^{}s}", s_val, col.width);
    empty_row_ = false;
    if (last_in_cycle) {
        PrintRow();
    }
}

void Table::Accept(PidStat const& value, bool last_in_cycle) {
    if (!settings_.show_pid_stats) return;
    auto const& c_pid = pid_col();
    auto const& c_status = pid_status_col();
    auto const& c_cpu = cpu_col(value.cpu);
    row_[c_pid.index].value = fmt::format("{:^{}d}", value.pid, c_pid.width);
    row_[c_cpu.index].value = fmt::format("{:^{}c}", 'x', c_cpu.width);
    row_[c_status.index].value = fmt::format(" {:<{}s}", ToString(value.state), c_status.width-1);
    empty_row_ = false;
    PrintRow();
}

size_t Table::full_width() const {
    if (full_width_) {
        return *full_width_;
    }
    *full_width_ = 0;
    for (auto const& col: columns_) {
        *full_width_ += (col.width + 1);
    }
    return *full_width_;
}

Table::Col const& Table::time_col() const {
    return columns_.at(0);
}

Table::Col const& Table::pid_col() const {
    if (settings_.show_pid_stats) {
        return columns_.at(1);
    } else {
        std::cerr << "Unexpected error: requested PID table column "
                     "while PID stats disabled\n";
        throw std::runtime_error("bad column");
    }
}

Table::Col const& Table::cpu_col(int cpu) const {
    if (settings_.show_cpu_stats) {
        return columns_.at((settings_.show_pid_stats ? 2 : 1) + cpu);
    } else {
        std::cerr << "Unexpected error: requested cpu" << cpu
                  << " table column while CPU stats disabled\n";
        throw std::runtime_error("bad column");
    }
}

Table::Col const& Table::pid_status_col() const {
    if (settings_.show_pid_stats) {
        return columns_.back();
    } else {
        std::cerr << "Unexpected error: requested process status "
                     "table column while PID stats disabled\n";
        throw std::runtime_error("bad column");
    }
}

void Table::PrintRow() {
    if (empty_row_) return;
    if (settings_.show_outer_delims) {
        std::cout << settings_.delim;
    }
    for (int i{}; i < row_.size(); i++) {
        auto& cell = row_[i];
        if (cell.value) {
            std::cout << *cell.value;
            cell.value = std::nullopt;
        } else {
            std::cout << fmt::format("{:<{}}", "", columns_[i].width);
        }
        if (settings_.show_outer_delims || (i+1) < row_.size()) {
            std::cout << settings_.delim;
        }
    }
    std::cout << std::endl;
    empty_row_ = true;
}

void Table::PrintEOL(bool force) {
    if (force || !empty_row_) {
        std::cout << std::endl;
        empty_row_ = true;
    }
}

void Table::PrintDivider() {
    PrintEOL();
    if (settings_.show_outer_delims) {
        std::cout << settings_.delim;
    }
    for (int i{}; i < columns_.size(); i++) {
        auto const& col = columns_.at(i);
        std::cout << fmt::format("{:-<{}}", "", col.width);
        if (settings_.show_outer_delims || (i+1) < row_.size()) {
            std::cout << "+";
        }
    }
    PrintEOL(true);
}
