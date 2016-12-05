// Copyright 2016 Richard R. Goodwin / Audio Morphology
//
// Author: Richard R. Goodwin (richard.goodwin@morphology.co.uk)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// 
// See http://creativecommons.org/licenses/MIT/ for more information.

#include <linux/input.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <linux/kd.h>
#include <pigpio.h>
#include <signal.h>

#include "europi.h"

extern int run_stop;
extern int disp_menu;
extern char *fbp;
extern int selected_step;
extern struct europi Europi;
extern struct screen_elements ScreenElements;
extern int prog_running;
extern enum encoder_focus_t encoder_focus;


/*
 * menu callback for setting Zero level on channels
 */
void config_setzero(void){
	int track;
	int runstop_save = run_stop;	// Save this, so we can revert to previous state when we're done
	run_stop = STOP;
	ScreenElements.MainMenu = 0;
	ScreenElements.SetZero = 1;
	encoder_focus = track_select;
	/* Slight pause to give some threads time to exist */
	sleep(2);
	/* clear down all Gate outputs */
	for (track = 0;track < MAX_TRACKS; track++){
		if (Europi.tracks[track].channels[GATE_OUT].enabled == TRUE ){
			GATESingleOutput(Europi.tracks[track].channels[GATE_OUT].i2c_handle, Europi.tracks[track].channels[GATE_OUT].i2c_channel,Europi.tracks[track].channels[GATE_OUT].i2c_device,0x00);
		}
	}
	/* Select the first Track with an Enabled CV output */
	track = 0;
	while(track < MAX_TRACKS){
		if(Europi.tracks[track].channels[CV_OUT].enabled == TRUE) {
			Europi.tracks[track].selected = TRUE;
			GATESingleOutput(Europi.tracks[track].channels[GATE_OUT].i2c_handle, Europi.tracks[track].channels[GATE_OUT].i2c_channel,Europi.tracks[track].channels[GATE_OUT].i2c_device,0x01);
			break;
		}
		track++;
	}
}

/* 
 * Set the zero volt level for the passed Track
 */
void set_zero(int Track, long ZeroVal){
	if(Europi.tracks[Track].channels[CV_OUT].enabled == TRUE){
		Europi.tracks[Track].channels[CV_OUT].scale_zero = ZeroVal;
	}
}

/*
 * Trigger a controlled shutdown
 */
void file_quit(void){
	prog_running = 0;
}

/*
 * adjust the pitch of the currently selected step / channel
 */
void pitch_adjust(int dir, int vel){
	char string_buf[20];
	if(selected_step >= 0){
		//hard-coded to track 0 for the moment
		Europi.tracks[0].channels[0].steps[selected_step].raw_value += vel*dir*10;
		//log_msg("Raw: %d\n",Europi.tracks[0].channels[0].steps[selected_step].raw_value);
		sprintf(string_buf,"%d",Europi.tracks[0].channels[0].steps[selected_step].raw_value);
		put_string(fbp, 108, 215, string_buf, HEX2565(0x000000), HEX2565(0xFFFFFF));
		if(Europi.tracks[0].channels[0].steps[selected_step].raw_value < 0) Europi.tracks[0].channels[0].steps[selected_step].raw_value = 0;
		if(Europi.tracks[0].channels[0].steps[selected_step].raw_value > 60000) Europi.tracks[0].channels[0].steps[selected_step].raw_value = 60000;
		quantize_track(0,Europi.tracks[0].channels[0].quantise);
	}
}

/*
 * change the gate type for the selected step
 */
void gate_onoff(int dir, int vel){
	char string_buf[20];
	if(selected_step >= 0){
		//hard-coded to track 0 for the moment
		if (dir > 0) {
			Europi.tracks[0].channels[1].steps[selected_step].gate_value = 1;
			put_string(fbp, 108, 215, "On    ", HEX2565(0x000000), HEX2565(0xFFFFFF));
		}
		else if (dir < 0) {
			Europi.tracks[0].channels[1].steps[selected_step].gate_value = 0;
			put_string(fbp, 108, 215, "Off   ", HEX2565(0x000000), HEX2565(0xFFFFFF));
		}
	}
}

