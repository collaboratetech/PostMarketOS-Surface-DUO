# Installation

## Before you start

- **Unlock the bootloader** (Microsoft's official procedure, `fastboot flashing unlock`). This wipes Android.
- **Back up every partition.** The Surface Duo 1 has no public EDL loader: if something goes wrong, your backup is
  the only way back. At minimum keep `boot_a`, `boot_b`, `super`, `persist`, `modem_*` and `vendor_boot`.
- Charge the phone above 50 %.
- Never choose **"Factory data reset"** on Android's recovery screen while Linux is installed: it formats `userdata`.

## Get the files

From the [latest release](../../releases): `boot-duo.img` and `duo-rootfs.tar.gz`, next to `installer/`.

## Linux PC

```sh
sudo apt install fastboot adb python3      # or your distro's equivalent
cd installer
./duo.sh live       # try it from RAM: nothing is written to the phone
./duo.sh install    # replace Android
```

The Duo appears as a **USB network adapter**. The phone runs a DHCP server; if your PC doesn't take an address
automatically, give the new interface `172.16.42.2/24`.

## Windows

Use **WSL2** (Ubuntu or Debian) and **usbipd-win** to hand the Duo's USB network adapter to WSL:

```powershell
winget install usbipd
usbipd bind --hardware-id 1d6b:0104          # once, as administrator
usbipd attach --wsl --hardware-id 1d6b:0104 --auto-attach
```

`fastboot` runs on Windows (platform-tools); the rest of `duo.sh` runs in WSL.
On this setup the USB link sometimes drops during big transfers: `duo_link.py push` sends files in verified chunks.

## What `install` does

1. Boots the Linux kernel from RAM (`fastboot boot`) and sends the system into RAM.
2. Copies the firmware from **your** phone's partitions (read-only): see `extract-firmware.sh`.
3. Formats `userdata` as ext4 and copies the system onto it (≈ 2 GB used, ≈ 205 GB free on a 256 GB model).
4. Writes `boot-duo.img` into `boot_a` and `boot_b` **from Linux** (`dd`, verified by checksum):
   on this phone the bootloader refuses `fastboot flash` on the boot partitions ("Device Error").

Optional fallback system: the boot image also looks for a small ext4 volume in the unused space of `super`
(see `boot/auto-snippet.sh`), used when `userdata` has no system.

## Updating

The system lives on the phone: install updates with `apk upgrade`, or copy changed files over the USB link.
The boot image can be rebuilt with `boot/mkboot.sh` and written again with `dd`.

## Back to Android

```sh
./duo.sh restore-android /path/to/your/boot_b.img
```

Android starts on a fresh setup screen (its data partition is reformatted by Android).
