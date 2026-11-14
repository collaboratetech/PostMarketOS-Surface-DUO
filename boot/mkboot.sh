# Image de démarrage AUTONOME : logo + montage du système installé (userdata) + session, sans le PC
set -e
MK=/mnt/c/DuoPort/mkbootimg; E=/mnt/c/DuoPort/export; W=$HOME/duo414/auto; rm -rf $W; mkdir -p $W/rd; cd $W
python3 $MK/unpack_bootimg.py --boot_img $E/boot-414-mire.img --out u > u.txt
CMD=$(sed -n "s/^command line args: //p" u.txt)
cd rd; gzip -dc ../u/ramdisk | cpio -id 2>/dev/null; cd ..
cp /mnt/c/DuoPort/tools/drmfill rd/bin/drmfill; chmod 755 rd/bin/drmfill
cp /mnt/c/DuoPort/k414/logo.raw rd/logo.raw; cp /mnt/c/DuoPort/tools/duo-dmlin rd/bin/duo-dmlin; chmod 755 rd/bin/duo-dmlin
sed 's/\r$//' /mnt/c/DuoPort/k414/start.sh > rd/start.sh
sed -i 's|/bin/drmfill 600 0 |/bin/drmfill 600 3 |' rd/init
# insère le démarrage autonome juste avant la boucle finale de l'init
python3 - rd/init /mnt/c/DuoPort/k414/auto-snippet.sh <<'P'
import sys
init, snip = sys.argv[1], sys.argv[2]
s = open(init, encoding='utf-8', errors='surrogateescape').read()
k = s.rindex('while true; do sleep 3600; done')
s = s[:k] + open(snip, encoding='utf-8').read().replace('\r', '') + s[k:]
open(init, 'w', encoding='utf-8', errors='surrogateescape').write(s)
P
grep -n "drmfill 600\|démarrage autonome\|sleep 3600" rd/init | head
(cd rd && find . | cpio -o -H newc 2>/dev/null | gzip -9) > ramdisk.gz
python3 $MK/mkbootimg.py --header_version 2 --pagesize 4096 --base 0x0 --kernel_offset 0x8000 --ramdisk_offset 0x1000000 \
  --second_offset 0x00f00000 --tags_offset 0x100 --dtb_offset 0x01f00000 --os_version 11.0.0 --os_patch_level 2023-08 \
  --kernel u/kernel --ramdisk ramdisk.gz --dtb u/dtb --cmdline "$CMD" --output $E/boot-duo-autonome.img
ls -la $E/boot-duo-autonome.img
