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
#include "../raylib/release/rpi/raylib.h"

extern int run_stop;
extern int disp_menu;
extern char *fbp;
extern int selected_step;
extern struct europi Europi;
extern struct screen_elements ScreenElements;
extern int prog_running;
extern int impersonate_hw;
extern enum encoder_focus_t encoder_focus;
extern int CalibCross;

/*
 * menu callback for Single Channel view
 */
void seq_singlechnl(void) {
	clear_screen_elements();
	ScreenElements.GridView = 0;
	ScreenElements.SingleChannel = 1;
	encoder_focus = track_select;
	select_first_track();	
}

/*
 * menu callback for New Sequence 
 */
void seq_new(void){
	int track;
	int step;
	for(track = 0;track < MAX_TRACKS;track++){
		Europi.tracks[track].selected = FALSE;
		Europi.tracks[track].track_busy = FALSE;
		Europi.tracks[track].last_step = MAX_STEPS;
		Europi.tracks[track].current_step = 0;
		Europi.tracks[track].channels[CV_OUT].quantise = 1;	// default quantization = semitones	
		Europi.tracks[track].channels[CV_OUT].transpose = 0;	
		for(step = 0;step < MAX_STEPS;step++){
			Europi.tracks[track].channels[CV_OUT].steps[step].raw_value = 0;
			Europi.tracks[track].channels[CV_OUT].steps[step].scaled_value = scale_value(track,0);
			Europi.tracks[track].channels[CV_OUT].steps[step].slew_type = Off;
			Europi.tracks[track].channels[CV_OUT].steps[step].slew_length = 0;
			Europi.tracks[track].channels[GATE_OUT].steps[step].retrigger = 1;
			Europi.tracks[track].channels[GATE_OUT].steps[step].gate_value = 1;
		}
	}
}
/*
 * Set all screen elements OFF
 * this is just a handy place to do it,
 * rather than relying on the various
 * menu functions to take care of it
 */
void clear_screen_elements(void){
	ScreenElements.MainMenu = 0;
	//ScreenElements.SingleChannel = 0;
	ScreenElements.SetZero = 0;
	ScreenElements.SetTen = 0;
	ScreenElements.CalibTouch = 0;
	ScreenElements.ScaleValue = 0;
	ScreenElements.SetLoop = 0;
	ScreenElements.SetPitch = 0;
	ScreenElements.SetQuantise = 0;
}
/*
 * Select First Track
 */
void select_first_track(void){
	/* Select the first Track with an Enabled CV output */
	int track;
	/* make sure nothing else is selected */
	for (track=0;track<MAX_TRACKS;track++){
		Europi.tracks[track].selected = FALSE;
	}
	/* select the first enabled track */
	track = 0;
	while(track < MAX_TRACKS){
		if(Europi.tracks[track].channels[CV_OUT].enabled == TRUE) {
			Europi.tracks[track].selected = TRUE;
			log_msg("Selected: %d\n",track);
			GATESingleOutput(Europi.tracks[track].channels[GATE_OUT].i2c_handle, Europi.tracks[track].channels[GATE_OUT].i2c_channel,Europi.tracks[track].channels[GATE_OUT].i2c_device,0x01);
			break;
		}
		track++;
	}
}
/* 
 * menu callback to set/display track quantisation
 */
void seq_quantise(void){
	clear_screen_elements();
	ScreenElements.SetQuantise = 1;
	encoder_focus = track_select;
	select_first_track();
}
/*
 * menu callback to set the pitch for each step 
 */
void seq_setpitch(void){
	clear_screen_elements();
	ScreenElements.SetPitch = 1;
	encoder_focus = track_select;
	run_stop = STOP;
	select_first_track();
}
/*
 * menu callback for Sequence -> Set Loop Points
 */
void seq_setloop(void){
	int runstop_save = run_stop;
	clear_screen_elements();
	ScreenElements.SetLoop = 1;
	encoder_focus = track_select;
	select_first_track();
}

