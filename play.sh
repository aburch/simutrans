if test x$1 = x
then 
	$0 /d/source/simutrans-experimental-BG-binaries
elif test x$2 = x
then
	$0 $1 /d/source/simutrans-experimental-BG
else
	echo Syntax: $0 [dst-path] [src-path]
	echo Syntax: dst-path = $1
	echo Syntax: src-path = $2

	if test $2/sim.exe -nt $1/sim.exe
	then
		echo copy $2/sim.exe to $1/sim.exe
		mv $1/sim.exe $1/sim.exe.bak
		cp $2/sim.exe $1
	fi
	cd $1
	sim -screensize 1280x1024 -fps 12 -freeplay -nomidi -nosound
	cd $2
fi
exit