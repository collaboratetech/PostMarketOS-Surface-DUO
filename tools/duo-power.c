/* duo-power : boutons physiques du Surface Duo sous Phosh (lancé dans la session D-Bus).
 *  - Power : Phosh éteint les écrans au 1er appui mais ne sait pas les rallumer sans session GNOME ; quand Power est
 *    relâchée et que les écrans étaient éteints à l'appui, on remet PowerSaveMode à 0 (Mutter DisplayConfig de Phosh).
 *  - Volume + (gpio-keys) / Volume - (qpnp_pon) : Phosh ne les gère pas ; ±5 % sur la sortie par défaut (wpctl,
 *    max 100 %), répétition si maintenu, et affichage de l'indicateur de volume de Phosh (org.gnome.Shell.ShowOSD).
 * Compil : aarch64-linux-gnu-gcc -static -O2 -o duo-power duo-power.c */
#include <fcntl.h>
#include <linux/input.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DC "gdbus call --session --dest org.gnome.Mutter.DisplayConfig --object-path /org/gnome/Mutter/DisplayConfig " \
           "--method org.freedesktop.DBus.Properties."

static int open_dev(const char *want)
{
	char path[64], name[256];
	for (int i = 0; i < 32; i++) {
		snprintf(path, sizeof(path), "/dev/input/event%d", i);
		int fd = open(path, O_RDONLY);
		if (fd < 0) continue;
		memset(name, 0, sizeof(name));
		ioctl(fd, EVIOCGNAME(sizeof(name) - 1), name);
		if (!strcmp(name, want)) { fprintf(stderr, "duo-power: %s = %s\n", want, path); return fd; }
		close(fd);
	}
	return -1;
}

/* écrans éteints par Phosh = connecteur DSI-1 « disabled » côté DRM */
static int ecrans_eteints(void)
{
	char b[16] = { 0 };
	FILE *f = fopen("/sys/class/drm/card0-DSI-1/enabled", "r");
	if (!f) return 0;
	if (!fgets(b, sizeof(b), f)) b[0] = 0;
	fclose(f);
	return !strncmp(b, "disabled", 8);
}

static void volume(int sens)
{
	char cmd[512];
	/* pendant un appel : volume de la voix (DSP), pas PipeWire (le régler coupait le son de l'appel, 12/11) */
	if (!access("/run/duo/appel-en-cours", F_OK)) {
		if (system(sens > 0 ? "/usr/local/sbin/duo-appel-volume + >/dev/null 2>&1"
		                    : "/usr/local/sbin/duo-appel-volume - >/dev/null 2>&1")) {}
		return;
	}
	if (system(sens > 0 ? "wpctl set-volume -l 1.0 @DEFAULT_AUDIO_SINK@ 5%+ >/dev/null 2>&1"
	                    : "wpctl set-volume @DEFAULT_AUDIO_SINK@ 5%- >/dev/null 2>&1") != 0) return;
	double v = 0; char buf[128] = { 0 };
	FILE *p = popen("wpctl get-volume @DEFAULT_AUDIO_SINK@ 2>/dev/null", "r");
	if (p) { if (fgets(buf, sizeof(buf), p)) { char *s = strpbrk(buf, "0123456789"); if (s) v = atof(s); } pclose(p); }
	const char *icone = v <= 0.001 ? "audio-volume-muted-symbolic" : v < 0.34 ? "audio-volume-low-symbolic"
	                  : v < 0.67 ? "audio-volume-medium-symbolic" : "audio-volume-high-symbolic";
	snprintf(cmd, sizeof(cmd), "gdbus call --session --dest org.gnome.Shell --object-path /org/gnome/Shell "
	         "--method org.gnome.Shell.ShowOSD \"{'icon': <'%s'>, 'level': <%.2f>}\" >/dev/null 2>&1 &", icone, v);
	if (system(cmd)) { }
}

int main(void)
{
	int pon = -1, gpio = -1;
	for (int i = 0; i < 30 && (pon < 0 || gpio < 0); i++) {
		if (pon < 0) pon = open_dev("qpnp_pon");
		if (gpio < 0) gpio = open_dev("gpio-keys");
		if (pon < 0 || gpio < 0) sleep(1);
	}
	if (pon < 0) { fprintf(stderr, "duo-power: qpnp_pon introuvable\n"); return 1; }
	struct pollfd pf[2] = { { .fd = pon, .events = POLLIN }, { .fd = gpio, .events = POLLIN } };
	int eteint_a_l_appui = 0;
	for (;;) {
		if (poll(pf, gpio >= 0 ? 2 : 1, -1) <= 0) continue;
		for (int k = 0; k < 2; k++) {
			if (!(pf[k].revents & POLLIN)) continue;
			struct input_event e;
			if (read(pf[k].fd, &e, sizeof(e)) != sizeof(e) || e.type != EV_KEY) continue;
			if (e.code == KEY_VOLUMEUP || e.code == KEY_VOLUMEDOWN) {
				if (e.value >= 1) volume(e.code == KEY_VOLUMEUP ? 1 : -1);	/* 1 = appui, 2 = répétition */
				continue;
			}
			if (e.code != KEY_POWER) continue;
			if (e.value == 1) { eteint_a_l_appui = ecrans_eteints(); continue; }
			if (e.value != 0 || !eteint_a_l_appui) continue;
			usleep(200000);
			if (system(DC "Set org.gnome.Mutter.DisplayConfig PowerSaveMode '<int32 0>' >/dev/null 2>&1") == 0)
				fprintf(stderr, "duo-power: écrans rallumés\n");
		}
	}
	return 0;
}
