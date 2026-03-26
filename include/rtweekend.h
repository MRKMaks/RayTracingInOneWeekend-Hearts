#ifndef RTWEEKEND_H
#define RTWEEKEND_H

#include <cmath>
#include <limits>
#include <memory>
#include <cstdlib>
#include <random> // Essential for modern C++ random generation

using std::shared_ptr;
using std::make_shared;
using std::sqrt;

const double infinity = std::numeric_limits<double>::infinity();
const double pi = 3.1415926535897932385;

inline double degrees_to_radians(double degrees) {
    return degrees * pi / 180.0;
}

inline double random_double() {
    // thread_local ensures each OpenMP thread gets its own generator instance without cross-thread data races
    static thread_local std::uniform_real_distribution<double> distribution(0.0, 1.0);
    // Initialize the generator with a true hardware random seed (std::random_device)
    static thread_local std::mt19937 generator(std::random_device{}());
    return distribution(generator);
}

inline double random_double(double min, double max) {
    return min + (max - min) * random_double();
}

#include "color.h"
#include "ray.h"
#include "vec3.h"

#endif
