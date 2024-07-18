PRINT "DEBUG START".
core:part:getmodule("kOSProcessor"):doevent("Open Terminal").

SET I TO 0.
SET LOGPATH TO "0:/logs/missionLog" + I + ".log".
PRINT "EXISTS(LOGPATH) " + EXISTS(LOGPATH).

UNTIL NOT EXISTS(LOGPATH) {
    IF NOT EXISTS(LOGPATH) {
        PRINT "CREATING LOG FILE".
        CREATE(LOGPATH).
    }
    SET I TO I + 1.
    SET LOGPATH TO "0:/logs/missionLog" + I + ".log".
}

FUNCTION LOGTOLOGPATH{
    PARAMETER LOGMSG.
    LOG LOGMSG + "@:" + time   TO logPath.
}

RUNPATH("0:/StaticControls.ks").
RUNPATH("0:/my_rocket_flight_plan.ks").
