#include <iostream>
#include <chrono>
#include <thread>
#include <krpc.hpp>
#include <krpc/services/space_center.hpp>
#include "SubOrbitalFlight.hpp"

#include <cmath>


int main() {

    float target_altitude = 80000;

    DebugKSP debug;
    debug.display();


    SubOrbitalFlight flight;
    flight.launch(target_altitude);

    //flight.deploy_sat();
    // flight initiate reentry to destination 
    // flight land safely to destination



    return 0;
}
