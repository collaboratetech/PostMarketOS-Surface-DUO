/* duo-swipe : simule un glissement à N doigts (protocole MT B) en injectant dans un écran tactile existant.
 * Usage : duo-swipe /dev/input/eventN doigts x0 x1 y   (ex. 3 doigts de x=300 à x=1100 à y=900)
 * Compil : aarch64-linux-gnu-gcc -static -O2 -o duo-swipe duo-swipe.c */
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int fd;
static void ev(int t, int c, int v) { struct input_event e = { .type = t, .code = c, .value = v }; if (write(fd, &e, sizeof e) < 0) perror("write"); }

int main(int argc, char **argv)
{
	if (argc < 6) { fprintf(stderr, "usage: duo-swipe dev doigts x0 x1 y [y1]  (y1 : glissement vertical)\n"); return 1; }
	fd = open(argv[1], O_WRONLY); if (fd < 0) { perror(argv[1]); return 1; }
	int n = atoi(argv[2]), x0 = atoi(argv[3]), x1 = atoi(argv[4]), y0 = atoi(argv[5]), steps = 12;
	int y1 = argc > 6 ? atoi(argv[6]) : y0;
	for (int s = 0; s <= steps; s++) {
		int x = x0 + (x1 - x0) * s / steps, y = y0 + (y1 - y0) * s / steps;
		for (int f = 0; f < n; f++) {
			ev(EV_ABS, ABS_MT_SLOT, f);
			if (s == 0) ev(EV_ABS, ABS_MT_TRACKING_ID, 900 + f);
			ev(EV_ABS, ABS_MT_POSITION_X, x); ev(EV_ABS, ABS_MT_POSITION_Y, y + f * 120);
		}
		if (s == 0) ev(EV_KEY, BTN_TOUCH, 1);
		ev(EV_SYN, SYN_REPORT, 0);
		usleep(25000);
	}
	for (int f = 0; f < n; f++) { ev(EV_ABS, ABS_MT_SLOT, f); ev(EV_ABS, ABS_MT_TRACKING_ID, -1); }
	ev(EV_KEY, BTN_TOUCH, 0); ev(EV_SYN, SYN_REPORT, 0);
	return 0;
}
