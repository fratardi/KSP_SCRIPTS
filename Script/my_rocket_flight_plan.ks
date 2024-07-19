
/// trying to set global structure 



FUNCTION PRINT_GLOBAL_DATA {
    PARAMETER FUNCTION_NAME.
    LOCAL Y TO 19.
    LOCAL X TO 17.
    PRINT FUNCTION_NAME AT(X ,Y + 1).
    PRINT "SHIP:APOAPSIS: " AT(X, Y +2).
    PRINT SHIP:APOAPSIS AT(X + 16, Y + 2).
    PRINT "ORBIT APOAPSIS: " AT(X, Y + 3).
    PRINT ORBIT:APOAPSIS AT(X + 16, Y + 3).
    PRINT "SHIP PERIAPSIS: " AT(X, Y + 4).
    PRINT SHIP:PERIAPSIS AT(X + 16, Y + 4).
    // Print orbit's periapsis if there's a specific condition to check
    PRINT "ORBIT PERIAPSIS: " AT(X, Y + 5).
    PRINT ORBIT:PERIAPSIS AT(X + 16, Y + 5).
    PRINT "SHIP:ALTITUDE: " AT(X, Y + 6).
    PRINT SHIP:ALTITUDE AT(X +16, Y + 6).
    PRINT "THROTTLE: " AT(X, Y + 7).
    PRINT THROTTLE AT(X+16, Y + 7).
    PRINT "ECCENTRICITY: " AT(X, Y + 8).
    PRINT ORBIT:ECCENTRICITY AT(X+16, Y + 8).
}




//hellolaunch

//First, we'll clear the terminal screen to make it look nice
CLEARSCREEN.
print "FLIGHT_PROGRAM_LOADED".
//Next, we'll lock our throttle to 100%.
LOCK THROTTLE TO 1.0.   // 1.0 is the max, 0.0 is idle.

//This is our countdown loop, which cycles from 10 to 0
PRINT "Counting down:".
FROM {local countdown is 3.} UNTIL countdown = 0 STEP {SET countdown to countdown - 1.} DO {
    PRINT "..." + countdown.
    WAIT 1. // pauses the script here for 1 second.
}

//This is a trigger that constantly checks to see if our thrust is zero.
//If it is, it will attempt to stage and then return to where the script
//left off. The PRESERVE keyword keeps the trigger active even after it
//has been triggered.
WHEN MAXTHRUST = 0 THEN {
    PRINT "Staging".
    STAGE.
    PRESERVE.
}.


WHEN SHIP:PERIAPSIS = SHIP:ALTITUDE THEN {
    LOGTOLOGPATH("DO  MANOEUVER HERE PERIAPSIS "  +"VELOCITY"+ SHIP:VELOCITY ).

}

WHEN SHIP:ALTITUDE = ORBIT:APOAPSIS THEN {
    LOGTOLOGPATH("DO  MANOEUVER HERE APOAPSIS" +"VELOCITY"+ SHIP:VELOCITY).
}


//This will be our main control loop for the ascent. It will
//cycle through continuously until our apoapsis is greater
//than 100km. Each cycle, it will check each of the IF
//statements inside and perform them if their conditions
//are met

