// Début du script
CLEARSCREEN.
PRINT "FLIGHT_PROGRAM_LOADED OP".



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




// Activation du premier étage


// Lancement vertical initial pour sortir de la densité atmosphérique la plus épaisse
LOCK STEERING TO HEADING(90, 90). // Orientation initiale verticale
WAIT UNTIL SHIP:ALTITUDE > 500. // Attente pour minimiser la résistance initiale

// Début du gravity turn progressif
LOCK STEERING TO HEADING(90, 85). // Commencement du gravity turn à 85 degrés
WAIT UNTIL SHIP:ALTITUDE > 1000.

// Réduction progressive du pitch en fonction de l'altitude
FROM { LOCAL currentAltitude IS 1000. } UNTIL currentAltitude > 45000 STEP { SET currentAltitude TO SHIP:ALTITUDE. } DO {
    LOCAL targetPitch IS MAX(5, 90 - (currentAltitude / 1000)). // Calcul dynamique du pitch
    LOCK STEERING TO HEADING(90, targetPitch).
    WAIT 0.1.
}

// Transition vers la navigation prograde pour finaliser la montée en orbite
LOCK STEERING TO SHIP:PROGRADE.
UNTIL SHIP:APOAPSIS >= 100000 {
    IF SHIP:APOAPSIS > 95000 {
        LOCK THROTTLE TO 0.5. // Réduction de la poussée pour affiner l'apoapsis
    }
    WAIT 0.1.
}

// Arrêt de la poussée une fois l'apoapsis désiré atteint
LOCK THROTTLE TO 0.
PRINT "Apoapsis of 100 km achieved. Preparing for orbital insertion.".

// Finalisation de l'insertion orbitale
WAIT UNTIL TIME:SECONDS > SHIP:ETA:APOAPSIS - 30.
LOCK THROTTLE TO 1.0.
WAIT UNTIL SHIP:PERIAPSIS >= 100000.
LOCK THROTTLE TO 0.
PRINT "Orbital insertion complete. Stable orbit achieved.".

// Fin du programme
PRINT "FLIGHT PROGRAM COMPLETED.".
