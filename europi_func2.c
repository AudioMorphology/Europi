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

//#include <linux/input.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <malloc.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <linux/kd.h>
#include <pigpio.h>
#include <signal.h>

#include "europi.h"
#include "../raylib/src/raylib.h"

extern int run_stop;
extern int save_run_stop;
extern int disp_menu;
extern int debug;
extern int TuningOn;
extern int last_track;
extern int edit_track;
extern uint16_t TuningVoltage; 
extern int SingleChannelOffset;
extern int VerticalScrollPercent;
extern char *fbp; 
extern char input_txt[];
extern char current_filename[];
extern char modal_dialog_txt1[];
extern char modal_dialog_txt2[];
extern char modal_dialog_txt3[];
extern char modal_dialog_txt4[];
extern int selected_step;
extern struct europi Europi;
extern struct europi_hw Europi_hw;
//extern struct screen_overlays ScreenOverlays;
extern uint32_t ActiveOverlays;
extern enum display_page_t DisplayPage;
extern int prog_running;
extern int impersonate_hw;
extern enum encoder_focus_t encoder_focus;
extern enum btnA_func_t btnA_func;
extern enum btnB_func_t btnB_func;
extern enum btnC_func_t btnC_func;
extern enum btnD_func_t btnD_func;
extern int btnA_state;
extern int btnB_state;
extern int btnC_state;
extern int btnD_state;
extern char **files;
extern size_t file_count;                      
extern int file_selected;
extern int first_file;
 

/*
 * menu callback for Single Channel view
 * This switches the display page to a single
 * page of the track noted in the global variable
 * edit_track
 */
void seq_singlechnl(void) {
	ClearScreenOverlays();
	ActiveOverlays |= ovl_SingleChannel;
	DisplayPage = SingleChannel;
	encoder_focus = track_select;
	SwitchChannelFunction(edit_track);
}
/*
 * menu callback for Grid Channel view
 */
void seq_gridview(void) {
    VerticalScrollPercent = 0;
	ClearScreenOverlays();
	DisplayPage = GridView;
    encoder_focus = none;
	select_first_track();	
}

/*
 * menu callback for 8x8 Channel view
 */
void seq_grid8x8(void) {
    VerticalScrollPercent = 0;
	ClearScreenOverlays();
	DisplayPage = Grid8x8;
    encoder_focus = none;
	select_first_track();	
}

/*
 * menu callback for New Sequence 
 */
void seq_new(void){
	int track;
	int step;
    int defaultpitch = (6000*4) + 3000;   //half way up.
	ClearScreenOverlays();
	DisplayPage = Grid8x8;
    encoder_focus = none;
	select_first_track();	
    //log_msg("Seq New\n");
	for(track = 0;track < last_track;track++){
		Europi.tracks[track].selected = FALSE;
		Europi.tracks[track].track_busy = FALSE;
		Europi.tracks[track].last_step = 16;
		Europi.tracks[track].current_step = 0;
        Europi.tracks[track].clock_divisor = 1;
        Europi.tracks[track].clock_divisor_counter = 0;
		Europi.tracks[track].channels[CV_OUT].quantise = 1;	// default quantization = semitones	
		Europi.tracks[track].channels[CV_OUT].transpose = 0;	 
		Europi.tracks[track].channels[CV_OUT].type = CHNL_TYPE_CV;
		Europi.tracks[track].channels[MOD_OUT].quantise = 1;	// default quantization = semitones	
		Europi.tracks[track].channels[MOD_OUT].transpose = 0;	 
		Europi.tracks[track].channels[MOD_OUT].type = CHNL_TYPE_MOD;
		Europi.tracks[track].channels[MOD_OUT].enabled = TRUE;
		for(step = 0;step < MAX_STEPS;step++){
			Europi.tracks[track].channels[CV_OUT].steps[step].raw_value = defaultpitch;
			Europi.tracks[track].channels[CV_OUT].steps[step].scaled_value = scale_value(track,defaultpitch);
			Europi.tracks[track].channels[CV_OUT].steps[step].slew_type = Off;
			Europi.tracks[track].channels[CV_OUT].steps[step].slew_length = 0;
			Europi.tracks[track].channels[MOD_OUT].steps[step].raw_value = defaultpitch;
			Europi.tracks[track].channels[MOD_OUT].steps[step].scaled_value = scale_value(track,defaultpitch);
			Europi.tracks[track].channels[MOD_OUT].steps[step].slew_type = Off;
			Europi.tracks[track].channels[MOD_OUT].steps[step].slew_length = 0;
			Europi.tracks[track].channels[MOD_OUT].steps[step].mod_shape = Mod_Triangle;
			Europi.tracks[track].channels[MOD_OUT].steps[step].min = 300;
			Europi.tracks[track].channels[MOD_OUT].steps[step].max = 50000;
			Europi.tracks[track].channels[MOD_OUT].steps[step].duty_cycle = 0;
			Europi.tracks[track].channels[GATE_OUT].steps[step].ratchets = 1;
			Europi.tracks[track].channels[GATE_OUT].steps[step].repetitions = 1;
            Europi.tracks[track].channels[GATE_OUT].steps[step].repeat_counter = 0;
			Europi.tracks[track].channels[GATE_OUT].steps[step].fill = 0;
			Europi.tracks[track].channels[GATE_OUT].steps[step].gate_type = Gate_95;
		}
	}
    // !!! Temp testing
    Europi.tracks[1].clock_divisor = 4;
}
/*
 * Set all screen overlays OFF
 * this is just a handy place to do it,
 * rather than relying on the various
 * menu functions to take care of it
 */
