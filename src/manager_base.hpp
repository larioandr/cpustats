#ifndef CPUSTATS_MANAGER_BASE_HPP
#define CPUSTATS_MANAGER_BASE_HPP

#include <string>
#include <vector>

class Manager {
public:
    struct Col {
        std::string title{};
        int max_width{};
    };

    virtual const std::vector<std::string>& columns() = 0;

    virtual void Init() = 0;
    virtual void Act() = 0;
    virtual void Finish() = 0;
};

#endif //CPUSTATS_MANAGER_BASE_HPP
