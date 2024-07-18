

// Écriture de logs dans le fichier
LOG "Début de la mission : " + TIME:SECONDS TO logPath.
WAIT 1.
LOG "Vérification du système en cours..." TO logPath.
// Vérification si le fichier existe déjà, sinon le créer


LIST PARTS IN partList.
FOR part IN partList {
    PRINT part:NAME.
}

LIST RESOURCES IN resourceList.
FOR resource IN resourceList {
    PRINT resource:NAME + " amount: " + resource:AMOUNT.
}



LIST RCS IN rcsList.
FOR rcs IN rcsList {
    PRINT rcs:PART:NAME + " thrust: " + rcs:THRUST.
}

LIST FILES IN fileList.
FOR file IN fileList {
    PRINT file:NAME + " size: " + file:SIZE.
}
LIST SENSORS IN sensorList.
FOR sensor IN sensorList {
    PRINT sensor:PART:NAME + " type: " + sensor:SENSOR_TYPE.
}

LIST DOCKINGPORTS IN portList.
FOR port IN portList {
    PRINT port:PART:NAME + " state: " + port:STATE.
}


LIST FILES IN fileList.
FOR file IN fileList {
    PRINT file:NAME + " size: " + file:SIZE.
}
LIST VOLUMES IN volumeList.
FOR volume IN volumeList {
    PRINT volume:NAME + " capacity: " + volume:CAPACITY.
}

// Déclarer les variables pour la masse totale et la masse à vide
LOCAL masseTotale IS SHIP:MASS.
LOCAL masseAVide IS 0.

// Calculer la masse à vide en soustrayant la masse de tous les réservoirs de carburant
FOR tank IN SHIP:PARTSTAGGED("fuelTank") {
    SET masseAVide TO masseAVide + (tank:MASS - tank:RESOURCEMASS).
}


// Script pour lister tous les panneaux solaires sur le vaisseau

// LOCAL solarPanels IS LIST().
// FOR part IN SHIP:PARTS {
//     IF part:NAME:CONTAINS("solarPanel") {
//         solarPanels:ADD(part).
//     }
// }

// IF solarPanels:LENGTH = 0 {
//     PRINT "Aucun panneau solaire trouvé sur le vaisseau.".
// } ELSE {
//     PRINT "Panneaux solaires trouvés : ".
//     FOR panel IN solarPanels {
//         PRINT   "panel status" + panel:STATUS + " - " + panel:NAME + " à la position " + panel:POSITION.
//     }

// }
// Script to find and activate solar panels



// Afficher les résultats
PRINT "La masse totale du vaisseau est : " + masseTotale + " tonnes.".
PRINT "La masse à vide du vaisseau est : " + masseAVide + " tonnes.".

LOGTOLOGPATH("CALLED FROM STATIC CONTROL").


WAIT 1.
LOG "Tous les systèmes sont opérationnels." TO logPath.

