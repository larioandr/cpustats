#ifndef CPUSTATS_CPU_MANAGER_HPP
#define CPUSTATS_CPU_MANAGER_HPP

#include <array>
#include <memory>
#include <vector>
#include <string>
#include <iostream>

#include "manager_base.hpp"
#include "linux_proc.hpp"


class CpuStatAcceptor {
public:
    virtual ~CpuStatAcceptor() = default;
    virtual void Accept(CpuStat const& value, bool last_in_iter = false) = 0;
};

class CpuInfoAcceptor {
public:
    virtual ~CpuInfoAcceptor() = default;
    virtual void Accept(CpuInfo const& value, bool last_in_iter = false) = 0;
};

struct CpuUtil {
    int cpu;
    double busy_rate;
    double idle_rate;
};

class CpuUtilAcceptor {
public:
    virtual ~CpuUtilAcceptor() = default;
    virtual void Accept(CpuUtil const& value, bool last_in_iter) = 0;
};


class CpuManager : public Manager {
public:
    void Init() override;
    void Update() override;
    void Finish() override;

    void add_acceptor(std::shared_ptr<CpuStatAcceptor> const& acceptor) {
        cpu_stat_acceptors_.push_back(acceptor);
    }

    void add_acceptor(std::shared_ptr<CpuInfoAcceptor> const& acceptor) {
        cpu_info_acceptors_.push_back(acceptor);
    }

    void add_acceptor(std::shared_ptr<CpuUtilAcceptor> const& acceptor) {
        cpu_util_acceptors_.push_back(acceptor);
    }
private:
    std::vector<std::shared_ptr<CpuStatAcceptor>> cpu_stat_acceptors_{};
    std::vector<std::shared_ptr<CpuInfoAcceptor>> cpu_info_acceptors_{};
    std::vector<std::shared_ptr<CpuUtilAcceptor>> cpu_util_acceptors_{};
    std::vector<CpuInfo> cpu_info_list_{};
    std::vector<CpuStat> cpu_stat_list_1_{};
    std::vector<CpuStat> cpu_stat_list_2_{};
    std::vector<CpuStat> *curr_cpu_stat_list_{&cpu_stat_list_1_};
    std::vector<CpuStat> *prev_cpu_stat_list_{&cpu_stat_list_2_};

    template<typename InputIt, typename AcceptorPtr>
    static void CallAcceptors(std::vector<AcceptorPtr> const& acceptors, InputIt begin, InputIt end) {
        for (auto it{begin}; it != end; ) {
            auto next_it = it + 1;
            bool last_in_cycle = next_it == end;
            for (auto& acceptor: acceptors) {
                acceptor->Accept(*it, last_in_cycle);
            }
            it = next_it;
        }
    }
};


#endif //CPUSTATS_CPU_MANAGER_HPP
