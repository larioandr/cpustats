#ifndef CPUSTATS_TABLE_HPP
#define CPUSTATS_TABLE_HPP

#include "cpu_manager.hpp"
#include "pid_manager.hpp"

#include <iostream>

class Table :
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
        std::string value{};
    };

    void use_cpu_stat(bool enabled) { use_cpu_stat_ = enabled; }
    void use_pid_stat(bool enabled) { use_pid_stat_ = enabled; }

    [[nodiscard]] bool use_cpu_stat() const { return use_cpu_stat_; }
    [[nodiscard]] bool use_pid_stat() const { return use_pid_stat_; }

    void OpenNewRow() {}
    void CloseAndPrintRow() {}

    void Accept(CpuInfo const& value, bool last_in_cycle = false) override { std::cout << value.cpu << ": " << value.model_name << std::endl; if (last_in_cycle) std::cout << "----------\n"; }
    void Accept(CpuUtil const& value, bool last_in_cycle = false) override { std::cout << value.cpu << ": " << value.busy_rate * 100.0 << "%" << std::endl; if (last_in_cycle) std::cout << "----------\n"; }
    void Accept(PidStat const& value, bool last_in_cycle = false) override { std::cout << value.pid << ": " << value.cpu << std::endl; if (last_in_cycle) std::cout << "----------\n"; }

private:
    bool use_cpu_stat_{false};
    bool use_pid_stat_{false};

    std::vector<Col> columns{};
    std::vector<Cell> row{};

    Col const& time_col() const {return columns.at(0);}
    Col const& pid_col() const {return columns.at(0);}
    Col const& cpu_col(int cpu) const {return columns.at(0);}
    Col const& pid_status_col() const {return columns.at(0);}
};

#endif //CPUSTATS_TABLE_HPP