/*
 * menu callback for Test->Scale Value
 */
void test_scalevalue(void){
	int track;
	int runstop_save = run_stop;	// Save this, so we can revert to previous state when we're done
	run_stop = STOP;
	clear_screen_elements();
	ScreenElements.ScaleValue = 1;
	encoder_focus = track_select;
	/* Slight pause to give some threads time to exist */
	sleep(2);
	/* clear down all Gate outputs */
	for (track = 0;track < MAX_TRACKS; track++){
		if (Europi.tracks[track].channels[GATE_OUT].enabled == TRUE ){
			GATESingleOutput(Europi.tracks[track].channels[GATE_OUT].i2c_handle, Europi.tracks[track].channels[GATE_OUT].i2c_channel,Europi.tracks[track].channels[GATE_OUT].i2c_device,0x00);
		}
	}
	select_first_track();
}

/*
 * menu callback for File->Save
 */
void file_save(void){
	run_stop = STOP;
	clear_screen_elements();
	FILE * file = fopen("default.seq","wb");
	if (file != NULL) {
		fwrite(&Europi,sizeof(struct europi),1,file);
		fclose(file);
	}
}

/*
 * menu callback for setting Zero level on channels
 */
void config_setzero(void){
	int track;
	int runstop_save = run_stop;	// Save this, so we can revert to previous state when we're done
	run_stop = STOP;
	clear_screen_elements();
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
	select_first_track();
}

/*
 * menu callback for setting 10v (MAX) level on channels
 */
void config_setten(void){
	int track;
	int runstop_save = run_stop;	// Save this, so we can revert to previous state when we're done
	run_stop = STOP;
	clear_screen_elements();
	ScreenElements.SetTen = 1;
	encoder_focus = track_select;
	/* Slight pause to give some threads time to exist */
	sleep(2);
	/* clear down all Gate outputs */
	for (track = 0;track < MAX_TRACKS; track++){
		if (Europi.tracks[track].channels[GATE_OUT].enabled == TRUE ){
			GATESingleOutput(Europi.tracks[track].channels[GATE_OUT].i2c_handle, Europi.tracks[track].channels[GATE_OUT].i2c_channel,Europi.tracks[track].channels[GATE_OUT].i2c_device,0x00);
		}
	}
	select_first_track();
}

/*
 * menu callback for calibrating the touchscreen
 */
void config_calibtouch(void){
	clear_screen_elements();
	ScreenElements.GridView = 0;
	ScreenElements.CalibTouch = 1;
	CalibCross = 0;
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
					break;
				case Linear:
					Europi.tracks[0].channels[0].steps[selected_step].slew_type = Logarithmic;
					break;
				case Logarithmic:
					Europi.tracks[0].channels[0].steps[selected_step].slew_type = Exponential;
					break;
				case Exponential:
					Europi.tracks[0].channels[0].steps[selected_step].slew_type = Off;
					break;
				default:
					Europi.tracks[0].channels[0].steps[selected_step].slew_type = Off;
				}
		}
		else if (dir < 0) {
			switch(Europi.tracks[0].channels[0].steps[selected_step].slew_type){
				case Off:
					Europi.tracks[0].channels[0].steps[selected_step].slew_type = Exponential;
					break;
				case Exponential:
					Europi.tracks[0].channels[0].steps[selected_step].slew_type = Logarithmic;
					break;
				case Logarithmic:
					Europi.tracks[0].channels[0].steps[selected_step].slew_type = Linear;
					break;
				case Linear:
					Europi.tracks[0].channels[0].steps[selected_step].slew_type = Off;
					break;
				default:
					Europi.tracks[0].channels[0].steps[selected_step].slew_type = Off;
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
	}
}

