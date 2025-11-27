#!/bin/sh
### BEGIN INIT INFO
# Provides:			egmemulator
# Required-Start:	$local_fs
# Required-Stop:	$local_fs
# Default-Start:	2 3 4 5
# Default-Stop: 	0 1 6
## END INIT INFO

EGMEMULATOR_HOME=/opt/egmemulator

# Debugging startup watchdog timeout, this helps see if startup took too long
seconds=$(awk '{print $1}' /proc/uptime)
echo "Uptime at egmemulator script start is $seconds seconds"

# This script usually is triggered about 15s after boot, after the watchdog has already started countdown.
# The watchdog on the PT206 defaults to 60s but has a hardware bug that makes it run 30% faster so that
# it actually times out at 40s.
# Ping it here to reset the countdown to 40s, in case this startup takes more than 25s.
if [ ! -e "/sdboot/DISABLE_WATCHDOG" ]; then
	echo 'ping watchdog before egmemulator start'
	echo 'hello' > /dev/watchdog
fi

case "$1" in
	start)
		. /etc/profile

		# Link the first partition of the SD boot card as the "os" drive.
		ln -sf /sdboot ${EGMEMULATOR_HOME}/os

		# Link the second partition of the SD boot card as the "sys" drive.
		ln -sf /run/media/mmcblk2p2 ${EGMEMULATOR_HOME}/sys

		# Link the first partition of the SD media card as the "mediadisk" drive.
		ln -sf /run/media/mmcblk3p1 ${EGMEMULATOR_HOME}/mediadisk

		export XDG_RUNTIME_DIR=/run/user/root

		cd /opt/egmemulator/bin

		# Debugging startup watchdog timeout, this helps see if startup took too long
		seconds=$(awk '{print $1}' /proc/uptime)
        echo "Starting EGMEmulator exe (uptime $seconds seconds)"
		./EGMEmulator > /var/log/egmemulator.log 2>&1 &
	;;

	stop)
		pkill EGMEmulator
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
