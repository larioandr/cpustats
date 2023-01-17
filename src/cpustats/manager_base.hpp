#ifndef CPUSTATS_MANAGER_BASE_HPP
#define CPUSTATS_MANAGER_BASE_HPP

#include <string>
#include <vector>

class Manager {
public:
    virtual void Init() = 0;
    virtual void Update() = 0;
    virtual void Finish() = 0;
};

#endif //CPUSTATS_MANAGER_BASE_HPP
