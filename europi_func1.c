/*
 * EUROPI_FUNC1.C
 * 
 * Common functions file
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <linux/kd.h>
#include <pigpio.h>

#include "europi.h"
#include "splash.c"
#include "front_panel.c"

extern struct fb_var_screeninfo vinfo;
extern struct fb_fix_screeninfo finfo;
extern struct fb_var_screeninfo orig_vinfo;

extern int fbfd;
extern char *fbp;
extern char *kbfds;
extern int kbfd;
extern int prog_running;
extern int run_stop;
extern int clock_period;
extern int clock_dirty;
extern int led_on;
extern unsigned int sequence[6][32][3];
extern int current_step;

/* Internal Clock
 * 
 * This is the main timing loop for the 
 * whole programme, and which runs continuously
 * from program start to end
 */
void master_clock(void)
{
	/* only calls the step generator if run_stop is true */
	if (run_stop == 1) {
	next_step();
	}
}

/* Function called to advance the sequence on to the next step */
void next_step(void)
{
	int previous_step, channel;
	previous_step = current_step;
	if (++current_step >= 32){current_step = 0;}
	/* turn off previous step and turn on current step */
	draw_button_number(previous_step, HEX2565(BTN_BACK_CLR));
	draw_button_number(current_step, HEX2565(BTN_STEP_CLR));
	for (channel=0;channel<CHANNELS;channel++){
		/* set the Gate State for each channel */
		gate_state(channel,sequence[channel][current_step][1]);
		channel_level(channel,sequence[channel][current_step][0]);
	}
}

/* Function to clear and recreate the
 * main front screen. Should be capable
 * of being called at any time, and it'll
 * repaint all controls in their current
 * state, though its main use is as the  v
 * prog launches to set out the display
 * area
 */
int paint_main(void)
{
	int channel;
	/* splash screen is a full 320x240 image, so will clear the screen */
	splash_screen();
	sleep(2);
	paint_front_panel();
	step_button_grid();
	/* Set all the gate & level indicators to off */
	for(channel=0;channel<CHANNELS;channel++){
		gate_state(channel,0);
		channel_level(channel,0);
	}
	/* start the main step generator */
	run_stop = 1;
}


/*
 * splash_screen - reads the splash image in splash.c
 * and displays it on the screen pixel by pixel. There
 * are probably better ways of doing this, but it works.
 */
void splash_screen(void)
{
	int i,j;
	for(i=0; i<splash_img.height; i++) {
    for(j=0; j<splash_img.width; j++) {
	int r = splash_img.pixel_data[(i*splash_img.width + j)*3 + 0];
	int g = splash_img.pixel_data[(i*splash_img.width + j)*3 + 1];
	int b = splash_img.pixel_data[(i*splash_img.width + j)*3 + 2];
	put_pixel_RGB565(fbp, j, i, RGB2565(r,g,b));
	  
    }
  }
}

/*
 * Front Panel - reads the Front Panel image in splash.c
 * and displays it on the screen
 */
void paint_front_panel(void)
{
	
	int i,j;
	for(i=0; i<front_panel.height; i++) {
    for(j=0; j<front_panel.width; j++) {
      int r = front_panel.pixel_data[(i*front_panel.width + j)*3 + 0];
	  int g = front_panel.pixel_data[(i*front_panel.width + j)*3 + 1];
	  int b = front_panel.pixel_data[(i*front_panel.width + j)*3 + 2];
	  put_pixel_RGB565(fbp, j, i, RGB2565(r,g,b));
	  
    }
  }
}
/*
 * Draws the complete grid of Step buttons.
 * absolute co-ordinates of the Top Left
 * corner of each button are calculated once
 * on startup and stored in the sequence array
 */
void step_button_grid(void)
{
int row, col, x, y;
for (row=0;row<BTN_STEP_ROWS;row++){
	for(col=0;col<BTN_STEP_COLS;col++){
		x = BTN_STEP_TLX + (col * (BTN_STEP_WIDTH + BTN_STEP_HGAP));
		y = BTN_STEP_TLY + (row * (BTN_STEP_HEIGHT + BTN_STEP_VGAP));
		draw_button(x,y,HEX2565(BTN_BACK_CLR));
	}
}
	
}
/* Draws a button, given just a number and a colour */
void draw_button_number(int button_number, unsigned short c)
{
	int row, col, x, y;
	row = (int)(button_number / BTN_STEP_COLS);
	col = button_number % BTN_STEP_COLS;
	x = BTN_STEP_TLX + (col * (BTN_STEP_WIDTH + BTN_STEP_HGAP));
	y = BTN_STEP_TLY + (row * (BTN_STEP_HEIGHT + BTN_STEP_VGAP));
	draw_button(x,y,c);
}
/*
 * draws a sequencer grid button
 * is passed the coordinates of the 
 * Top Left corner
 */
