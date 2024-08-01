#include <iostream>
#include <chrono>
#include <cmath>
#include <thread>
#include <krpc.hpp>
#include <krpc/services/space_center.hpp>
#include "SubOrbitalFlight.hpp"

#include <krpc/services/ui.hpp>
#include <krpc/services/drawing.hpp>

void DebugKSP::display() {
  krpc::Client conn = krpc::connect("User Interface Example");
  krpc::services::SpaceCenter space_center(&conn);
  krpc::services::UI ui(&conn);
  auto canvas = ui.stock_canvas();

  // Get the size of the game window in pixels
  auto screen_size = canvas.rect_transform().size();

  // Add a panel to contain the UI elements
  auto panel = canvas.add_panel();

  // Position the panel on the left of the screen
  auto rect = panel.rect_transform();
  rect.set_size(std::make_tuple(200, 100));
  rect.set_position(std::make_tuple(110-(std::get<0>(screen_size)/2), 0));

  // Add a button to set the throttle to maximum
	auto throttle_button = panel.add_button("Full Throttle");
	throttle_button.rect_transform().set_position(std::make_tuple(0, 20));

  	auto escape_button = panel.add_button("Launching.");
  	escape_button.rect_transform().set_position(std::make_tuple(0, 122));
	auto escape_button_clicked = escape_button.clicked_stream();



  // Add some text displaying the total engine thrust
  auto text = panel.add_text("Thrust: 0 kN");
  text.rect_transform().set_position(std::make_tuple(0, -20));
  text.set_color(std::make_tuple(1, 1, 1));
  text.set_size(18);

  // Set up a stream to monitor the throttle button
  auto throttle_button_clicked = throttle_button.clicked_stream();



  auto vessel = space_center.active_vessel();
  while (true) {
	// Handle the throttle button being clicked
	if (throttle_button_clicked()) {
	  vessel.control().set_throttle(1);
	  throttle_button.set_clicked(false);
	}
	if (escape_button_clicked()) {
	  escape_button.set_clicked(false);
	    // Countdown...
  		std::cout << "3..." << std::endl;
  		std::this_thread::sleep_for(std::chrono::seconds(1));
  		std::cout << "2..." << std::endl;
  		std::this_thread::sleep_for(std::chrono::seconds(1));
  		std::cout << "1..." << std::endl;
  		std::this_thread::sleep_for(std::chrono::seconds(1));
  		std::cout << "Launch!" << std::endl;
	  break;
	}

	// Update the thrust text
	text.set_content("Thrust: " + std::to_string((int)(vessel.thrust()/1000)) + " kN");

	std::this_thread::sleep_for(std::chrono::seconds(1));
	
  }
}