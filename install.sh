#!/bin/bash

# Définir le chemin de base pour le répertoire de KSP
KSP_BASE="/home/$(whoami)/snap/steam/common/.local/share/Steam/steamapps/common/Kerbal Space Program"

# Le répertoire dans votre chemin de travail actuel où les fichiers sont modifiés
WORK_DIR="$(pwd)"

# Répertoires spécifiques que vous souhaitez synchroniser
SCRIPT_DIR="$WORK_DIR/Script"
ROCKET_SET_DIR="$WORK_DIR/Ships/VAB"

# Suppression des anciens répertoires dans la destination
rm -rf "$KSP_BASE/Script"
rm -rf "$KSP_BASE/Ships/VAB"

# Création de liens symboliques des répertoires modifiés
ln -s "$SCRIPT_DIR" "$KSP_BASE/Ships/Script"
ln -s "$ROCKET_SET_DIR" "$KSP_BASE/Ships/VAB"

echo "Les liens symboliques pour les répertoires ont été créés."
