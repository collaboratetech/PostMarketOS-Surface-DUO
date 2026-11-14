# Credits and licences

| Component | Origin | Licence |
|---|---|---|
| Kernel msm-4.14 | Microsoft, [surface-duo-oss-kernel.msm-4.14](https://github.com/microsoft/surface-duo-oss-kernel.msm-4.14); build recipe from the Droidian Surface Duo port (alghiffaryfa19, verbovoy) | GPL-2.0 |
| Touch heat-map driver (`tools/duoheat.c`, modified), Bluetooth (`duo-bt`), camera (`matrix-camera`, `duo-camgrab`), sensor/audio/Wi-Fi recipes and patches, `duo-blmirror` | [Matrix-Hardline/matrix-duo](https://github.com/Matrix-Hardline/matrix-duo) | GPL-3.0-or-later / MIT |
| Alpine Linux packages | [alpinelinux.org](https://alpinelinux.org) | various (see each package) |
| Phosh, phoc (patched), squeekboard | Purism / GNOME | GPL-3.0 |
| Mesa (Turnip, Zink) | [mesa3d.org](https://mesa3d.org) | MIT |
| hexagonrpc, libssc | [linux-msm](https://github.com/linux-msm) / postmarketOS community | GPL / LGPL |
| qrtr, rmtfs, pd-mapper, tqftpserv | [linux-msm](https://github.com/linux-msm) | BSD-3-Clause |
| lisgd | [lisgd](https://git.sr.ht/~mil/lisgd) | MIT |
| initramfs base (busybox) | postmarketOS / pmbootstrap | GPL-2.0 |
| Our own scripts, tools and patches (this repository) | the contributors | GPL-3.0-or-later |

**Firmware is not distributed.** The installer copies it from the user's own phone at install time.
