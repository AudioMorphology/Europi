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

extern int clock_freq;
extern int step_ticks;
extern int prog_running;
extern int run_stop; 
extern int save_run_stop;
extern int last_track;
extern int SingleChannelOffset;
extern Vector2 touchPosition;
extern int currentGesture1;
extern int lastGesture;
extern menu Menu[]; 
extern char input_txt[]; 
extern char current_filename[];
extern Texture2D KeyboardTexture;
extern Texture2D DialogTexture;
extern Texture2D TextInputTexture;
extern Texture2D Text2chTexture;
extern Texture2D Text5chTexture;
extern Texture2D MainScreenTexture;
extern Texture2D TopBarTexture;
extern Texture2D ButtonBarTexture; 
extern Texture2D VerticalScrollBarTexture;
extern Texture2D ScrollHandleTexture;
extern int VerticalScrollPercent;
extern char *kbd_chars[4][11];
extern char debug_messages[10][80];
extern int next_debug_slot;
extern int kbd_char_selected;
extern enum encoder_focus_t encoder_focus;
extern enum btnA_func_t btnA_func;
extern enum btnB_func_t btnB_func;
extern enum btnC_func_t btnC_func;
extern enum btnD_func_t btnD_func;
extern int btnA_state;
extern int btnB_state;
extern int btnC_state;
extern int btnD_state;
extern int edit_track;
extern int edit_step;
extern struct screen_overlays ScreenOverlays;
extern enum display_page_t DisplayPage;
extern struct europi Europi;
extern char **files;
extern size_t file_count;                      
extern int file_selected;
extern int first_file;
extern int debug;


/*
 * GUI_8x8 Attempts to display more detail in a subset of tracks and steps
 * 8 Tracks x 8 Steps
 */
void gui_8x8(void){
    Rectangle stepRectangle = {0,0,0,0};
    Rectangle trackRectangle = {0,0,0,0};
    int start_track,track, column;
    int step, offset, txt_len;
    char txt[20]; 
    /* Depending on the total number of tracks, and the position 
     * of the vertical scroll bar, the starting track will vary
     */
    if(last_track > 8){
        start_track = ((last_track-8) * VerticalScrollPercent) / 100;
        ScreenOverlays.VerticalScrollBar = 1;
    }
    else {
        start_track = 0;
        ScreenOverlays.VerticalScrollBar = 0;
    }
    BeginDrawing();
    DrawTexture(MainScreenTexture,0,0,WHITE);
    for(track = 0; track < 8; track++){
        // Can only display 8 tracks, so need to know which
        // track we are starting with, and display the next 7
        offset = Europi.tracks[start_track+track].current_step / 8;
        sprintf(txt,"%02d-%d:",start_track+track+1,(offset * 8)+1);
        txt_len = MeasureText(txt,20);
        DrawText(txt,68-txt_len,12+(track * 25),20,DARKGRAY);
        DrawRectangleLines(4,8+(track * 25),67,26,DARKGRAY);
        // Check for Track select
        trackRectangle.x = 4;
        trackRectangle.y = 8 + (track * 25);
        trackRectangle.width = 67;
        trackRectangle.height = 26;
        if(ScreenOverlays.MainMenu == 0){
            // Only if no menus or overlays active
            if (CheckCollisionPointRec(touchPosition, trackRectangle) && (currentGesture1 != GESTURE_NONE)){
                // Open this track in a Single Channel view
                edit_track = start_track+track;
                select_track(start_track+track);
                ClearScreenOverlays();
                DisplayPage = SingleChannel;
                ScreenOverlays.SingleChannel = 1;
                SingleChannelOffset = 0;
                encoder_focus = none;
                btnA_func = btnA_none;
                btnB_func = btnB_tr_minus;
                btnC_func = btnC_tr_plus;
                btnD_func = btnD_done; 
                save_run_stop = run_stop;

            }
        }
        for(column=0;column<8;column++){
            stepRectangle.x = 70 + (column * 25);
            stepRectangle.y = 10 + (track * 25);
            stepRectangle.width = 22;
            stepRectangle.height = 22;
            // Check gesture collision
            if(ScreenOverlays.MainMenu == 0){
                // Only if no Menus or Overlays active
                if (CheckCollisionPointRec(touchPosition, stepRectangle) && (currentGesture1 != GESTURE_NONE)){
                    // Open this step in the Single Step editor
                    edit_track = start_track+track;
                    edit_step = (offset*8)+column;
                    select_track(start_track+track);
                    ClearScreenOverlays();
                    DisplayPage = SingleStep;
                    ScreenOverlays.SingleStep = 1;
                    encoder_focus = none;
                    btnA_func = btnA_none;
                    btnB_func = btnB_prev;
                    btnC_func = btnC_next;
                    btnD_func = btnD_done; 
                    save_run_stop = run_stop;
                }
            }
            if((offset*8)+column == Europi.tracks[start_track+track].last_step){
                // Paint last step
                DrawRectangleRec(stepRectangle, BLACK); 
            }
            else if((offset*8)+column == Europi.tracks[start_track+track].current_step){
                // Paint current step
                DrawRectangleRec(stepRectangle, LIME);   
            }
            else {
                // paint blank step
                DrawRectangleRec(stepRectangle, MAROON); 
            }
        }  
        // Print the end-step number at the RHS of each row
        sprintf(txt,":%d",(offset * 8)+8);
        DrawText(txt,270,12+(track * 25),20,DARKGRAY);

    }
    // Handle any screen overlays - these need to 
    // be added within the Drawing loop
    ShowScreenOverlays();
    EndDrawing();
}
/*
 * GUI_SINGLESTEP enables editing of all params 
 * to do with a single step
 */
