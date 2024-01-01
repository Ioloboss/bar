all:
	convert ./barImageSheet.png ./barImageSheet.rgb
	gcc ./bar.c ./xdg-shell-protocol.c -o runBar -lwayland-client
