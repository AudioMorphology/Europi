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

#include <linux/fb.h>

#include "europi.h"
#include "font_8x8.c"
#include "font_8x16.c"

extern struct fb_fix_screeninfo finfo;

// 8-Bit Colour depth - Colour is just an
// index into the pallette
void put_pixel(char *fbp, int x, int y, int c)
{
    // calculate the pixel's byte offset inside the buffer
    unsigned int pix_offset = x + y * finfo.line_length;

    // now this is about the same as 'fbp[pix_offset] = value'
    *((char*)(fbp + pix_offset)) = c;

}


// 24-Bit colour depth - R,G & B each have their own value
void put_pixel_RGB24(char *fbp, int x, int y, int r, int g, int b)
{

    // calculate the pixel's byte offset inside the buffer
    // note: x * 3 as every pixel is 3 consecutive bytes
    unsigned int pix_offset = x * 3 + y * finfo.line_length;

    // now this is about the same as 'fbp[pix_offset] = value'
    *((char*)(fbp + pix_offset)) = r;
    *((char*)(fbp + pix_offset + 1)) = g;
    *((char*)(fbp + pix_offset + 2)) = b;

}

// This is the default colour mode on the RPi - RGB is 
// encoded into a 2-Byte value, with the individual colours
// occupying 5, 6 & 5 bits respectively (hence the 565!)
// This function is passed the colour already converted to 565
// format as an Unsigned Short
void put_pixel_RGB565(char *fbp, int x, int y, unsigned short c)
{
    // calculate the pixel's byte offset inside the buffer
    // note: x * 2 as every pixel is 2 consecutive bytes
    unsigned int pix_offset = x * 2 + y * finfo.line_length;
    // write 'two bytes at once'
    *((unsigned short*)(fbp + pix_offset)) = c;
}
/* Takes separate Red, Green & Blue integers and returns them packed in 565 format*/
unsigned short RGB2565(int r, int g, int b)
{
 return (unsigned short)((r / 8) << 11) + ((g / 4) << 5) + (b / 8);
}


/*Takes a HEX Colour RGB value and returns it packed as a 565 short*/
unsigned  short HEX2565(long c)
{
return (unsigned short)(((c & 0xF80000) >> 19)+((c & 0x00FC00) >> 5) + ((c & 0x0000F8) << 8));
}
// Bresenham's line algorithm
void draw_line(char *fbp, int x0, int y0, int x1, int y1, int c) {
  int dx = x1 - x0;
  dx = (dx >= 0) ? dx : -dx; // abs()
  int dy = y1 - y0;
  dy = (dy >= 0) ? dy : -dy; // abs()
  int sx;
  int sy;
  if (x0 < x1)
    sx = 1;
  else
    sx = -1;
  if (y0 < y1)
    sy = 1;
  else
    sy = -1;
  int err = dx - dy;
  int e2;
  int done = 0;
  while (!done) {
    put_pixel(fbp, x0, y0, c);
    if ((x0 == x1) && (y0 == y1))
      done = 1;
    else {
      e2 = 2 * err;
      if (e2 > -dy) {
        err = err - dy;
        x0 = x0 + sx;
      }
      if (e2 < dx) {
        err = err + dx;
        y0 = y0 + sy;
      }
    }
  }
}

// Draw Rectangle
// (x0, y0) = left top corner coordinates
// w = width and h = height
void draw_rect(char *fbp, int x0, int y0, int w, int h, int c) {
  draw_line(fbp, x0, y0, x0 + w, y0, c); // top
  draw_line(fbp, x0, y0, x0, y0 + h, c); // left
  draw_line(fbp, x0, y0 + h, x0 + w, y0 + h, c); // bottom
  draw_line(fbp, x0 + w, y0, x0 + w, y0 + h, c); // right
}

// Fill Rectangle
void fill_rect(char *fbp, int x0, int y0, int w, int h, int c) {
  int y;
  for (y = 0; y <= h; y++) {
    draw_line(fbp, x0, y0 + y, x0 + w, y0 + y, c);
  }
}

// Draw Circle
void draw_circle(char *fbp, int x0, int y0, int r, int c)
{
  int x = r;
  int y = 0;
  int radiusError = 1 - x;

  while(x >= y)
  {
    // top left
    put_pixel(fbp, -y + x0, -x + y0, c);
    // top right
    put_pixel(fbp, y + x0, -x + y0, c);
    // upper middle left
    put_pixel(fbp, -x + x0, -y + y0, c);
    // upper middle right
    put_pixel(fbp, x + x0, -y + y0, c);
    // lower middle left
    put_pixel(fbp, -x + x0, y + y0, c);
    // lower middle right
    put_pixel(fbp, x + x0, y + y0, c);
    // bottom left
    put_pixel(fbp, -y + x0, x + y0, c);
    // bottom right
    put_pixel(fbp, y + x0, x + y0, c);

    y++;
    if (radiusError < 0)
 {
   radiusError += 2 * y + 1;
 } else {
      x--;
   radiusError+= 2 * (y - x + 1);
    }
  }
}

// Fill circle
void fill_circle(char *fbp, int x0, int y0, int r, int c) {
  int x = r;
  int y = 0;
  int radiusError = 1 - x;

  while(x >= y)
  {
    // top
    draw_line(fbp, -y + x0, -x + y0, y + x0, -x + y0, c);
    // upper middle
    draw_line(fbp, -x + x0, -y + y0, x + x0, -y + y0, c);
    // lower middle
    draw_line(fbp, -x + x0, y + y0, x + x0, y + y0, c);
    // bottom 
    draw_line(fbp, -y + x0, x + y0, y + x0, x + y0, c);

    y++;
    if (radiusError < 0)
    {
      radiusError += 2 * y + 1;
    } else {
      x--;
      radiusError+= 2 * (y - x + 1);
    }
  }
}

