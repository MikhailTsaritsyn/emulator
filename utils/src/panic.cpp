//
// Created by Mikhail Tsaritsyn on Apr 25, 2025.
//

#include "mtl/panic.hpp"

#include <iostream>

namespace mtl {
void panic(const std::string_view message) {
    std::cerr << message << std::endl;
    __builtin_trap();
}
} // namespace mtl
