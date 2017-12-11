// Copyright 2016 Richard R. Goodwin / Audio Morphology / Olivier Gillet
//
// Authors: Richard R. Goodwin (richard.goodwin@morphology.co.uk)
//          Olivier Gillet (ol.gillet@gmail.com)
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
//-------------------------------------------------------------------------------

// The values in the scale_values array and, indeed, the names of the various scales
// came from the Mutable Instruments Braids software, released by Olivier Gillet 
// under a CC License. So, I have incorporated the same license, and have included 
// Olivier's License declaration, below, to make it clear that I am not claiming 
// ownership or proprietary rights over any aspects of these quantisation scales

//-------------------------------------------------------------------------------
// Copyright 2015 Olivier Gillet.
//
// Author: Olivier Gillet (ol.gillet@gmail.com)
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
//
// -----------------------------------------------------------------------------

/*
 * QUANTIZATION SCALES
 * 
 * These arrays set the Lower and Upper boundaries for
 * a given note in a given scale, and identify the absolute
 * value that should be returned.
 * 
 * Values are based on 6000 discrete values per Octave
 */

static int lower_boundary[49][12] = {
{0},
{0,250,750,1250,1750,2250,2750,3250,3750,4250,4750,5250},
{0,500,1500,2250,3000,4000,5000},
{0,500,1250,2000,3000,4000,4750},
{0,250,1000,2000,3000,3750,4500},
{0,500,1500,2500,3250,4000,5000},
{0,500,1500,2250,3000,4000,4750},
{0,500,1250,2000,3000,3750,4500},
{0,250,1000,2000,2750,3500,4500},
{0,750,1750,2750,4000,4750,},
{0,750,2000,2750,3250,4250,},
{0,500,1500,2750,4000,},
{0,750,2000,3000,4250,},
{0,250,1000,1750,2250,3000,3750,4500},
{0,250,1500,3000,3750},
{0,250,1000,2500,3750},
{0,500,1250,2250,3250,3750,4750},
{0,250,1250,2250,3000,3750,4750},
{0,250,1250,2250,3000,3750,4500},
{0,500,1500,2500,3500,4500,2500},
{0,225,735,1245,1755,2264,2773,3285,3737,4246,4755,5265},
{0,250,750,1250,1625,2125,2750,3250,3750,4250,4750,5125},
{0,250,750,1250,1625,2125,2750,3250,3750,4250,4750,5250},
{0,250,750,1250,1625,2125,2750,3250,3750,4125,4625,5250},
{0,225,1190,2209,3000,3737,4701},
{0,280,1524,3000,3791},
{0,280,1245,2440,3686,4932},
{0,225,1190,2440,3231,3737,4701},
{0,280,1245,2440,3231,3791,4756},
{0,510,1475,2209,3000,4022,4986},
{0,510,1530,2549,3286,4022,5041},
{0,455,1190,1979,3000,3967,4701},
{0,510,1299,2033,3000,4022,4811},
{0,510,1245,1979,3000,3737,4471},
{0,510,1475,2209,3000,3967,4701},
{0,510,1475,2209,3000,4022,4756,5266},
{0,510,1245,1979,3000,3967,4701,5211},
{0,225,764,1783,3455,4701},
{0,510,1245,1979,3000,4477},
{0,965,2209,3000,3737,4471},
{0,510,1985,3231,3967,4701},
{0,510,1245,1979,3000,4022,4756},
{0,510,1475,2209,3000,3737,4701},
{0,510,1530,2264,3225,4471},
{0,225,1469,3000,4246},
{0,225,959,2491,4246},
{0,735,1979,3000,4246},
{0,735,1979,2719,3455},
{0,735,1700,2209,3455,4701},
};

