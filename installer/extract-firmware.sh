#!/bin/sh
# Runs ON THE DUO (initramfs shell, system unpacked in $R). Copies the proprietary firmware and device configuration
# from the phone's OWN partitions into the Linux system. Everything is mounted READ-ONLY; nothing is written to them.
#   modem<slot>     (FAT)  /image/*            -> /lib/firmware          (DSPs, modem, Wi-Fi, IPA)
#   bluetooth<slot> (FAT)  /image/*            -> /lib/firmware/qca      (Bluetooth)
#   vendor<slot> (in super, ext4) /firmware/*  -> /lib/firmware          (GPU a6xx, IPA)
#                 /etc/wifi/WCNSS_qcom_cfg.ini -> /lib/firmware/wlan/qca_cld
#                 /etc/sensors/*               -> /usr/share/duo/hexagon/sensors   (sensor configuration)
#   persist (ext4) /sensors/registry/*         -> /usr/share/duo/hexagon/sensors/registry (sensor calibration)
R=${1:-/r}
set -e
S=$(tr ' ' '\n' < /proc/cmdline | sed -n 's/^androidboot.slot_suffix=//p'); S=${S:-_b}
part() { for b in /sys/class/block/sd*; do grep -q "^PARTNAME=$1$" $b/uevent 2>/dev/null && { n=${b##*/}; [ -e /dev/$n ] || mknod /dev/$n b $(tr ':' ' ' < $b/dev); echo /dev/$n; return; }; done; }
F=$R/lib/firmware; H=$R/usr/share/duo/hexagon/sensors
mkdir -p $F/qca $F/wlan/qca_cld $H/config $H/registry /mnt/fw
echo "slot $S"
for p in modem bluetooth; do
  d=$(part $p$S); [ -n "$d" ] || { echo "missing $p$S"; exit 1; }
  mount -t vfat -o ro $d /mnt/fw
  if [ $p = modem ]; then cp -a /mnt/fw/image/. $F/; else cp -a /mnt/fw/image/. $F/qca/; fi
  umount /mnt/fw; echo "$p$S: ok"
done
SUP=$(part super); [ -n "$SUP" ] || { echo "missing super"; exit 1; }
idx=0; [ "$S" = _b ] && idx=1
mkdir -p $R/tmp; dd if=$SUP of=$R/tmp/super-meta.bin bs=1M count=1 2>/dev/null     # LP metadata only (1 MiB)
SEG=$(chroot $R python3 /usr/share/duo-installer/lp_extents.py /tmp/super-meta.bin vendor$S $idx)
/bin/duo-dmlin duovendor $SUP $SEG
mount -t ext4 -o ro /dev/mapper/duovendor /mnt/fw
cp -a /mnt/fw/firmware/. $F/
cp /mnt/fw/etc/wifi/WCNSS_qcom_cfg.ini $F/wlan/qca_cld/
cp /mnt/fw/etc/sensors/config/*.json $H/config/
cp /mnt/fw/etc/sensors/sns_reg_config $H/sns_reg.conf
umount /mnt/fw; echo "vendor$S: ok"
d=$(part persist); mount -t ext4 -o ro $d /mnt/fw
cp -a /mnt/fw/sensors/registry/. $H/registry/ 2>/dev/null || true
umount /mnt/fw; echo "persist: ok"
echo "FIRMWARE_OK $(ls $F | wc -l) files"
