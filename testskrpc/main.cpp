#include <iostream>
#include <chrono>
#include <thread>
#include <krpc.hpp>
#include <krpc/services/space_center.hpp>
#include "SubOrbitalFlight.hpp"

#include <cmath>


int main() {

    // DebugKSP debug;
    // debug.display();


    SubOrbitalFlight flight;
    flight.launch(85000);
    return 0;
}
