#ifndef CPUSTATS_TABLE_HPP
#define CPUSTATS_TABLE_HPP

#include "../managers/cpu_manager.hpp"
#include "../managers/pid_manager.hpp"
#include "consumer_base.hpp"

#include <iostream>
#include <optional>

class Table :
        public Consumer,
        public CpuUtilAcceptor,
        public CpuInfoAcceptor,
        public PidStatAcceptor {
public:
    struct Col {
        int index;
        std::string head{};
        size_t width{};
    };

    struct Cell {
        int col;
        std::optional<std::string> value{};
    };

    struct Settings {
        bool show_heading{true};
        bool show_cpu_stats{true};
        bool show_pid_stats{false};
        char delim{'|'};
        bool show_divider{true};
        bool show_outer_delims{false};
        int num_cpus{};
        bool normalize_cpu_utility{false};
    };

    explicit Table(Settings const& settings);

    Settings const& settings() const { return settings_; }

    bool Start() override;
    void BeginIter() override;
    void EndIter() override;
    void Finish() override;

    void Accept(CpuInfo const& value, bool last_in_cycle = false) override;
    void Accept(CpuUtil const& value, bool last_in_cycle = false) override;
    void Accept(PidStat const& value, bool last_in_cycle = false) override;

    size_t full_width() const;

private:
    Settings settings_{};

    std::vector<Col> columns_{};
    std::vector<Cell> row_{};
    mutable std::optional<size_t> full_width_;
    bool empty_row_{};

    Col const& time_col() const;
    Col const& pid_col() const;
    Col const& cpu_col(int cpu) const;
    Col const& pid_status_col() const;

    void PrintRow();

    /** Prints end-of-line if row is not empty or `force = true` */
    void PrintEOL(bool force = false);

    void PrintDivider();
};

#endif //CPUSTATS_TABLE_HPP
