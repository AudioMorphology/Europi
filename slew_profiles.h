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

/*
* Slew Profiles
* These are a series of Integer arrays, which hold normalised values that 
* can be scaled to apply various Slew profiles to CV transitions
*/
static int slew_profiles[6][100]={
    // Linear Rising
    {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,
    20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,
    40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,
    60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,
    80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99},
    // Logarithmic Rising
    {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,
    20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,
    40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,
    60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,
    80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99},
    // Exponential Rising
    {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,
    20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,
    40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,
    60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,
    80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99},
    // Linear Falling
    {100,99,98,97,96,95,94,93,92,91,90,89,88,87,86,85,84,83,82,81,
    80,79,78,77,76,75,74,73,72,71,70,69,68,67,66,65,64,63,62,61,
    60,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,42,41,
    40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,
    20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0},
    // Logarithmic Falling
    {100,99,98,97,96,95,94,93,92,91,90,89,88,87,86,85,84,83,82,81,
    80,79,78,77,76,75,74,73,72,71,70,69,68,67,66,65,64,63,62,61,
    60,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,42,41,
    40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,
    20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0},
    // Exponential Falling
    {100,99,98,97,96,95,94,93,92,91,90,89,88,87,86,85,84,83,82,81,
    80,79,78,77,76,75,74,73,72,71,70,69,68,67,66,65,64,63,62,61,
    60,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,42,41,
    40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,
    20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0},
    
};
static const char * slew_names[] = {
    "Linear Rising",
    "Logarithmic Rising",
    "Exponential Rising",
    "Linear Falling",
    "Logarithmic Falling",
    "Exponential Falling"
};