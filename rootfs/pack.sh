# applique l'overlay et refait root.tar.gz
R=$HOME/duo414/root
sudo cp -a /mnt/c/DuoPort/k414/overlay/. $R/
sudo chown 0:0 $R/root; sudo chmod 700 $R/root; sudo chown -R 0:0 $R/root/.config 2>/dev/null
sudo chmod 755 $R/usr/local/bin/*
sudo ln -sf /usr/share/zoneinfo/Europe/Paris $R/etc/localtime; echo Europe/Paris | sudo tee $R/etc/timezone > /dev/null
# pare-feu : le noyau Android n a pas nftables -> iptables « legacy » (NAT du partage de connexion)
for t in iptables iptables-restore iptables-save ip6tables ip6tables-restore ip6tables-save; do sudo ln -sf xtables-legacy-multi $R/usr/sbin/$t; done
sudo chmod 600 $R/etc/NetworkManager/system-connections/*.nmconnection 2>/dev/null; sudo chown root:root $R/etc/NetworkManager/system-connections/* 2>/dev/null
sudo chroot $R gtk-update-icon-cache -f -t /usr/share/icons/hicolor > /dev/null 2>&1 || true
cd $R && sudo tar czf $HOME/duo414/root.tar.gz . && ls -la $HOME/duo414/root.tar.gz
