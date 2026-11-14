/* duo-move : envoie Super+Maj+Gauche/Droite par un clavier virtuel (uinput) pour que phoc déplace
 * la fenêtre active sur l'écran voisin (raccourci « move-to-monitor-left/right » de GNOME/phoc).
 * Usage : duo-move left|right      Compil : aarch64-linux-gnu-gcc -static -O2 -o duo-move duo-move.c */
#include <fcntl.h>
#include <linux/uinput.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

static void ev(int fd, int type, int code, int val)
{
	struct input_event e = { .type = type, .code = code, .value = val };
	if (write(fd, &e, sizeof(e)) < 0) perror("write");
}

int main(int argc, char **argv)
{
	int dir = (argc > 1 && !strcmp(argv[1], "left")) ? KEY_LEFT : KEY_RIGHT;
	int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
	if (fd < 0) { perror("/dev/uinput"); return 1; }
	ioctl(fd, UI_SET_EVBIT, EV_KEY);
	int keys[] = { KEY_LEFTMETA, KEY_LEFTSHIFT, KEY_LEFT, KEY_RIGHT };
	for (unsigned i = 0; i < sizeof(keys) / sizeof(*keys); i++) ioctl(fd, UI_SET_KEYBIT, keys[i]);
	struct uinput_user_dev ud = { 0 };
	snprintf(ud.name, sizeof(ud.name), "Surface Duo move-window keys");
	ud.id.bustype = BUS_VIRTUAL; ud.id.vendor = 0x045e; ud.id.product = 0x0c10;
	if (write(fd, &ud, sizeof(ud)) < 0 || ioctl(fd, UI_DEV_CREATE)) { perror("uinput"); return 1; }
	usleep(400000);	/* laisser le compositeur découvrir le clavier */
	ev(fd, EV_KEY, KEY_LEFTMETA, 1); ev(fd, EV_KEY, KEY_LEFTSHIFT, 1); ev(fd, EV_SYN, SYN_REPORT, 0);
	ev(fd, EV_KEY, dir, 1); ev(fd, EV_SYN, SYN_REPORT, 0);
	ev(fd, EV_KEY, dir, 0); ev(fd, EV_SYN, SYN_REPORT, 0);
	ev(fd, EV_KEY, KEY_LEFTSHIFT, 0); ev(fd, EV_KEY, KEY_LEFTMETA, 0); ev(fd, EV_SYN, SYN_REPORT, 0);
	usleep(200000);
	ioctl(fd, UI_DEV_DESTROY);
	close(fd);
	return 0;
}
