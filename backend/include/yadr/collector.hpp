#pragma once

#include <chrono>

#include "yadr/snapshot.hpp"

namespace yadr {

// базовый класс сборщиков -- каждая реализация хранит состояние между тиками
// (счётчики с предыдущего чтения) для вычисления скоростей
class Collector {
public:
    virtual ~Collector() = default;
    virtual void sample(Snapshot& out, double dt_s) = 0;
};

}  // namespace yadr
