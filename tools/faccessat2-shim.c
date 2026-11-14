/* faccessat2-shim : le noyau 4.14 du Surface Duo n'a pas l'appel système faccessat2 (Linux 5.8+).
 * musl le tente pour faccessat(..., AT_EACCESS / AT_SYMLINK_NOFOLLOW) puis se rabat sur un processus fils
 * qui plante (AppStream -> Phosh tombe dès que le catalogue d'applis est rafraîchi).
 * Ce shim (LD_PRELOAD) remplace faccessat/eaccess/euidaccess par faccessat (sans drapeaux) + newfstatat.
 * Tout tourne en root : uid réel == uid effectif, AT_EACCESS ne change rien.
 * Aucune dépendance à une libc (appels système directs) ; seul __errno_location vient de musl à l'exécution.
 * Compil : aarch64-linux-gnu-gcc -shared -fPIC -nostdlib -O2 -o libduo-faccessat2.so faccessat2-shim.c */
#define AT_FDCWD            -100
#define AT_SYMLINK_NOFOLLOW 0x100
#define AT_EACCESS          0x200
#define F_OK 0
#define S_IFMT  0170000
#define S_IFLNK 0120000
#define NR_faccessat   48
#define NR_newfstatat  79

extern int *__errno_location(void);

static long sc4(long n, long a, long b, long c, long d)
{
	register long x8 __asm__("x8") = n, x0 __asm__("x0") = a, x1 __asm__("x1") = b,
		x2 __asm__("x2") = c, x3 __asm__("x3") = d;
	__asm__ volatile("svc 0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2), "r"(x3) : "memory");
	return x0;
}

static int ret(long r) { if (r < 0 && r > -4096) { *__errno_location() = (int)-r; return -1; } return (int)r; }

struct kstat { unsigned long dev, ino; unsigned int mode, nlink, uid, gid; unsigned long rdev, pad1; long size;
	int blksize, pad2; long blocks, at, atn, mt, mtn, ct, ctn; unsigned int unused[2]; };

int faccessat(int fd, const char *path, int mode, int flag)
{
	if (flag & AT_SYMLINK_NOFOLLOW) {
		struct kstat st;
		long r = sc4(NR_newfstatat, fd, (long)path, (long)&st, AT_SYMLINK_NOFOLLOW);
		if (r < 0) return ret(r);
		if (mode == F_OK || (st.mode & S_IFMT) == S_IFLNK) return 0;
	}
	return ret(sc4(NR_faccessat, fd, (long)path, mode, 0));
}

int eaccess(const char *path, int mode) { return faccessat(AT_FDCWD, path, mode, 0); }
int euidaccess(const char *path, int mode) { return faccessat(AT_FDCWD, path, mode, 0); }