void draw_button(int x, int y, unsigned short c)
{
fill_rect_RGB565(fbp, x, y, BTN_STEP_WIDTH, BTN_STEP_HEIGHT, c);	
}
/*
 * channel_levels
 * Draws the current levels 
 * for the Channel level indicators. Full scale
 * is 4096, so we are drawing two rectangles where
 * the height of the lower one is proportional
 * to level/4096
 */
void channel_level(int channel,int level)
{
	int x, y, illuminated_height;
	illuminated_height = (int)(((float)level/4096)*CHNL_LEVEL_HEIGHT);
	x = CHNL_LEVEL_TLX + (channel * (CHNL_LEVEL_WIDTH+CHNL_LEVEL_HGAP));
	/* Draw the un-lit portion */
	y = CHNL_LEVEL_TLY;
	fill_rect_RGB565(fbp, x, y, CHNL_LEVEL_WIDTH, CHNL_LEVEL_HEIGHT-illuminated_height, HEX2565(CHNL_BACK_CLR));	
	/* Draw the illuminated portion */
	y = CHNL_LEVEL_TLY + CHNL_LEVEL_HEIGHT - illuminated_height;
	fill_rect_RGB565(fbp, x, y, CHNL_LEVEL_WIDTH, illuminated_height, HEX2565(CHNL_FORE_CLR));	
	
}
/*
 * gate_states
 * Draws the current state for 
 * for the Gate State indicator
 */
void gate_state(int channel, int state)
{
	int x, y;
	y = GATE_STATE_TLY;
		x = GATE_STATE_TLX + (channel * (GATE_STATE_WIDTH+GATE_STATE_HGAP));
		if (state == 1){
			fill_rect_RGB565(fbp, x, y, GATE_STATE_WIDTH, GATE_STATE_HEIGHT, HEX2565(GATE_FORE_CLR));	
		}
		else {
			fill_rect_RGB565(fbp, x, y, GATE_STATE_WIDTH, GATE_STATE_HEIGHT, HEX2565(GATE_BACK_CLR));	
		}
}
/*
 * paint_menu
 * Function draws the Menu panel, including the current
 * state of any displayed controls / texst etc., so it
 * can be called after any menu information has been 
 * updated.
 */
void paint_menu(void){
	fill_rect_RGB565(fbp, MENU_X, MENU_Y, MENU_WIDTH, MENU_HEIGHT, MENU_BACK_CLR);	
}

 int startup(void)
 {
	 //initialise the sequence for testing purposes
	 rnd_sequence();
	// PIGPIO Function initialisation
	if (gpioInitialise()<0) return 1;
	// TEMP for testing with the K-Sharp screen
	// Use one of the buttons to quit the app
	gpioSetMode(BUTTON1_IN, PI_INPUT);
	gpioSetPullUpDown(BUTTON1_IN, PI_PUD_UP);
	gpioSetMode(BUTTON2_IN, PI_INPUT);
	gpioSetPullUpDown(BUTTON2_IN, PI_PUD_UP);
	gpioSetMode(BUTTON3_IN, PI_INPUT);
	gpioSetPullUpDown(BUTTON3_IN, PI_PUD_UP);
	gpioSetMode(TOUCH_INT, PI_INPUT);
	gpioSetPullUpDown(TOUCH_INT, PI_PUD_UP);
	// Register a callback routine
	gpioSetAlertFunc(BUTTON1_IN, controlled_exit);
	gpioSetAlertFunc(BUTTON2_IN, ksharp_1);
	gpioSetAlertFunc(BUTTON3_IN, ksharp_2);
	gpioSetAlertFunc(TOUCH_INT, touch_interrupt);
	// Start by attempting to open the Framebuffer
	// for reading and writing
	fbfd = open("/dev/fb1", O_RDWR);
	if (!fbfd) {
		log_msg("Error: cannot open framebuffer device.");
		return(1);
	}
	unsigned int screensize = 0;
	// Get current screen metrics
	if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
		log_msg("Error reading variable information.");
	}
	// printf("Original %dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel );
	// Store this for when the prog closes (copy vinfo to vinfo_orig)
	// because we'll need to re-instate all the existing parameters
	memcpy(&orig_vinfo, &vinfo, sizeof(struct fb_var_screeninfo));
  if (ioctl(fbfd, FBIOGET_VSCREENINFO, &orig_vinfo)) {
    log_msg("Error reading variable information.");
  }
  // Change variable info - force 16 bit and resolution
  vinfo.bits_per_pixel = 16;
  vinfo.xres = X_MAX;
  vinfo.yres = Y_MAX;
  vinfo.xres_virtual = vinfo.xres;
  vinfo.yres_virtual = vinfo.yres;
  
  if (ioctl(fbfd, FBIOPUT_VSCREENINFO, &vinfo)) {
    log_msg("Error setting variable information.");
  }
  
  // Get fixed screen information
  if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
    log_msg("Error reading fixed information.");
  }

  // map fb to user mem 
  screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
  fbp = (char*)mmap(0, 
        screensize, 
        PROT_READ | PROT_WRITE, 
        MAP_SHARED, 
        fbfd, 
        0);

  if ((int)fbp == -1) {
    log_msg("Failed to mmap.");
	return -1;
  }
  
  	// Turn the cursor off
    kbfd = open(kbfds, O_WRONLY);
    if (kbfd >= 0) {
        ioctl(kbfd, KDSETMODE, KD_GRAPHICS);
    }
    else {
        log_msg("Could not open kbfds.");
    }
	/* Start the internal sequencer clock */
	run_stop = 0;		/* master clock is running, but step generator is halted */
	gpioSetTimerFunc(0, clock_period, master_clock);
	prog_running = 1;
  return(0);
}

