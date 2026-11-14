/* duo-reboot : redémarre le Duo, éventuellement vers un mode du chargeur (ex. « bootloader » = fastboot).
 * Usage : duo-reboot [bootloader|recovery]
 * Compil : aarch64-linux-gnu-gcc -static -O2 -o duo-reboot duo-reboot.c */
#include <linux/reboot.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	sync();
	if (argc > 1)
		syscall(SYS_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_RESTART2, argv[1]);
	else
		syscall(SYS_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_RESTART, NULL);
	perror("reboot");
	return 1;
}
