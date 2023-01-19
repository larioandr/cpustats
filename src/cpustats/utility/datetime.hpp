#ifndef CPUSTATS_DATETIME_HPP
#define CPUSTATS_DATETIME_HPP

#include <date.h>
#include <string>

template <class Precision>
std::string GetISOCurrentTime()
{
    auto now = std::chrono::system_clock::now();
    return date::format("%T", date::floor<Precision>(now));
}

#endif //CPUSTATS_DATETIME_HPP