void init_sequence_old1(void)
{
	FILE * file = fopen("default.seq","rb");
	if (file != NULL) {
		fread(&Europi, sizeof(struct europi), 1, file);
		fclose(file);
	}
	// Temp: Quantize all tracks to semitone scale
	int track;
	int step;
//	for (track=0;track<MAX_TRACKS;track++){
//		Europi.tracks[track].channels[CV_OUT].quantise = track;
//	}
	for (track = 1;track < 3; track++){
		for (step = 0; step < MAX_STEPS; step++){
			Europi.tracks[track].channels[CV_OUT].steps[step].slew_length = 0;
			Europi.tracks[track].channels[CV_OUT].steps[step].slew_type = Off;
			Europi.tracks[track].channels[GATE_OUT].steps[step].gate_value = 1;
			Europi.tracks[track].channels[GATE_OUT].steps[step].retrigger = 1;
		}
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
		if (Europi.tracks[track].channels[0].enabled == TRUE){
			Europi.tracks[track].track_busy = FALSE;
			Europi.tracks[track].last_step =  8; //rand() % 32;
			Europi.tracks[track].channels[CV_OUT].scale_zero = 0;
			Europi.tracks[track].channels[CV_OUT].scale_max = 60000;

			for (step=0;step<MAX_STEPS;step++){
			Europi.tracks[track].channels[0].steps[step].scaled_value = 280; //410;
			}
		}
	}
	Europi.tracks[0].last_step = 32; /* track 0 always 32 steps */
	for (track=0;track<MAX_TRACKS;track++){
		if (Europi.tracks[track].channels[CV_OUT].enabled == TRUE){
			

			
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

			Europi.tracks[track].channels[0].steps[0].raw_value = 18395;
			Europi.tracks[track].channels[0].steps[1].raw_value = 14318;
			Europi.tracks[track].channels[0].steps[2].raw_value = 14388;
			Europi.tracks[track].channels[0].steps[3].raw_value = 9039;
			Europi.tracks[track].channels[0].steps[4].raw_value = 16850;
			Europi.tracks[track].channels[0].steps[5].raw_value = 15982;
			Europi.tracks[track].channels[0].steps[6].raw_value = 2037;
			Europi.tracks[track].channels[0].steps[7].raw_value = 14393;

/*			Europi.tracks[track].channels[0].steps[0].raw_value = 2500;
			Europi.tracks[track].channels[0].steps[1].raw_value = 2500;
			Europi.tracks[track].channels[0].steps[2].raw_value = 0000;
			Europi.tracks[track].channels[0].steps[3].raw_value = 1500;
			Europi.tracks[track].channels[0].steps[4].raw_value = 2500;
			Europi.tracks[track].channels[0].steps[5].raw_value = 2500;
			Europi.tracks[track].channels[0].steps[6].raw_value = 0000;
			Europi.tracks[track].channels[0].steps[7].raw_value = 1500;
*/			Europi.tracks[track].channels[0].steps[8].raw_value = 2500;
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
			

		}
		
		}

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
	
	output_scaling = ((float)Europi.tracks[track].channels[CV_OUT].scale_max - (float)Europi.tracks[track].channels[CV_OUT].scale_zero) / (float)60000;
	for(step=0;step<MAX_STEPS;step++){
		quantized = quantize(Europi.tracks[track].channels[CV_OUT].steps[step].raw_value, scale);
		Europi.tracks[track].channels[CV_OUT].steps[step].scaled_value = (uint16_t)(output_scaling * quantized) + Europi.tracks[track].channels[CV_OUT].scale_zero;
	}
}

/*
 * scale_value
 * 
 * Takes a raw value on a 0 to 60,000 scale (10 Octaves) and,
 * given a Track number (and hence the values required to output
 * 0 and 10 volts) returns a value for output
 */
uint16_t scale_value(int track,uint16_t raw_value)
{
	float output_scaling;
	output_scaling = ((float)Europi.tracks[track].channels[CV_OUT].scale_max - (float)Europi.tracks[track].channels[CV_OUT].scale_zero) / (float)60000;
	return (uint16_t)(output_scaling * raw_value) + Europi.tracks[track].channels[CV_OUT].scale_zero;
} 

