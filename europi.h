/*
* EUROPI.H
*
*/
#ifndef EUROPI_H
#define EUROPI_H

#include <pigpio.h>

/* GPIO Port Assignments 		*/
/* RPi Header pins in comments	*/
/* Gates 3-6 are on the Minion	*/
#define GATE1_OUT	5	/* PIN 29	*/
#define GATE2_OUT	6	/* PIN 31	*/
#define BUTTON1_IN	18	/* PIN 12	*/
#define BUTTON2_IN	23	/* PIN 16	*/
#define BUTTON3_IN	24	/* PIN 18	*/
#define BUTTON4_IN	25	/* PIN 22	*/
#define ENCODERA_IN	4	/* PIN 7	*/
#define ENCODERB_IN	14  /* PIN 8	*/
#define ENCODER_BTN 15	/* PIN 10 	*/
#define CLOCK_IN	12	/* PIN 32	*/
#define CLOCK_OUT	13	/* PIN 33	*/
#define STEP1_OUT	19	/* PIN 35	*/
#define INTEXT_IN	12	/* PIN 38	*/
#define RUNSTOP_IN	26	/* PIN 37	*/
#define RESET_IN	16	/* PIN 36	*/
#define TOUCH_INT	17	/* PIN 11	*/


/* Function Prototypes in europi_func1 */
int startup(void);
int shutdown(void);
void paint_menu(void);
void log_msg(char []);
void controlled_exit(int gpio, int level, uint32_t tick);
void master_clock(void);
void ksharp_1(int gpio, int level, uint32_t tick);
void ksharp_2(int gpio, int level, uint32_t tick);
void next_step(void);
void splash_screen(void);
void paint_front_panel(void);
void step_button_grid(void);
void draw_button(int x, int y, unsigned short c);
void draw_button_number(int button_number, unsigned short c);
void channel_level(int channel, int level);
void gate_state(int channel, int state);
void rnd_sequence(void);
void touch_interrupt(int gpio, int level, uint32_t tick);
 
/* Function Prototypes for europi_framebuffer_utils */
void put_pixel(char *, int x, int y, int c);
void put_pixel_RGB24(char *, int x, int y, int r, int g, int b);
void put_pixel_RGB565(char *, int x, int y, unsigned short c);
void draw_line(char *, int x0, int y0, int x1, int y1, int c);
void draw_line_RGB565(char *, int x0, int y0, int x1, int y1, unsigned short c);
void draw_rect(char *, int x0, int y0, int w, int h, int c);
void draw_rect_RGB565(char *, int x0, int y0, int w, int h, unsigned short c);
void fill_rect(char *, int x0, int y0, int w, int h, int c);
void fill_rect_RGB565(char *, int x0, int y0, int w, int h, unsigned short c);
void draw_circle(char *, int x0, int y0, int r, int c);
void draw_circle_RGB565(char *, int x0, int y0, int rad,  unsigned short c);
void fill_circle(char *, int x0, int y0, int r, int c);
void fill_circle_RGB565(char *, int x0, int y0, int rad,  unsigned short c);
unsigned short RGB2565(int r, int g, int b);
unsigned short HEX2565(long c);
void put_string(char *, int x, int y, char *s, unsigned colidx);
void put_char(char *, int x, int y, int c, int colidx);

/* Screen Colours */
#define SCR_BACK_CLR		0xD936CF	/* main background colour */
#define SCR_FRAMEBORDER_CLR	0xFFFFFF	/* border around main frames */
#define LED_OFF				0xFF00FF	/* Unlit LED */
#define LED_ON				0x00FF00	/* Lit LED */
#define MENU_BACK_CLR		4			/* Menu Panel background colour */
#define BTN_BACK_CLR		0xf8ea53	/* Default background colour for the grid of sequence buttons */
#define BTN_STEP_CLR		0x59ef10	/* Colour of the current step  */
#define CHNL_BACK_CLR		0x000000	/* Background (non-illuminated) level for each channel indicator */
#define CHNL_FORE_CLR		0x32e30f	/* Foreground (illuminated) colour for each channel indicator */
#define GATE_BACK_CLR		0x000000	/* Background (non-illuminated) of each Gate State indicator */
#define GATE_FORE_CLR		0xec0718	/* Foreground (illuminate) colour for each Gate State indicator */

/* Screen Object Sizes */
#define LED_SIZE			8
#define	MENU_WIDTH			317
#define MENU_HEIGHT			30
#define	MENU_X				1
#define	MENU_Y				209
#define BTN_STEP_WIDTH		16			/* width of each step button */
#define BTN_STEP_HEIGHT		32			/* height of each step button */
#define BTN_STEP_HGAP		13			/* gap between each step button horizontally */
#define BTN_STEP_VGAP		14			/* gap between each step button vertically */
#define BTN_STEP_COLS		8			/* how many Step buttons to display horizontally */
#define BTN_STEP_ROWS		4			/* how many rows of buttons */
#define BTN_STEP_TLX		15			/* X coordinate of Top Left of 1st button */
#define BTN_STEP_TLY		14			/* Y coordinate of Top Left of 1st button */
#define CHNL_LEVEL_WIDTH	4			/* Width of each channel level indicator */
#define CHNL_LEVEL_HEIGHT	160			/* Height of each channel level indicator */
#define CHNL_LEVEL_HGAP		6			/* Gap between each channel level indicator */
#define CHNL_LEVEL_TLX		252			/* X coordinate of Top Left of 1st Channel Level indicator */
#define CHNL_LEVEL_TLY		14			/* Y coordinate of Top Left of 1st Channel Level indicator */
#define CHANNELS			6			/* How many CV / Gate Channels */
#define GATE_STATE_WIDTH	4			/* Width of the Gate State indicator */
#define GATE_STATE_HEIGHT	4			/* Height of the Gate State indicators */
#define GATE_STATE_HGAP		6			/* Gap between each Gate State indicator */
#define GATE_STATE_TLX		252			/* X Coordinate of Top Left of 1st channel Gate State indicator */
#define GATE_STATE_TLY		183			/* Y Coordinate of Top Left of 1st channel Gate State indicator */

/* Global Constants */
// Some global variables that we'll need all over the place
#define X_MAX 320 
#define Y_MAX 240


#endif /* EUROPI_H */