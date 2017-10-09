CC_LIN=gcc
CC_WIN=x86_64-w64-mingw32-gcc

SDK=/home/tridge/project/UAV/XPlane/SDK
XPLM=$(SDK)/CHeaders/XPLM
SDK_WIN_LIB=$(SDK)/Libraries/Win

LIN_CFLAGS=-Wall -fPIC -O2 -I$(XPLM) -DLIN -m64 -shared

WIN_CFLAGS=-Wall -O2 -I$(XPLM) -DIBM -shared -undefined_warning
WIN_LIBS=$(SDK_WIN_LIB)/XPLM_64.lib $(SDK_WIN_LIB)/XPWidgets_64.lib

all: lin.xpl win.xpl

lin.xpl: ArduPilot.c
	$(CC_LIN) $(LIN_CFLAGS) -o $@ $^

win.xpl: ArduPilot.c
	$(CC_WIN) $(WIN_CFLAGS) -o $@ $^ $(WIN_LIBS)
