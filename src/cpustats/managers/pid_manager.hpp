#ifndef CPUSTATS_PID_MANAGER_HPP
#define CPUSTATS_PID_MANAGER_HPP

#include "manager_base.hpp"
#include "../system/linux_proc.hpp"

#include <memory>


class PidStatAcceptor {
public:
    virtual ~PidStatAcceptor() = default;
    virtual void Accept(PidStat const& value, bool last_in_cycle = false) = 0;
};


class PidManager : public Manager {
public:
    void set_track_all(bool enabled) { track_all_ = enabled; }
    [[nodiscard]] bool track_all() const { return track_all_; }

    void Init() override;
    void Update() override;
    void Finish() override;

    void add_acceptor(std::shared_ptr<PidStatAcceptor> acceptor) {
        acceptors_.push_back(std::move(acceptor));
    }

    void add_pid(int pid) {
        pids_list_.push_back(pid);
    }

private:
    std::vector<std::shared_ptr<PidStatAcceptor>> acceptors_{};
    std::vector<int> pids_list_{};
    bool track_all_{false};
};




#endif //CPUSTATS_PID_MANAGER_HPP
