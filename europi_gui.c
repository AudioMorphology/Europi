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
extern int prog_running;
extern Vector2 touchPosition;
extern int currentGesture1;
extern int lastGesture;
extern menu Menu[]; 
extern char input_txt[]; 
extern Texture2D KeyboardTexture;
extern Texture2D DialogTexture;
extern Texture2D TextInputTexture;
extern Texture2D Text2chTexture;
extern Texture2D Text5chTexture;
extern Texture2D MainScreenTexture;
extern Texture2D TopBarTexture;
extern Texture2D ButtonBarTexture; 
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
    int track, column;
    int step, offset, txt_len;
    char txt[20]; 

    BeginDrawing();
    DrawTexture(MainScreenTexture,0,0,WHITE);
    for(track = 0; track < 8; track++){
        // for each track, we need to know where the 
        // current step in the sequence is for that
        // particular track, and display the appropriate
        // 8 steps that contain the current step
        offset = Europi.tracks[track].current_step / 8;
        // Display the step offset at the start of each track
        sprintf(txt,"%d",offset);
        txt_len = MeasureText(txt,20);
        DrawText(txt,20-txt_len,10+(track * 25),20,DARKGRAY);
        // Check for doube-tap on the Track Number
        stepRectangle.x = 20-txt_len;
        stepRectangle.y = 10+(track * 25);
        stepRectangle.width = txt_len; 
        stepRectangle.height = 20;
        if (CheckCollisionPointRec(touchPosition, stepRectangle) && (currentGesture1 == GESTURE_TAP)){
            
            //if (currentGesture != lastGesture){
                // Open this Track in isolation
                //currentGesture = GESTURE_NONE;
                ClearScreenOverlays();
                DisplayPage = SingleChannel;
                select_track(track);
            //}
        }
        for(column=0;column<8;column++){
            stepRectangle.x = 30 + (column * 25);
            stepRectangle.y = 10 + (track * 25);
            stepRectangle.width = 22;
            stepRectangle.height = 22;
            // Check gesture collision
            if (CheckCollisionPointRec(touchPosition, stepRectangle) && (currentGesture1 != GESTURE_NONE)){
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
    int row;
    Rectangle stepRectangle = {0,0,0,0};
    BeginDrawing();
    DrawTexture(MainScreenTexture,0,0,WHITE);
    for (track = 0; track < MAX_TRACKS; track++){
        if (Europi.tracks[track].selected == TRUE){
            sprintf(track_no,"%d",track+1);
            txt_len = MeasureText(track_no,10);
            //DrawText(track_no,12-txt_len,220,10,DARKGRAY);
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
        DrawTexture(Text2chTexture,155,2,WHITE); // Box for Step Number
        DrawText("Track",5,5,20,DARKGRAY);
        DrawText("Loop",105,5,20,DARKGRAY);
        
        for(track = 0; track < MAX_TRACKS; track++) {
            if (Europi.tracks[track].selected == TRUE){
                sprintf(strTrack,"%02d",track+1);
                sprintf(strLoop,"%02d",Europi.tracks[track].last_step);
                DrawText(strTrack,75,5,20,DARKGRAY);
                DrawText(strLoop,160,5,20,DARKGRAY);
                if(encoder_focus == track_select){
                    DrawRectangleLines(71,3,30,22,RED);
                    DrawRectangleLines(72,4,28,20,RED);
                }
                else if (encoder_focus == set_loop) {
                    DrawRectangleLines(156,3,30,22,RED);
                    DrawRectangleLines(157,4,28,20,RED);
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
            char filename[100]; 
            snprintf(filename, sizeof(filename), "resources/sequences/%s", files[file_selected]);
            load_sequence(filename);
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
            if (btnB_state == 1){
                btnB_state = 0;

            }
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
                    char *filename[80];
                    sprintf(filename,"resources/sequences/%s",input_txt);
                    FILE * file = fopen(filename,"wb");
                    if (file != NULL) {
                        fwrite(&Europi,sizeof(struct europi),1,file);
                        fclose(file);
                    }
                }
            }
        break;
        case btnB_val_down:
            DrawText("Val -",95,217,20,DARKGRAY);
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
                ClearScreenOverlays();
                buttonsDefault();
                ClearMenus();
                MenuSelectItem(0,0);
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
* whatever else is going on on the screen
*/
void gui_debug(void){
    if(debug == FALSE) return;
    int i;
    DrawRectangle(5,100,310,112,WHITE);
    for(i = 0; i <10; i++){
        // leave the next slot blank so that
        // we can tell which is the latest message
        if(i != next_debug_slot) DrawText(debug_messages[i],7,104+(i*11),10,BLACK);
    }
}