/* Things to do as the prog closess */
int shutdown(void)
 {
	 /* Clear the screen to black */
	fill_rect_RGB565(fbp, 0, 0, X_MAX - 1, Y_MAX - 1, HEX2565(0x000000));
	/* put up our splash screen (why not!) */
	splash_screen();
	/* unload the PIGPIO library */
	gpioTerminate();
	unsigned int screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
	if (kbfd >= 0) {
		ioctl(kbfd, KDSETMODE, KD_TEXT);
	}
	else {
		log_msg("Could not reset keyboard mode.");
	}
	munmap(fbp, screensize);
	if (ioctl(fbfd, FBIOPUT_VSCREENINFO, &orig_vinfo)) {
		log_msg("Error re-setting variable information.");
	}
	close(fbfd);
	return(0);
 }

/* Deals with messages that need to be logged
 * This could be modified to write them to a 
 * log file. In the first instance, it simply
 * printf's them
 */
void log_msg(char msg[])
{
	printf(msg);
	printf("\n");
	//sleep(1);
}

/* function called whenever the Touch Screen detects an input */
void touch_interrupt(int gpio, int level, uint32_t tick)
{
	printf("Touch touched\n");
}

/* Called to initiate a controlled shutdown and
 * exist of the prog
 */
void controlled_exit(int gpio, int level, uint32_t tick)
{
	prog_running = 0;
}
/* Middle button on ksharp TFT pressed */
void ksharp_1(int gpio, int level, uint32_t tick)
{
	/* speed increase by 10 bpm*/
	double new_period = 60000/((60000/clock_period)+10);
	if ((int)new_period > 10) { 
		clock_period = (int)new_period; 
		clock_dirty = 1;
		}
	printf("period now %d\n",clock_period);
}

/* Bottom button on ksharp TFT pressed */
void ksharp_2(int gpio, int level, uint32_t tick)
{
	int new_period;
	int bpm;
	/* speed decrease by 10 bpm - ie add  */
	bpm = 60000/clock_period;
	bpm -= 10;
	printf("clock_period: %d, bpm: %d\n",clock_period,bpm);
	new_period = 60000/bpm;
	if (new_period < 6000) { 
		clock_period = new_period; 
		clock_dirty = 1;
		}
	else 
	{
		new_period = 6000;
	}
	
}
	
/*
 * generates a random sequence = 32 steps, 6 channels
 */
void rnd_sequence(void)
{
	int channel,step;
	for (channel=0;channel<6;channel++){
		for (step=0;step<32;step++){
			sequence[channel][step][0] = rand() % 4095;
			sequence[channel][step][1] = rand() % 2;
			sequence[channel][step][2] = rand() % 2;
			
		}
	}
}