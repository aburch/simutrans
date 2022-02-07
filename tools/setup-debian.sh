
echo "We need to have a current system or it fails"
apt update
apt -y upgrade

echo "Now installing dependencies"
# neccessary packets
apt -y install autoconf pkg-config subversion zip unzip cmake make
apt -y install gcc g++ libbz2-dev libfreetype-dev libpng-dev libminiupnpc-dev libzstd-dev libfluidsynth-dev

# optional for SLD2 builds:
apt -y install libsdl2-dev

