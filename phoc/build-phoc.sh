# phoc : + oubli des doigts fantomes
set -e
B=$HOME/duo414/build; S=$B/src/phoc-0.47.0
sudo python3 /mnt/c/DuoPort/k414/phoc/fantomes.py $S/src
for d in dev proc sys; do mountpoint -q $B/$d || sudo mount --bind /$d $B/$d; done
sudo chroot $B /bin/sh -c 'cd /src/phoc-0.47.0 && ninja -C _b -j12 > /tmp/phoc-ninja.log 2>&1 || { grep -E "error" /tmp/phoc-ninja.log | head; tail -5 /tmp/phoc-ninja.log; exit 1; }'
for d in sys proc dev; do sudo umount $B/$d 2>/dev/null || true; done
sudo cp $S/_b/src/phoc /mnt/c/DuoPort/k414/phoc/phoc && cp /mnt/c/DuoPort/k414/phoc/phoc /mnt/c/DuoPort/k414/overlay/usr/bin/phoc && md5sum /mnt/c/DuoPort/k414/phoc/phoc && echo PHOC_OK
