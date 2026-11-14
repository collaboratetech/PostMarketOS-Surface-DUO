/* drmfill : allume chaque écran DRM connecté avec une mire (raw ioctls, sans libdrm).
 * Usage : drmfill [secondes] [motif]   motif 0 = mire (blanc haut / dégradé couleur bas), 1 = blanc, 2 = rouge,
 *         3 = logo de démarrage (/logo.raw : largeur, hauteur en uint32 puis pixels XRGB8888), fond noir
 * Compil : aarch64-linux-gnu-gcc -static -O2 -o drmfill drmfill.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <drm/drm.h>
#include <drm/drm_mode.h>

static int fd;
static void *xcalloc(size_t n, size_t s) { void *p = calloc(n ? n : 1, s); if (!p) exit(1); return p; }

static uint32_t logo_w, logo_h, *logo;
static void charge_logo(void)
{
	FILE *f = fopen("/logo.raw", "rb"); uint32_t t[2];
	if (!f || fread(t, 4, 2, f) != 2 || t[0] > 4096 || t[1] > 4096) { if (f) fclose(f); return; }
	logo = xcalloc((size_t)t[0] * t[1], 4);
	if (fread(logo, 4, (size_t)t[0] * t[1], f) == (size_t)t[0] * t[1]) { logo_w = t[0]; logo_h = t[1]; }
	fclose(f);
}

static void paint(uint32_t *px, int w, int h, int pitch, int motif, int idx)
{
	if (motif == 3) {	/* logo centré sur le 1er écran, noir sur l'autre */
		for (int y = 0; y < h; y++) memset((char *)px + y * pitch, 0, w * 4);
		if (idx || !logo) return;
		int ox = (w - (int)logo_w) / 2, oy = (h - (int)logo_h) / 2;
		for (uint32_t y = 0; y < logo_h; y++) {
			if (oy + (int)y < 0 || oy + (int)y >= h) continue;
			uint32_t *row = (uint32_t *)((char *)px + (oy + y) * pitch);
			for (uint32_t x = 0; x < logo_w; x++)
				if (ox + (int)x >= 0 && ox + (int)x < w) row[ox + x] = logo[y * logo_w + x];
		}
		return;
	}
	for (int y = 0; y < h; y++) {
		uint32_t *row = (uint32_t *)((char *)px + y * pitch);
		for (int x = 0; x < w; x++) {
			uint32_t c;
			if (motif == 1) c = 0xffffff;
			else if (motif == 2) c = 0xff0000;
			else if (y < h / 3) c = 0xffffff;                       /* tiers haut blanc */
			else if (y < 2 * h / 3) c = (x < w / 3) ? 0xff0000 : (x < 2 * w / 3) ? 0x00ff00 : 0x0000ff;
			else { uint32_t g = x * 255 / w; c = (g << 16) | (g << 8) | g; } /* dégradé gris */
			if (idx && y > h / 3 - 40 && y < h / 3 && x < w / 2) c = 0xffff00; /* repère écran 2 */
			row[x] = c;
		}
	}
}

