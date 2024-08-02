#include <iostream>
#include <chrono>
#include <cmath>
#include <thread>
#include <krpc.hpp>
#include <krpc/services/space_center.hpp>
#include "SubOrbitalFlight.hpp"
#include <krpc/services/ui.hpp>
#include <krpc/services/drawing.hpp>




krpc::services::SpaceCenter::Vessel init_flight_params(krpc::services::SpaceCenter::Vessel vessel)
{
  vessel.control().set_sas(false);
  vessel.control().set_rcs(false);
  vessel.control().set_throttle(1);
  return(vessel);
}



void execute_node(krpc::services::SpaceCenter::Control control, krpc::services::SpaceCenter::Node node) {


auto burn_vector = node.remaining_burn_vector(node.reference_frame());
double norm = std::sqrt(std::get<0>(burn_vector) * std::get<0>(burn_vector) +
                        std::get<1>(burn_vector) * std::get<1>(burn_vector) +
                        std::get<2>(burn_vector) * std::get<2>(burn_vector));





    control.set_throttle(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Wait for engines to stabilize
    while (norm > 0.1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    control.set_throttle(0);
    node.remove();
}


void perform_rendezvous(krpc::services::SpaceCenter::Vessel& vessel, krpc::services::SpaceCenter::Vessel& target_vessel) {
    try {
        krpc::Client conn = krpc::connect("Launch into orbit");
        krpc::services::SpaceCenter space_center(&conn);

        double mu = vessel.orbit().body().gravitational_parameter();
        double r1 = vessel.orbit().apoapsis();
        double r2 = target_vessel.orbit().apoapsis();

        // Calculating the semi-major axis of the transfer orbit
        // double a_transfer = (r1 + r2) / 2;

        // Calculate delta-v for the burns
        double delta_v1 = std::sqrt(mu/r1) * (std::sqrt(2*r2/(r1+r2)) - 1);
        double delta_v2 = std::sqrt(mu/r2) * (1 - std::sqrt(2*r1/(r1+r2)));

        std::cout << "Planning first burn at apoapsis." << std::endl;
        auto node1 = vessel.control().add_node(space_center.ut() + vessel.orbit().time_to_apoapsis(), delta_v1, 0, 0);
        execute_node(vessel.control(), node1);

        std::cout << "Planning second burn at periapsis." << std::endl;
        auto node2 = vessel.control().add_node(space_center.ut() + vessel.orbit().time_to_periapsis() + vessel.orbit().period() / 2, delta_v2, 0, 0);
        execute_node(vessel.control(), node2);

        std::cout << "Rendezvous maneuver completed." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
    }
}



void SubOrbitalFlight::launch( float target_altitude ) {


  // DebugKSP debug;

    // Logique de lancement
    std::cout << "Launching the vessel." << std::endl;

  krpc::Client conn = krpc::connect("Launch into orbit");
  krpc::services::SpaceCenter space_center(&conn);
  
  // debug.display();
  
  
  auto vessel = space_center.active_vessel();


  float turn_start_altitude = TURN_START_ALTITUDE;
  float turn_end_altitude = TURN_END_ALTITUDE;
(krpc::services::SpaceCenter::Vessel)vessel = vessel;
  // Set up streams for telemetry
  auto ut = space_center.ut_stream();
  auto altitude = vessel.flight().mean_altitude_stream();
  auto apoapsis = vessel.orbit().apoapsis_altitude_stream();
  auto stage_2_resources = vessel.resources_in_decouple_stage(2, false);
  auto srb_fuel = stage_2_resources.amount_stream("SolidFuel");


  vessel = init_flight_params(  vessel);


  // Pre-launch setup
  // vessel.control().set_sas(false);
  // vessel.control().set_rcs(false);
  // vessel.control().set_throttle(1);


  // Activate the first stage
  vessel.control().activate_next_stage();
  vessel.auto_pilot().engage();
  vessel.auto_pilot().target_pitch_and_heading(90, 90);

  // Main ascent loop
  bool srbs_separated = false;
  double turn_angle = 0;
  while (true) {
    // Gravity turn
    if (altitude() > turn_start_altitude && altitude() < turn_end_altitude) {
      double frac = (altitude() - turn_start_altitude)
                    / (turn_end_altitude - turn_start_altitude);
      double new_turn_angle = frac * 90.0;
      if (std::abs(new_turn_angle - turn_angle) > 0.5) {
        turn_angle = new_turn_angle;
        vessel.auto_pilot().target_pitch_and_heading(90.0 - turn_angle, 90.0);
      }
    }

    // Separate SRBs when finished
    if (!srbs_separated) {
      if (srb_fuel() < 0.1) {
        vessel.control().activate_next_stage();
        srbs_separated = true;
        std::cout << "SRBs separated" << std::endl;
      }
    }

    // Decrease throttle when approaching target apoapsis
    if (apoapsis() > target_altitude * 0.9) {
      std::cout << "Approaching target apoapsis" << std::endl;
      break;
    }
  }

  // Disable engines when target apoapsis is reached
  vessel.control().set_throttle(0.25);
  while (apoapsis() < target_altitude) {
  }
  std::cout << "Target apoapsis reached" << std::endl;
  vessel.control().set_throttle(0);

  // Wait until out of atmosphere
  std::cout << "Coasting out of atmosphere" << std::endl;
  while (altitude() < 70500) {
  }

  // Plan circularization burn (using vis-viva equation)
  std::cout << "Planning circularization burn" << std::endl;
  double mu = vessel.orbit().body().gravitational_parameter();
  double r = vessel.orbit().apoapsis();
  double a1 = vessel.orbit().semi_major_axis();
  double a2 = r;
  double v1 = std::sqrt(mu * ((2.0 / r) - (1.0 / a1)));
  double v2 = std::sqrt(mu * ((2.0 / r) - (1.0 / a2)));
  double delta_v = v2 - v1;
  auto node = vessel.control().add_node(
    ut() + vessel.orbit().time_to_apoapsis(), delta_v);

  // Calculate burn time (using rocket equation)
  double F = vessel.available_thrust();
  double Isp = vessel.specific_impulse() * 9.82;
  double m0 = vessel.mass();
  double m1 = m0 / std::exp(delta_v / Isp);
  double flow_rate = F / Isp;
  double burn_time = (m0 - m1) / flow_rate;

  // Orientate ship
  std::cout << "Orientating ship for circularization burn" << std::endl;
  vessel.auto_pilot().set_reference_frame(node.reference_frame());
  vessel.auto_pilot().set_target_direction(std::make_tuple(0.0, 1.0, 0.0));
  vessel.auto_pilot().wait();

  // Wait until burn
  std::cout << "Waiting until circularization burn" << std::endl;
  double burn_ut = ut() + vessel.orbit().time_to_apoapsis() - (burn_time / 2.0);
  double lead_time = 5;
  space_center.warp_to(burn_ut - lead_time);

  // Execute burn
  std::cout << "Ready to execute burn" << std::endl;
  auto time_to_apoapsis = vessel.orbit().time_to_apoapsis_stream();
  while (time_to_apoapsis() - (burn_time / 2.0) > 0) {
  }
  std::cout << "Executing burn" << std::endl;
  vessel.control().set_throttle(1);
  std::this_thread::sleep_for(
    std::chrono::milliseconds(static_cast<int>((burn_time - 0.1) * 1000)));
  std::cout << "Fine tuning" << std::endl;
  vessel.control().set_throttle(0.05);
  auto remaining_burn = node.remaining_burn_vector_stream(node.reference_frame());


  std::cout << node.delta_v() << std::endl;
  std::cout << node.delta_v() << std::endl;
  while (std::get<0>(remaining_burn()) > 0) {
  }


  vessel.control().set_throttle(0);
  node.remove();

  std::cout << "Launch complete" << std::endl;






} 