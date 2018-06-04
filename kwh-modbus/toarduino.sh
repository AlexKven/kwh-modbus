if (($# == 0))
then
	set $1 "linux-arduino-library-folder-path"
fi

cp -R libraries/. $1
