#ifndef SUBORBITAL_FLIGHT_HPP
#define SUBORBITAL_FLIGHT_HPP

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



class DebugKSP{
	public:
		void display();
};


#endif // SUBORBITAL_FLIGHT_HPP
