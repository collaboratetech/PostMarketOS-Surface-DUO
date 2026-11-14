/* bindsh : shell sur TCP (port 2323 par défaut), un processus par connexion. Statique, sans dépendance.
 * Uniquement pour l'initramfs de test du Duo, sur le lien USB local. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/socket.h>

int main(int argc, char **argv)
{
	int port = argc > 1 ? atoi(argv[1]) : 2323, one = 1;
	signal(SIGCHLD, SIG_IGN);
	int s = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	struct sockaddr_in a = { .sin_family = AF_INET, .sin_port = htons(port), .sin_addr.s_addr = INADDR_ANY };
	if (bind(s, (struct sockaddr *)&a, sizeof(a)) || listen(s, 4)) { perror("bindsh"); return 1; }
	for (;;) {
		int c = accept(s, 0, 0);
		if (c < 0) continue;
		if (!fork()) {
			close(s); setsid();
			dup2(c, 0); dup2(c, 1); dup2(c, 2);
			execl("/bin/sh", "sh", "-i", (char *)0);
			_exit(1);
		}
		close(c);
	}
}
