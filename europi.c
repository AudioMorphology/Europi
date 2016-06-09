/*
* EUROPI.C
*
*
*/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <linux/kd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <pigpio.h>

#include "europi.h"


struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
struct fb_var_screeninfo orig_vinfo;
char *fbp = 0;
int fbfd = 0;
int kbfd = 0;
int prog_running=0;		/*Setting this to 0 will force the prog to quit*/
int run_stop=0;			/* 0=Stop 1=Run: Halts the main step generator */
int clock_period=1000;	/*speed of the main internal clock in Milliseconds. It can be varied between 10ms and 60000ms*/
int clock_dirty=0;		/* records whether the master clock period has changed */
int led_on = 0;
int current_step = 0;
/* sequence array:
 * Holds details of our Sequence [CHNL][STEP][PARAMETER]
 * [CHNL] = Channel 0-5
 * [STEP] = Sequence step 0-31
 * where the parameters are as follows:
 * [0] = cv value
 * [1] = gate (0=off, 1=gate, 2=trigger)
 * [2] = active (1 = step Active, 0 = step inactive)
 */
unsigned int sequence[6][32][3];	
unsigned int button_grid[32][1][1];	/* X & Y coordinates of Top Left of each button in the grid */

char *kbfds = "/dev/tty";


// application entry point
int main(int argc, char* argv[])
{
	/* things to do when prog first starts */
	startup();
	/* draw the main front screen */
	paint_main();
	/* scribble some text */
	put_string(fbp, 20, 215, "Hang that g and j below", (unsigned)15);

while (prog_running == 1){
		/* If clock_dirty has been set, then the time period 
	 * needs to be reset
	 */
	 if (clock_dirty == 1) {
		 /* clear and re-create the timer */
		gpioSetTimerFunc(0, clock_period, NULL);
		gpioSetTimerFunc(0, clock_period, master_clock);
		clock_dirty = 0;
	 }

    sleep(1);
}
	shutdown();
	return 0;
  
}