void ClearScreenOverlays(void){
	ActiveOverlays = 0;
	//log_msg("Clear Overlays %d",gpioTick());
}

/*
 * OverlayActive returns TRUE is any
 * screen overlays are currently active
 * This is useful to ensure touch
 * events are only taken by the Overlay
 * 
 * The IgnoreOverlay argument can pass in any
 * number of Overlays that are ignored when deciding
 * if any overlays are active or not
 */
int OverlayActive(uint32_t IgnoreOverlay){
    if((ActiveOverlays & ~IgnoreOverlay) > 0) return 1;
    else return 0;
}

/*
 * simple function that just returns True or False
 * if there is something at the top of the screen and
 * therefore the Scroll Bar should be shorter, and the 
 * track displays etc should start lower down the screen
 */
int ShortScroll(){
	if(ActiveOverlays & (
	ovl_MainMenu |
	ovl_SetZero | 
	ovl_SetTen | 
	ovl_SetLoop | 
	ovl_SetPitch | 
	ovl_SetSlew | 
	ovl_SetDirection | 
	ovl_SetQuantise)) return 1;
	else return 0;
}

/*
 * Sets the default soft menu buttons
 */
void buttonsDefault(void){
    btnA_func = btnA_quit;
    btnB_func = btnB_menu;
    btnC_func = btnC_bpm_dn;
    btnD_func = btnD_bpm_up;
    btnA_state = 0;
    btnB_state = 0;
    btnC_state = 0;
    btnD_state = 0;
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
			edit_track = track;
			GATESingleOutput(Europi.tracks[track].channels[GATE_OUT].i2c_handle, Europi.tracks[track].channels[GATE_OUT].i2c_channel,Europi.tracks[track].channels[GATE_OUT].i2c_device,0x01);
			break;
		}
		track++;
	}
}
/*
 * Select a specific Track - makes certain no others are selected
 */
void select_track(int track){
    int ThisTrack;
    for(ThisTrack=0;ThisTrack<MAX_TRACKS;ThisTrack++){
        if(ThisTrack == track) Europi.tracks[ThisTrack].selected = TRUE;
        else Europi.tracks[ThisTrack].selected = FALSE;
    }
}
/*
 * Select Next Track - dir specifies whether to select next up or down (+/-1)
 */
void select_next_track(int dir){
    if(dir == 1){
        int track = 0;
        int prev_selected = 0;
        int found_new = FALSE;
        while(track < last_track){
            if(Europi.tracks[track].selected == TRUE){
                // deselect this track
                prev_selected = track;
                Europi.tracks[track].selected = FALSE;
                GATESingleOutput(Europi.tracks[track].channels[GATE_OUT].i2c_handle, Europi.tracks[track].channels[GATE_OUT].i2c_channel,Europi.tracks[track].channels[GATE_OUT].i2c_device,0x00);
                // select the next enabled one
                if(track < last_track - 1) track++;
                while(track < last_track){
                    if(Europi.tracks[track].channels[CV_OUT].enabled == TRUE){
                        Europi.tracks[track].selected = TRUE;
						edit_track = track;
                        found_new = TRUE;
                        GATESingleOutput(Europi.tracks[track].channels[GATE_OUT].i2c_handle, Europi.tracks[track].channels[GATE_OUT].i2c_channel,Europi.tracks[track].channels[GATE_OUT].i2c_device,0x01);
                        break;
                    }
                    track++;
                }
            }
            track++;
        }
        // Didn't find a new one to select, so re-select the previous one
        if (found_new == FALSE){
            Europi.tracks[prev_selected].selected = TRUE;
			edit_track = prev_selected;
        }
    }
    else {
        int track = last_track - 1;
        int prev_selected = last_track - 1;
        int found_new = FALSE;
        while(track >= 0){
            if(Europi.tracks[track].selected == TRUE){
                // deselect this track
                prev_selected = track;
                Europi.tracks[track].selected = FALSE;
                GATESingleOutput(Europi.tracks[track].channels[GATE_OUT].i2c_handle, Europi.tracks[track].channels[GATE_OUT].i2c_channel,Europi.tracks[track].channels[GATE_OUT].i2c_device,0x00);
                // select the next enabled one
                if(track > 0) track--;
                while(track >= 0){
                    if(Europi.tracks[track].channels[CV_OUT].enabled == TRUE){
                        Europi.tracks[track].selected = TRUE;
						edit_track = track;
                        found_new = TRUE;
                        GATESingleOutput(Europi.tracks[track].channels[GATE_OUT].i2c_handle, Europi.tracks[track].channels[GATE_OUT].i2c_channel,Europi.tracks[track].channels[GATE_OUT].i2c_device,0x01);
                        break;
                    }
                    track--;
                }
            }
            track--;
        }
        // Didn't find a new one to select, so re-select the previous one
        if (found_new == FALSE){
            Europi.tracks[prev_selected].selected = TRUE;
			edit_track = prev_selected;
        }
    }
    
}
/*
 * Select Next Step - Dir specifies next up or down
 */