/* the 565 Graphics mode equivalent for all those 8-Bit drawing functions */

// Bresenham's line algorithm
void draw_line_RGB565(char *fbp, int x0, int y0, int x1, int y1, unsigned short c) {
  int dx = x1 - x0;
  dx = (dx >= 0) ? dx : -dx; // abs()
  int dy = y1 - y0;
  dy = (dy >= 0) ? dy : -dy; // abs()
  int sx;
  int sy;
  if (x0 < x1)
    sx = 1;
  else
    sx = -1;
  if (y0 < y1)
    sy = 1;
  else
    sy = -1;
  int err = dx - dy;
  int e2;
  int done = 0;
  while (!done) {
    put_pixel_RGB565(fbp, x0, y0, c);
    if ((x0 == x1) && (y0 == y1))
      done = 1;
    else {
      e2 = 2 * err;
      if (e2 > -dy) {
        err = err - dy;
        x0 = x0 + sx;
      }
      if (e2 < dx) {
        err = err + dx;
        y0 = y0 + sy;
      }
    }
  }
}

// Draw Rectangle
// (x0, y0) = left top corner coordinates
// w = width and h = height
void draw_rect_RGB565(char *fbp, int x0, int y0, int w, int h, unsigned short c) {
  draw_line_RGB565(fbp, x0, y0, x0 + w, y0, c); // top
  draw_line_RGB565(fbp, x0, y0, x0, y0 + h, c); // left
  draw_line_RGB565(fbp, x0, y0 + h, x0 + w, y0 + h, c); // bottom
  draw_line_RGB565(fbp, x0 + w, y0, x0 + w, y0 + h, c); // right
}

// Fill Rectangle
void fill_rect_RGB565(char *fbp, int x0, int y0, int w, int h, unsigned short c) {
  int y;
  for (y = 0; y < h; y++) {
    draw_line_RGB565(fbp, x0, y0 + y, x0 + w, y0 + y, c);
  }
}

// Draw Circle
void draw_circle_RGB565(char *fbp, int x0, int y0, int rad, unsigned short c)
{
  int x = rad;
  int y = 0;
  int radiusError = 1 - x;

  while(x >= y)
  {
    // top left
    put_pixel_RGB565(fbp, -y + x0, -x + y0, c);
    // top right
    put_pixel_RGB565(fbp, y + x0, -x + y0, c);
    // upper middle left
    put_pixel_RGB565(fbp, -x + x0, -y + y0, c);
    // upper middle right
    put_pixel_RGB565(fbp, x + x0, -y + y0, c);
    // lower middle left
    put_pixel_RGB565(fbp, -x + x0, y + y0, c);
    // lower middle right
    put_pixel_RGB565(fbp, x + x0, y + y0, c);
    // bottom left
    put_pixel_RGB565(fbp, -y + x0, x + y0, c);
    // bottom right
    put_pixel_RGB565(fbp, y + x0, x + y0, c);

    y++;
    if (radiusError < 0)
 {
   radiusError += 2 * y + 1;
 } else {
      x--;
   radiusError+= 2 * (y - x + 1);
    }
  }
}

// Fill circle
void fill_circle_RGB565(char *fbp, int x0, int y0, int rad, unsigned short c) {
  int x = rad;
  int y = 0;
  int radiusError = 1 - x;

  while(x >= y)
  {
    // top
    draw_line_RGB565(fbp, -y + x0, -x + y0, y + x0, -x + y0, c);
    // upper middle
    draw_line_RGB565(fbp, -x + x0, -y + y0, x + x0, -y + y0, c);
    // lower middle
    draw_line_RGB565(fbp, -x + x0, y + y0, x + x0, y + y0, c);
    // bottom 
    draw_line_RGB565(fbp, -y + x0, x + y0, y + x0, x + y0, c);

    y++;
    if (radiusError < 0)
    {
      radiusError += 2 * y + 1;
    } else {
      x--;
      radiusError+= 2 * (y - x + 1);
    }
  }
}

/* put_char
 * 
 * writes a character out to the screen
 * from the included font file font_8x16.c
 * 
 * The 8x16 font is fixed-width, 8 pixels
 * wide, but at 16 pixels high, you get
 * nicer-looking tails on g, j etc.
 * 
 * Function is passed a foreground and 
 * a background colour in 565 format, If the Foreground colour
 * is the same as the background colour, then the
 * background is assumed to be transparent, so isn't
 * written
 */
void put_char(char *fbp, int x, int y, int c, unsigned short fgc, unsigned short bgc)
{
	int i,j,bits;
	for (i = 0; i < font_vga_8x16.height; i++) {
	bits = font_vga_8x16.data [font_vga_8x16.height * c + i];
		for (j = 0; j < font_vga_8x16.width; j++, bits <<= 1)
			if (bits & 0x80){
				put_pixel_RGB565(fbp, x+j, y+i, fgc);
			}
			else if (fgc != bgc) {
				put_pixel_RGB565(fbp, x+j, y+i, bgc);
			}
		}
}
/* put_string
 * 
 * Calls put_char multiple times to write a string
 * to the screen using an included 8x16 font
 * 
 * This takes a foreground colour and a background
 * colour in 565 format.  
 */
void put_string(char *fbp, int x, int y, char *s, unsigned short fgc, unsigned short bgc)
{
	int i;
	for (i = 0; *s; i++, x += font_vga_8x16.width, s++)
	put_char (fbp, x, y, *s, fgc, bgc);
}