int main(int argc, char **argv)
{
	int secs = argc > 1 ? atoi(argv[1]) : 30, motif = argc > 2 ? atoi(argv[2]) : 0;
	if (motif == 3) charge_logo();
	fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
	if (fd < 0) { perror("card0"); return 1; }
	if (ioctl(fd, DRM_IOCTL_SET_MASTER, 0)) perror("SET_MASTER (on continue)");

	struct drm_mode_card_res res = {0};
	if (ioctl(fd, DRM_IOCTL_MODE_GETRESOURCES, &res)) { perror("GETRESOURCES"); return 1; }
	uint32_t *crtcs = xcalloc(res.count_crtcs, 4), *conns = xcalloc(res.count_connectors, 4),
		 *encs = xcalloc(res.count_encoders, 4), *fbs = xcalloc(res.count_fbs, 4);
	res.crtc_id_ptr = (uintptr_t)crtcs; res.connector_id_ptr = (uintptr_t)conns;
	res.encoder_id_ptr = (uintptr_t)encs; res.fb_id_ptr = (uintptr_t)fbs;
	ioctl(fd, DRM_IOCTL_MODE_GETRESOURCES, &res);
	printf("crtcs %u connecteurs %u encodeurs %u\n", res.count_crtcs, res.count_connectors, res.count_encoders);

	int used_crtc[16] = {0}, lit = 0;
	for (unsigned i = 0; i < res.count_connectors; i++) {
		struct drm_mode_get_connector c = {0};
		c.connector_id = conns[i];
		ioctl(fd, DRM_IOCTL_MODE_GETCONNECTOR, &c);
		struct drm_mode_modeinfo *modes = xcalloc(c.count_modes, sizeof(*modes));
		uint32_t *cencs = xcalloc(c.count_encoders, 4);
		uint32_t *props = xcalloc(c.count_props, 4); uint64_t *pv = xcalloc(c.count_props, 8);
		c.modes_ptr = (uintptr_t)modes; c.encoders_ptr = (uintptr_t)cencs;
		c.props_ptr = (uintptr_t)props; c.prop_values_ptr = (uintptr_t)pv;
		ioctl(fd, DRM_IOCTL_MODE_GETCONNECTOR, &c);
		printf("connecteur %u type %u état %u modes %u\n", c.connector_id, c.connector_type, c.connection, c.count_modes);
		if (c.connection != 1 || !c.count_modes) continue;
		struct drm_mode_modeinfo *m = &modes[0];
		printf("  mode %s %ux%u@%u\n", m->name, m->hdisplay, m->vdisplay, m->vrefresh);

		/* crtc libre compatible avec un encodeur du connecteur */
		int crtc = -1;
		for (unsigned e = 0; e < c.count_encoders && crtc < 0; e++) {
			struct drm_mode_get_encoder enc = { .encoder_id = cencs[e] };
			ioctl(fd, DRM_IOCTL_MODE_GETENCODER, &enc);
			for (unsigned k = 0; k < res.count_crtcs; k++)
				if ((enc.possible_crtcs & (1u << k)) && !used_crtc[k]) { crtc = k; break; }
		}
		if (crtc < 0) { printf("  pas de crtc libre\n"); continue; }
		used_crtc[crtc] = 1;

		struct drm_mode_create_dumb cd = { .width = m->hdisplay, .height = m->vdisplay, .bpp = 32 };
		if (ioctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &cd)) { perror("CREATE_DUMB"); continue; }
		struct drm_mode_fb_cmd fb = { .width = cd.width, .height = cd.height, .pitch = cd.pitch,
					      .bpp = 32, .depth = 24, .handle = cd.handle };
		if (ioctl(fd, DRM_IOCTL_MODE_ADDFB, &fb)) { perror("ADDFB"); continue; }
		struct drm_mode_map_dumb md = { .handle = cd.handle };
		ioctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &md);
		uint32_t *px = mmap(0, cd.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, md.offset);
		if (px == MAP_FAILED) { perror("mmap"); continue; }
		paint(px, cd.width, cd.height, cd.pitch, motif, lit);

		struct drm_mode_crtc sc = { .crtc_id = crtcs[crtc], .fb_id = fb.fb_id,
					    .set_connectors_ptr = (uintptr_t)&c.connector_id, .count_connectors = 1,
					    .mode = *m, .mode_valid = 1 };
		if (ioctl(fd, DRM_IOCTL_MODE_SETCRTC, &sc)) perror("  SETCRTC");
		else { printf("  allumé sur crtc %u\n", crtcs[crtc]); lit++; }
		/* écrans en mode commande : signaler la zone modifiée */
		struct drm_clip_rect clip = { 0, 0, cd.width, cd.height };
		struct drm_mode_fb_dirty_cmd d = { .fb_id = fb.fb_id, .num_clips = 1, .clips_ptr = (uintptr_t)&clip };
		ioctl(fd, DRM_IOCTL_MODE_DIRTYFB, &d);
	}
	printf("%d écran(s) allumé(s), attente %d s\n", lit, secs);
	fflush(stdout);
	sleep(secs);
	return 0;
}
