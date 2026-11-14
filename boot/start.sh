# sur le Duo : prépare /r (déjà extrait) et lance la session tactile
touch /tmp/rester
killall drmfill sway phoc phosh duoheat 2>/dev/null; sleep 1
mkdir -p /dev/shm; mountpoint -q /dev/shm || mount -t tmpfs -o mode=1777 shm /dev/shm
[ -e /dev/ptmx ] || ln -s pts/ptmx /dev/ptmx
R=/r
for d in dev proc sys tmp; do mountpoint -q $R/$d || mount --rbind /$d $R/$d; done
mkdir -p $R/run; mountpoint -q $R/run || mount -t tmpfs tmpfs $R/run
# /run/duo partagé : duoheat (hors chroot) lit les fichiers « mute-A/B » qu'écrit duo-posture (dans le chroot)
mkdir -p /run/duo $R/run/duo; mountpoint -q $R/run/duo || mount --bind /run/duo $R/run/duo
# tous les firmwares (GPU, modem, Wi-Fi, son) sont dans /r/lib/firmware ; le noyau les cherche tantôt depuis la
# racine de démarrage, tantôt depuis /r : on rend /lib/firmware identique dans les deux (montage lié)
mkdir -p /lib/firmware; mountpoint -q /lib/firmware || mount --bind /r/lib/firmware /lib/firmware
echo -n /lib/firmware > /sys/module/firmware_class/parameters/path
setsid nohup /r/usr/local/bin/duoheat /dev/hidraw0 > /tmp/duoheat.log 2>&1 < /dev/null &
# vibreur : pont feedbackd -> da7280 (périphérique virtuel duo-vibra)
setsid nohup chroot $R /usr/local/bin/duo-vibra > /tmp/vibra.log 2>&1 < /dev/null &
sleep 1
setsid chroot $R /usr/local/bin/${SESSION:-duo-phosh} &
sleep 2   # (était 10 : démarrage accéléré le 13/11, duo-wifi attend lui-même le bus D-Bus)
# Wi-Fi : modem (rmtfs en lecture seule) + wlan.ko, en arrière-plan
setsid /r/usr/local/bin/duo-wifi > /dev/null 2>&1 &
# son : DSP audio puis modules dans l'ordre (script de Matrix, dans le système)
(sleep 1; setsid chroot $R bash /usr/local/sbin/duo-audio-up.sh > /tmp/audio.log 2>&1; chroot $R /usr/local/sbin/duo-micro >> /tmp/audio.log 2>&1; touch /tmp/duo-son-pret) &
# Bluetooth : attend le DSP audio, puis bluetoothd + duo-bt
setsid /r/usr/local/bin/duo-bt-start > /tmp/bt-start.log 2>&1 &
# accessoires USB : bascule automatique hôte / périphérique du port USB-C
setsid /r/usr/local/sbin/duo-usb-hote > /dev/null 2>&1 < /dev/null &
# capteurs (charnière, posture, accéléromètre) : DSP SLPI + hexagonrpcd, puis service de posture
setsid /r/usr/local/bin/duo-capteurs > /tmp/capteurs.log 2>&1 < /dev/null &
for p in duoheat phoc phosh squeekboard sway; do echo "$p=$(pidof $p)"; done
grep -iE 'error|critical|fail' /tmp/phosh.log | head -12
