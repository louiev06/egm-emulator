#!/bin/sh
### BEGIN INIT INFO
# Provides:			sentinel
# Required-Start:	$local_fs weston
# Required-Stop:	$local_fs weston
# Default-Start:	2 3 4 5
# Default-Stop: 	0 1 6
## END INIT INFO

SENTINEL_HOME=/opt/ncompass

# Debugging startup watchdog timeout, this helps see if startup took too long
seconds=$(awk '{print $1}' /proc/uptime)
echo "Uptime at sentinel script start is $seconds seconds"

case "$1" in
	start)
		. /etc/profile

		# TODO-PORT - Should the following link from "${SENTINEL_HOME}/os" be done in the Makefile for sentinel.img?
		# TODO-PORT - When or will the "/sdboot" directory be changed to "/sdboot"?
		# Link the first partition of the SD boot card as the "os" drive.
		# A tester should be able to put a "CardList.txt" in the root of this drive to enable the auto card in/out test functionality for Release images.
		# 2016-12-21 - Something should have already mounted the first partition of the SD boot card as /sdboot.
		ln -sf /sdboot ${SENTINEL_HOME}/os

		# TODO-PORT - Should the following link from "${SENTINEL_HOME}/sys" be done in the Makefile for sentinel.img?
		# Link the second partition of the SD boot card as the "sys" drive.
		# 2016-12-20 - automount should have already mounted the second partition of the SD boot card as /run/media/mmcblk2p2.
		# The Sentinel application now creates a correct default eeprom.bin file, so it is simpler to omit the eeprom.bin file from the sentinel.img.
		ln -sf /run/media/mmcblk2p2 ${SENTINEL_HOME}/sys

		# TODO-PORT - Should the following link from "${SENTINEL_HOME}/mediadisk" be done in the Makefile for sentinel.img?
		# TODO-PORT - Change the link from "mediadisk" to "media" when the Sentinel executable no longer uses "media" for other purposes.
		# Link the first partition of the SD media card as the "mediadisk" drive.
		# 2016-12-20 - automount should have already mounted the first partition of the SD media card as /run/media/mmcblk3p1.
		ln -sf /run/media/mmcblk3p1 ${SENTINEL_HOME}/mediadisk
		ln -sf ${SENTINEL_HOME}/mediadisk/custom ${SENTINEL_HOME}/media/custom

		# Configure a web diagnostics directory to use temporary file storage and link ncompass.html to the same directory.
		# For backwards compatibility, ncompass.html and changeable files (like StatusDump.html) will be served from the same directory.
		# Use a (small) FAT file system, so the URLs are effectively case-insensitive.
		echo "Making web diagnostic ramdisk"
		/usr/sbin/mkfs.vfat /dev/ram0 8192
		mkdir -p ${SENTINEL_HOME}/www
		mount /dev/ram0 ${SENTINEL_HOME}/www

		export XDG_RUNTIME_DIR=/run/user/root
		if [[ ! -f /sdboot/MEDIA_WINDOW_SUPPORT ]]; then
			until weston-info > /dev/null 2>&1
			do
				echo "Waiting for weston"
				sleep 1
			done
		fi

		cd /opt/ncompass/bin
		chmod 755 killchrome.sh

		mkdir -p /usr/local/share/ca-certificates

		# Get the model number from /proc/cmdline
		model=$(awk -F 'model=' '{print $2}' /proc/cmdline)

		# Check if the model number is equal to 206 or 306
		if [[ "$model" == "206" || "$model" == "306" ]]; then
		  	echo "Loading chromium into tmpfs..."
			find /usr/share/icons | xargs touch
			find /usr/lib/chromium | xargs touch			
		fi

		# Debugging startup watchdog timeout, this helps see if startup took too long
		seconds=$(awk '{print $1}' /proc/uptime)
        echo "Starting Sentinel exe (uptime $seconds seconds)"
		./Sentinel > /var/log/sentinel.log 2>&1 &

		# Launch Chrome GUI in kiosk mode (waits for HTTP server to be ready)
		echo "Launching Chrome GUI..."
		./launchchrome.sh &
	;;

	stop)
		pkill Sentinel
		cd /opt/ncompass/bin
		./killchrome.sh
	;;

	restart)
		$0 stop
		sleep 5
		$0 start
	;;

	*)
		echo "usage: $0 ( start | stop | restart )"
	;;

esac
