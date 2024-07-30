// Début du script
CLEARSCREEN.
PRINT "FLIGHT_PROGRAM_LOADED OP".


HUDTEXT("lib_pid.ks has been superseded by the kOS inbuilt PIDloop() function.", 10, 2, 30, RED, FALSE).
wait 0.5.
HUDTEXT("It is maintained for example purposes only, please see the kOS documentation for more.", 10, 4, 30, RED, FALSE).

function PID_init {
  parameter
    Kp,      // gain of position
    Ki,      // gain of integral
    Kd,      // gain of derivative
    cMin,  // the bottom limit of the control range (to protect against integral windup)
    cMax.  // the the upper limit of the control range (to protect against integral windup)

  local SeekP is 0. // desired value for P (will get set later).
  local P is 0.     // phenomenon P being affected.
  local I is 0.     // crude approximation of Integral of P.
  local D is 0.     // crude approximation of Derivative of P.
  local oldT is -1. // (old time) start value flags the fact that it hasn't been calculated
  local oldInput is 0. // previous return value of PID controller.

  // Because we don't have proper user structures in kOS (yet?)
  // I'll store the PID tracking values in a list like so:
  //
  local PID_array is list(Kp, Ki, Kd, cMin, cMax, SeekP, P, I, D, oldT, oldInput).

  return PID_array.
}.

function PID_seek {
  parameter
    PID_array, // array built with PID_init.
    seekVal,   // value we want.
    curVal.    // value we currently have.

  // Using LIST() as a poor-man's struct.

  local Kp   is PID_array[0].
  local Ki   is PID_array[1].
  local Kd   is PID_array[2].
  local cMin is PID_array[3].
  local cMax is PID_array[4].
  local oldS   is PID_array[5].
  local oldP   is PID_array[6].
  local oldI   is PID_array[7].
  local oldD   is PID_array[8].
  local oldT   is PID_array[9]. // Old Time
  local oldInput is PID_array[10]. // prev return value, just in case we have to do nothing and return it again.

  local P is seekVal - curVal.
  local D is oldD. // default if we do no work this time.
  local I is oldI. // default if we do no work this time.
  local newInput is oldInput. // default if we do no work this time.

  local t is time:seconds.
  local dT is t - oldT.

  if oldT < 0 {
    // I have never been called yet - so don't trust any
    // of the settings yet.
  } else {
    if dT > 0 { // Do nothing if no physics tick has passed from prev call to now.
     set D to (P - oldP)/dT. // crude fake derivative of P
     local onlyPD is Kp*P + Kd*D.
     if (oldI > 0 or onlyPD > cMin) and (oldI < 0 or onlyPD < cMax) { // only do the I turm when within the control range
      set I to oldI + P*dT. // crude fake integral of P
     }.
     set newInput to onlyPD + Ki*I.
    }.
  }.

  set newInput to max(cMin,min(cMax,newInput)).

  // remember old values for next time.
  set PID_array[5] to seekVal.
  set PID_array[6] to P.
  set PID_array[7] to I.
  set PID_array[8] to D.
  set PID_array[9] to t.
  set PID_array[10] to newInput.

  return newInput.
}.

// Verrouillage de la poussée initiale à 100%
LOCK THROTTLE TO 1.0.

// Compte à rebours avant le lancement
PRINT "Counting down:".
FROM { LOCAL countdown IS 3. } UNTIL countdown = 0 STEP { SET countdown TO countdown - 1. } DO {
    PRINT "..." + countdown.
    WAIT 1.
}

WHEN MAXTHRUST = 0 THEN {
    PRINT "Staging".
    STAGE.
    PRESERVE.
}.

// Initialisation des contrôleurs PID pour la poussée et le tangage
local throttlePID is PID_init(0.1, 0.005, 0.1, 0, 1).
local pitchPID is PID_init(0.5, 0.01, 0.5, -1, 1).

// Définition des variables de la mission
local targetAltitude is 100000.  // Altitude cible pour l'orbite basse de Kerbin
local targetPitch is 90.         // Commence à 90 degrés pour un vol vertical
local turnStartAltitude is 1000. // Altitude à laquelle commencer à incliner
local turnEndAltitude is 45000.  // Altitude à laquelle finir l'inclinaison à 0 degrés
local finalOrbitPitch is 0.      // Tangage en orbite

// Allumer les moteurs
lock throttle to 1.


// Boucle principale de vol jusqu'à l'orbite
until apoapsis > targetAltitude {

    // Mise à jour du tangage cible en fonction de l'altitude actuelle
    if altitude > turnStartAltitude and altitude < turnEndAltitude {
        // Calculer le tangage linéairement entre 90 degrés et 0 degrés
        set targetPitch to 90 - (90 * (altitude - turnStartAltitude) / (turnEndAltitude - turnStartAltitude)).
    } else if ship:altitude >= turnEndAltitude {
        set targetPitch to finalOrbitPitch.
    }

    // Mise à jour du contrôle de tangage
    local currentPitch is ship:facing:pitch.
    local pitchAdjustment is PID_seek(pitchPID, targetPitch, currentPitch).
    set ship:control:pitch to pitchAdjustment.

    // Contrôle de la poussée pour optimiser l'apoapsis
    local currentApoapsis is apoapsis.
    local thrustAdjustment is PID_seek(throttlePID, targetAltitude, currentApoapsis).
    set ship:control:mainthrottle to thrustAdjustment.
    HUDTEXT(targetPitch , 10, 4, 30, BLUE, FALSE).
    HUDTEXT(targetAltitude , 10, 3, 30, BLUE, FALSE).

    wait 0.1.
}

// Couper les moteurs lorsque l'apoapsis est à la cible
lock throttle to 0.

// Attendre d'atteindre l'apoapsis
wait until altitude > targetAltitude - 500 and altitude < targetAltitude + 500.

// Allumer les moteurs pour circulariser
lock throttle to 1.
wait until periapsis > targetAltitude - 500.
lock throttle to 0.

print "Orbit Achieved at " + round(altitude) + " meters!".
