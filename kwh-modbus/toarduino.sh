if (($# == 0))
then
	set $1 "/usr/share/arduino/libraries"
fi

cp -R libraries/. $1
