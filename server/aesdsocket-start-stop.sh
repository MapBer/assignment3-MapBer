#!/bin/sh
### BEGIN INIT INFO
# Provides:          aesdsocket
# Required-Start:    $remote_fs $syslog
# Required-Stop:     $remote_fs $syslog
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: AESD socket daemon
# Description:       Start/stop aesdsocket using start-stop-daemon
### END INIT INFO

DAEMON=/usr/bin/aesdsocket
DAEMON_NAME=aesdsocket
PIDFILE=/var/run/${DAEMON_NAME}.pid

case "$1" in
    start)
        echo "Starting $DAEMON_NAME..."
        start-stop-daemon --start \
            --background \
            --make-pidfile \
            --pidfile $PIDFILE \
            --exec $DAEMON -- -d
        ;;
    stop)
        echo "Stopping $DAEMON_NAME..."
        start-stop-daemon --stop \
            --pidfile $PIDFILE \
            --signal SIGTERM
        ;;
    restart)
        echo "Restarting $DAEMON_NAME..."
        $0 stop
        sleep 1
        $0 start
        ;;
    *)
        echo "Usage: $0 {start|stop|restart}"
        exit 1
        ;;
esac

exit 0