/*
 * change the slew type for the selected step
 */
void slew_adjust(int dir, int vel){
	char string_buf[20];
	if(selected_step >= 0){
		//hard-coded to track 0 for the moment
		if (dir > 0) {
			switch(Europi.tracks[0].channels[0].steps[selected_step].slew_type){
				case Off:
					Europi.tracks[0].channels[0].steps[selected_step].slew_type = Linear;
					put_string(fbp, 108, 215, "LIN", HEX2565(0x000000), HEX2565(0xFFFFFF));
					break;
				case Linear:
					Europi.tracks[0].channels[0].steps[selected_step].slew_type = Logarithmic;
					put_string(fbp, 108, 215, "LOG", HEX2565(0x000000), HEX2565(0xFFFFFF));
					break;
				case Logarithmic:
					Europi.tracks[0].channels[0].steps[selected_step].slew_type = Exponential;
					put_string(fbp, 108, 215, "EXP", HEX2565(0x000000), HEX2565(0xFFFFFF));
					break;
				case Exponential:
					Europi.tracks[0].channels[0].steps[selected_step].slew_type = Off;
					put_string(fbp, 108, 215, "OFF", HEX2565(0x000000), HEX2565(0xFFFFFF));
					break;
				default:
					Europi.tracks[0].channels[0].steps[selected_step].slew_type = Off;
					put_string(fbp, 108, 215, "OFF", HEX2565(0x000000), HEX2565(0xFFFFFF));
				}
		}
		else if (dir < 0) {
			switch(Europi.tracks[0].channels[0].steps[selected_step].slew_type){
				case Off:
					Europi.tracks[0].channels[0].steps[selected_step].slew_type = Exponential;
					put_string(fbp, 108, 215, "EXP", HEX2565(0x000000), HEX2565(0xFFFFFF));
					break;
				case Exponential:
					Europi.tracks[0].channels[0].steps[selected_step].slew_type = Logarithmic;
					put_string(fbp, 108, 215, "LOG", HEX2565(0x000000), HEX2565(0xFFFFFF));
					break;
				case Logarithmic:
					Europi.tracks[0].channels[0].steps[selected_step].slew_type = Linear;
					put_string(fbp, 108, 215, "LIN", HEX2565(0x000000), HEX2565(0xFFFFFF));
					break;
				case Linear:
					Europi.tracks[0].channels[0].steps[selected_step].slew_type = Off;
					put_string(fbp, 108, 215, "OFF", HEX2565(0x000000), HEX2565(0xFFFFFF));
					break;
				default:
					Europi.tracks[0].channels[0].steps[selected_step].slew_type = Off;
					put_string(fbp, 108, 215, "OFF", HEX2565(0x000000), HEX2565(0xFFFFFF));
				}
			
		}
	}
}

/*
 * Change the ratchet value for the selected step / channel
 */
void step_repeat(int dir, int vel){
	char string_buf[20];
	if(selected_step >= 0){
		//hard-coded to track 0 for the moment
		if (dir > 0) {
			switch(Europi.tracks[0].channels[1].steps[selected_step].retrigger){
				case 1:
					Europi.tracks[0].channels[1].steps[selected_step].retrigger = 2;
					break;
				case 2:
					Europi.tracks[0].channels[1].steps[selected_step].retrigger = 3;
					break;
				case 3:
					Europi.tracks[0].channels[1].steps[selected_step].retrigger = 4;
					break;
				default:
					Europi.tracks[0].channels[1].steps[selected_step].retrigger = 1;
					break;
			}
		}
		else if (dir < 0) {
			switch(Europi.tracks[0].channels[1].steps[selected_step].retrigger){
				case 1:
					Europi.tracks[0].channels[1].steps[selected_step].retrigger = 4;
					break;
				case 2:
					Europi.tracks[0].channels[1].steps[selected_step].retrigger = 1;
					break;
				case 3:
					Europi.tracks[0].channels[1].steps[selected_step].retrigger = 2;
					break;
				case 4:
					Europi.tracks[0].channels[1].steps[selected_step].retrigger = 3;
					break;
				default:
					Europi.tracks[0].channels[1].steps[selected_step].retrigger = 1;
					break;
			}
		}
		sprintf(string_buf,"%d",Europi.tracks[0].channels[1].steps[selected_step].retrigger);
		put_string(fbp, 108, 215, string_buf, HEX2565(0x000000), HEX2565(0xFFFFFF));
	}
}


