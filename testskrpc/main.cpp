#include <iostream>
#include <chrono>
#include <thread>
#include <krpc.hpp>
#include <krpc/services/space_center.hpp>
#include "SubOrbitalFlight.hpp"

#include <cmath>

std::string vesselTypeToString(krpc::services::SpaceCenter::VesselType type) {
    switch (type) {
        case krpc::services::SpaceCenter::VesselType::ship:
            return "Ship";
        case krpc::services::SpaceCenter::VesselType::station:
            return "Station";
        case krpc::services::SpaceCenter::VesselType::probe:
            return "Probe";
        case krpc::services::SpaceCenter::VesselType::rover:
            return "Rover";
        case krpc::services::SpaceCenter::VesselType::lander:
            return "Lander";
        case krpc::services::SpaceCenter::VesselType::base:
            return "Base";
        case krpc::services::SpaceCenter::VesselType::plane:
            return "Plane";
        case krpc::services::SpaceCenter::VesselType::relay:
            return "Relay";
        default:
            return "Unknown";
    }
}

void listerVaisseauxEtSatellites() {
    krpc::Client conn = krpc::connect("Liste des vaisseaux");
    krpc::services::SpaceCenter centre_spatial(&conn);

    auto vaisseaux = centre_spatial.vessels();

    for (auto& vaisseau : vaisseaux) {
        std::string nom = vaisseau.name();
        auto type = vesselTypeToString(vaisseau.type());
        std::cout << "Nom: " << nom << ", Type: " << type << std::endl;
    }
}

int main() {

    float target_altitude = 80000;
    listerVaisseauxEtSatellites();
    DebugKSP debug;
    debug.display("hello");


    SubOrbitalFlight flight;
    flight.launch(target_altitude);

    //flight.deploy_sat();
    // flight initiate reentry to destination 
    // flight land safely to destination



    return 0;
}
