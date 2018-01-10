#!/usr/bin/env bash

NAME=uhttpd

uhttpd_start () {
    echo "starting uhttpd server..."
    UHTTPD_OPTS="-s 0.0.0.0:80 -n 20 -T 10 -h /home/tww -x /cgi-bin"

    /usr/bin/uhttpd $UHTTPD_OPTS  && {
    	echo "start uhttpd success "
    }
}

uhttpd_stop () {
    if [ -z "$PIDFILE" ]; then
        log_failure_msg \
            "$MPDCONF must have pid_file set; cannot stop daemon."
        exit 1
    fi

    log_daemon_msg "Stopping $DESC" "$NAME"
    start-stop-daemon --stop --quiet --oknodo --retry 5 --pidfile "$PIDFILE" \
        --exec $DAEMON
    log_end_msg $?
}

case "$1" in
    start)
        uhttpd_start
        ;;
    stop)
        uhttpd_stop
        ;;
    restart|force-reload|reload)
        uhttpd_stop
        uhttpd_start
        ;;
    *)
        echo "Usage: $0 {start|stop|restart|force-reload}"
        exit 2
        ;;
esac