void gui_singlestep(void){
    int column, offset, txt_len;
    char txt[20]; 
    Rectangle stepRectangle = {0,0,0,0};
    BeginDrawing();
    DrawTexture(MainScreenTexture,0,0,WHITE);
    // Items to cover:
    // Track Type (CV / MIDI)
    // Quantization
    // Pitch
    // Gate / Trigger
    // Slew
    // Step Repeat
    DrawRectangle(6,29, 308, 183, CLR_LIGHTBLUE);  
    // Draw the current Track 8-Step segement
    offset = Europi.tracks[edit_track].current_step / 8;
    sprintf(txt,"%02d-%d:",edit_track+1,(offset * 8)+1);
    txt_len = MeasureText(txt,20);
    DrawText(txt,68-txt_len,34,20,DARKGRAY);
    for(column=0;column<8;column++){
        stepRectangle.x = 70 + (column * 25);
        stepRectangle.y = 33;
        stepRectangle.width = 22;
        stepRectangle.height = 22;
        // Check gesture collision
        if (CheckCollisionPointRec(touchPosition, stepRectangle) && (currentGesture1 != GESTURE_NONE)){
            // Open this step in the Single Step editor
            edit_step = (offset*8)+column;
            ClearScreenOverlays();
            DisplayPage = SingleStep;
            ScreenOverlays.SingleStep = 1;
            encoder_focus = none;
            btnA_func = btnA_none;
            btnB_func = btnB_prev;
            btnC_func = btnC_next;
            btnD_func = btnD_done; 
            save_run_stop = run_stop;
        }
        else {
            if((offset*8)+column == Europi.tracks[edit_track].last_step){
                // Paint last step
                DrawRectangleRec(stepRectangle, BLACK); 
            }
            else if((offset*8)+column == Europi.tracks[edit_track].current_step){
                // Paint current step
                DrawRectangleRec(stepRectangle, LIME);   
            }
            else if((offset*8)+column == edit_step){
                // Paint step currently being edited
                DrawRectangleRec(stepRectangle, YELLOW);   
            }
            else {
                // paint blank step
                DrawRectangleRec(stepRectangle, MAROON); 
            }
        } 
    }  
    // Print the end-step number at the RHS of each row
    sprintf(txt,":%d",(offset * 8)+8);
    DrawText(txt,270,34,20,DARKGRAY);
    
    DrawText("Quantization:",8,56,20,DARKGRAY);
    DrawText(scale_names[Europi.tracks[edit_track].channels[CV_OUT].quantise],140,56,20,DARKGRAY);
    DrawText("Gate Type:",26,76,20,DARKGRAY);
    DrawTexture(Text5chTexture, 140,76,WHITE);
    DrawText("Slew Type:",30,96,20,DARKGRAY);
    DrawTexture(Text5chTexture, 140,96,WHITE);
    DrawText("Slew Shape:",20,116,20,DARKGRAY);
    DrawTexture(Text5chTexture, 140,116,WHITE);
    // Draw the Pitch 'Bar Graph' display
    DrawRectangleLines(280,76,20,120,BLACK);
    int Octave,Partial,Pitch,i;
    Octave = Europi.tracks[edit_track].channels[CV_OUT].steps[edit_step].raw_value / 6000;
    Partial = Europi.tracks[edit_track].channels[CV_OUT].steps[edit_step].raw_value % 6000;
    //Check for collision within the Partial bar
    stepRectangle.x = 280;
    stepRectangle.y = 76;
    stepRectangle.width = 20;
    stepRectangle.height = 120;
    if (CheckCollisionPointRec(touchPosition, stepRectangle) && (currentGesture1 != GESTURE_NONE)){
        // Work out how far up the Partial bar we are
        if((touchPosition.y >= (float)76) && (touchPosition.y <= (float)(76+120))) {
            Partial = (int)(((120-(touchPosition.y - (float)76)) / (float)(120)) * 5999);
            int newpitch = quantize((6000 * Octave) + Partial,Europi.tracks[edit_track].channels[CV_OUT].quantise);
            Europi.tracks[edit_track].channels[CV_OUT].steps[edit_step].raw_value = newpitch;
            // Update scaled value 
            Europi.tracks[edit_track].channels[CV_OUT].steps[edit_step].scaled_value = scale_value(edit_track,newpitch);
        }
    }

    sprintf(txt,"%d, %d, %d",Europi.tracks[edit_track].channels[CV_OUT].steps[edit_step].raw_value, Octave, Partial);
    DrawText(txt,10,180,20,DARKGRAY);    
    for(i=0;i<10;i++){
        DrawRectangleLines(260,(i*12)+78,15,10,BLACK);
        if((9-i) == Octave){
            // Colour the current Octave
            DrawRectangle(261,(i*12)+78+1,13,8,RED);
        }
        //Check for collision within the Octave Bar
        stepRectangle.x = 260;
        stepRectangle.y = (i*12)+78;
        stepRectangle.width = 15;
        stepRectangle.height = 10;
        if (CheckCollisionPointRec(touchPosition, stepRectangle) && (currentGesture1 != GESTURE_NONE)){
            // Set the Octave for this Step & Quantize the RAW value
            int newpitch = quantize((6000 * (9-i)) + Partial,Europi.tracks[edit_track].channels[CV_OUT].quantise);
            Europi.tracks[edit_track].channels[CV_OUT].steps[edit_step].raw_value = newpitch;
            // Update scaled value 
            Europi.tracks[edit_track].channels[CV_OUT].steps[edit_step].scaled_value = scale_value(edit_track,newpitch);
        }
    }
    // Fill the partial column = this is the percentage of an 
    // Octave, so 6000 would == 120 pixels
    Pitch = (Partial * 120) / 6000;
    DrawRectangle(281,76+120-Pitch-1,18,Pitch,BLUE); 
    
    // Handle any screen overlays - these need to 
    // be added within the Drawing loop
    ShowScreenOverlays();
    EndDrawing();
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
                if (Europi.tracks[track].channels[GATE_OUT].steps[Europi.tracks[track].current_step].gate_type != Gate_Off){
                    if (Europi.tracks[track].channels[GATE_OUT].steps[Europi.tracks[track].current_step].repetitions > 1) {
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
/*
 * gui_SingleChannel displays just the currently
 * selected channel on a page of its own, but only
 * in blocks of 8 steps
 */
void gui_SingleChannel(void){
    Rectangle touchRectangle = {0,0,0,0};
    int track, step, column, i;
    int Octave, Partial, Pitch;
    char track_no[20];
    char step_no[5];
    int txt_len;
    int row;
    Rectangle stepRectangle = {0,0,0,0};
    BeginDrawing();
    DrawTexture(MainScreenTexture,0,0,WHITE);
    for(track = 0; track < last_track; track++){
        if(Europi.tracks[track].selected == TRUE){
            for(column = 0; column < 8; column++){
                Octave = Europi.tracks[track].channels[CV_OUT].steps[SingleChannelOffset+column].raw_value / 6000;
                Partial = Europi.tracks[track].channels[CV_OUT].steps[SingleChannelOffset+column].raw_value % 6000;
                //Check for collision within the Partial bar
                touchRectangle.x = 22+(column*39);
                touchRectangle.y = 30;
                touchRectangle.width = 18;
                touchRectangle.height = 120;
                if (CheckCollisionPointRec(touchPosition, touchRectangle) && (currentGesture1 != GESTURE_NONE)){
                    // Work out how far up the Partial bar we are
                    if((touchPosition.y >= (float)30) && (touchPosition.y <= (float)(30+120))) {
                        Partial = (int)(((120-(touchPosition.y - (float)30)) / (float)(120)) * 5999);
                        int newpitch = quantize((6000 * Octave) + Partial,Europi.tracks[track].channels[CV_OUT].quantise);
                        Europi.tracks[track].channels[CV_OUT].steps[SingleChannelOffset+column].raw_value = newpitch;
                        // Update scaled value 
                        Europi.tracks[track].channels[CV_OUT].steps[SingleChannelOffset+column].scaled_value = scale_value(track,newpitch);
                    }
                }
                //Column of Octave buttons
                for(i=0;i<10;i++){
                    DrawRectangleLines(6+(column*39),(i*12)+32,15,10,BLACK);
                    if((9-i) == Octave){
                        // Colour the current Octave
                        if(SingleChannelOffset+column == Europi.tracks[track].current_step){
                            DrawRectangle(6+(column*39)+1,(i*12)+32+1,13,8,LIME);
                        }
                        else{
                            DrawRectangle(6+(column*39)+1,(i*12)+32+1,13,8,RED);
                        }
                    }
                    //Check for collision within the Octave Bar
                    touchRectangle.x = 6+(column*39);
                    touchRectangle.y = (i*12)+32;
                    touchRectangle.width = 15;
                    touchRectangle.height = 10;
                    if (CheckCollisionPointRec(touchPosition, touchRectangle) && (currentGesture1 != GESTURE_NONE)){
                        // Set the Octave for this Step & Quantize the RAW value
                        int newpitch = quantize((6000 * (9-i)) + Partial,Europi.tracks[track].channels[CV_OUT].quantise);
                        Europi.tracks[track].channels[CV_OUT].steps[SingleChannelOffset+column].raw_value = newpitch;
                        // Update scaled value 
                        Europi.tracks[track].channels[CV_OUT].steps[SingleChannelOffset+column].scaled_value = scale_value(track,newpitch);
                    }

                }
                // Fill the partial column = this is the percentage of an 
                // Octave, so 6000 would == 120 pixels
                Pitch = (Partial * 120) / 6000;
                if(SingleChannelOffset+column == Europi.tracks[track].current_step){
                    DrawRectangle(22+(column*39)+1,30+120-Pitch,16,Pitch,LIME); 
                }
                else {
                    DrawRectangle(22+(column*39)+1,30+120-Pitch,16,Pitch,BLUE); 
                }
                //Fill the column THEN draw the Partial bar, though I actually
                //prefer the display without this box round the partial bar, hence
                //why it is commented out.
                /*DrawRectangleLines(22+(column*39),30,18,120,BLACK);*/
                //Step number boxes
                DrawRectangleLines(6+(column*39),152,33,20,BLACK);
                // Centre the Step Number in the box
                sprintf(step_no,"%d",SingleChannelOffset+column+1);
                txt_len = MeasureText(step_no,20);
                DrawText(step_no,22+(column*39)-(txt_len/2),153,20,DARKGRAY);
                //Check for tap within the Step Number box
                touchRectangle.x = 6+(column*39);
                touchRectangle.y = 160; //Ignores touches in the top few pixels, as it's close to the Partial bar
                touchRectangle.width = 33;
                touchRectangle.height = 12;
                if (CheckCollisionPointRec(touchPosition, touchRectangle) && (currentGesture1 != GESTURE_NONE)){
                    //Open this Step in the Single Step editor
                    edit_track = track;
                    edit_step = SingleChannelOffset+column;
                    ClearScreenOverlays();
                    DisplayPage = SingleStep;
                    ScreenOverlays.SingleStep = 1;
                    encoder_focus = none;
                    btnA_func = btnA_none;
                    btnB_func = btnB_prev;
                    btnC_func = btnC_next;
                    btnD_func = btnD_done; 
                    save_run_stop = run_stop;

                }
                //Draw the gate lane for this Step
                touchRectangle.y = 175;
                touchRectangle.width = 8;
                touchRectangle.height = 8;
                touchRectangle.x = 6+(column*39);
                Color gate_colour;
                if(SingleChannelOffset+column == Europi.tracks[track].current_step){
                    gate_colour = LIME;
                }
                else{
                    gate_colour = BLUE;
                }
                if(Europi.tracks[track].channels[GATE_OUT].steps[SingleChannelOffset+column].ratchets <= 1){
                    switch(Europi.tracks[track].channels[GATE_OUT].steps[SingleChannelOffset+column].gate_type){
                        case    Gate_On: 
                            touchRectangle.width = 39;
                            DrawRectangleRec(touchRectangle,gate_colour);
                        break;
                        case    Trigger: 
                            touchRectangle.width = 3;
                            DrawRectangleRec(touchRectangle,gate_colour);
                        break;
                        case    Gate_25:
                            touchRectangle.width = 9;
                            DrawRectangleRec(touchRectangle,gate_colour);
                        break;
                        case    Gate_50:
                            touchRectangle.width = 19;
                            DrawRectangleRec(touchRectangle,gate_colour);
                        break;
                        case    Gate_75:
                            touchRectangle.width = 29;
                            DrawRectangleRec(touchRectangle,gate_colour);
                        break;
                        case    Gate_95:
                            touchRectangle.width = 37;
                            DrawRectangleRec(touchRectangle,gate_colour);
                        break;
                        case    Gate_Off:
                        default:
                        break;
                    }
                }
                else {
                    // Ratchetting Gate
                    if(Europi.tracks[track].channels[GATE_OUT].steps[SingleChannelOffset+column].ratchets <= 16){
                        int GateWidth = 38 / Europi.tracks[track].channels[GATE_OUT].steps[SingleChannelOffset+column].ratchets - 1;
                        touchRectangle.width = GateWidth;
                        for(i=0;i<Europi.tracks[track].channels[GATE_OUT].steps[SingleChannelOffset+column].ratchets;i++){
                            if(polyrhythm(Europi.tracks[track].channels[GATE_OUT].steps[SingleChannelOffset+column].ratchets,Europi.tracks[track].channels[GATE_OUT].steps[SingleChannelOffset+column].fill,i)){
                                DrawRectangleRec(touchRectangle,gate_colour);
                            }
                            touchRectangle.x += GateWidth + 1;
                        }
                    }
                    else {
                        // Too many ratchets to show meaningfully, so just draw a 95% fill line
                        touchRectangle.width = 38;
                        DrawRectangleRec(touchRectangle,gate_colour);
                    }
                }
            }
            // Draw all Steps in the track along the bottom of the screen
            int x = 14;
            for(step = 0; step<Europi.tracks[track].last_step; step++){
                if(step == SingleChannelOffset){
                    //Highlight behind current block of 8 steps
                    touchRectangle.x = x+1;
                    touchRectangle.y = 202;
                    touchRectangle.width = 8*9+1;
                    touchRectangle.height = 10;
                    DrawRectangleRec(touchRectangle,DARKGRAY);
                    x += 2;
                }
                if (step == SingleChannelOffset+8) x+= 2;
                touchRectangle.x = x;
                touchRectangle.y = 203;
                touchRectangle.width = 8;
                touchRectangle.height = 8;
                if(step == Europi.tracks[track].current_step){
                    DrawRectangleRec(touchRectangle,LIME);
                }
                else{
                    DrawRectangleRec(touchRectangle,RED);
                }
                if (CheckCollisionPointRec(touchPosition, touchRectangle) && (currentGesture1 != GESTURE_NONE)){
                    //Move the displayed 8-step segment to this position
                    if(Europi.tracks[track].last_step >= 8){
                        if(step <= (Europi.tracks[track].last_step - 8)) SingleChannelOffset = step;
                        else SingleChannelOffset = Europi.tracks[track].last_step - 8;
                    }
                }
                x += 9;
            }
        }
        
    }
    // Handle any screen overlays - these need to 
    // be added within the Drawing loop
    ShowScreenOverlays();
    EndDrawing();
}


/*
 * gui_SingleChannel displays just the currently
 * selected channel on a page of its own
 */
void gui_SingleChannel_old2(void){
    int track;
    int step;
    int val;
    char track_no[20];
    int row;
    Rectangle stepRectangle = {0,0,0,0};
    BeginDrawing();
    DrawTexture(MainScreenTexture,0,0,WHITE);
    for (track = 0; track < MAX_TRACKS; track++){
        if (Europi.tracks[track].selected == TRUE){
            sprintf(track_no,"%d",track+1);
            for (step = 0; step < MAX_STEPS; step++){
                row = step / 16;
                if(step < Europi.tracks[track].last_step){
                    int Octave = (int)((float)Europi.tracks[track].channels[CV_OUT].steps[step].scaled_value / (float)6000);
                    val = (int)(((float)Europi.tracks[track].channels[CV_OUT].steps[step].scaled_value - (Octave * 6000)) / (float)60);
                    stepRectangle.x = ((step-(row*16)) * 18)+20;
                    stepRectangle.y = ((row+1) * 100)-val;
                    /* Width of the bar is the Octave */
                    stepRectangle.width = Octave+5;
                    stepRectangle.height = val;
                    if(step == Europi.tracks[track].current_step){
                        DrawRectangleRec(stepRectangle,MAROON);
                    }
                    else{
                        DrawRectangleRec(stepRectangle,LIME);
                    }
/*                    // Gate State
                    if (Europi.tracks[track].channels[GATE_OUT].steps[step].gate_value == 1){
                        sprintf(track_no,"%d",Europi.tracks[track].channels[GATE_OUT].steps[step].retrigger);
                        DrawText(track_no,15 + (step*9),220,10,DARKGRAY);
                    }
                    // Slew
                    if (Europi.tracks[track].channels[CV_OUT].steps[step].slew_type != Off){
                        switch (Europi.tracks[track].channels[CV_OUT].steps[step].slew_shape){
                            case Both:
                                DrawText("V",15 + (step*9),230,10,DARKGRAY);
                            break;
                            case Rising:
                                DrawText("/",15 + (step*9),230,10,DARKGRAY);
                            break;
                            case Falling:
                                DrawText("\\",15 + (step*9),230,10,DARKGRAY);
                            break;
                        }
                    } */
                }
                /* Draws a block to show the last step (provided it's less than MAX_STEPS) */
                if(step == Europi.tracks[track].last_step - 1) {
                    val = (int)(((float)Europi.tracks[track].channels[CV_OUT].steps[step].scaled_value / (float)10000) * 100);
                    stepRectangle.x += 18;
                    stepRectangle.y = ((row+1) * 100)-val;
                    stepRectangle.width = 5;
                    stepRectangle.height = val;
                    DrawRectangleRec(stepRectangle,BLACK);
                }
            }
        }
    }
    // Handle any screen overlays - these need to 
    // be added within the Drawing loop
    ShowScreenOverlays();
    EndDrawing();
}

void gui_SingleChannel_old1(void){
    int track;
    int step;
    int val;
    char track_no[20];
    int row;
    Rectangle stepRectangle = {0,0,0,0};
    BeginDrawing();
    DrawTexture(MainScreenTexture,0,0,WHITE);
    for (track = 0; track < MAX_TRACKS; track++){
        if (Europi.tracks[track].selected == TRUE){
            sprintf(track_no,"%d",track+1);
            for (step = 0; step < MAX_STEPS; step++){
                row = step / 16;
                if(step < Europi.tracks[track].last_step){
                    //val = (int)(((float)Europi.tracks[track].channels[CV_OUT].steps[step].scaled_value / (float)60000) * 55);
                    val = (int)(((float)Europi.tracks[track].channels[CV_OUT].steps[step].scaled_value / (float)10000) * 100);
                    stepRectangle.x = ((step-(row*16)) * 18)+20;
                    stepRectangle.y = ((row+1) * 100)-val;
                    stepRectangle.width = 15;
                    stepRectangle.height = val;
                    if(step == Europi.tracks[track].current_step){
                        DrawRectangleRec(stepRectangle,MAROON);
                    }
                    else{
                        DrawRectangleRec(stepRectangle,LIME);
                    }
/*                    // Gate State
                    if (Europi.tracks[track].channels[GATE_OUT].steps[step].gate_value == 1){
                        sprintf(track_no,"%d",Europi.tracks[track].channels[GATE_OUT].steps[step].retrigger);
                        DrawText(track_no,15 + (step*9),220,10,DARKGRAY);
                    }
                    // Slew
                    if (Europi.tracks[track].channels[CV_OUT].steps[step].slew_type != Off){
                        switch (Europi.tracks[track].channels[CV_OUT].steps[step].slew_shape){
                            case Both:
                                DrawText("V",15 + (step*9),230,10,DARKGRAY);
                            break;
                            case Rising:
                                DrawText("/",15 + (step*9),230,10,DARKGRAY);
                            break;
                            case Falling:
                                DrawText("\\",15 + (step*9),230,10,DARKGRAY);
                            break;
                        }
                    } */
                }
            }
        }
    }
    // Handle any screen overlays - these need to 
    // be added within the Drawing loop
    ShowScreenOverlays();
    EndDrawing();
}

void gui_SingleChannel_Old(void){
    int track;
    int step;
    int val;
    char track_no[20];
    int txt_len;
    Rectangle stepRectangle = {0,0,0,0};
    BeginDrawing();
    ClearBackground(RAYWHITE);
    for (track = 0; track < MAX_TRACKS; track++){
        if (Europi.tracks[track].selected == TRUE){
            sprintf(track_no,"%d",track+1);
            txt_len = MeasureText(track_no,10);
            DrawText(track_no,12-txt_len,220,10,DARKGRAY);
            for (step = 0; step < MAX_STEPS; step++){
                if(step < Europi.tracks[track].last_step){
                    val = (int)(((float)Europi.tracks[track].channels[CV_OUT].steps[step].scaled_value / (float)60000) * 220);
                    if(step == Europi.tracks[track].current_step){
                        DrawRectangle(15 + (step*9),220-val,8,val,MAROON);
                    }
                    else{
                        DrawRectangle(15 + (step*9),220-val,8,val,LIME);
                    }
                    // Gate State
                    if (Europi.tracks[track].channels[GATE_OUT].steps[step].gate_type != Gate_Off){
                        sprintf(track_no,"%d",Europi.tracks[track].channels[GATE_OUT].steps[step].repetitions);
                        DrawText(track_no,15 + (step*9),220,10,DARKGRAY);
                    }
                    // Slew
                    if (Europi.tracks[track].channels[CV_OUT].steps[step].slew_type != Off){
                        switch (Europi.tracks[track].channels[CV_OUT].steps[step].slew_shape){
                            case Both:
                                DrawText("V",15 + (step*9),230,10,DARKGRAY);
                            break;
                            case Rising:
                                DrawText("/",15 + (step*9),230,10,DARKGRAY);
                            break;
                            case Falling:
                                DrawText("\\",15 + (step*9),230,10,DARKGRAY);
                            break;
                        }
                    }
                }
            }
        }
    }
    // Handle any screen overlays - these need to 
    // be added within the Drawing loop
    ShowScreenOverlays();
    EndDrawing();
}


/*
 * ShowScreenOverlays is called from within the 
 * drawing loop of each main type of display
 * page, and enables Menus, user input controls
 * etc to be overlayed on top of the currently
 * displayed page
 */
void ShowScreenOverlays(void){
    if(ScreenOverlays.MainMenu == 1){
        ClearScreenOverlays();
        ScreenOverlays.MainMenu = 1;
        gui_MainMenu();
    }
    if(ScreenOverlays.SetLoop == 1){
        int track = 0;
        char str[80];
        char strTrack[5];
        char strLoop[5];
        DrawTexture(TopBarTexture,0,0,WHITE);
        DrawTexture(Text2chTexture,70,2,WHITE); // Box for Track Number
        DrawTexture(Text2chTexture,185,2,WHITE); // Box for Step Number
        DrawText("Track",5,5,20,DARKGRAY);
        DrawText("Length",112,5,20,DARKGRAY);
        
        for(track = 0; track < MAX_TRACKS; track++) {
            if (Europi.tracks[track].selected == TRUE){
                sprintf(strTrack,"%02d",track+1);
                sprintf(strLoop,"%02d",Europi.tracks[track].last_step);
                DrawText(strTrack,75,5,20,DARKGRAY);
                DrawText(strLoop,190,5,20,DARKGRAY);
                if(encoder_focus == track_select){
                    DrawRectangleLines(71,3,30,22,RED);
                    DrawRectangleLines(72,4,28,20,RED);
                }
                else if (encoder_focus == set_loop) {
                    DrawRectangleLines(186,3,30,22,RED);
                    DrawRectangleLines(187,4,28,20,RED);
                }
            }
        }
        // Check for Select button
        if (btnA_state == 1){
            btnA_state = 0;
            if(encoder_focus == track_select) encoder_focus = set_loop;
            else encoder_focus = track_select;
        }
        if (btnB_state == 1){
            // Check for Val -
            btnB_state = 0;
            if(encoder_focus == track_select) select_next_track(DOWN);
            else set_loop_point(DOWN);
        }
        if (btnC_state == 1){
            // Check for Val +
            btnC_state = 0;
            if(encoder_focus == track_select) select_next_track(UP);
            else set_loop_point(UP);
        }

    }
    if(ScreenOverlays.SetSlew == 1){
        //TODO: This isn't finished yet, and probably needs a 
        //double-height control panel to get all the detail in 
        int track = 0;
        char strTrack[5];
        char strStep[5];
        char strSlew[9];
        DrawTexture(TopBarTexture,0,0,WHITE);
        DrawTexture(Text2chTexture,70,2,WHITE); // Box for Track Number
        DrawTexture(Text2chTexture,155,2,WHITE); // Box for Step Number
        DrawTexture(Text5chTexture,245,2,WHITE); // Box for Slew Value
        
        DrawText("Track",5,5,20,DARKGRAY);
        DrawText("Step",105,5,20,DARKGRAY);
        DrawText("Slew",190,5,20,DARKGRAY);
        
        for(track = 0; track < MAX_TRACKS; track++) {
            if (Europi.tracks[track].selected == TRUE){
                sprintf(strTrack,"%02d",track+1);
                sprintf(strStep,"%02d",Europi.tracks[track].current_step+1);
                sprintf(strSlew,"%05d",Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].raw_value);
                DrawText(strTrack,75,5,20,DARKGRAY);
                DrawText(strStep,160,5,20,DARKGRAY);
                DrawText(strSlew,250,5,20,DARKGRAY);
                if(encoder_focus == track_select){
                    DrawRectangleLines(71,3,30,22,RED);
                    DrawRectangleLines(72,4,28,20,RED);
                }
                else if (encoder_focus == step_select) {
                    DrawRectangleLines(156,3,30,22,RED);
                    DrawRectangleLines(157,4,28,20,RED);
                }
                else if (encoder_focus == slew_type){
                    DrawRectangleLines(246,3,66,22,RED);
                    DrawRectangleLines(247,4,64,20,RED);
                }
            }
        }
        // Check for Select button
        if (btnA_state == 1){
            btnA_state = 0;
            if(encoder_focus == track_select) encoder_focus = step_select;
            else if(encoder_focus == step_select) encoder_focus = slew_type;
            else encoder_focus = track_select;
        }
        if (btnB_state == 1){
            // Check for Val -
            btnB_state = 0;
            if(encoder_focus == track_select) select_next_track(DOWN);
            else set_loop_point(DOWN);
        }
        if (btnC_state == 1){
            // Check for Val +
            btnC_state = 0;
            if(encoder_focus == track_select) select_next_track(UP);
            else set_loop_point(UP);
        }

    }
    if(ScreenOverlays.SetPitch == 1){ 
        int track = 0;
        char strTrack[5];
        char strStep[5];
        char strPitch[9];
        DrawTexture(TopBarTexture,0,0,WHITE);
        DrawTexture(Text2chTexture,70,2,WHITE); // Box for Track Number
        DrawTexture(Text2chTexture,155,2,WHITE); // Box for Step Number
        DrawTexture(Text5chTexture,245,2,WHITE); // Box for Pitch Value
        
        DrawText("Track",5,5,20,DARKGRAY);
        DrawText("Step",105,5,20,DARKGRAY);
        DrawText("Pitch",190,5,20,DARKGRAY);
        for(track = 0; track < MAX_TRACKS; track++) {
            if (Europi.tracks[track].selected == TRUE){
                sprintf(strTrack,"%02d",track+1);
                sprintf(strStep,"%02d",Europi.tracks[track].current_step+1);
                sprintf(strPitch,"%05d",Europi.tracks[track].channels[CV_OUT].steps[Europi.tracks[track].current_step].raw_value);
                DrawText(strTrack,75,5,20,DARKGRAY);
                DrawText(strStep,160,5,20,DARKGRAY);
                DrawText(strPitch,250,5,20,DARKGRAY);
                if(encoder_focus == track_select){
                    DrawRectangleLines(71,3,30,22,RED);
                    DrawRectangleLines(72,4,28,20,RED);
                }
                else if (encoder_focus == step_select) {
                    DrawRectangleLines(156,3,30,22,RED);
                    DrawRectangleLines(157,4,28,20,RED);
                }
                else if (encoder_focus == set_pitch){
                    DrawRectangleLines(246,3,66,22,RED);
                    DrawRectangleLines(247,4,64,20,RED);
                }
            }
        }
         // Check for Select button
        if (btnA_state == 1){
            btnA_state = 0;
            switch(encoder_focus){
                case track_select:
                    encoder_focus = step_select;
                break;
                case step_select:
                    encoder_focus = set_pitch;
                break;
                case set_pitch:
                default:
                    encoder_focus = track_select;
                break;
            }
        }
        if (btnB_state == 1){
            // Check for Val -
            btnB_state = 0;
            switch(encoder_focus){
                case track_select:
                    select_next_track(DOWN);
                break;
                case step_select:
                    select_next_step(DOWN);
                break;
                case set_pitch:
                    set_step_pitch(DOWN,1);  // no velocity available
                break;
                default:
                break;
            }
        }
        if (btnC_state == 1){
            // Check for Val +
            btnC_state = 0;
            switch(encoder_focus){
                case track_select:
                    select_next_track(UP);
                break;
                case step_select:
                    select_next_step(UP);
                break;
                case set_pitch:
                    set_step_pitch(UP,1);  // no velocity available
                break;
                default:
                break;
            }
        }
       
    }
    if(ScreenOverlays.SetQuantise == 1){
        int track = 0;
        char str[80];
        char strTrack[5];
        char strScale[30];
        DrawTexture(TopBarTexture,0,0,WHITE);
        DrawTexture(Text2chTexture,70,2,WHITE); 
        DrawTexture(TextInputTexture,103,2,WHITE);
        DrawText("Track",5,5,20,DARKGRAY);
        
        for(track = 0; track < MAX_TRACKS; track++) {
            if (Europi.tracks[track].selected == TRUE){
                sprintf(strTrack,"%02d",track+1);
                sprintf(strScale,"%s",scale_names[Europi.tracks[track].channels[CV_OUT].quantise]);
                DrawText(strTrack,75,5,20,DARKGRAY);
                DrawText(strScale,110,5,20,DARKGRAY);
                if(encoder_focus == track_select){
                    DrawRectangleLines(71,3,30,22,RED);
                    DrawRectangleLines(72,4,28,20,RED);
                }
                else if (encoder_focus == set_quantise) {
                    DrawRectangleLines(103,3,213,22,RED);
                    DrawRectangleLines(104,4,211,20,RED);

                }
            }
        }
        // Check for Select button
        if (btnA_state == 1){
            btnA_state = 0;
            if(encoder_focus == track_select) encoder_focus = set_quantise;
            else encoder_focus = track_select;
        }
        if (btnB_state == 1){
            // Check for Val -
            btnB_state = 0;
            if(encoder_focus == track_select) select_next_track(DOWN);
            else select_next_quantisation(DOWN);
        }
        if (btnC_state == 1){
            // Check for Val +
            btnC_state = 0;
            if(encoder_focus == track_select) select_next_track(UP);
            else select_next_quantisation(UP);
        }
 
   }

    if(ScreenOverlays.SetDirection == 1){   // track Direction
        int track = 0;
        char str[80];
        char strTrack[5];
        char strDirection[20];
        DrawTexture(TopBarTexture,0,0,WHITE);
        DrawTexture(Text2chTexture,70,2,WHITE); 
        DrawTexture(TextInputTexture,103,2,WHITE);
        DrawText("Track",5,5,20,DARKGRAY);
        for(track = 0; track < MAX_TRACKS; track++) {
            if (Europi.tracks[track].selected == TRUE){
                sprintf(strTrack,"%02d",track+1);
                DrawText(strTrack,75,5,20,DARKGRAY);
                switch (Europi.tracks[track].direction){
                    default:
                    case Forwards:
                        DrawText("Forwards",110,5,20,DARKGRAY);
                    break;
                    case Backwards:
                        DrawText("Backwards",110,5,20,DARKGRAY);
                    break;
                    case Pendulum_F:
                    case Pendulum_B:
                        DrawText("Pendulum",110,5,20,DARKGRAY);
                    break;
                    case Random:
                        DrawText("Random",110,5,20,DARKGRAY);
                    break;
                    
                }
                if(encoder_focus == track_select){
                    DrawRectangleLines(71,3,30,22,RED);
                    DrawRectangleLines(72,4,28,20,RED);
                }
                else if (encoder_focus == set_direction) {
                    DrawRectangleLines(103,3,213,22,RED);
                    DrawRectangleLines(104,4,211,20,RED);

                }
            }
        }
        // Check for Select button
        if (btnA_state == 1){
            btnA_state = 0;
            if(encoder_focus == track_select) encoder_focus = set_direction;
            else encoder_focus = track_select;
        }
        if (btnB_state == 1){
            // Check for Val -
            btnB_state = 0;
            if(encoder_focus == track_select) select_next_track(DOWN);
            else select_next_direction(DOWN);
        }
        if (btnC_state == 1){
            // Check for Val +
            btnC_state = 0;
            if(encoder_focus == track_select) select_next_track(UP);
            else select_next_direction(UP);
        }
 
   }

    
    
    if(ScreenOverlays.Keyboard == 1){
//        lastGesture = currentGesture;
        Rectangle btnHighlight = {0,0,0,0};
//        touchPosition = GetTouchPosition(0);
//        currentGesture = GetGestureDetected();
        int button;
        int row, col;
        DrawTexture(KeyboardTexture,KBD_GRID_TL_X,KBD_GRID_TL_Y,WHITE);
        for(button=0;button < (KBD_ROWS * KBD_COLS);button++){
            row = button / KBD_COLS;
            col = button % KBD_COLS;
            btnHighlight.x = (KBD_GRID_TL_X + KBD_BTN_TL_X) + (col * KBD_COL_WIDTH);
            btnHighlight.y = (KBD_GRID_TL_Y + KBD_BTN_TL_Y) + (row * KBD_ROW_HEIGHT);
            btnHighlight.width = KBD_BTN_WIDTH;
            btnHighlight.height = KBD_BTN_HEIGHT;
            if(button == kbd_char_selected){
                //Highlight this button
                DrawRectangleLines((KBD_GRID_TL_X + KBD_BTN_TL_X) + (col * KBD_COL_WIDTH),
                (KBD_GRID_TL_Y + KBD_BTN_TL_Y) + (row * KBD_ROW_HEIGHT),
                KBD_BTN_WIDTH,
                KBD_BTN_HEIGHT,WHITE);
            }
            // Check for touch input
            if (CheckCollisionPointRec(touchPosition, btnHighlight) && (currentGesture1 != GESTURE_NONE)){
                if(currentGesture1 != lastGesture){
                    kbd_char_selected = button;
                    row = button / KBD_COLS;
                    col = button % KBD_COLS;
                    //Add this to the input_txt buffer
                    sprintf(input_txt,"%s%s", input_txt,kbd_chars[row][col]);
                }
            }
        }
    }
    if(ScreenOverlays.FileOpen == 1){
        Rectangle fileHighlight = {0,0,0,0};
        //touchPosition = GetTouchPosition(0);
        //currentGesture = GetGestureDetected();
        DrawTexture(DialogTexture,0,0,WHITE);
        DrawText("File Open",10,5,20,DARKGRAY);
        // List the files
        int i;
        int j=0;
        for(i=0;i<file_count;i++){
            if((i >= first_file) & (i < first_file + DLG_ROWS)){
                fileHighlight.x=5;
                fileHighlight.y=27+(j*20);
                fileHighlight.width=310;
                fileHighlight.height=20;
                // Check for touch input
                if (CheckCollisionPointRec(touchPosition, fileHighlight) && (currentGesture1 != GESTURE_NONE)){
                    file_selected = i;
                }

                if(i == file_selected) {
                    //highlight this file
                    DrawRectangleRec(fileHighlight,LIGHTGRAY);
                }
                DrawText(files[i],10,27+(j*20),20,DARKGRAY);
                j++;
            }
        }
        // Check for Open button
        if (btnB_state == 1){
            // Open Button Touched
            btnB_state = 0;
            //char filename[100]; 
            snprintf(current_filename, 100, "resources/sequences/%s", files[file_selected]);
            load_sequence(current_filename);
            ClearScreenOverlays();
            buttonsDefault();
        }
        if (btnC_state == 1){
            // Cancel Button Touched
            btnC_state = 0;
            ClearScreenOverlays();
            buttonsDefault();
        }
        
    }
    if(ScreenOverlays.FileSaveAs == 1){
        // Top data entry bar
        DrawTexture(TopBarTexture,0,0,WHITE);
        DrawTexture(TextInputTexture,103,1,WHITE);
        DrawText("Save As:",10,MENU_TOP_MARGIN,20,DARKGRAY);
        DrawText(input_txt,110,4,20,DARKGRAY);
        // Keyboard
         Rectangle btnHighlight = {0,0,0,0};
        int button;
        int row, col;
        DrawTexture(KeyboardTexture,KBD_GRID_TL_X,KBD_GRID_TL_Y,WHITE);
        for(button=0;button < (KBD_ROWS * KBD_COLS);button++){
            row = button / KBD_COLS;
            col = button % KBD_COLS;
            btnHighlight.x = (KBD_GRID_TL_X + KBD_BTN_TL_X) + (col * KBD_COL_WIDTH);
            btnHighlight.y = (KBD_GRID_TL_Y + KBD_BTN_TL_Y) + (row * KBD_ROW_HEIGHT);
            btnHighlight.width = KBD_BTN_WIDTH;
            btnHighlight.height = KBD_BTN_HEIGHT;
            if(button == kbd_char_selected){
                //Highlight this button
                DrawRectangleLines((KBD_GRID_TL_X + KBD_BTN_TL_X) + (col * KBD_COL_WIDTH),
                (KBD_GRID_TL_Y + KBD_BTN_TL_Y) + (row * KBD_ROW_HEIGHT),
                KBD_BTN_WIDTH,
                KBD_BTN_HEIGHT,WHITE);
            }
            // Check for touch input
            if (CheckCollisionPointRec(touchPosition, btnHighlight) && (currentGesture1 != GESTURE_NONE)){
                if(currentGesture1 != lastGesture){
                    kbd_char_selected = button;
                    row = button / KBD_COLS;
                    col = button % KBD_COLS;
                    //Add this to the input_txt buffer
                    sprintf(input_txt,"%s%s", input_txt,kbd_chars[row][col]);
                }
            }
        }       
    }
    if(ScreenOverlays.VerticalScrollBar == 1){
        // Vertical scroll bar on RHS of screen
        DrawTexture(VerticalScrollBarTexture,303,4,WHITE);
        int ScrollHandlePosn = (((VERTICAL_SCROLL_MAX-VERTICAL_SCROLL_MIN) * VerticalScrollPercent) / 100) + VERTICAL_SCROLL_MIN;
        DrawTexture(ScrollHandleTexture,304,ScrollHandlePosn,WHITE);
        // test rectangles for buttons
        Rectangle scrollUpButton = {303,4,13,15};
        Rectangle scrollDownButton = {303,198,13,15};
        Rectangle scrollHandleRec = {303,ScrollHandlePosn-2,14,15}; //note: setting the collision box a couple of pixels above the actual button makes it easier to drag upwards!!
        if (CheckCollisionPointRec(touchPosition, scrollUpButton) && (currentGesture1 != GESTURE_NONE)){
            DrawRectangleLines(304, 4, 12, 14, CLR_DARKBLUE);
            DrawRectangleLines(305, 5, 10, 12, CLR_DARKBLUE);
            if(VerticalScrollPercent >= 1) VerticalScrollPercent--;
        }
        if (CheckCollisionPointRec(touchPosition, scrollDownButton) && (currentGesture1 != GESTURE_NONE)){
            DrawRectangleLines(304, 199, 12, 14, CLR_DARKBLUE); 
            DrawRectangleLines(305, 200, 10, 12, CLR_DARKBLUE); 
            if(VerticalScrollPercent <= 99) VerticalScrollPercent++;
        }
        
        if (CheckCollisionPointRec(touchPosition, scrollHandleRec) && (currentGesture1 == GESTURE_DRAG)){
            DrawRectangleLines(303, ScrollHandlePosn, 14, 13, CLR_DARKBLUE);
            DrawRectangleLines(304, ScrollHandlePosn+1, 12, 11, CLR_DARKBLUE);
            if((touchPosition.y >= (float)VERTICAL_SCROLL_MIN) && (touchPosition.y <= (float)VERTICAL_SCROLL_MAX)) {
                VerticalScrollPercent = (int)(((touchPosition.y - (float)VERTICAL_SCROLL_MIN) / (float)(VERTICAL_SCROLL_MAX - VERTICAL_SCROLL_MIN)) * 100);
            }
        }
    }
    if(ScreenOverlays.SingleStep == 1){
            char strTrack[5];
            char strStep[5];
            DrawTexture(TopBarTexture,0,0,WHITE);
            DrawTexture(Text2chTexture,75,2,WHITE); // Box for Track Number
            DrawTexture(Text2chTexture,180,2,WHITE); // Box for Step Number
            DrawText("Track:",5,5,20,DARKGRAY);
            DrawText("Step:",125,5,20,DARKGRAY);
            sprintf(strTrack,"%02d",edit_track+1);
            sprintf(strStep,"%02d",edit_step+1);
            DrawText(strTrack,80,5,20,DARKGRAY);
            DrawText(strStep,185,5,20,DARKGRAY);
            if (btnB_state == 1){
                // Check for Prev
                btnB_state = 0;
                if(edit_step <= 0){
                    edit_step=Europi.tracks[edit_track].last_step-1;
                }
                else {
                    edit_step--;
                }
            }
            if (btnC_state == 1){
                // Check for Next
                if(++edit_step >= Europi.tracks[edit_track].last_step){
                    edit_step=0;
                }
                btnC_state = 0;
            }

    }
    if(ScreenOverlays.SingleChannel == 1){
            char strTrack[5];
            DrawTexture(TopBarTexture,0,0,WHITE);
            DrawTexture(Text2chTexture,75,2,WHITE); // Box for Track Number
            DrawText("Track:",5,5,20,DARKGRAY);
            sprintf(strTrack,"%02d",edit_track+1);
            DrawText(strTrack,80,5,20,DARKGRAY);
            if (btnB_state == 1){
                // Check for Prev
                btnB_state = 0;
                if(edit_track <= 0){
                    edit_track = last_track-1;
                }
                else {
                    edit_track--;
                }
                SingleChannelOffset = 0;
                select_track(edit_track);
            }
            if (btnC_state == 1){
                // Check for Next
                if(++edit_track >= last_track){
                    edit_track=0;
                }
                btnC_state = 0;
                SingleChannelOffset = 0;
                select_track(edit_track);
            }

    }
    // The soft button function bar is always displayed 
    // at the bottom of the screen
    gui_ButtonBar();
    gui_debug();
}

/*
 * gui_ButtonBar
 * Draws the button bar at the bottom of the screen according 
 * to the current function of each of the 4 Soft Buttons. It
 * also scans for Touch Input for each button, and calls the
 * same function that the button would
 */
void gui_ButtonBar(void){
    Rectangle buttonRectangle = {0,0,0,0};
    //DrawTexture(ButtonBarTexture,0,213,WHITE);
    // Button A
    buttonRectangle.x = 0;
    buttonRectangle.y = 213;
    buttonRectangle.width = 80;
    buttonRectangle.height = 17;
    if (CheckCollisionPointRec(touchPosition, buttonRectangle) && (currentGesture1 != GESTURE_NONE)){
        btnA_state = 1;
    }
    switch(btnA_func){
        case btnA_quit:
            DrawText("Quit",17,217,20,DARKGRAY);
            if (btnA_state == 1){
                btnA_state = 0;
                prog_running = 0;
            }

        break;
        case btnA_select:
            DrawText("SEL",17,217,20,DARKGRAY);
        case btnA_none:
        default:
        break;
    }
    // Button B
    buttonRectangle.x = 80;
    buttonRectangle.y = 213;
    buttonRectangle.width = 80;
    buttonRectangle.height = 17;
    if (CheckCollisionPointRec(touchPosition, buttonRectangle) && (currentGesture1 != GESTURE_NONE)){
        btnB_state = 1;
    }
    switch(btnB_func){
        case btnB_menu:
            DrawText("Menu",95,217,20,DARKGRAY);
            if (btnB_state == 1){
                btnB_state = 0;
                ScreenOverlays.MainMenu ^= 1;
                if(ScreenOverlays.MainMenu == 1){
                    encoder_focus = menu_on;
                }
                else {
                    ClearMenus();
                    MenuSelectItem(0,0);    // Select just the first item of the first branch
                }
            }
        break;
        case btnB_open:
            DrawText("Open",95,217,20,DARKGRAY);
        break;
        case btnB_save:
            DrawText("Save",95,217,20,DARKGRAY);
            if (btnB_state == 1){
                btnB_state = 0;
                //Check what we're saving...
                if(ScreenOverlays.FileSaveAs == 1){
                    ClearScreenOverlays();
                    buttonsDefault();
                    ClearMenus();
                    MenuSelectItem(0,0);
                    sprintf(current_filename,"resources/sequences/%s",input_txt);
                    FILE * file = fopen(current_filename,"wb");
                    if (file != NULL) {
                        fwrite(&Europi,sizeof(struct europi),1,file);
                        fclose(file);
                        log_msg("Saved as %s",current_filename);
                    }
                    else{
                        log_msg("Error saving as: %s\n",current_filename);
                    }
                }
            }
        break;
        case btnB_val_down:
            DrawText("Val -",95,217,20,DARKGRAY);
        break;
        case btnB_prev:
            DrawText("Prev",95,217,20,DARKGRAY);
        break;
        case btnB_tr_minus:
            DrawText("Trk-",95,217,20,DARKGRAY);
        break;
        case btnB_none:
        default:
        break;
    }
    // Button C
    buttonRectangle.x = 160;
    buttonRectangle.y = 213;
    buttonRectangle.width = 80;
    buttonRectangle.height = 17;
    if (CheckCollisionPointRec(touchPosition, buttonRectangle) && (currentGesture1 != GESTURE_NONE)){
        btnC_state = 1;
    }
    switch(btnC_func){
        case btnC_bpm_dn:
            DrawText("BPM-",177,217,20,DARKGRAY);
            if (btnC_state == 1){
                btnC_state = 0;
                clock_freq -= 10;
                if (clock_freq < 1) clock_freq = 1;
                gpioHardwarePWM(MASTER_CLK,clock_freq,500000);
            }
        break;
        case btnC_cancel:
            DrawText("Cancel",167,217,20,DARKGRAY);
            if (btnC_state == 1){
                btnC_state = 0;
                // Don't know what we're cancelling, and don't care
                ClearScreenOverlays();
                buttonsDefault();
                ClearMenus();
                MenuSelectItem(0,0);
            }
        break;
        case btnC_val_up:
            DrawText("Val +",167,217,20,DARKGRAY);
        break;
        case btnC_next:
            DrawText("Next",178,217,20,DARKGRAY);
        break;
        case btnC_tr_plus:
            DrawText("Trk+",178,217,20,DARKGRAY);
        break;
        case btnC_none:
        default:
        break;
    }
    // Button D
    buttonRectangle.x = 240;
    buttonRectangle.y = 213;
    buttonRectangle.width = 80;
    buttonRectangle.height = 17;
    if (CheckCollisionPointRec(touchPosition, buttonRectangle) && (currentGesture1 != GESTURE_NONE)){
        btnD_state = 1;
    }
    switch(btnD_func){
        case btnD_bpm_up:
            DrawText("BPM+",257,217,20,DARKGRAY);
            if (btnD_state == 1){
                btnD_state = 0;
                clock_freq += 10;
                gpioHardwarePWM(MASTER_CLK,clock_freq,500000);
            }
        break;
        case btnD_done:
            DrawText("Done",257,217,20,DARKGRAY);
            if (btnD_state == 1){
                btnD_state = 0;
                // Don't know what we're done doing, but don't care
                if(DisplayPage == SingleStep) DisplayPage = GridView;
                if(DisplayPage == SingleChannel) DisplayPage = GridView;
                ClearScreenOverlays();
                buttonsDefault();
                ClearMenus();
                MenuSelectItem(0,0);
                run_stop = save_run_stop;   // Restores the previous run_stop state
            }

        case btnD_none:
        default:
        break;
    }
}

void gui_MainMenu(void){
    int x = 5;
    int txt_len;
    int PanelHeight = MENU_FONT_SIZE + MENU_TB_MARGIN;
    Color menu_colour;
    Rectangle menuRectangle = {0,0,0,0}; 
    //lastGesture = currentGesture;
    //currentGesture = GetGestureDetected();
    //touchPosition = GetTouchPosition(0);
    // Top Bar
    DrawTexture(TopBarTexture,0,0,WHITE);
    int i = 0;
    while(Menu[i].name != NULL){
        txt_len = MeasureText(Menu[i].name,MENU_FONT_SIZE);
        //Draw a box a bit bigger than this
        if (Menu[i].highlight == 1) menu_colour = CLR_LIGHTBLUE; else menu_colour = CLR_DARKBLUE;
        menuRectangle.x = x;
        menuRectangle.y = 2;
        menuRectangle.width = txt_len+(MENU_LR_MARGIN * 2);
        menuRectangle.height = PanelHeight;
        if (CheckCollisionPointRec(touchPosition, menuRectangle) && (currentGesture1 == GESTURE_HOLD)){
            // Toggle the expansion of this menu item
            if (Menu[i].highlight == 0) {
                // this Menu item not currently highlighted, so
                // de-hilight the existing one, and highlight this
                int m = 0;
                while(Menu[m].name != NULL){
                    Menu[m].highlight = 0;
                    Menu[m].expanded = 0;
                    m++;
                }
                Menu[i].highlight = 1;
                Menu[m].expanded = 1;
            }
            toggle_menu();
        }
        DrawRectangleRec(menuRectangle,menu_colour);
        DrawText(Menu[i].name,x+MENU_LR_MARGIN,MENU_TOP_MARGIN,MENU_FONT_SIZE,DARKGRAY);
        if(Menu[i].expanded == 1){
            // Draw sub-menus
            int j = 0;
            int y = PanelHeight;
            int sub_len = 0;
            int tmp_len;
            // Measure the length of the longest sub menu
            while(Menu[i].child[j]->name != NULL){
                tmp_len = MeasureText(Menu[i].child[j]->name,MENU_FONT_SIZE);
                if(tmp_len > sub_len) sub_len = tmp_len;
                j++;
            }
            j = 0;
            while(Menu[i].child[j]->name != NULL){
                if (Menu[i].child[j]->highlight == 1) menu_colour = CLR_LIGHTBLUE; else menu_colour = CLR_DARKBLUE;
                // Deal with the direction this leaf opens
                switch (Menu[i].child[j]->direction){
                    case dir_right:
                        menuRectangle.x = x;
                    break;
                    case dir_left:
                        menuRectangle.x = x + txt_len - sub_len;
                    break;
                    default:
                        menuRectangle.x = x;
                    break;
                }
                menuRectangle.y = y;
                menuRectangle.width = sub_len+(MENU_LR_MARGIN * 2);
                menuRectangle.height = PanelHeight;
                if (CheckCollisionPointRec(touchPosition, menuRectangle) && (currentGesture1 == GESTURE_HOLD)){
                    // Call function pointed to by this menu item
                    MenuSelectItem(i,j);
                    if (Menu[i].child[j]->funcPtr != NULL) Menu[i].child[j]->funcPtr();
                }
                DrawRectangleRec(menuRectangle,menu_colour);
                DrawText(Menu[i].child[j]->name,menuRectangle.x+MENU_LR_MARGIN,y+MENU_TB_MARGIN,MENU_FONT_SIZE,DARKGRAY);
                y+=PanelHeight;
                j++;
            }
        }
        x+=txt_len+(MENU_LR_MARGIN * 2)+MENU_HORIZ_SPACE;
        i++;
    }
}

/*
* gui_debug()
* if the global variable debug is set == TRUE
* then this will display debug messages on top of
* whatever else is going on on the screen. This
* starts at the oldest message (ie the one that
* will be overwritten next) that way the most recent
* message will always be at the bottom of the screen
*/
void gui_debug(void){
    if(debug == FALSE) return;
    int i;
    int current_row = next_debug_slot;
    DrawRectangle(5,100,310,112,WHITE);
    for(i = 0; i <10; i++){
        DrawText(debug_messages[current_row++],7,104+(i*11),10,BLACK);
        if(current_row >= 10) current_row = 0;
    }
}
