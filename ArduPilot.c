/*
 * ArduPilot.c
 * 
 * This plugin implements a fast SITL interface for ArduPilot
 * 
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "XPLMDisplay.h"
#include "XPLMGraphics.h"
#include "XPLMProcessing.h"

static XPLMWindowID gWindow;

static bool enabled;

/*
  callback for window draw
 */
static void DrawWindowCallback(XPLMWindowID         inWindowID,    
                               void *               inRefcon)
{
	int left, top, right, bottom;
	float	color[] = { 1.0, 1.0, 1.0 }; 	/* White */

        // draw a box to indicate the window area
	XPLMGetWindowGeometry(inWindowID, &left, &top, &right, &bottom);
	XPLMDrawTranslucentDarkBox(left, top, right, bottom);

        // draw a message at the top
	XPLMDrawString(color, left + 5, top - 20, "ArduPilot SITL", NULL, xplmFont_Basic);
}                                   

/*
  callback for mouse click in our status window
 */
static int MouseClickCallback(XPLMWindowID         inWindowID,    
                              int                  x,    
                              int                  y,    
                              XPLMMouseStatus      inMouse,    
                              void *               inRefcon)
{
        printf("******* got click ******\n");
        // accept click
	return 1;
}                                      


/*
  callback for each iteration of the flight loop
 */
static float FlightLoopCallback(float                inElapsedSinceLastCall,    
                                float                inElapsedTimeSinceLastFlightLoop,    
                                int                  inCounter,    
                                void *               inRefcon)
{
    printf("ArduPilot: loop %f %f rate=%.2f %d\n",
           (double)inElapsedSinceLastCall,
           (double)inElapsedTimeSinceLastFlightLoop,
           1.0/inElapsedSinceLastCall,
           inCounter);
    if (enabled) {
        // call on next loop
        return -1;
    }
    // don't call again when disabled
    return 0;
}


/*
  main plugin entry point
 */
PLUGIN_API int XPluginStart(char *		outName,
                            char *		outSig,
                            char *		outDesc)
{
	strcpy(outName, "ArduPilot");
	strcpy(outSig, "ArduPilot.org.SITL");
	strcpy(outDesc, "A plugin for ArduPilot SITL.");

        // create a status window
	gWindow = XPLMCreateWindow(50, 300, 300, 200,			/* Area of the window. */
                                   1,							/* Start visible. */
                                   DrawWindowCallback,		/* Callbacks */
                                   NULL,
                                   MouseClickCallback,
                                   NULL);						/* Refcon - not used. */
					
        // register our flight loop callback
        XPLMRegisterFlightLoopCallback(FlightLoopCallback, -1, NULL);
	return 1;
}

/*
  stop the plugin
 */
PLUGIN_API void	XPluginStop(void)
{
	XPLMDestroyWindow(gWindow);
}

/*
 * XPluginDisable
 */
PLUGIN_API void XPluginDisable(void)
{
    enabled = false;
    printf("ArduPilot: disabled\n");
}

/*
 * XPluginEnable.
 */
PLUGIN_API int XPluginEnable(void)
{
    enabled = true;
    printf("ArduPilot: enabled\n");
    return 1;
}

/*
 * XPluginReceiveMessage
 */
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID	inFromWho,
                                      long			inMessage,
                                      void *			inParam)
{
}

