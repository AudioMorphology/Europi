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

extern Vector2 touchPosition;
extern int currentGesture;
extern int lastGesture;
extern menu Menu[];
extern struct screen_overlays ScreenOverlays;
extern enum display_page_t DisplayPage;
extern struct europi Europi;

/*
 * GUI_8x8 Attempts to display more detail in a subset of tracks and steps
 * 8 Tracks x 8 Steps
 */
void gui_8x8(void){
    lastGesture = currentGesture;
    touchPosition = GetTouchPosition(0);
    Rectangle stepRectangle = {0,0,0,0};
    BeginDrawing();
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
        // Check for doube-tap on the Track Number
        stepRectangle.x = 20-txt_len;
        stepRectangle.y = 20+(track * 25);
        stepRectangle.width = txt_len;
        stepRectangle.height = 20;
        currentGesture = GetGestureDetected();
        if (CheckCollisionPointRec(touchPosition, stepRectangle) && (currentGesture == GESTURE_TAP)){
            
            //if (currentGesture != lastGesture){
                // Open this Track in isolation
                currentGesture = GESTURE_NONE;
                ClearScreenOverlays();
                DisplayPage = SingleChannel;
                select_track(track);
            //}
        }
        for(column=0;column<8;column++){
            stepRectangle.x = 30 + (column * 25);
            stepRectangle.y = 20 + (track * 25);
            stepRectangle.width = 22;
            stepRectangle.height = 22;
            // Check gesture collision
            if (CheckCollisionPointRec(touchPosition, stepRectangle) && (currentGesture != GESTURE_NONE)){
                // Paint this step Blue
                DrawRectangleRec(stepRectangle, BLUE); 
            }
            else {
                if((offset*8)+column == Europi.tracks[track].last_step){
                    // Paint last step
                    DrawRectangleRec(stepRectangle, BLACK); 
                }
                else if((offset*8)+column == Europi.tracks[track].current_step){
                    // Paint current step
                    DrawRectangleRec(stepRectangle, LIME);   
                }
                else {
                    // paint blank step
                    DrawRectangleRec(stepRectangle, MAROON); 
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
/*
 * gui_SingleChannel displays just the currently
 * selected channel on a page of its own
 */
void gui_SingleChannel(void){
    int track;
    int step;
    int val;
    char track_no[20];
    int txt_len;
    lastGesture = currentGesture;
    currentGesture = GetGestureDetected();
    touchPosition = GetTouchPosition(0);
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
        gui_MainMenu();
    }
 
}

void gui_MainMenu(void){
    int x = 5;
    int txt_len;
    Color menu_colour;
    Rectangle menuRectangle = {0,0,0,0}; 
    lastGesture = currentGesture;
    currentGesture = GetGestureDetected();
    touchPosition = GetTouchPosition(0);
    int i = 0;
    while(Menu[i].name != NULL){
        txt_len = MeasureText(Menu[i].name,10);
        //Draw a box a bit bigger than this
        if (Menu[i].highlight == 1) menu_colour = BLUE; else menu_colour = BLACK;
        menuRectangle.x = x;
        menuRectangle.y = 0;
        menuRectangle.width = txt_len+6;
        menuRectangle.height = 12;
        if (CheckCollisionPointRec(touchPosition, menuRectangle) && (currentGesture == GESTURE_HOLD)){
            // Toggle the expansion of this menu item
            currentGesture = GESTURE_NONE;
            if (Menu[i].highlight == 0) {
                //this Menu item not currently highlighted, so
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
        DrawText(Menu[i].name,x+3,1,10,WHITE);
        if(Menu[i].expanded == 1){
            // Draw sub-menus
            int j = 0;
            int y = 12;
            int sub_len = 0;
            int tmp_len;
            // Measure the length of the longest sub menu
            while(Menu[i].child[j]->name != NULL){
                tmp_len = MeasureText(Menu[i].child[j]->name,10);
                if(tmp_len > sub_len) sub_len = tmp_len;
                j++;
            }
            j = 0;
            while(Menu[i].child[j]->name != NULL){
                if (Menu[i].child[j]->highlight == 1) menu_colour = BLUE; else menu_colour = BLACK;
                menuRectangle.x = x;
                menuRectangle.y = y;
                menuRectangle.width = sub_len+6;
                menuRectangle.height = 12;
                if (CheckCollisionPointRec(touchPosition, menuRectangle) && (currentGesture == GESTURE_HOLD)){
                    // Call this function
                    currentGesture = GESTURE_NONE;
                    MenuSelectItem(i,j);
                    if (Menu[i].child[j]->funcPtr != NULL) Menu[i].child[j]->funcPtr();
                }
                DrawRectangleRec(menuRectangle,menu_colour);
                DrawText(Menu[i].child[j]->name,x+3,y+1,10,WHITE);
                y+=12;
                j++;
            }
        }
        x+=txt_len+6+2;
        i++;
    }
}