void select_next_step(int dir){
    int track = 0;
    while (track < MAX_TRACKS){
        if(Europi.tracks[track].selected == TRUE){
            if (dir == 1) {
                if (Europi.tracks[track].current_step < (Europi.tracks[track].last_step -1)) Europi.tracks[track].current_step++;
            }
            else {
                if (Europi.tracks[track].current_step > 0) Europi.tracks[track].current_step--;
            }
            break;
        }
        track++;
    }
    /* Output the current value for this track / step, so we can hear what's going on */
    DACSingleChannelWrite(track,Europi.tracks[track].channels[CV_OUT].i2c_handle, Europi.tracks[track].channels[CV_OUT].i2c_address, Europi.tracks[track].channels[CV_OUT].i2c_channel, Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].raw_value);
    // And turn the Gate output on
    GATESingleOutput(Europi.tracks[track].channels[GATE_OUT].i2c_handle, Europi.tracks[track].channels[GATE_OUT].i2c_channel,Europi.tracks[track].channels[GATE_OUT].i2c_device,0x01);
}
/* SwitchChannelFunction sets the appropriate Display Page
 * and screen Overlays depending on whether this is a CV, Mod
 * Gate or MIDI cchannel.It felt convenient to do this at 
 * a single point, as this context switch will need to happen 
 * in several different places as you scroll up and down tracks
 */
void SwitchChannelFunction(int track){
    // Need to check which sort of Single channel display to set
    if(Europi.tracks[track].channels[CV_OUT].type == CHNL_TYPE_CV){
        DisplayPage = SingleChannel;
        ActiveOverlays |= ovl_SingleChannel;
        SingleChannelOffset = 0;
        encoder_focus = track_select;
        btnA_func = btnA_none;
        btnB_func = btnB_tr_minus;
        btnC_func = btnC_tr_plus;
        btnD_func = btnD_done; 
        save_run_stop = run_stop;
    }
}

/*
 * Set Loop Point moves the current
 * last step in a track one step up or down
 */
void set_loop_point(int dir){
    if(dir == 1){
        int track=0;
        while(track < MAX_TRACKS){
            if(Europi.tracks[track].selected == TRUE){
                if(Europi.tracks[track].last_step < MAX_STEPS){
                    Europi.tracks[track].last_step++;
                }
                break;
            }
            track++;
        }
    }
    else {
        int track=0;
        while(track < MAX_TRACKS){
            if(Europi.tracks[track].selected == TRUE){
                if(Europi.tracks[track].last_step > 1){
                    Europi.tracks[track].last_step--;
                }
                break;
            }
            track++;
        }
        
    }
}
/*
 * Select the next slew type up or down
 */
void select_next_slew(int dir){

    int track = 0;
    while (track < MAX_TRACKS){
        if(Europi.tracks[track].selected == TRUE){
            if (dir == 1) {
                switch(Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].slew_type){
                    case Off:
                        Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].slew_type = Linear;
                    break;
                    case Linear:
                        Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].slew_type = Exponential;
                    break;
                    case Exponential:
                        Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].slew_type = RevExp;
                    break;
                    case RevExp:
                        Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].slew_type = Log;
                    break;
                    case Log:
                        Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].slew_type = RevLog;
                    break;
                    case RevLog:
                        Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].slew_type = Sine;
                    break;
                    case Sine:
                        Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].slew_type = RevSine;
                    break;
                    case RevSine:
                        Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].slew_type = Cosine;
                    break;
                    case Cosine:
                        Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].slew_type = Off;
                    break;
                }
            }
            else {
                switch(Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].slew_type){
                    case Off:
                        Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].slew_type = Cosine;
                    break;
                    case Linear:
                        Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].slew_type = Off;
                    break;
                    case Exponential:
                        Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].slew_type = Linear;
                    break;
                    case RevExp:
                        Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].slew_type = Exponential;
                    break;
                    case Log:
                        Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].slew_type = RevExp;
                    break;
                    case RevLog:
                        Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].slew_type = Log;
                    break;
                    case Sine:
                        Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].slew_type = RevLog;
                    break;
                    case RevSine:
                        Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].slew_type = Sine;
                    break;
                    case Cosine:
                        Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].slew_type = RevSine;
                    break;
                }
            }
            break;
        }
        track++;
    }

}

/*
 * Select the next quantisation either up or down
 */
void select_next_quantisation(int dir){
    int track = 0;
    while (track < MAX_TRACKS){
        if(Europi.tracks[track].selected == TRUE){
            if (dir == 1) {
                if (Europi.tracks[track].channels[CV_OUT].quantise < 47) Europi.tracks[track].channels[CV_OUT].quantise++;
            }
            else {
                if (Europi.tracks[track].channels[CV_OUT].quantise > 0) Europi.tracks[track].channels[CV_OUT].quantise--;
            }
            // Re-Quantise track
            quantize_track(track,Europi.tracks[track].channels[CV_OUT].quantise);
            break;
        }
        track++;
    }
}

/*
 * Select the next direction either up or down
 */
