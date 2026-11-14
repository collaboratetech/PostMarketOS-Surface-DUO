/* duo-dmlin : crée un volume device-mapper « linear » à plusieurs segments (sans dmsetup ni libdevmapper).
 * Usage : duo-dmlin NOM PERIPHERIQUE debut1:longueur1 [debut2:longueur2 ...]   (en secteurs de 512 octets)
 *   -> /dev/mapper/NOM (nœud créé ici, pas de udev au démarrage)
 * Compil : aarch64-linux-gnu-gcc -static -O2 -o duo-dmlin duo-dmlin.c */
#include <fcntl.h>
#include <linux/dm-ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>

static void init(struct dm_ioctl *io, size_t taille, const char *nom)
{
	memset(io, 0, taille);
	io->version[0] = DM_VERSION_MAJOR; io->version[1] = 0; io->version[2] = 0;
	io->data_size = taille; io->data_start = sizeof(struct dm_ioctl);
	strncpy(io->name, nom, DM_NAME_LEN - 1);
}

int main(int argc, char **argv)
{
	if (argc < 4) { fprintf(stderr, "usage: duo-dmlin NOM PERIPH debut:longueur...\n"); return 2; }
	const char *nom = argv[1], *dev = argv[2];
	int n = argc - 3;
	int fd = open("/dev/mapper/control", O_RDWR);
	if (fd < 0) { mkdir("/dev/mapper", 0755); mknod("/dev/mapper/control", S_IFCHR | 0600, makedev(10, 236));
		fd = open("/dev/mapper/control", O_RDWR); }
	if (fd < 0) { perror("/dev/mapper/control"); return 1; }

	char buf[16384]; struct dm_ioctl *io = (struct dm_ioctl *)buf;
	init(io, sizeof(buf), nom);
	if (ioctl(fd, DM_DEV_CREATE, io)) { perror("DM_DEV_CREATE"); return 1; }

	init(io, sizeof(buf), nom);
	io->target_count = n;
	char *p = buf + sizeof(struct dm_ioctl);
	unsigned long long pos = 0;
	for (int i = 0; i < n; i++) {
		unsigned long long deb, lon;
		if (sscanf(argv[3 + i], "%llu:%llu", &deb, &lon) != 2) { fprintf(stderr, "segment invalide: %s\n", argv[3 + i]); return 2; }
		struct dm_target_spec *t = (struct dm_target_spec *)p;
		memset(t, 0, sizeof(*t));
		t->sector_start = pos; t->length = lon; strcpy(t->target_type, "linear");
		char *param = p + sizeof(*t);
		int l = snprintf(param, 256, "%s %llu", dev, deb) + 1;
		size_t total = sizeof(*t) + l; total = (total + 7) & ~7UL;
		t->next = total; p += total; pos += lon;
	}
	if (ioctl(fd, DM_TABLE_LOAD, io)) { perror("DM_TABLE_LOAD"); return 1; }

	init(io, sizeof(buf), nom);
	if (ioctl(fd, DM_DEV_SUSPEND, io)) { perror("DM_DEV_SUSPEND (reprise)"); return 1; }   /* sans DM_SUSPEND_FLAG = reprise */

	init(io, sizeof(buf), nom);
	if (ioctl(fd, DM_DEV_STATUS, io)) { perror("DM_DEV_STATUS"); return 1; }
	char chemin[300]; snprintf(chemin, sizeof chemin, "/dev/mapper/%s", nom);
	unlink(chemin);
	if (mknod(chemin, S_IFBLK | 0600, io->dev)) { perror(chemin); return 1; }
	printf("%s : %llu secteurs (%llu Mo), %d segments\n", chemin, pos, pos / 2048, n);
	return 0;
}