void init_sequence(void)
{
	int track,channel,step;
	int raw;
	
	//Temp - set all outputs to 0
	for (track=0;track<MAX_TRACKS;track++){
		Europi.tracks[track].track_busy = FALSE;
		Europi.tracks[track].last_step = rand() % 32;
		if (Europi.tracks[track].channels[0].enabled == TRUE){
			for (step=0;step<MAX_STEPS;step++){
			Europi.tracks[track].channels[0].steps[step].scaled_value = 280; //410;
			}
		}
	}
	Europi.tracks[0].last_step = 32; /* track 0 always 32 steps */
	for (track=0;track<MAX_TRACKS;track++){

			

			
/*			for (step=0;step<=31;step++){
				Europi.tracks[track].channels[CV_OUT].steps[step].raw_value = 1000 * step;
				Europi.tracks[track].channels[CV_OUT].steps[step].slew_length = 30000;
				Europi.tracks[track].channels[CV_OUT].steps[step].slew_type = Linear;
				Europi.tracks[track].channels[CV_OUT].steps[step].slew_shape = Both;
				Europi.tracks[track].channels[GATE_OUT].steps[step].gate_value = 1;
				Europi.tracks[track].channels[GATE_OUT].steps[step].retrigger = 1; 
 
			}
			quantize_track(track,0);
*/			

			Europi.tracks[track].channels[0].steps[0].raw_value = 2500;
			Europi.tracks[track].channels[0].steps[1].raw_value = 2500;
			Europi.tracks[track].channels[0].steps[2].raw_value = 0000;
			Europi.tracks[track].channels[0].steps[3].raw_value = 1500;
			Europi.tracks[track].channels[0].steps[4].raw_value = 2500;
			Europi.tracks[track].channels[0].steps[5].raw_value = 2500;
			Europi.tracks[track].channels[0].steps[6].raw_value = 0000;
			Europi.tracks[track].channels[0].steps[7].raw_value = 1500;
			Europi.tracks[track].channels[0].steps[8].raw_value = 2500;
			Europi.tracks[track].channels[0].steps[9].raw_value = 2500;
			Europi.tracks[track].channels[0].steps[10].raw_value = 0000;
			Europi.tracks[track].channels[0].steps[11].raw_value = 1500;
			Europi.tracks[track].channels[0].steps[12].raw_value = 2500;
			Europi.tracks[track].channels[0].steps[13].raw_value = 4000;
			Europi.tracks[track].channels[0].steps[14].raw_value = 0000;
			Europi.tracks[track].channels[0].steps[15].raw_value = 1500;
			Europi.tracks[track].channels[0].steps[16].raw_value = 2500;
			Europi.tracks[track].channels[0].steps[17].raw_value = 2500;
			Europi.tracks[track].channels[0].steps[18].raw_value = 0000;
			Europi.tracks[track].channels[0].steps[19].raw_value = 1500;
			Europi.tracks[track].channels[0].steps[20].raw_value = 2500;
			Europi.tracks[track].channels[0].steps[21].raw_value = 2500;
			Europi.tracks[track].channels[0].steps[22].raw_value = 0000;
			Europi.tracks[track].channels[0].steps[23].raw_value = 1500;
			Europi.tracks[track].channels[0].steps[24].raw_value = 2500;
			Europi.tracks[track].channels[0].steps[25].raw_value = 6001;
			Europi.tracks[track].channels[0].steps[26].raw_value = 5000;
			Europi.tracks[track].channels[0].steps[27].raw_value = 3500;
			Europi.tracks[track].channels[0].steps[28].raw_value = 4000;
			Europi.tracks[track].channels[0].steps[29].raw_value = 2500;
			Europi.tracks[track].channels[0].steps[30].raw_value = 1500;
			Europi.tracks[track].channels[0].steps[31].raw_value = 0000;
			// Quantize the track to copy Raw to scaled
			// Quantize each to a different scale for fun
			quantize_track(track,0);
//			quantize_track(track, track);
			
			// gate needed for each step
			for (step=0;step<32;step++){
				int gate_prob = rand() % 100;
				//if (gate_prob > 20){
					Europi.tracks[track].channels[GATE_OUT].steps[step].gate_value = 1;
					Europi.tracks[track].channels[GATE_OUT].steps[step].retrigger = 1;
				//}
				//else {
				//	Europi.tracks[track].channels[1].steps[step].gate_value = 0;
				//}
			}
			// some ratchets to make it more interesting
			Europi.tracks[track].channels[1].steps[0].retrigger = 2;
			Europi.tracks[track].channels[1].steps[1].retrigger = 2;
			Europi.tracks[track].channels[1].steps[5].retrigger = 2;
			Europi.tracks[track].channels[1].steps[6].retrigger = 2;
			Europi.tracks[track].channels[1].steps[9].retrigger = 3;
			Europi.tracks[track].channels[1].steps[10].retrigger = 3;
			Europi.tracks[track].channels[1].steps[13].retrigger = 4;
			Europi.tracks[track].channels[1].steps[15].retrigger = 2;
			Europi.tracks[track].channels[1].steps[16].retrigger = 2;
			Europi.tracks[track].channels[1].steps[17].retrigger = 2;
			Europi.tracks[track].channels[1].steps[21].retrigger = 2;
			Europi.tracks[track].channels[1].steps[22].retrigger = 2;
			Europi.tracks[track].channels[1].steps[25].retrigger = 3;
			Europi.tracks[track].channels[1].steps[26].retrigger = 3;
			Europi.tracks[track].channels[1].steps[29].retrigger = 4;
			Europi.tracks[track].channels[1].steps[31].retrigger = 2;
			// And some slews
			Europi.tracks[track].channels[0].steps[0].slew_length = 30000;
			Europi.tracks[track].channels[0].steps[0].slew_type = Linear;
			Europi.tracks[track].channels[0].steps[0].slew_shape = Rising;			
			Europi.tracks[track].channels[0].steps[2].slew_length = 30000;
			Europi.tracks[track].channels[0].steps[2].slew_type = Linear;
			Europi.tracks[track].channels[0].steps[2].slew_shape = Both;			
			Europi.tracks[track].channels[0].steps[4].slew_length = 30000;
			Europi.tracks[track].channels[0].steps[4].slew_type = Linear;
			Europi.tracks[track].channels[0].steps[4].slew_shape = Rising;			
			Europi.tracks[track].channels[0].steps[6].slew_length = 30000;
			Europi.tracks[track].channels[0].steps[6].slew_type = Linear;
			Europi.tracks[track].channels[0].steps[6].slew_shape = Both;			
			Europi.tracks[track].channels[0].steps[8].slew_length = 30000;
			Europi.tracks[track].channels[0].steps[8].slew_type = Linear;
			Europi.tracks[track].channels[0].steps[8].slew_shape = Rising;			
			Europi.tracks[track].channels[0].steps[12].slew_length = 30000;
			Europi.tracks[track].channels[0].steps[12].slew_type = Linear;
			Europi.tracks[track].channels[0].steps[12].slew_shape = Rising;			
			Europi.tracks[track].channels[0].steps[14].slew_length = 30000;
			Europi.tracks[track].channels[0].steps[14].slew_type = Linear;
			Europi.tracks[track].channels[0].steps[14].slew_shape = Both;			
			Europi.tracks[track].channels[0].steps[16].slew_length = 30000;
			Europi.tracks[track].channels[0].steps[16].slew_type = Linear;
			Europi.tracks[track].channels[0].steps[16].slew_shape = Rising;			
			Europi.tracks[track].channels[0].steps[20].slew_length = 30000;
			Europi.tracks[track].channels[0].steps[20].slew_type = Linear;
			Europi.tracks[track].channels[0].steps[20].slew_shape = Rising;			
			Europi.tracks[track].channels[0].steps[24].slew_length = 30000;
			Europi.tracks[track].channels[0].steps[24].slew_type = Linear;
			Europi.tracks[track].channels[0].steps[24].slew_shape = Rising;			
			Europi.tracks[track].channels[0].steps[25].slew_length = 30000;
			Europi.tracks[track].channels[0].steps[25].slew_type = Linear;
			Europi.tracks[track].channels[0].steps[25].slew_shape = Falling;			
			Europi.tracks[track].channels[0].steps[28].slew_length = 30000;
			Europi.tracks[track].channels[0].steps[28].slew_type = Linear;
			Europi.tracks[track].channels[0].steps[28].slew_shape = Rising;			

		if(track > 2){
			for (step=0;step<=31;step++){
				Europi.tracks[track].channels[CV_OUT].steps[step].raw_value = 1000 * step;
				//Europi.tracks[track].channels[CV_OUT].steps[step].slew_length = rand() % 30000;
				Europi.tracks[track].channels[CV_OUT].steps[step].slew_length = 50000;
				int gate_prob = rand() % 100;
				if (gate_prob > 30){
				Europi.tracks[track].channels[CV_OUT].steps[step].slew_type = Off;
				}
				else {
				Europi.tracks[track].channels[CV_OUT].steps[step].slew_type = Linear;
				}
				Europi.tracks[track].channels[CV_OUT].steps[step].slew_shape = Both;
				gate_prob = rand() % 100;
				if (gate_prob > 20){
					Europi.tracks[track].channels[GATE_OUT].steps[step].gate_value = 1;
				}
				else {
					Europi.tracks[track].channels[GATE_OUT].steps[step].gate_value = 0;
					}
				Europi.tracks[track].channels[GATE_OUT].steps[step].retrigger = rand() %4; 
 
			}
			quantize_track(track,12);
		}
			/*for (step=0;step<=31;step++){
				Europi.tracks[track].channels[CV_OUT].steps[step].raw_value = 1000 * step;
				Europi.tracks[track].channels[CV_OUT].steps[step].slew_length = 0;
				Europi.tracks[track].channels[CV_OUT].steps[step].slew_type = Linear;
				Europi.tracks[track].channels[CV_OUT].steps[step].slew_shape = Both;
				Europi.tracks[track].channels[GATE_OUT].steps[step].gate_value = 1;
				Europi.tracks[track].channels[GATE_OUT].steps[step].retrigger = 1; 
 
			}
			quantize_track(track,0);
			*/
		
		}
		// test AD ramp
		Europi.tracks[2].channels[GATE_OUT].steps[0].gate_value = 1;
		Europi.tracks[2].channels[GATE_OUT].steps[0].retrigger = 1; 
		Europi.tracks[2].channels[GATE_OUT].steps[1].gate_value = 0;
		Europi.tracks[2].channels[GATE_OUT].steps[1].retrigger = 1; 
		Europi.tracks[2].channels[GATE_OUT].steps[2].gate_value = 1;
		Europi.tracks[2].channels[GATE_OUT].steps[2].retrigger = 1; 
		Europi.tracks[2].last_step = 2;
		Europi.tracks[2].channels[CV_OUT].steps[0].slew_type = AD;

		
}