void select_next_direction(int dir){
    int track = 0;
    while (track < MAX_TRACKS){
        if(Europi.tracks[track].selected == TRUE){
            if (dir == 1) {
                switch (Europi.tracks[track].direction){
                    case Forwards:
                        Europi.tracks[track].direction = Backwards;
                    break;
                    case Backwards:
                        Europi.tracks[track].direction = Pendulum_F;
                    break;
                    case Pendulum_F:
                    case Pendulum_B:
                        Europi.tracks[track].direction = Random;
                    break;
                    default:
                    case Random:
                        Europi.tracks[track].direction = Forwards;
                    break;
                }
            }
            else {
                switch (Europi.tracks[track].direction){
                    case Forwards:
                        Europi.tracks[track].direction = Random;
                    break;
                    case Random:
                        Europi.tracks[track].direction = Pendulum_F;
                    break;
                    case Pendulum_F:
                    case Pendulum_B:
                        Europi.tracks[track].direction = Backwards;
                    break;
                    default:
                    case Backwards:
                        Europi.tracks[track].direction = Forwards;
                    break;
                }
            }
            break;
        }
        track++;
    }
}

void set_step_pitch(int dir, int vel){
    int track = 0;
    int raw_val;
    int current;
    int newpitch;
    while (track < MAX_TRACKS){
        if(Europi.tracks[track].selected == TRUE){
            if(Europi.tracks[track].channels[CV_OUT].quantise > 0){
                // track is quantised, so just use the direction to 
                // move to the next note in the scale
                if(dir == UP){
                    // move the raw value of this step up until the next quantisation 
                    // boundary is passed
                    raw_val = Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].raw_value;
                    current = quantize(raw_val,Europi.tracks[track].channels[CV_OUT].quantise);
                    newpitch = current;
                    while((current == newpitch) && (raw_val <= 60000)){
                        raw_val += 10;
                        newpitch = quantize(raw_val,Europi.tracks[track].channels[CV_OUT].quantise);
                    }
                    if(raw_val > 60000) raw_val = 60000;
                    // We've got a new raw value that returns a different quantised value
                    log_msg("newpitch: %d\n",newpitch);
                    Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].raw_value = newpitch;
                    Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].scaled_value = scale_value(track,newpitch);
                }
                else{
                    raw_val = Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].raw_value;
                    current = quantize(raw_val,Europi.tracks[track].channels[CV_OUT].quantise);
                    newpitch = current;
                    while((current == newpitch) && (raw_val > 0)){
                        raw_val -= 10;
                        newpitch = quantize(raw_val,Europi.tracks[track].channels[CV_OUT].quantise);
                    }
                    if(raw_val < 0) raw_val = 0;
                    // We've got a new raw value that returns a different quantised value
                    Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].raw_value = newpitch;
                    Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].scaled_value = scale_value(track,newpitch);
                }
                Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].scaled_value = Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].raw_value; //= (uint16_t)(output_scaling * quantized) + Europi.tracks[track].channels[CV_OUT].scale_zero;
                DACSingleChannelWrite(track,Europi.tracks[track].channels[CV_OUT].i2c_handle, Europi.tracks[track].channels[CV_OUT].i2c_address, Europi.tracks[track].channels[CV_OUT].i2c_channel, Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].scaled_value);
            }
            else {
                // unquantised tracks use the velocity to move
                // the pitch up or down as appropriate
                if(vel > 3) vel *= 10;
                if (dir == 1) {
                    if (Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].raw_value <= (60000 - vel)) Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].raw_value += vel;
                }
                else {
                    if (Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].raw_value >= vel) Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].raw_value -= vel;
                }
                Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].scaled_value = Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].raw_value; //= (uint16_t)(output_scaling * quantized) + Europi.tracks[track].channels[CV_OUT].scale_zero;
                DACSingleChannelWrite(track,Europi.tracks[track].channels[CV_OUT].i2c_handle, Europi.tracks[track].channels[CV_OUT].i2c_address, Europi.tracks[track].channels[CV_OUT].i2c_channel, Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].scaled_value);
            }            
            break;
        }
	}
}
/* 
 * menu callback to set/display track quantisation
 */
void seq_quantise(void){
    save_run_stop = run_stop;
	ClearScreenOverlays();
	ActiveOverlays |= ovl_SetQuantise;
    ClearMenus();
    MenuSelectItem(0,0);
    btnA_func = btnA_select;
    btnB_func = btnB_val_down;
    btnC_func = btnC_val_up;
    btnD_func = btnD_done;    
	encoder_focus = track_select;
	select_first_track();
}
/* 
 * menu callback to set/display track direction
 */
void seq_setdir(void){
    save_run_stop = run_stop;
	ClearScreenOverlays();
	ActiveOverlays |= ovl_SetDirection;
    ClearMenus();
    MenuSelectItem(0,0);
    btnA_func = btnA_select;
    btnB_func = btnB_val_down;
    btnC_func = btnC_val_up;
    btnD_func = btnD_done;    
	encoder_focus = track_select;
	select_first_track();
}
/*
 * menu callback to set the pitch for each step 
 */
void seq_setpitch(void){
    save_run_stop = run_stop;
	run_stop = STOP;
	ClearScreenOverlays();
	ActiveOverlays |= ovl_SetPitch;
    ClearMenus();
    MenuSelectItem(0,0);
    btnA_func = btnA_select;
    btnB_func = btnB_val_down;
    btnC_func = btnC_val_up;
    btnD_func = btnD_done;    
	encoder_focus = track_select;
	select_first_track();
}

