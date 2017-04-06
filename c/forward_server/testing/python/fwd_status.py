#!/usr/bin/env python3


class CMStatus:
	init 		= "init"
	reg  		= "reg"
	getticket	= "getticket"
	connfwd		= "connfwd"
	est		= "est"
	error		= "error"


class CCStatus:
	default			= "default"
	wait_response		= "wait_response"
	wait_peer		= "wait_peer"
	wait_connect		= "wait_connect"
	wait_connet_peer	= "wait_connet_peer"
	send_request		= "send_request"
	wait_response_peer	= "wait_response_peer"
	heart_beat		= "heart_beat"
	send_file		= "send_file"
	shutdown_session	= "shutdown_session"

