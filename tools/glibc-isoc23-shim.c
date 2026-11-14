/* glibc-isoc23-shim : fonctions « C23 » de glibc 2.38+ absentes de gcompat (Alpine/musl), pour faire tourner
 * les outils caméra compilés sous Fedora par Matrix (duo-camgrab, matrix-camera). Chacune renvoie vers la
 * fonction musl équivalente (le comportement C23 ne diffère que pour les préfixes 0b, inutilisés ici).
 * Compil : aarch64-linux-gnu-gcc -shared -fPIC -nostdlib -O2 -o libduo-isoc23.so glibc-isoc23-shim.c */
#include <stdarg.h>
#include <stddef.h>

typedef struct _IO_FILE FILE;
extern long strtol(const char *, char **, int);
extern unsigned long strtoul(const char *, char **, int);
extern long long strtoll(const char *, char **, int);
extern unsigned long long strtoull(const char *, char **, int);
extern int vsscanf(const char *, const char *, va_list);
extern int vfscanf(FILE *, const char *, va_list);
extern int vscanf(const char *, va_list);

long __isoc23_strtol(const char *s, char **e, int b) { return strtol(s, e, b); }
unsigned long __isoc23_strtoul(const char *s, char **e, int b) { return strtoul(s, e, b); }
long long __isoc23_strtoll(const char *s, char **e, int b) { return strtoll(s, e, b); }
unsigned long long __isoc23_strtoull(const char *s, char **e, int b) { return strtoull(s, e, b); }
long __isoc23_strtoimax(const char *s, char **e, int b) { return strtoll(s, e, b); }
unsigned long __isoc23_strtoumax(const char *s, char **e, int b) { return strtoull(s, e, b); }
int __isoc23_vsscanf(const char *s, const char *f, va_list a) { return vsscanf(s, f, a); }
int __isoc23_vfscanf(FILE *fp, const char *f, va_list a) { return vfscanf(fp, f, a); }
int __isoc23_sscanf(const char *s, const char *f, ...) { va_list a; va_start(a, f); int r = vsscanf(s, f, a); va_end(a); return r; }
int __isoc23_fscanf(FILE *fp, const char *f, ...) { va_list a; va_start(a, f); int r = vfscanf(fp, f, a); va_end(a); return r; }
int __isoc23_scanf(const char *f, ...) { va_list a; va_start(a, f); int r = vscanf(f, a); va_end(a); return r; }

/* glibc 2.39 : lancement de processus par pidfd (Rust std l'essaie d'abord). Absent de musl : on renvoie ENOSYS,
 * Rust se rabat alors sur fork/exec classique. */
extern int *__errno_location(void);
#define ENOSYS_ 38
int pidfd_spawnp(int *pidfd, const char *file, const void *fa, const void *attr, char *const argv[], char *const envp[])
{ (void)pidfd; (void)file; (void)fa; (void)attr; (void)argv; (void)envp; return ENOSYS_; }
int pidfd_getpid(int fd) { (void)fd; *__errno_location() = ENOSYS_; return -1; }