/*
 * pre-loads two sequences into Tracks 0 & 1
 * so that there is something there for testing
 * and instant gratification purposes
 */
void init_sequence_old(void)
{
	int track,channel,step;
	int raw;
	
	//Temp - set all outputs to 0
	for (track=0;track<MAX_TRACKS;track++){
		Europi.tracks[track].track_busy = FALSE;
		Europi.tracks[track].last_step = rand() % 32;
		if (Europi.tracks[track].channels[0].enabled == TRUE){
			for (step=0;step<MAX_STEPS;step++){
			Europi.tracks[track].channels[0].steps[step].scaled_value = 280; //410;
			}
		}
	}
	Europi.tracks[0].last_step = 32; /* track 0 always 32 steps */
	for (track=0;track<MAX_TRACKS;track++){

			

			
/*			for (step=0;step<=31;step++){
				Europi.tracks[track].channels[CV_OUT].steps[step].raw_value = 1000 * step;
				Europi.tracks[track].channels[CV_OUT].steps[step].slew_length = 30000;
				Europi.tracks[track].channels[CV_OUT].steps[step].slew_type = Linear;
				Europi.tracks[track].channels[CV_OUT].steps[step].slew_shape = Both;
				Europi.tracks[track].channels[GATE_OUT].steps[step].gate_value = 1;
				Europi.tracks[track].channels[GATE_OUT].steps[step].retrigger = 1; 
 
			}
			quantize_track(track,0);
*/			

			Europi.tracks[track].channels[0].steps[0].raw_value = 2500;
			Europi.tracks[track].channels[0].steps[1].raw_value = 2500;
			Europi.tracks[track].channels[0].steps[2].raw_value = 0000;
			Europi.tracks[track].channels[0].steps[3].raw_value = 1500;
			Europi.tracks[track].channels[0].steps[4].raw_value = 2500;
			Europi.tracks[track].channels[0].steps[5].raw_value = 2500;
			Europi.tracks[track].channels[0].steps[6].raw_value = 0000;
			Europi.tracks[track].channels[0].steps[7].raw_value = 1500;
			Europi.tracks[track].channels[0].steps[8].raw_value = 2500;
			Europi.tracks[track].channels[0].steps[9].raw_value = 2500;
			Europi.tracks[track].channels[0].steps[10].raw_value = 0000;
			Europi.tracks[track].channels[0].steps[11].raw_value = 1500;
			Europi.tracks[track].channels[0].steps[12].raw_value = 2500;
			Europi.tracks[track].channels[0].steps[13].raw_value = 4000;
			Europi.tracks[track].channels[0].steps[14].raw_value = 0000;
			Europi.tracks[track].channels[0].steps[15].raw_value = 1500;
			Europi.tracks[track].channels[0].steps[16].raw_value = 2500;
			Europi.tracks[track].channels[0].steps[17].raw_value = 2500;
			Europi.tracks[track].channels[0].steps[18].raw_value = 0000;
			Europi.tracks[track].channels[0].steps[19].raw_value = 1500;
			Europi.tracks[track].channels[0].steps[20].raw_value = 2500;
			Europi.tracks[track].channels[0].steps[21].raw_value = 2500;
			Europi.tracks[track].channels[0].steps[22].raw_value = 0000;
			Europi.tracks[track].channels[0].steps[23].raw_value = 1500;
			Europi.tracks[track].channels[0].steps[24].raw_value = 2500;
			Europi.tracks[track].channels[0].steps[25].raw_value = 6001;
			Europi.tracks[track].channels[0].steps[26].raw_value = 5000;
			Europi.tracks[track].channels[0].steps[27].raw_value = 3500;
			Europi.tracks[track].channels[0].steps[28].raw_value = 4000;
			Europi.tracks[track].channels[0].steps[29].raw_value = 2500;
			Europi.tracks[track].channels[0].steps[30].raw_value = 1500;
			Europi.tracks[track].channels[0].steps[31].raw_value = 0000;
			// Quantize the track to copy Raw to scaled
			// Quantize each to a different scale for fun
			quantize_track(track,0);
//			quantize_track(track, track);
			
			// gate needed for each step
			for (step=0;step<32;step++){
				int gate_prob = rand() % 100;
				//if (gate_prob > 20){
					Europi.tracks[track].channels[1].steps[step].gate_value = 1;
					Europi.tracks[track].channels[1].steps[step].retrigger = 1;
				//}
				//else {
				//	Europi.tracks[track].channels[1].steps[step].gate_value = 0;
				//}
			}
			// some ratchets to make it more interesting
			Europi.tracks[track].channels[1].steps[0].retrigger = 2;
			Europi.tracks[track].channels[1].steps[1].retrigger = 2;
			Europi.tracks[track].channels[1].steps[5].retrigger = 2;
			Europi.tracks[track].channels[1].steps[6].retrigger = 2;
			Europi.tracks[track].channels[1].steps[9].retrigger = 3;
			Europi.tracks[track].channels[1].steps[10].retrigger = 3;
			Europi.tracks[track].channels[1].steps[13].retrigger = 4;
			Europi.tracks[track].channels[1].steps[15].retrigger = 2;
			Europi.tracks[track].channels[1].steps[16].retrigger = 2;
			Europi.tracks[track].channels[1].steps[17].retrigger = 2;
			Europi.tracks[track].channels[1].steps[21].retrigger = 2;
			Europi.tracks[track].channels[1].steps[22].retrigger = 2;
			Europi.tracks[track].channels[1].steps[25].retrigger = 3;
			Europi.tracks[track].channels[1].steps[26].retrigger = 3;
			Europi.tracks[track].channels[1].steps[29].retrigger = 4;
			Europi.tracks[track].channels[1].steps[31].retrigger = 2;
			// And some slews
			Europi.tracks[track].channels[0].steps[0].slew_length = 30000;
			Europi.tracks[track].channels[0].steps[0].slew_type = Linear;
			Europi.tracks[track].channels[0].steps[0].slew_shape = Rising;			
			Europi.tracks[track].channels[0].steps[2].slew_length = 30000;
			Europi.tracks[track].channels[0].steps[2].slew_type = Linear;
			Europi.tracks[track].channels[0].steps[2].slew_shape = Both;			
			Europi.tracks[track].channels[0].steps[4].slew_length = 30000;
			Europi.tracks[track].channels[0].steps[4].slew_type = Linear;
			Europi.tracks[track].channels[0].steps[4].slew_shape = Rising;			
			Europi.tracks[track].channels[0].steps[6].slew_length = 30000;
			Europi.tracks[track].channels[0].steps[6].slew_type = Linear;
			Europi.tracks[track].channels[0].steps[6].slew_shape = Both;			
			Europi.tracks[track].channels[0].steps[8].slew_length = 30000;
			Europi.tracks[track].channels[0].steps[8].slew_type = Linear;
			Europi.tracks[track].channels[0].steps[8].slew_shape = Rising;			
			Europi.tracks[track].channels[0].steps[12].slew_length = 30000;
			Europi.tracks[track].channels[0].steps[12].slew_type = Linear;
			Europi.tracks[track].channels[0].steps[12].slew_shape = Rising;			
			Europi.tracks[track].channels[0].steps[14].slew_length = 30000;
			Europi.tracks[track].channels[0].steps[14].slew_type = Linear;
			Europi.tracks[track].channels[0].steps[14].slew_shape = Both;			
			Europi.tracks[track].channels[0].steps[16].slew_length = 30000;
			Europi.tracks[track].channels[0].steps[16].slew_type = Linear;
			Europi.tracks[track].channels[0].steps[16].slew_shape = Rising;			
			Europi.tracks[track].channels[0].steps[20].slew_length = 30000;
			Europi.tracks[track].channels[0].steps[20].slew_type = Linear;
			Europi.tracks[track].channels[0].steps[20].slew_shape = Rising;			
			Europi.tracks[track].channels[0].steps[24].slew_length = 30000;
			Europi.tracks[track].channels[0].steps[24].slew_type = Linear;
			Europi.tracks[track].channels[0].steps[24].slew_shape = Rising;			
			Europi.tracks[track].channels[0].steps[25].slew_length = 30000;
			Europi.tracks[track].channels[0].steps[25].slew_type = Linear;
			Europi.tracks[track].channels[0].steps[25].slew_shape = Falling;			
			Europi.tracks[track].channels[0].steps[28].slew_length = 30000;
			Europi.tracks[track].channels[0].steps[28].slew_type = Linear;
			Europi.tracks[track].channels[0].steps[28].slew_shape = Rising;			

		if(track > 2){
			for (step=0;step<=31;step++){
				Europi.tracks[track].channels[CV_OUT].steps[step].raw_value = 1000 * step;
				//Europi.tracks[track].channels[CV_OUT].steps[step].slew_length = rand() % 30000;
				Europi.tracks[track].channels[CV_OUT].steps[step].slew_length = 50000;
				int gate_prob = rand() % 100;
				if (gate_prob > 30){
				Europi.tracks[track].channels[CV_OUT].steps[step].slew_type = Off;
				}
				else {
				Europi.tracks[track].channels[CV_OUT].steps[step].slew_type = Linear;
				}
				Europi.tracks[track].channels[CV_OUT].steps[step].slew_shape = Both;
				gate_prob = rand() % 100;
				if (gate_prob > 20){
					Europi.tracks[track].channels[GATE_OUT].steps[step].gate_value = 1;
				}
				else {
					Europi.tracks[track].channels[GATE_OUT].steps[step].gate_value = 0;
					}
				Europi.tracks[track].channels[GATE_OUT].steps[step].retrigger = rand() %4; 
 
			}
			quantize_track(track,12);
		}
			/*for (step=0;step<=31;step++){
				Europi.tracks[track].channels[CV_OUT].steps[step].raw_value = 1000 * step;
				Europi.tracks[track].channels[CV_OUT].steps[step].slew_length = 0;
				Europi.tracks[track].channels[CV_OUT].steps[step].slew_type = Linear;
				Europi.tracks[track].channels[CV_OUT].steps[step].slew_shape = Both;
				Europi.tracks[track].channels[GATE_OUT].steps[step].gate_value = 1;
				Europi.tracks[track].channels[GATE_OUT].steps[step].retrigger = 1; 
 
			}
			quantize_track(track,0);
			*/
		
		}
		// test AD ramp
		Europi.tracks[2].channels[GATE_OUT].steps[0].gate_value = 1;
		Europi.tracks[2].channels[GATE_OUT].steps[0].retrigger = 1; 
		Europi.tracks[2].channels[GATE_OUT].steps[1].gate_value = 0;
		Europi.tracks[2].channels[GATE_OUT].steps[1].retrigger = 1; 
		Europi.tracks[2].channels[GATE_OUT].steps[2].gate_value = 1;
		Europi.tracks[2].channels[GATE_OUT].steps[2].retrigger = 1; 
		Europi.tracks[2].last_step = 2;
		//Europi.tracks[2].channels[CV_OUT].steps[0].slew_type = AD;

}

/*
 * QUANTIZE_TRACK
 * 
 * spins through all steps of the passed track, takes
 * the raw value from each step and, using the passed
 * scale, quantizes and scales it using the scaling
 * info from the track
 */
void quantize_track(int track, int scale)
{
	int step;
	int quantized;
	float output_scaling;
	
	output_scaling = (float)Europi.tracks[track].channels[0].scale_max / (float)60000;
	for(step=0;step<MAX_STEPS;step++){
		quantized = quantize(Europi.tracks[track].channels[0].steps[step].raw_value, scale);
		Europi.tracks[track].channels[0].steps[step].scaled_value = (uint16_t)(output_scaling * quantized);
	}
}
