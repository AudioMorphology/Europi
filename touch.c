/*
        A simple program that demonstrates how to program for a
        touch screen. Specifically on the Raspberry Pi.
        This prgram can be used for most Linux based systems.
        For more details: ozzmaker.com

    Copyright (C) 2013  Mark Williams

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
    MA 02111-1307, USA
*/
#define KWHT  "\x1B[37m"
#define KYEL  "\x1B[33m"

int fd;


int openTouchScreen()
{
    /*    if ((fd = open(TOUCH_EVENT_HANDLER, O_RDONLY)) < 0) {
                return 1;
        }
		 */
int i;
char name[256];
/* Attempt to open each input device
 * in turn, to find which one is the 
 * touchscreen
 */
for (i = 0; 4; i++)
	{
	sprintf(name, "/dev/input/event%i", i);
	fd = open(name, O_RDONLY);
	if (fd < 0) break; /* no more devices */
	ioctl(fd, EVIOCGNAME(sizeof(name)), name);
	if (strcmp(name, "ADS7846 Touchscreen") == 0){
		log_msg("Found touchscreen: Event%d fd:%d\n",i,fd);
		break; /* found the Touchscreen! */
		}
	else {
		log_msg("Found this: %s\n",name);
		close(fd); /* we don't need this device */
		}
	}
if (fd < 0) return 1;
}


void getTouchScreenDetails(int *screenXmin,int *screenXmax,int *screenYmin,int *screenYmax)
{
	unsigned short id[4];
        unsigned long bit[EV_MAX][NBITS(KEY_MAX)];
        char name[256] = "Unknown";
        int abs[6] = {0};

        ioctl(fd, EVIOCGNAME(sizeof(name)), name);
        log_msg("Input device name: \"%s\"\n", name);

        memset(bit, 0, sizeof(bit));
        ioctl(fd, EVIOCGBIT(0, EV_MAX), bit[0]);
        log_msg("Supported events:\n");

        int i,j,k;
        for (i = 0; i < EV_MAX; i++)
                if (test_bit(i, bit[0])) {
                        log_msg("  Event type %d (%s)\n", i, events[i] ? events[i] : "?");
                        if (!i) continue;
                        ioctl(fd, EVIOCGBIT(i, KEY_MAX), bit[i]);
                        for (j = 0; j < KEY_MAX; j++){
                                if (test_bit(j, bit[i])) {
                                        log_msg("    Event code %d (%s)\n", j, names[i] ? (names[i][j] ? names[i][j] : "?") : "?");
                                        if (i == EV_ABS) {
                                                ioctl(fd, EVIOCGABS(j), abs);
                                                for (k = 0; k < 5; k++)
                                                        if ((k < 3) || abs[k]){
                                                                log_msg("     %s %6d\n", absval[k], abs[k]);
                                                                if (j == 0){
                                                                        if (absval[k] == "Min  ") *screenXmin =  abs[k];
                                                                        if (absval[k] == "Max  ") *screenXmax =  abs[k];
                                                                }
                                                                if (j == 1){
                                                                        if (absval[k] == "Min  ") *screenYmin =  abs[k];
                                                                        if (absval[k] == "Max  ") *screenYmax =  abs[k];
                                                                }
                                                        }
                                                }

                                        }
                                }
                        }
}


void getTouchSample(int *rawX, int *rawY, int *rawPressure)
{
	int i;
        /* how many bytes were read */
        size_t rb;
        /* the events (up to 64 at once) */
		/* I dropped this to just a single event. Otherwise
		 * it seemed to stop at this point waiting to 
		 * read input events??
		 * */
        struct input_event ev[1];
		rb=read(fd,ev,sizeof(struct input_event)*1);
        /*for (i = 0;  i <  (rb / sizeof(struct input_event)); i++){
              if (ev[i].type ==  EV_SYN)
                        printf("Event type is %s%s%s = Start of New Event\n",KYEL,events[ev[i].type],KWHT);

                else if (ev[i].type == EV_KEY && ev[i].code == 330 && ev[i].value == 1)
                        printf("Event type is %s%s%s & Event code is %sTOUCH(330)%s & Event value is %s1%s = Touch Starting\n", KYEL,events[ev[i].type],KWHT,KYEL,KWHT,KYEL,KWHT);

                else if (ev[i].type == EV_KEY && ev[i].code == 330 && ev[i].value == 0)
			printf("Event type is %s%s%s & Event code is %sTOUCH(330)%s & Event value is %s0%s = Touch Finished\n", KYEL,events[ev[i].type],KWHT,KYEL,KWHT,KYEL,KWHT);

                else if (ev[i].type == EV_ABS && ev[i].code == 0 && ev[i].value > 0){
                        printf("Event type is %s%s%s & Event code is %sX(0)%s & Event value is %s%d%s\n", KYEL,events[ev[i].type],KWHT,KYEL,KWHT,KYEL,ev[i].value,KWHT);
			*rawX = ev[i].value;
		}
                else if (ev[i].type == EV_ABS  && ev[i].code == 1 && ev[i].value > 0){
                        printf("Event type is %s%s%s & Event code is %sY(1)%s & Event value is %s%d%s\n", KYEL,events[ev[i].type],KWHT,KYEL,KWHT,KYEL,ev[i].value,KWHT);
			*rawY = ev[i].value;
		}
                else if (ev[i].type == EV_ABS  && ev[i].code == 24 && ev[i].value > 0){
                        printf("Event type is %s%s%s & Event code is %sPressure(24)%s & Event value is %s%d%s\n", KYEL,events[ev[i].type],KWHT,KYEL,KWHT,KYEL,ev[i].value,KWHT);
			*rawPressure = ev[i].value;
		}

	} */
	for (i = 0;  i <  (rb / sizeof(struct input_event)); i++){
		if (ev[i].type == EV_ABS && ev[i].code == 0 && ev[i].value > 0){
			*rawX = ev[i].value;
		}
        else if (ev[i].type == EV_ABS  && ev[i].code == 1 && ev[i].value > 0){
			*rawY = ev[i].value;
		}
        else if (ev[i].type == EV_ABS  && ev[i].code == 24 && ev[i].value > 0){
			*rawPressure = ev[i].value;
		}

	}
}