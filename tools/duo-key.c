/* duo-key : appuie sur une touche ou une combinaison via un clavier virtuel (uinput).
 * Usage : duo-key CODE [CODE...] [/dev/input/eventN]
 *   ex. duo-key 116            (KEY_POWER)
 *       duo-key 56 105         (Alt + Gauche = « Retour » dans les applis GNOME et Firefox)
 *   Dernier argument commençant par « / » : injection directe dans ce périphérique existant (ex. vrai bouton).
 * Les touches sont enfoncées dans l'ordre puis relâchées dans l'ordre inverse.
 * Compil : aarch64-linux-gnu-gcc -static -O2 -o duo-key duo-key.c */
#include <fcntl.h>
#include <linux/uinput.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

static void ev(int fd, int type, int code, int val)
{
	struct input_event e = { .type = type, .code = code, .value = val };
	if (write(fd, &e, sizeof(e)) < 0) perror("write");
}

static void combo(int fd, int *k, int n)
{
	for (int i = 0; i < n; i++) { ev(fd, EV_KEY, k[i], 1); ev(fd, EV_SYN, SYN_REPORT, 0); }
	usleep(60000);
	for (int i = n - 1; i >= 0; i--) { ev(fd, EV_KEY, k[i], 0); ev(fd, EV_SYN, SYN_REPORT, 0); }
}

int main(int argc, char **argv)
{
	int k[8], n = 0;
	const char *dev = NULL;
	for (int i = 1; i < argc && n < 8; i++) {
		if (argv[i][0] == '/') dev = argv[i];
		else k[n++] = atoi(argv[i]);
	}
	if (!n) k[n++] = KEY_POWER;
	if (dev) {	/* injecter directement dans un périphérique existant */
		int d = open(dev, O_WRONLY);
		if (d < 0) { perror(dev); return 1; }
		combo(d, k, n); close(d); return 0;
	}
	int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
	if (fd < 0) { perror("/dev/uinput"); return 1; }
	ioctl(fd, UI_SET_EVBIT, EV_KEY);
	for (int i = 0; i < n; i++) ioctl(fd, UI_SET_KEYBIT, k[i]);
	struct uinput_user_dev ud = { 0 };
	snprintf(ud.name, sizeof(ud.name), "Surface Duo virtual key");
	ud.id.bustype = BUS_VIRTUAL; ud.id.vendor = 0x045e; ud.id.product = 0x0c11;
	if (write(fd, &ud, sizeof(ud)) < 0 || ioctl(fd, UI_DEV_CREATE)) { perror("uinput"); return 1; }
	usleep(400000);	/* laisser le compositeur découvrir le clavier */
	combo(fd, k, n);
	usleep(200000);
	ioctl(fd, UI_DEV_DESTROY); close(fd);
	return 0;
}
