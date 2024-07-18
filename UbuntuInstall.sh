
$WOKINGDIR ""
$DESTINATION""




# remove the Scriptfile for yours
rm -rf   "/home/$(whoami)/snap/steam/common/.local/share/Steam/steamapps/common/Kerbal Space Program/Ships/Script"
cp -r "$(pwd)/Script/." "/home/$(whoami)/snap/steam/common/.local/share/Steam/steamapps/common/Kerbal Space Program/Ships/Script"

cd   "/home/$(whoami)/snap/steam/common/.local/share/Steam/steamapps/common/Kerbal Space Program/Ships/Script/."


#Copy rocketSet to dest 
cp -r "$(pwd)/Ships/VAB/." "/home/$(whoami)/snap/steam/common/.local/share/Steam/steamapps/common/Kerbal Space Program/Ships/VAB/."
