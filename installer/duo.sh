#!/bin/sh
# Surface Duo 1 — postmarketOS-style Linux installer (run on a Linux PC; Windows: WSL2 + usbipd-win, see docs/INSTALL.md)
#
#   ./duo.sh live                       boot Linux from RAM — NOTHING is written to the phone
#   ./duo.sh install                    install Linux: replaces Android (userdata is formatted), boots on its own
#   ./duo.sh restore-android BOOT.img   write a stock boot image back (from your own backup) -> Android again
#
# Needs in the current directory (from the GitHub release): boot-duo.img, duo-rootfs.tar.gz
# Tools: fastboot, adb (optional), python3. The phone must have an UNLOCKED bootloader.
set -e
cd "$(dirname "$0")"
L="python3 duo_link.py"
pick() { [ -f "$1" ] && echo "$1" || echo "../$1"; }   # files next to the script or in the parent folder
BOOT=${BOOT:-$(pick boot-duo.img)}; ROOTFS=${ROOTFS:-$(pick duo-rootfs.tar.gz)}
run() { printf '%s\n' "$1" > /tmp/duo-cmd.sh; $L run /tmp/duo-cmd.sh "${2:-4}"; }

to_fastboot() {
  if fastboot devices | grep -q .; then return; fi
  if adb devices 2>/dev/null | grep -q 'device$'; then echo "-> rebooting to fastboot"; adb reboot bootloader; fi
  echo "Waiting for fastboot (or hold Volume Down + Power)..."
  until fastboot devices | grep -q .; do sleep 2; done
}

boot_linux() {
  to_fastboot
  fastboot boot "$BOOT"
  echo "Waiting for the USB link (the Duo shows up as a USB network adapter, 172.16.42.1)..."
  $L wait || { echo "No link: check that the PC gets an address on the new USB network interface."; exit 1; }
}

live() {
  boot_linux
  echo "Sending the system into RAM ($(du -h "$ROOTFS" | cut -f1))..."
  run 'touch /tmp/rester; mkdir -p /r; mountpoint -q /r || mount -t tmpfs -o size=3g tmpfs /r; cd /r; nc -l -p 5555 | tar xzf - ; echo EXTRACTED' 1 > /dev/null &
  sleep 2; $L stream "$ROOTFS" 5555; wait
  until run 'pidof tar >/dev/null && echo BUSY || echo DONE' 2 | grep -q DONE; do sleep 3; done
  echo "Copying firmware from the phone's own partitions (read-only)..."
  run 'sh /r/usr/share/duo-installer/extract-firmware.sh /r' 30 | tail -6
  echo "Starting the session..."
  run 'sh /start.sh > /tmp/start.log 2>&1 &' 3 > /dev/null
  echo "Linux is running from RAM. Reboot the phone to go back to Android."
}

install_linux() {
  echo "This REPLACES ANDROID: the userdata partition (all Android data) will be FORMATTED."
  printf "Type YES to continue: "; read ans; [ "$ans" = YES ] || exit 1
  live
  echo "Formatting userdata and copying the system..."
  run '
U=""; for b in /sys/class/block/sd*; do grep -q "^PARTNAME=userdata$" $b/uevent && U=${b##*/}; done
[ -e /dev/$U ] || mknod /dev/$U b $(tr ":" " " < /sys/class/block/$U/dev)
echo off > /r/root/.config/duo/veille
chroot /r /sbin/mke2fs -t ext4 -F -q -L duodata -O ^metadata_csum -E lazy_itable_init=1,lazy_journal_init=1 /dev/$U
mkdir -p /mnt/data; mount -t ext4 -o noatime /dev/$U /mnt/data
cd /r; for d in * .[!.]*; do case "$d" in proc|sys|dev|tmp|run|'"'"'.[!.]*'"'"') mkdir -p /mnt/data/$d; continue;; esac; cp -a /r/$d /mnt/data/; done
chmod 1777 /mnt/data/tmp; echo on > /mnt/data/root/.config/duo/veille; sync; umount /mnt/data; echo COPY_OK' 240 | tail -2
  echo "Writing the boot image to boot_a and boot_b (fastboot flash is refused on this phone, so from Linux)..."
  $L push "$BOOT" /tmp/boot-duo.img
  want=$(md5sum "$BOOT" | cut -c1-32); n=$(( $(stat -c %s "$BOOT") / 4096 ))
  run "for p in boot_a boot_b; do for b in /sys/class/block/sd*; do grep -q \"^PARTNAME=\$p\$\" \$b/uevent && d=\${b##*/}; done
[ -e /dev/\$d ] || mknod /dev/\$d b \$(tr ':' ' ' < /sys/class/block/\$d/dev)
dd if=/tmp/boot-duo.img of=/dev/\$d bs=1M conv=fsync 2>/dev/null; sync; echo 3 > /proc/sys/vm/drop_caches
echo \"\$p \$(dd if=/dev/\$d bs=4096 count=$n 2>/dev/null | md5sum | cut -c1-32)\"; done" 20 | grep -E 'boot_[ab]'
  echo "(expected $want)"
  echo "Done. Reboot the phone: it now starts Linux on its own."
}

restore_android() {
  [ -f "$1" ] || { echo "usage: $0 restore-android your-boot_b-backup.img"; exit 1; }
  boot_linux
  $L push "$1" /tmp/boot-stock.img
  run 'for p in boot_a boot_b; do for b in /sys/class/block/sd*; do grep -q "^PARTNAME=$p$" $b/uevent && d=${b##*/}; done
[ -e /dev/$d ] || mknod /dev/$d b $(tr ":" " " < /sys/class/block/$d/dev); dd if=/tmp/boot-stock.img of=/dev/$d bs=1M conv=fsync 2>/dev/null; done; sync; echo RESTORED' 20 | tail -1
  echo "Rebooting: Android will start on a fresh setup screen (choose \"Try again\", never \"Factory reset\", if asked)."
  run 'sync; (sleep 1; echo b > /proc/sysrq-trigger) &' 2 > /dev/null
}

case "$1" in
  live) live ;;
  install) install_linux ;;
  restore-android) restore_android "$2" ;;
  *) sed -n 2,9p "$0"; exit 1 ;;
esac
