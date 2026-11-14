/* duo-ipa-data : prépare le chemin de données mobiles du pilote IPA (noyau Android msm-4.14), comme netmgrd :
 *   formats de données QMAP entrée/sortie sur rmnet_ipa0, canal de multiplexage, et lecture des « tuyaux » IPA
 *   à déclarer au modem (QMI DPM open-port).
 * Usage : duo-ipa-data [mux_id [nom_canal]]   -> affiche « epid=N cons=C prod=P »
 * Compil : aarch64-linux-gnu-gcc -static -O2 -o duo-ipa-data duo-ipa-data.c */
#include <net/if.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#define RMNET_IOCTL_EXTENDED                0x89FD
#define RMNET_IOCTL_GET_EPID                0x0003
#define RMNET_IOCTL_ADD_MUX_CHANNEL         0x0005
#define RMNET_IOCTL_SET_EGRESS_DATA_FORMAT  0x0006
#define RMNET_IOCTL_SET_INGRESS_DATA_FORMAT 0x0007
#define RMNET_IOCTL_GET_EP_PAIR             0x0010
#define EGRESS_MAP   (1 << 1)
#define INGRESS_MAP  (1 << 1)

struct ext {
	uint32_t extended_ioctl;
	union {
		uint32_t data;
		int8_t if_name[IFNAMSIZ];
		struct { uint32_t mux_id; int8_t vchannel_name[IFNAMSIZ]; } mux;
		struct { uint32_t consumer_pipe_num, producer_pipe_num; } pair;
		struct { uint32_t data, agg_size, agg_count; } ingress;
		uint8_t pad[64];
	} u;
};

static int fd;

static int x(const char *nom, struct ext *e)
{
	struct ifreq ifr;
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, "rmnet_ipa0", IFNAMSIZ - 1);
	ifr.ifr_ifru.ifru_data = (void *)e;
	int r = ioctl(fd, RMNET_IOCTL_EXTENDED, &ifr);
	if (r < 0) { fprintf(stderr, "%s : ", nom); perror(""); }
	return r;
}

int main(int argc, char **argv)
{
	uint32_t mux = argc > 1 ? (uint32_t)atoi(argv[1]) : 1;
	const char *nom = argc > 2 ? argv[2] : "rmnet_data0";
	struct ext e;
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) { perror("socket"); return 1; }

	memset(&e, 0, sizeof(e)); e.extended_ioctl = RMNET_IOCTL_GET_EPID;
	if (x("GET_EPID", &e)) return 1;
	uint32_t epid = e.u.data;

	memset(&e, 0, sizeof(e)); e.extended_ioctl = RMNET_IOCTL_GET_EP_PAIR;
	if (x("GET_EP_PAIR", &e)) return 1;
	uint32_t cons = e.u.pair.consumer_pipe_num, prod = e.u.pair.producer_pipe_num;

	memset(&e, 0, sizeof(e)); e.extended_ioctl = RMNET_IOCTL_SET_INGRESS_DATA_FORMAT;
	e.u.data = INGRESS_MAP;
	x("SET_INGRESS", &e);

	memset(&e, 0, sizeof(e)); e.extended_ioctl = RMNET_IOCTL_SET_EGRESS_DATA_FORMAT;
	e.u.data = EGRESS_MAP;
	x("SET_EGRESS", &e);

	memset(&e, 0, sizeof(e)); e.extended_ioctl = RMNET_IOCTL_ADD_MUX_CHANNEL;
	e.u.mux.mux_id = mux;
	strncpy((char *)e.u.mux.vchannel_name, nom, IFNAMSIZ - 1);
	x("ADD_MUX_CHANNEL", &e);

	printf("epid=%u cons=%u prod=%u\n", epid, cons, prod);
	return 0;
}