SET MYSTEER TO HEADING(90,90).
LOCK STEERING TO MYSTEER. // from now on we'll be able to change steering by just assigning a new value to MYSTEER
UNTIL SHIP:APOAPSIS > 100000 { //Remember, all altitudes will be in meters, not kilometers
 PRINT_GLOBAL_DATA("UNTIL  SHIP:APOAPSIS > 100000").
    //For the initial ascent, we want our steering to be straight
    //up and rolled due east
    IF SHIP:VELOCITY:SURFACE:MAG < 100 {
        //This sets our steering 90 degrees up and yawed to the compass
        //heading of 90 degrees (east)
        SET MYSTEER TO HEADING(90,90).

    //Once we pass 100m/s, we want to pitch down ten degrees
    } ELSE IF SHIP:VELOCITY:SURFACE:MAG >= 100 AND SHIP:VELOCITY:SURFACE:MAG < 200 {
        SET MYSTEER TO HEADING(90,80).
        PRINT "Pitching to 80 degrees" AT(0,15).
        PRINT ROUND(SHIP:APOAPSIS,0) AT (0,16).

    //Each successive IF statement checks to see if our velocity
    //is within a 100m/s block and adjusts our heading down another
    //ten degrees if so
    } ELSE IF SHIP:VELOCITY:SURFACE:MAG >= 200 AND SHIP:VELOCITY:SURFACE:MAG < 300 {
        SET MYSTEER TO HEADING(90,70).
        PRINT "Pitching to 70 degrees" AT(0,15).
        PRINT ROUND(SHIP:APOAPSIS,0) AT (0,16).

    } ELSE IF SHIP:VELOCITY:SURFACE:MAG >= 300 AND SHIP:VELOCITY:SURFACE:MAG < 400 {
        SET MYSTEER TO HEADING(90,60).
        PRINT "Pitching to 60 degrees" AT(0,15).
        PRINT ROUND(SHIP:APOAPSIS,0) AT (0,16).

    } ELSE IF SHIP:VELOCITY:SURFACE:MAG >= 400 AND SHIP:VELOCITY:SURFACE:MAG < 500 {
        SET MYSTEER TO HEADING(90,50).
        PRINT "Pitching to 50 degrees" AT(0,15).
        PRINT ROUND(SHIP:APOAPSIS,0) AT (0,16).

    } ELSE IF SHIP:VELOCITY:SURFACE:MAG >= 500 AND SHIP:VELOCITY:SURFACE:MAG < 600 {
        SET MYSTEER TO HEADING(90,40).
        PRINT "Pitching to 40 degrees" AT(0,15).
        PRINT ROUND(SHIP:APOAPSIS,0) AT (0,16).

    } ELSE IF SHIP:VELOCITY:SURFACE:MAG >= 600 AND SHIP:VELOCITY:SURFACE:MAG < 800 {
        SET MYSTEER TO HEADING(90,30).
        PRINT "Pitching to 30 degrees" AT(0,15).
        PRINT ROUND(SHIP:APOAPSIS,0) AT (0,16).

    } ELSE IF SHIP:VELOCITY:SURFACE:MAG >= 800 AND SHIP:VELOCITY:SURFACE:MAG < 900 {
        SET MYSTEER TO HEADING(90,20).
        PRINT "Pitching to 20 degrees" AT(0,15).
        PRINT ROUND(SHIP:APOAPSIS,0) AT (0,16).

    //Beyond 800m/s, we can keep facing towards 10 degrees above the horizon and wait
    //for the main loop to recognize that our apoapsis is above 100km
    } ELSE IF SHIP:VELOCITY:SURFACE:MAG >= 900 {
        SET MYSTEER TO HEADING(90,15).
        PRINT "Pitching to 10 degrees" AT(0,15).
        PRINT ROUND(SHIP:APOAPSIS,0) AT (0,16).
        LOCK THROTTLE TO 0.9.
    }.

}.



UNTIL SHIP:APOAPSIS >= 150000  {    
        IF ORBIT:APOAPSIS <= 120000 {
            LOCK THROTTLE TO 1.
        }
        SET MYSTEER TO SHIP:PROGRADE.
        PRINT_GLOBAL_DATA("UNTIL  SHIP:APOAPSIS >= 130000").
}


PRINT_GLOBAL_DATA("LOCK THROTTLE                           ").
LOCK THROTTLE TO 0.


// //// placer deploiement satellite ici 


UNTIL  ORBIT:PERIAPSIS >= 90000 {
  
        SET MYSTEER TO SHIP:PROGRADE.

        IF ORBIT:PERIAPSIS > 1 {
            LOCK THROTTLE TO 1.
        }
        IF ORBIT:PERIAPSIS >= 100000 {
        LOCK THROTTLE TO 0.1.
        }
        PRINT_GLOBAL_DATA("SHIP:ALTITUDE >= ORBIT:APOAPSIS              ").
}

lOCK THROTTLE TO 0.
PRINT_GLOBAL_DATA("PASSE AU DESSUS BOUCLE               ").

LOGTOLOGPATH(SHIP).
LOGTOLOGPATH(ORBIT).

PRINT "END".

