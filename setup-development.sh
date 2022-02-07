
if grep "Ubuntu" /etc/*-release
	then
	echo "Setup for Ubuntu, please be patient!"
	sudo sh tools/setup-debian.sh
elif grep -q MSYS2 /etc/*-release
	then
	echo "Setup for Mingw, please be patient!"
	tools/setup-mingw.sh
elif grep "Debian" /etc/*-release
	then
	echo "Setup for Debian, please be patient!"
	su -c "sh tools/setup-debian.sh"
fi

read -p "Confirm compile simutrans? (Yy) " reply
if [ $reply = "y" -o $reply = "Y" ]; then
	echo "Compiling using make"
	autoconf
	./configure
	make -j $(nproc)
	echo "Update translations"
	tools/distribute.sh
fi
