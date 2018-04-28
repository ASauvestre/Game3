#include "math_m.h"

double lerp(double a, double b, double t) {
    return a*(1-t) + b*t;
}
