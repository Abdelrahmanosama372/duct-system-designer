#pragma once

#include <stdexcept>

namespace dsd::sizing {

class NoSolutionWithinBounds : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
};

class NumericalConvergenceError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

} // namespace dsd::sizing