/*
 * menu callback for Sequence -> Set Loop Points
 */
void seq_setloop(void){
    save_run_stop = run_stop;
	ClearScreenOverlays();
	ActiveOverlays |= ovl_SetLoop;
    ClearMenus();
    MenuSelectItem(0,0);
    btnA_func = btnA_select;
    btnB_func = btnB_val_down;
    btnC_func = btnC_val_up;
    btnD_func = btnD_done;    
	encoder_focus = track_select;
	select_first_track();
}
/*
 * menu Callback to set Slew type for each step
 */
void seq_setslew(void){
    save_run_stop = run_stop;
	run_stop = STOP;
	ClearScreenOverlays();
	ActiveOverlays |= ovl_SetSlew;
    ClearMenus();
    MenuSelectItem(0,0);
    btnA_func = btnA_select;
    btnB_func = btnB_val_down;
    btnC_func = btnC_val_up;
    btnD_func = btnD_done;    
	encoder_focus = track_select;
	select_first_track();
}



/*
 * menu callback for Test->Scale Value
 */
void test_scalevalue(void){
	int track;
    save_run_stop = run_stop;
	run_stop = STOP;
	ClearScreenOverlays();
	ActiveOverlays |= ovl_ScaleValue;
    ClearMenus();
    MenuSelectItem(0,0);
    btnA_func = btnA_none;
    btnB_func = btnB_open;
    btnC_func = btnC_cancel;
    btnD_func = btnD_none;
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
 * Menu callback for test->Keyboard
 */
void test_keyboard(void){
    save_run_stop = run_stop;
    ClearScreenOverlays();
	//ActiveOverlays |= ovl_Keyboard;
    encoder_focus = keyboard_input;
} 
/*  
 * menu callback for File->Open
 */
void file_open(void){
    save_run_stop = run_stop;
    //run_stop = STOP;
    ClearScreenOverlays();
    ClearMenus();
    MenuSelectItem(0,0);
    btnA_func = btnA_none;
    btnB_func = btnB_open;
    btnC_func = btnC_cancel;
    btnD_func = btnD_none;
	ActiveOverlays |= ovl_FileOpen;
    encoder_focus = file_open_focus;
    // Read the list of files in the resources/sequences directory
    file_count = file_list("resources/sequences", &files);
    // Sort the array of filenames
    qsort(files, file_count, sizeof(char *), cstring_cmp);
    file_selected = 0;
    first_file = 0;
} 
/*  
 * menu callback for File->Save
 */
void file_save(void){
    save_run_stop = run_stop;
	//run_stop = STOP;
	ClearScreenOverlays();
    ClearMenus();
    MenuSelectItem(0,0);
	FILE * file = fopen(current_filename,"wb");
	if (file != NULL) {
		fwrite(&Europi,sizeof(struct europi),1,file);
		fclose(file);
        log_msg("Saved as: %s\n",current_filename);
	}
    else{
        log_msg("Error in file_save: %s",current_filename);
    }
}

/*  
 * menu callback for File->New
 */
void file_new(void){
    seq_new();
    save_run_stop = run_stop;
	//run_stop = STOP;
	ClearScreenOverlays();
    btnA_func = btnA_none;
    btnB_func = btnB_save;
    btnC_func = btnC_cancel;
    btnD_func = btnD_none;
	ActiveOverlays |= ovl_FileSaveAs;
	ActiveOverlays |= ovl_Keyboard;
    encoder_focus = keyboard_input;
    //sprintf(input_txt,""); 
    *input_txt = 0;
}

/*
 * menu callback for File->SaveAs
 */
void file_saveas(void){
    save_run_stop = run_stop;
	//run_stop = STOP;
	ClearScreenOverlays();
    btnA_func = btnA_none;
    btnB_func = btnB_save;
    btnC_func = btnC_cancel;
    btnD_func = btnD_none;
	ActiveOverlays |= ovl_FileSaveAs;
	ActiveOverlays |= ovl_Keyboard;
    encoder_focus = keyboard_input;
    //sprintf(input_txt,""); 
    *input_txt = 0;
}

/*
 * menu callback for setting Zero level on channels
 */
void config_setzero(void){
	int track;
    save_run_stop = run_stop;
	run_stop = STOP;
    btnA_func = btnA_select;
    btnB_func = btnB_val_down;
    btnC_func = btnC_val_up;
    btnD_func = btnD_done;
	ClearScreenOverlays();
	ActiveOverlays |= ovl_SetZero;
	encoder_focus = track_select;
	select_first_track();
	/* Slight pause to give some threads time to exist */
	sleep(2);
	/* clear down all Gate outputs */
	for (track = 0;track < MAX_TRACKS; track++){
		if (Europi.tracks[track].channels[GATE_OUT].enabled == TRUE ){
			GATESingleOutput(Europi.tracks[track].channels[GATE_OUT].i2c_handle, Europi.tracks[track].channels[GATE_OUT].i2c_channel,Europi.tracks[track].channels[GATE_OUT].i2c_device,0x00);
		}
	}
}

/*
 * menu callback for setting 10v (MAX) level on channels
 */
void config_setten(void){
	int track;
    save_run_stop = run_stop;
	run_stop = STOP;
	ClearScreenOverlays();
    btnA_func = btnA_select;
    btnB_func = btnB_val_down;
    btnC_func = btnC_val_up;
    btnD_func = btnD_done;
	ActiveOverlays |= ovl_SetTen;
	encoder_focus = track_select;
	select_first_track();
	/* Slight pause to give some threads time to exist */
	sleep(2);
	/* clear down all Gate outputs */
	for (track = 0;track < MAX_TRACKS; track++){
		if (Europi.tracks[track].channels[GATE_OUT].enabled == TRUE ){
			GATESingleOutput(Europi.tracks[track].channels[GATE_OUT].i2c_handle, Europi.tracks[track].channels[GATE_OUT].i2c_channel,Europi.tracks[track].channels[GATE_OUT].i2c_device,0x00);
		}
	}
}

/*
 * menu callback to set debug on/off
 */
 void config_debug(void){
    save_run_stop = run_stop;
    if(debug == TRUE) debug = FALSE;
    else debug = TRUE;
    ClearMenus();
    MenuSelectItem(0,0);
	ActiveOverlays &= !ovl_MainMenu;
 }

/*
 * menu callback to set Global Tuning flag on/off
 */
 void config_tune(void){
    save_run_stop = run_stop;
    if(TuningOn == TRUE) TuningOn = FALSE;
    else TuningOn = TRUE;
    ClearMenus();
    MenuSelectItem(0,0);
	ActiveOverlays &= !ovl_MainMenu;
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
 * load_sequence()
 * Reads the specified sequence 
 * TODO: Probably ought to worry about whether
 * the sequence being read is the same size & shape
 * as our Europi Object?
 */
void load_sequence(const char *filename){
    sprintf(current_filename,"%s",filename);
	FILE * file = fopen(filename,"rb");
	if (file != NULL) {
		fread(&Europi, sizeof(struct europi), 1, file);
		fclose(file);
        // note the file we've just opened
        sprintf(current_filename,"%s",filename);
        log_msg("Current: %s\n",current_filename);
	}
    else {
        log_msg("Load Sequence error: %s",filename);
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
					break;
				case Linear:
					Europi.tracks[0].channels[0].steps[selected_step].slew_type = Exponential;
					break;
				case Exponential:
					Europi.tracks[0].channels[0].steps[selected_step].slew_type = RevExp;
					break;
				case RevExp:
					Europi.tracks[0].channels[0].steps[selected_step].slew_type = Off;
					break;
				default:
					Europi.tracks[0].channels[0].steps[selected_step].slew_type = Off;
				}
		}
		else if (dir < 0) {
			switch(Europi.tracks[0].channels[0].steps[selected_step].slew_type){
				case Off:
					Europi.tracks[0].channels[0].steps[selected_step].slew_type = RevExp;
					break;
				case RevExp:
					Europi.tracks[0].channels[0].steps[selected_step].slew_type = Exponential;
					break;
				case Exponential:
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
 * Change the repeat value for the selected step / channel
 */
void step_repeat(int dir, int vel){
	char string_buf[20];
	if(selected_step >= 0){
		//hard-coded to track 0 for the moment
		if (dir > 0) {
			switch(Europi.tracks[0].channels[1].steps[selected_step].repetitions){
				case 1:
					Europi.tracks[0].channels[1].steps[selected_step].repetitions = 2;
					break;
				case 2:
					Europi.tracks[0].channels[1].steps[selected_step].repetitions = 3;
					break;
				case 3:
					Europi.tracks[0].channels[1].steps[selected_step].repetitions = 4;
					break;
				default:
					Europi.tracks[0].channels[1].steps[selected_step].repetitions = 1;
					break;
			}
		}
		else if (dir < 0) {
			switch(Europi.tracks[0].channels[1].steps[selected_step].repetitions){
				case 1:
					Europi.tracks[0].channels[1].steps[selected_step].repetitions = 4;
					break;
				case 2:
					Europi.tracks[0].channels[1].steps[selected_step].repetitions = 1;
					break;
				case 3:
					Europi.tracks[0].channels[1].steps[selected_step].repetitions = 2;
					break;
				case 4:
					Europi.tracks[0].channels[1].steps[selected_step].repetitions = 3;
					break;
				default:
					Europi.tracks[0].channels[1].steps[selected_step].repetitions = 1;
					break;
			}
		}
	}
}

void init_sequence_old1(void)
{
    sprintf(current_filename,"resources/sequences/default.seq");
    log_msg("Init Seq: %s\n",current_filename);
	FILE * file = fopen(current_filename,"rb");
	if (file != NULL) {
		fread(&Europi, sizeof(struct europi), 1, file);
		fclose(file);
	}
}


/*
 * pre-loads two sequences into Tracks 0 & 1
 * so that there is something there for testing
 * and instant gratification purposes
 */
void init_sequence(void)
{
	int track,channel,step;
	int raw;
	
	//Temp - set all outputs to 0
	for (track=0;track<MAX_TRACKS;track++){
		if (Europi.tracks[track].channels[CV_OUT].enabled == TRUE){
			Europi.tracks[track].track_busy = FALSE;
            Europi.tracks[track].direction = Forwards;
			Europi.tracks[track].last_step =  8; //rand() % 32;

			for (step=0;step<MAX_STEPS;step++){
			Europi.tracks[track].channels[CV_OUT].steps[step].scaled_value = 280; //410;
			}
		}
	}
	//Europi.tracks[0].last_step = 32; /* track 0 always 32 steps */


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
                Europi.tracks[track].channels[1].steps[step].gate_type = Gate_On;
                Europi.tracks[track].channels[1].steps[step].ratchets = 8;
                Europi.tracks[track].channels[1].steps[step].fill = rand()%8;
                Europi.tracks[track].channels[1].steps[step].repetitions = 1;
                Europi.tracks[track].channels[1].steps[step].repeat_counter = 0;
			}
            
			// some ratchets to make it more interesting
/*			Europi.tracks[track].channels[1].steps[0].ratchets = 1;
			Europi.tracks[track].channels[1].steps[0].repetitions = 1;
			Europi.tracks[track].channels[1].steps[1].ratchets = 1;
			Europi.tracks[track].channels[1].steps[5].ratchets = 16;
			Europi.tracks[track].channels[1].steps[6].ratchets = 2;
			Europi.tracks[track].channels[1].steps[6].repetitions = 2;
			Europi.tracks[track].channels[1].steps[9].ratchets = 3;
			Europi.tracks[track].channels[1].steps[10].ratchets = 3;
			Europi.tracks[track].channels[1].steps[13].ratchets = 4;
			Europi.tracks[track].channels[1].steps[15].ratchets = 2;
			Europi.tracks[track].channels[1].steps[16].ratchets = 2;
			Europi.tracks[track].channels[1].steps[17].ratchets = 2;
			Europi.tracks[track].channels[1].steps[21].ratchets = 2;
			Europi.tracks[track].channels[1].steps[22].ratchets = 2;
			Europi.tracks[track].channels[1].steps[25].ratchets = 3;
			Europi.tracks[track].channels[1].steps[26].ratchets = 3;
			Europi.tracks[track].channels[1].steps[29].ratchets = 4;
			Europi.tracks[track].channels[1].steps[31].ratchets = 2;
*/

			// And some slews
			Europi.tracks[track].channels[0].steps[0].slew_length = 30000;
			Europi.tracks[track].channels[0].steps[0].slew_type = Linear;
			Europi.tracks[track].channels[0].steps[0].slew_shape = Rising;			
			Europi.tracks[track].channels[0].steps[2].slew_length = 30000;
			Europi.tracks[track].channels[0].steps[2].slew_type = Linear;
			Europi.tracks[track].channels[0].steps[2].slew_shape = Both;			
			Europi.tracks[track].channels[0].steps[4].slew_length = 30000;
			Europi.tracks[track].channels[0].steps[4].slew_type = Exponential;
			Europi.tracks[track].channels[0].steps[4].slew_shape = Rising;			
			Europi.tracks[track].channels[0].steps[6].slew_length = 30000;
			Europi.tracks[track].channels[0].steps[6].slew_type = RevExp;
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
 * 0 and 10 volts) returns a quantised and scaled value for output
 */
uint16_t scale_value(int track,uint16_t raw_value)
{
	float output_scaling;
    int quantised_value = quantize(raw_value,Europi.tracks[track].channels[CV_OUT].quantise);
    // If the global Tuning Flag is set, then don't quantise the value, just scale it
    if(TuningOn == TRUE) quantised_value = raw_value;
    output_scaling = ((float)Europi.tracks[track].channels[CV_OUT].scale_max - (float)Europi.tracks[track].channels[CV_OUT].scale_zero) / (float)60000;
    //log_msg("Tr: %d Min: %d, Max: %d\n",track,Europi.tracks[track].channels[CV_OUT].scale_zero,Europi.tracks[track].channels[CV_OUT].scale_max);
	return (uint16_t)(output_scaling * quantised_value) + Europi.tracks[track].channels[CV_OUT].scale_zero;
} 

/*
 * Creates an array of file names given
 * a Directory path. NOTE: This ignores
 * anything starting with a ".", specifically
 * the self and parent directories, plus files
 * such as .gitignore
 */
size_t file_list(const char *path, char ***ls) {
    size_t count = 0;
    size_t length = 0;
    DIR *dp = NULL;
    struct dirent *ep = NULL;
    char *p;

    dp = opendir(path);
    if(NULL == dp) {
        log_msg("no such directory: '%s'\n", path);
        return 0;
    }

    *ls = NULL;
    ep = readdir(dp);
    while(NULL != ep){
        p = ep->d_name;
        if(p[0] != 0x2E){
            count++;
        }
        ep = readdir(dp);
    }

    rewinddir(dp);
    *ls = calloc(count, sizeof(char *));

    count = 0;
    ep = readdir(dp);
    while(NULL != ep){
        p = ep->d_name;
        if(p[0] != 0x2E){
            (*ls)[count++] = strdup(ep->d_name);
        }
        ep = readdir(dp);
    }
    closedir(dp);
    return count;
}

/* qsort C-string comparison function */ 
int cstring_cmp(const void *a, const void *b) 
{ 
    const char **ia = (const char **)a;
    const char **ib = (const char **)b;
    return strcmp(*ia, *ib);
} 

/* POLYRHYTHM
 * Takes the number of steps, the fill level, and which step we're on
 * and returns a True / False value as to whether that step should
 * sound or not.
 * 
 * This looks up that particular value in a pre-prepared table of 32x32
 * bitmaps. Prepared according to an implementation of E.Bjoklund's
 * algorithm
 */
int polyrhythm(uint32_t steps, uint32_t fill, uint32_t ThisStep){
    if(fill > steps) fill = steps;
    if(steps > 32 || steps < 2) return TRUE;
    return bjorklund_patterns[steps-2][fill] & (0x01 << ThisStep);
}
/* HARDWARE_CONFIG
 * Compares the current hardware configuration (as recorded at startup)
 * with the hardware config saved to disk and, if they are the same,
 * updates the Scale_zero and Scale_max figures for each channel from
 * the version saved to disk. If the hardware config is different to
 * that saved to disk, it warns the user to run the config routines
 */
void hardware_config(void){
    struct europi_hw SavedConfig;
    
	FILE * file = fopen("resources/hardware.conf","rb");
	if (file != NULL) {
        log_msg("Config file found and read\n");
		fread(&SavedConfig, sizeof(struct europi_hw), 1, file);
		fclose(file);
        //Compare the Read config with the generated config initially
        //to check that the Hardware Configurations are identical
        int Diff = FALSE;
        int track;
        for(track=0;track<MAX_TRACKS;track++){
            if((SavedConfig.hw_tracks[track].hw_channels[CV_OUT].enabled != Europi_hw.hw_tracks[track].hw_channels[CV_OUT].enabled) ||
            (SavedConfig.hw_tracks[track].hw_channels[CV_OUT].type != Europi_hw.hw_tracks[track].hw_channels[CV_OUT].type) ||
            (SavedConfig.hw_tracks[track].hw_channels[CV_OUT].i2c_handle != Europi_hw.hw_tracks[track].hw_channels[CV_OUT].i2c_handle) ||
            (SavedConfig.hw_tracks[track].hw_channels[CV_OUT].i2c_device != Europi_hw.hw_tracks[track].hw_channels[CV_OUT].i2c_device) ||
            (SavedConfig.hw_tracks[track].hw_channels[CV_OUT].i2c_address != Europi_hw.hw_tracks[track].hw_channels[CV_OUT].i2c_address) ||
            (SavedConfig.hw_tracks[track].hw_channels[CV_OUT].i2c_channel != Europi_hw.hw_tracks[track].hw_channels[CV_OUT].i2c_channel) ||		
            (SavedConfig.hw_tracks[track].hw_channels[GATE_OUT].enabled != Europi_hw.hw_tracks[track].hw_channels[GATE_OUT].enabled) ||
            (SavedConfig.hw_tracks[track].hw_channels[GATE_OUT].type != Europi_hw.hw_tracks[track].hw_channels[GATE_OUT].type) ||
            (SavedConfig.hw_tracks[track].hw_channels[GATE_OUT].i2c_handle != Europi_hw.hw_tracks[track].hw_channels[GATE_OUT].i2c_handle) ||
            (SavedConfig.hw_tracks[track].hw_channels[GATE_OUT].i2c_device != Europi_hw.hw_tracks[track].hw_channels[GATE_OUT].i2c_device) ||
            (SavedConfig.hw_tracks[track].hw_channels[GATE_OUT].i2c_channel != Europi_hw.hw_tracks[track].hw_channels[GATE_OUT].i2c_channel)){
            Diff = TRUE;
            }
        }
        if(Diff == FALSE){
            //Current config same as Saved config, so update scale_zero and scale_max from saved config
            for(track=0;track<MAX_TRACKS;track++){
                //Europi.tracks[track].channels[CV_OUT].scale_zero = SavedConfig.hw_tracks[track].hw_channels[CV_OUT].scale_zero;
                //Europi.tracks[track].channels[CV_OUT].scale_max = SavedConfig.hw_tracks[track].hw_channels[CV_OUT].scale_max;
            }
        }
        else{
            //Current Hardware Config is different to the saved config, so warn the user
            log_msg("hardware configuration change detected\n");
            sprintf(modal_dialog_txt1,"New hardware configuration");
            sprintf(modal_dialog_txt2,"detected.");
            sprintf(modal_dialog_txt3,"It is strongly recommended");
            sprintf(modal_dialog_txt4,"that you re-run Setup");
            ClearScreenOverlays();
			ActiveOverlays |= ovl_ModalDialog;
            btnA_func = btnA_none;
            btnB_func = btnB_cancel;
            btnC_func = btnC_OK;
            btnD_func = btnD_none;
        }
	}
    else {
        //Config file doesn't exist, so just save the one generated during startup
        FILE * file = fopen("resources/hardware.conf","wb");
        if (file != NULL) {
            fwrite(&Europi_hw,sizeof(struct europi_hw),1,file);
            fclose(file);
            log_msg("Hardware Config Saved\n");
        }
        else{
            log_msg("Error: Unable to save current Config\n");
        }
        
    }
    
    
}
