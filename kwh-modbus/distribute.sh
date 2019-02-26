if (($# == 0))
then
	set $1 "/usr/share/arduino/libraries"
fi

cp -R libraries/. $1

filename="destinations.txt"
if [ -e $filename ]
then
    cat $filename | while read LINE; do
        cp -R libraries/. $LINE
        echo $LINE
    done
fi