static int upper_boundary[49][12] = {
{6000},
{250,750,1250,1750,2250,2750,3250,3750,4250,4750,5250,6000},
{500,1500,2250,3000,4000,5000,6000},
{500,1250,2000,3000,4000,4750,6000},
{250,1000,2000,3000,3750,4500,6000},
{500,1500,2500,3250,4000,5000,6000},
{500,1500,2250,3000,4000,4750,6000},
{500,1250,2000,3000,3750,4500,6000},
{250,1000,2000,2750,3500,4500,6000},
{750,1750,2750,4000,4750,6000},
{750,2000,2750,3250,4250,6000},
{500,1500,2750,4000,6000},
{750,2000,3000,4250,6000},
{250,1000,1750,2250,3000,3750,4500,6000},
{250,1500,3000,3750,6000},
{250,1000,2500,3750,6000},
{500,1250,2250,3250,3750,4750,6000},
{250,1250,2250,3000,3750,4750,6000},
{250,1250,2250,3000,3750,4500,6000},
{500,1500,2500,3500,4500,6000},
{225,735,1245,1755,2264,2773,3285,3737,4246,4756,5266,6000},
{250,750,1250,1625,2125,2750,3250,3750,4250,4750,5125,6000},
{250,750,1250,1625,2125,2750,3250,3750,4250,4750,5250,6000},
{250,750,1250,1625,2125,2750,3250,3750,4125,4625,5250,6000},
{225,1190,2209,3000,3737,4701,6000},
{280,1524,3000,3791,6000},
{280,1245,2440,3686,4932,6000},
{225,1190,2440,3231,3737,4701,6000},
{280,1245,2440,3231,3791,4756,6000},
{510,1475,2209,3000,4022,4986,6000},
{510,1530,2549,3286,4022,5041,6000},
{455,1190,1979,3000,3967,4701,6000},
{510,1299,2033,3000,4022,4811,6000},
{510,1245,1979,3000,3737,4471,6000},
{510,1475,2209,3000,3967,4701,6000},
{510,1475,2209,3000,4022,4756,5266,6000},
{510,1245,1979,3000,3967,4701,5211,6000},
{225,764,1783,3455,4701,6000},
{510,1245,1979,3000,4477,6000},
{965,2209,3000,3737,4471,6000},
{510,1985,3231,3967,4701,6000},
{510,1245,1979,3000,4022,4756,6000},
{510,1475,2209,3000,3737,4701,6000},
{510,1530,2264,3225,4471,6000},
{225,1469,3000,4246,6000},
{225,959,2491,4246,6000},
{735,1979,3000,4246,6000},
{735,1979,2719,3455,6000},
{735,1700,2209,3455,4701,6000},
};

static int scale_values[49][12] = {
//0: Off,
{ },
//1: Semitones
{0,500,1000,1500,2000,2500,3000,3500,4000,4500,5000,5500},
//2: Ionian
{0,1000,2000,2500,3500,4500,5500},
//3: Dorian
{0,1000,1500,2500,3500,4500,5000},
//4: Phrygian
{0,500,1500,2500,3500,4000,5000},
//5: Lydian
{0,1000,2000,3000,3500,4500,5500},
//6: Mixolydian
{0,1000,2000,2500,3500,4500,5000},
//7: Aeolian
{0,1000,1500,2500,3500,4000,5000},
//8: Locrian
{0,500,1500,2500,3000,4000,5000},
//9: Blues Major
{0,1500,2000,3500,4500,5000},
//10: Blues Minor
{0,1500,2500,3000,3500,5000},
//11: Pentatonic Major
{0,1000,2000,3500,4500},
//12: Pentatonic Minor
{0,1500,2500,3500,5000},
//13: Folk
{0,500,1500,2000,2500,3500,4000,5000},
//14: Japanese
{0,500,2500,3500,4000},
//15: Gamelan
{0,500,1500,3500,4000},
//16: Gypsy
{0,1000,1500,3000,3500,4000,5500},
//17: Arabian
{0,500,2000,2500,3500,4000,5500},
//18: Flamenco
{0,500,2000,2500,3500,4000,5000},
//19: Whole Tone
{0,1000,2000,3000,4000,5000},
//20: Pythagorean
{0,450,1020,1469,2040,2488,3058,3512,3961,4531,4980,5551},
//21: 1_4_eb
{0,500,1000,1500,1750,2500,3000,3500,4000,4500,5000,5250},
//22: 1_4_eb
{0,500,1000,1500,1750,2500,3000,3500,4000,4500,5000,5500},
//23: 1_4_ea
{0,500,1000,1500,1750,2500,3000,3500,4000,4250,5000,5500},
//24: Bhairay
{0,449,1930,2488,3512,3961,5441},
//25: Gunakri
{0,559,2488,3512,4070},
//26: Marwa
{0,559,1930,2949,4422,5441},
//27: Shree
{0,449,1930,2949,3512,3961,5441},
//28: Purvi
{0,559,1930,2949,3512,4070,5441},
//29: Bilawal
{0,1020,1930,2488,3512,4531,5441},
//30: Yaman
{0,1020,2039,3059,3512,4531,5551},
//31: Kafi
{0,910,1469,2488,3512,4422,4980},
//32: Bhimpalasree
{0,1020,1578,2488,3512,4531,5090},
//33: Darbari
{0,1020,1469,2488,3512,3961,4980},
//34: Rageshree
{0,1020,1930,2488,3512,4422,4980},
//35: Khamaj
{0,1020,1930,2488,3512,4531,4980,5551},
//36: Mimal
{0,1020,1469,2488,3512,4422,4980,5441},
//37: Parmeshwari
{0,449,1078,2488,4422,4980},
//38: Rangeshwari
{0,1020,1469,2488,3512,5441},
//39: Gangeshwari
{0,1930,2488,3512,3961,4980},
//40: Kameshwari
{0,1020,2949,3512,4422,4980},
//41: Pa__kafi
{0,1020,1469,2488,3512,4531,4980},
//42: Natbhairay
{0,1020,1930,2488,3512,3961,5441},
//43: m_kauns
{0,1020,2039,2488,3961,4980},
//44: Bairagi
{0,449,2488,3512,4980},
//45: b_todi
{0,449,1469,3512,4980},
//46: Chandradeep
{0,1469,2488,3512,4980},
//47: Kaushik_todi
{0,1469,2488,2949,3961},
//48: Jogeshwari
{0,1469,1930,2488,4422,4980}
};


