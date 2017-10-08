CC=gcc
XPLM=/home/tridge/project/UAV/XPlane/SDK/CHeaders/XPLM

CFLAGS=-Wall -fPIC -O0 -I$(XPLM) -DLIN

lin.xpl: ArduPilot.c
	$(CC) $(CFLAGS) -m64 -shared -o $@ $^ $(LIBS)
