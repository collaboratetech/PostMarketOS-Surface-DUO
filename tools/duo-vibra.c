/* duo-vibra : pont entre feedbackd et le vibreur du Surface Duo (da7280, noyau 4.14 downstream).
 * Le pilote n'accepte que FF_CONSTANT (FF_RUMBLE -> EFAULT) et n'arrête pas tout seul l'effet à la fin de sa durée.
 * On crée un périphérique virtuel « duo-vibra » (uinput, FF_RUMBLE/PERIODIC/CONSTANT/GAIN) que feedbackd reconnaît
 * (règle udev FEEDBACKD_TYPE=vibra), on mémorise les effets qu'il téléverse et on les rejoue sur le da7280
 * en FF_CONSTANT, avec arrêt explicite après la durée demandée.
 * Compil : aarch64-linux-gnu-gcc -static -O2 -o duo-vibra duo-vibra.c */
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#define NEFF 16
struct eff { int used, ms, mag; } effs[NEFF];
static int hw = -1, hw_id = -1, gain = 0xFFFF;
static long long fin_ms;	/* heure de fin de la vibration en cours (0 = arrêtée) */

static long long now_ms(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec * 1000LL + t.tv_nsec / 1000000; }

static int find_hw(void)
{
	DIR *d = opendir("/dev/input"); struct dirent *e; char p[300], name[128];
	if (!d) return -1;
	while ((e = readdir(d))) {
		if (strncmp(e->d_name, "event", 5)) continue;
		snprintf(p, sizeof p, "/dev/input/%s", e->d_name);
		int fd = open(p, O_RDWR); if (fd < 0) continue;
		memset(name, 0, sizeof name); ioctl(fd, EVIOCGNAME(sizeof name - 1), name);
		if (!strcmp(name, "da7280-haptic")) { closedir(d); fprintf(stderr, "duo-vibra: vibreur %s\n", p); return fd; }
		close(fd);
	}
	closedir(d); return -1;
}

static void hw_ev(int code, int val) { struct input_event ev = { .type = EV_FF, .code = code, .value = val }; if (write(hw, &ev, sizeof ev) < 0) perror("duo-vibra: write"); }

static void stop(void) { if (fin_ms && hw_id >= 0) hw_ev(hw_id, 0); fin_ms = 0; }

static void play(int ms, int mag)
{
	stop();
	struct ff_effect e = { .type = FF_CONSTANT, .id = hw_id };
	long lvl = (long)mag * gain / 0xFFFF / 2;	/* niveau FF_CONSTANT : -32768..32767 */
	if (lvl < 0x3000) lvl = 0x3000;
	/* impulsions courtes (frappe clavier, boutons) : trop faibles pour être senties sur le da7280 ;
	 * recette Matrix pour le clavier : 12 ms à 0x5000 */
	if (ms < 15) { ms = 12; if (lvl < 0x5000) lvl = 0x5000; }
	if (lvl > 0x7FFF) lvl = 0x7FFF;
	e.u.constant.level = (short)lvl; e.replay.length = ms;
	if (ioctl(hw, EVIOCSFF, &e) < 0) { perror("duo-vibra: EVIOCSFF"); return; }
	hw_id = e.id;
	hw_ev(hw_id, 1);
	fin_ms = now_ms() + ms;
	if (access("/run/duo/vibra-journal", F_OK) == 0)	/* diagnostic : touch /run/duo/vibra-journal */
		fprintf(stderr, "duo-vibra: vibration %d ms, niveau %ld\n", ms, lvl);
}

int main(void)
{
	for (int i = 0; i < 60 && (hw = find_hw()) < 0; i++) sleep(1);
	if (hw < 0) { fprintf(stderr, "duo-vibra: da7280-haptic introuvable\n"); return 1; }
	int ui = open("/dev/uinput", O_RDWR);
	if (ui < 0) { perror("/dev/uinput"); return 1; }
	ioctl(ui, UI_SET_EVBIT, EV_FF);
	int bits[] = { FF_RUMBLE, FF_PERIODIC, FF_CONSTANT, FF_SINE, FF_SQUARE, FF_TRIANGLE, FF_GAIN };
	for (unsigned i = 0; i < sizeof bits / sizeof *bits; i++) ioctl(ui, UI_SET_FFBIT, bits[i]);
	struct uinput_user_dev u = { 0 };
	snprintf(u.name, sizeof u.name, "duo-vibra"); u.id.bustype = BUS_VIRTUAL; u.id.vendor = 0x045e; u.id.product = 0x0d00; u.ff_effects_max = NEFF;
	if (write(ui, &u, sizeof u) != sizeof u || ioctl(ui, UI_DEV_CREATE) < 0) { perror("duo-vibra: uinput"); return 1; }
	fprintf(stderr, "duo-vibra: périphérique virtuel créé\n");

	for (;;) {
		int to = -1;
		if (fin_ms) { long long r = fin_ms - now_ms(); to = r > 0 ? (int)r : 0; }
		struct pollfd p = { .fd = ui, .events = POLLIN };
		int n = poll(&p, 1, to);
		if (fin_ms && now_ms() >= fin_ms) stop();
		if (n <= 0) continue;
		struct input_event ev;
		if (read(ui, &ev, sizeof ev) != sizeof ev) continue;
		if (ev.type == EV_UINPUT && ev.code == UI_FF_UPLOAD) {
			struct uinput_ff_upload up = { .request_id = ev.value };
			ioctl(ui, UI_BEGIN_FF_UPLOAD, &up);
			struct ff_effect *e = &up.effect; int mag = 0x8000;
			if (e->type == FF_RUMBLE) mag = e->u.rumble.strong_magnitude > e->u.rumble.weak_magnitude ? e->u.rumble.strong_magnitude : e->u.rumble.weak_magnitude;
			else if (e->type == FF_PERIODIC) mag = abs(e->u.periodic.magnitude) * 2;
			else if (e->type == FF_CONSTANT) mag = abs(e->u.constant.level) * 2;
			if (e->id >= 0 && e->id < NEFF) { effs[e->id].used = 1; effs[e->id].ms = e->replay.length ? e->replay.length : 1000; effs[e->id].mag = mag; up.retval = 0; }
			else up.retval = -EINVAL;
			ioctl(ui, UI_END_FF_UPLOAD, &up);
		} else if (ev.type == EV_UINPUT && ev.code == UI_FF_ERASE) {
			struct uinput_ff_erase er = { .request_id = ev.value };
			ioctl(ui, UI_BEGIN_FF_ERASE, &er);
			if (er.effect_id < NEFF) effs[er.effect_id].used = 0;
			er.retval = 0;
			ioctl(ui, UI_END_FF_ERASE, &er);
		} else if (ev.type == EV_FF && ev.code == FF_GAIN) {
			gain = ev.value;
		} else if (ev.type == EV_FF && ev.code < NEFF && effs[ev.code].used) {
			if (ev.value > 0) play(effs[ev.code].ms, effs[ev.code].mag); else stop();
		}
	}
}
