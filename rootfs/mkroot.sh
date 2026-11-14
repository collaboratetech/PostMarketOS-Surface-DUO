# rootfs Alpine aarch64 pour le Duo (chargé en RAM par USB) : sway + foot, rendu logiciel (pixman)
set -e
R=$HOME/duo414/root; APK=$HOME/.local/var/pmbootstrap/apk.static
sudo rm -rf $R; mkdir -p $R/etc/apk
echo "https://dl-cdn.alpinelinux.org/alpine/v3.22/main
https://dl-cdn.alpinelinux.org/alpine/v3.22/community" | sudo tee $R/etc/apk/repositories >/dev/null
sudo $APK --arch aarch64 --root $R --initdb --allow-untrusted --no-progress -U add \
  alpine-base busybox-extras sway swaybg foot font-dejavu seatd xkeyboard-config libinput eudev \
  mesa-dri-gallium evtest kbd-bkeymaps 2>&1 | tail -3
sudo cp /etc/resolv.conf $R/etc/
du -sh $R
cd $R && sudo tar czf $HOME/duo414/root.tar.gz . && ls -la $HOME/duo414/root.tar.gz
