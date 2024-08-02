#include <krpc.hpp>
#include <krpc/services/space_center.hpp>

#include <krpc/services/ui.hpp>
#include <krpc/services/drawing.hpp>




#ifndef SUBORBITAL_FLIGHT_HPP
#define SUBORBITAL_FLIGHT_HPP
#define SHIPNAME ""
#define TURN_START_ALTITUDE 250;
#define TURN_END_ALTITUDE 45000;
class SubOrbitalFlight {
public:
	void launch(float target_altitude);
};

typedef struct t_elemetry
{
  
void* ut;
void* altitude;
void* apoapsis;
void* stage_2_resources;
void* srb_fuel;

} t_elemetry;


class DebugKSP {
private:
    krpc::Client conn;
    krpc::services::SpaceCenter::Vessel vessel;
    krpc::services::UI::Canvas canvas;
    krpc::services::UI::Panel panel;

    // Handle button events
    void handleButtonEvents(krpc::Stream<bool>& throttleClicked, krpc::Stream<bool>& escapeClicked, krpc::services::UI::Text& text);
    // Update thrust text on UI
    void updateThrustText(krpc::services::UI::Text& text);

public:
    // DebugKSP(const std::string& name);
    void display(const std::string& name);
	void setupUI();
    // Constructor initializes the connection and UI components

};



#endif // SUBORBITAL_FLIGHT_HPP
