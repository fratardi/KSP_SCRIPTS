#include <iostream>
#include <chrono>
#include <thread>
#include <krpc.hpp>
#include <krpc/services/space_center.hpp>
#include "SubOrbitalFlight.hpp"

#include <cmath>


int main() {
    SubOrbitalFlight flight;
    flight.launch();
    return 0;
}
