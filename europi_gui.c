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

#include <stdio.h>

#include "europi.h"
#include "../raylib/release/rpi/raylib.h"

extern struct europi Europi;

/*
 * GUI_8x8 Attempts to display more detail in a subset of tracks and steps
 * 8 Tracks x 8 Steps
 */
void gui_8x8(void){
    ClearBackground(RAYWHITE);
    int track, column;
    int step, offset, txt_len;
    char txt[20];
    for(track = 0; track < 8; track++){
        // for each track, we need to know where the 
        // current step in the sequence is for that
        // particular track, and display the appropriate
        // 8 steps that contain the current step
        offset = Europi.tracks[track].current_step / 8;
        // Display the step offset at the start of each track
        sprintf(txt,"%d",offset);
        txt_len = MeasureText(txt,20);
        DrawText(txt,20-txt_len,20+(track * 25),20,DARKGRAY);
        for(column=0;column<8;column++){
            if((offset*8)+column == Europi.tracks[track].last_step){
                // Paint last step
                DrawRectangle(30 + (column * 25), 20+(track * 25), 20, 20, BLACK); 
            }
            else if((offset*8)+column == Europi.tracks[track].current_step){
                // Paint current step
                DrawRectangle(30 + (column * 25), 20+(track * 25), 20, 20, LIME);   
            }
            else {
                // paint blank step
                DrawRectangle(30 + (column * 25), 20+(track * 25), 20, 20, MAROON); 
            }

        }        
    }
}

/*
 * GUI_GRID Displays a grid of 32 steps by 24 channels - a bit
 * too dense to be useable on a TFT display, but an interesting
 * experiment in displaying a lot of data on a small screen
 */
void gui_grid(void){
    ClearBackground(RAYWHITE);
    int track;
    int step;
    char track_no[20];
    int txt_len;
    for(track=0;track<24;track++){
        // Track Number
        sprintf(track_no,"%d",track+1);
        txt_len = MeasureText(track_no,10);
        DrawText(track_no,12-txt_len,track * 10,10,DARKGRAY);
        for(step=0;step<32;step++){
            if(step == Europi.tracks[track].last_step){
                // Paint last step
                DrawRectangle(15 + (step * 9), track * 10, 8, 9, BLACK); 
            }
            else if(step == Europi.tracks[track].current_step){
                // Paint current step
                DrawRectangle(15 + (step * 9), track * 10, 8, 9, LIME); 
                // Gate state for current step
                if (Europi.tracks[track].channels[GATE_OUT].steps[Europi.tracks[track].current_step].gate_value == 1){
                    if (Europi.tracks[track].channels[GATE_OUT].steps[Europi.tracks[track].current_step].retrigger > 0) {
                        DrawRectangle(15 + (32 * 9), track * 10, 8, 9, BLACK);	
                    }
                    else {
                        DrawRectangle(15 + (32 * 9), track * 10, 8, 9, VIOLET);	
                    }	
                }
                else {
                    DrawRectangle(15 + (32 * 9), track * 10, 8, 9, WHITE);	
                }
            }
            else {
                // paint blank step
                DrawRectangle(15 + (step * 9), track * 10, 8, 9, MAROON); 
            }
        }
    }
}