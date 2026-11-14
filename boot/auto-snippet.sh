# --- démarrage autonome (13/11/2026) : système sur la partition userdata (228 Go, ext4 « duodata ») en priorité,
# sinon sur le système de secours dans l'espace libre de « super » (volume dm « duolinux », 5 Go). Sans système installé, rien ne change : le PC peut envoyer le système par USB.
(
  for i in $(seq 30); do S=""; U=""; for b in /sys/class/block/sd*; do
      grep -q "^PARTNAME=super$" $b/uevent 2>/dev/null && S=${b##*/}; grep -q "^PARTNAME=userdata$" $b/uevent 2>/dev/null && U=${b##*/}; done
    [ -n "$S" ] && break; sleep 1; done
  mkdir -p /r
  ok=0
  if [ -n "$U" ]; then
    [ -e /dev/$U ] || mknod /dev/$U b $(tr ':' ' ' < /sys/class/block/$U/dev)
    mount -t ext4 -o noatime /dev/$U /r 2>/tmp/auto.log && [ -x /r/usr/local/bin/duo-phosh ] && ok=1 || umount /r 2>/dev/null
  fi
  if [ $ok = 0 ] && [ -n "$S" ] && [ "$(cat /sys/class/block/$S/size)" = 29360128 ]; then
    [ -e /dev/$S ] || mknod /dev/$S b $(tr ':' ' ' < /sys/class/block/$S/dev)
    /bin/duo-dmlin duolinux /dev/$S 2314480:5574000 10257280:1065120 12735312:1945776 27414352:1945776 >> /tmp/auto.log 2>&1
    mount -t ext4 -o noatime /dev/mapper/duolinux /r 2>>/tmp/auto.log && [ -x /r/usr/local/bin/duo-phosh ] && ok=2 || umount /r 2>/dev/null
    [ $ok = 2 ] && echo "duo414: système de SECOURS (super) utilisé" > /dev/kmsg
  fi
  if [ $ok != 0 ]; then
    touch /tmp/rester
    echo "duo414: système installé trouvé -> démarrage autonome" > /dev/kmsg
    sh /start.sh >> /tmp/auto.log 2>&1
  else
    echo "duo414: pas de système installé -> attente du PC" > /dev/kmsg
  fi
) &