// Number of Notes in each particular scale
static int scale_notes[49] = {
  // Off
  0,
  // Semitones
  12,
  // Ionian (From midipal/BitT source code)
  7,
  // Dorian (From midipal/BitT source code)
  7,
  // Phrygian (From midipal/BitT source code)
  7,
  // Lydian (From midipal/BitT source code)
  7,
  // Mixolydian (From midipal/BitT source code)
  7,
  // Aeolian (From midipal/BitT source code)
  7,
  // Locrian (From midipal/BitT source code)
  7,
  // Blues major (From midipal/BitT source code)
  6,
  // Blues minor (From midipal/BitT source code)
  6,
  // Pentatonic major (From midipal/BitT source code)
  5,
  // Pentatonic minor (From midipal/BitT source code)
  5,
  // Folk (From midipal/BitT source code)
  8,
  // Japanese (From midipal/BitT source code)
  5,
  // Gamelan (From midipal/BitT source code)
  5,
  // Gypsy
  7,
  // Arabian
  7,
  // Flamenco
  7,
  // Whole tone (From midipal/BitT source code)
  6,
  // pythagorean (From yarns source code)
  12,
  // 1_4_eb (From yarns source code)
  12,
  // 1_4_e (From yarns source code)
  12,
  // 1_4_ea (From yarns source code)
  12,
  // bhairav (From yarns source code)
  7,
  // gunakri (From yarns source code)
  5,
  // marwa (From yarns source code)
  6,
  // shree (From yarns source code)
  7,
  // purvi (From yarns source code)
  7,
  // bilawal (From yarns source code)
  7,
  // yaman (From yarns source code)
  7,
  // kafi (From yarns source code)
  7,
  // bhimpalasree (From yarns source code)
  7,
  // darbari (From yarns source code)
  7,
  // rageshree (From yarns source code)
  7,
  // khamaj (From yarns source code)
  8,
  // mimal (From yarns source code)
  8,
  // parameshwari (From yarns source code)
  6,
  // rangeshwari (From yarns source code)
  6,
  // gangeshwari (From yarns source code)
  6,
  // kameshwari (From yarns source code)
  6,
  // pa__kafi (From yarns source code)
  7,
  // natbhairav (From yarns source code)
  7,
  // m_kauns (From yarns source code)
  6,
  // bairagi (From yarns source code)
  5,
  // b_todi (From yarns source code)
  5,
  // chandradeep (From yarns source code)
  5,
  // kaushik_todi (From yarns source code)
  5,
  // jogeshwari (From yarns source code)
  6
};

// Array of strings representing the name of each scale
static const char * scale_names[] = {
"Off",
"Semitones",
"Ionian",
"Dorian",
"Phrygian", 
"Lydian",
"Mixolydian",
"Aeolian", 
"Locrian", 
"Blues Major", 
"Blues Minor", 
"Pentatonic Major", 
"Pentatonic Minor", 
"Folk", 
"Japanese", 
"Gamelan", 
"Gypsy", 
"Arabian", 
"Flamenco", 
"Whole Tone", 
"Pythagorean", 
"1_4_eb", 
"1_4_e", 
"1_4_ea", 
"bhairav", 
"Gunakri", 
"Marwa", 
"Shree", 
"Purvi", 
"Bilawal", 
"Kafi", 
"Bhimpalascree", 
"Darbari", 
"Rageshree", 
"Khamaj", 
"Mimal", 
"Parameshwari", 
"Rangeshwari", 
"Gangeshwari", 
"Kameshwari", 
"pa__kafi", 
"Natbhairav", 
"m_kauns", 
"Bairagi", 
"b_todi", 
"Chandradeep", 
"Kaushik_todi", 
"Jogeshwari"
};

