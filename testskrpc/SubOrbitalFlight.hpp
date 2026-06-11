#ifndef SUBORBITAL_FLIGHT_HPP
#define SUBORBITAL_FLIGHT_HPP

#include <memory>
#include <tuple>
#include <krpc.hpp>
#include <krpc/services/space_center.hpp>


class SubOrbitalFlight {
public:
    // Defaults: low Kerbin orbit at 80 km (10 km above the atmosphere edge).
    // The gravity turn finishes well inside the atmosphere; circularization
    // happens at apoapsis safely above the atmosphere boundary.
    // Defaults tuned for Kerbin → 80 km circular:
    //  - turn starts almost immediately so we don't waste dv going vertical
    //  - turn ends at 45 km using a sqrt curve so the ship is already mostly
    //    horizontal by ~20 km; classic "vertical apoapsis" trajectories
    //    (turn ending at 45 km on a linear curve) leave residual radial
    //    velocity and a 4×-too-large circularization burn.
    SubOrbitalFlight(float turn_start_altitude =   250.0f,
                     float turn_end_altitude   = 35000.0f,
                     float target_altitude     = 80000.0f);
    ~SubOrbitalFlight();

    // Entry point: orchestrates every step of the flight
    void launch();

    // --- Externalized steps ---
    void connect();
    void setupTelemetryStreams();
    void preLaunchSetup();
    void countdown();
    void engageAutopilot();
    void ascentLoop();
    void coastToTargetApoapsis();
    void coastOutOfAtmosphere();
    void planCircularizationBurn();
    void orientateForBurn();
    void waitUntilBurn();
    void executeBurn();
    void deploySatellite();
    void deorbitLauncher();

private:
    // Flight parameters
    float turn_start_altitude_;
    float turn_end_altitude_;
    float target_altitude_;

    // kRPC connection / objects
    std::unique_ptr<krpc::Client>                       conn_;
    std::unique_ptr<krpc::services::SpaceCenter>        space_center_;
    std::unique_ptr<krpc::services::SpaceCenter::Vessel> vessel_;

    // Telemetry streams
    std::unique_ptr<krpc::Stream<double>> ut_stream_;
    std::unique_ptr<krpc::Stream<double>> altitude_stream_;
    std::unique_ptr<krpc::Stream<double>> apoapsis_stream_;
    std::unique_ptr<krpc::Stream<float>>  srb_fuel_stream_;
    std::unique_ptr<krpc::Stream<double>> time_to_apoapsis_stream_;

    // Circularization burn data (computed in planCircularizationBurn)
    std::unique_ptr<krpc::services::SpaceCenter::Node> node_;
    double delta_v_;
    double burn_time_;
};

#endif // SUBORBITAL_FLIGHT_HPP
