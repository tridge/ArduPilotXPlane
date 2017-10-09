/*
 * ArduPilot.c
 * 
 * This plugin implements a fast SITL interface for ArduPilot
 * 
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>
#include "XPLMDisplay.h"
#include "XPLMGraphics.h"
#include "XPLMProcessing.h"
#include "XPLMUtilities.h"
#include "XPLMDataAccess.h"

static XPLMWindowID gWindow;

static bool enabled;
static bool use_pause;
static char status_string[60] = "starting";
static XPLMCreateFlightLoop_t flight_loop;
static bool flight_loop_registered;
static XPLMFlightLoopID flight_loop_id;
static float frame_rate;

// dref handles
static struct {
    float quaternion[4];
    double latitude;
    double longitude;
} dref;

// list of drefs matching name to handles
static struct {
    const char *name;
    void *ptr;
    unsigned type;
    XPLMDataRef ref;
} dref_map[] = {
    { "sim/flightmodel/position/q", &dref.quaternion, xplmType_FloatArray },
    { "sim/flightmodel/position/latitude", &dref.latitude, xplmType_Double },
    { "sim/flightmodel/position/longitude", &dref.longitude, xplmType_Double },
    { NULL, NULL }
};


/*
  find all dref handles. This is done when the plugin is enabled. 
 */
static void find_data_refs(void)
{
    for (uint16_t i=0; dref_map[i].name; i++) {
        dref_map[i].ref = XPLMFindDataRef(dref_map[i].name);
        if (dref_map[i].ref == NULL) {
            printf("Failed to find dref '%s'\n", dref_map[i].name);
            snprintf(status_string, sizeof(status_string)-1, "failed ref %u", (unsigned)i);
            continue;
        }
        if ((dref_map[i].type & XPLMGetDataRefTypes(dref_map[i].ref)) == 0) {
            printf("Bad dref type '%s'\n", dref_map[i].name);
            snprintf(status_string, sizeof(status_string)-1, "bad ref type %u", (unsigned)i);            
        }
    }
}


/*
  fill all data ref values
 */
static void fill_data_refs(void)
{
    for (uint16_t i=0; dref_map[i].name; i++) {
        switch (dref_map[i].type) {
        case xplmType_FloatArray:
            XPLMGetDatavf(dref_map[i].ref, dref_map[i].ptr, 0, 4);
            break;
        case xplmType_Double:
            *((double *)dref_map[i].ptr) = XPLMGetDatad(dref_map[i].ref);
            break;
        }
    }
}


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
	XPLMDrawString(color, left + 5, top - 40, status_string, NULL, xplmFont_Basic);
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
        if (inMouse == xplm_MouseDown) {
            use_pause = !use_pause;
            printf("use_pause=%u\n", (unsigned)use_pause);
            snprintf(status_string, sizeof(status_string)-1, "use_pause=%u", (unsigned)use_pause);
        }
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
    fill_data_refs();

    frame_rate = 1.0 / inElapsedSinceLastCall;

    snprintf(status_string, sizeof(status_string)-1, "%.1f fps", (double)frame_rate);
    
#if LIN
    printf("ArduPilot: loop %f %f rate=%.2f %d (%f %f)\n",
           (double)inElapsedSinceLastCall,
           (double)inElapsedTimeSinceLastFlightLoop,
           frame_rate,
           inCounter,
           dref.latitude, dref.longitude);
#endif

    if (use_pause) {
        // pause the simulator
        XPLMCommandKeyStroke(xplm_key_pause);

        // now we do network operations to ArduPilot

        // unpause again
        XPLMCommandKeyStroke(xplm_key_pause);
    }
        
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
    if (enabled) {
        XPLMScheduleFlightLoop(flight_loop_id, 0, false);        
    }
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
    if (!flight_loop_registered) {

        find_data_refs();
        
        flight_loop.structSize = sizeof(flight_loop);
        flight_loop.phase = xplm_FlightLoop_Phase_AfterFlightModel;
        flight_loop.callbackFunc = FlightLoopCallback;
        flight_loop.refcon = NULL;
        flight_loop_registered = true;
        flight_loop_id = XPLMCreateFlightLoop(&flight_loop);
    }
    XPLMScheduleFlightLoop(flight_loop_id, -1, false);
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


#if IBM
// we need a DllMain for windows
#include <windows.h>
BOOL APIENTRY DllMain( HANDLE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}
#endif
