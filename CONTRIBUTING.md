# Contributing

This is an open-source community port: contributions are very welcome — bug reports, fixes, new features,
documentation, translations, or just testing on your own Surface Duo 1.

- **Bugs / ideas:** open an [issue](../../issues). Please include what you did, what happened, and if possible
  `dmesg`, `/tmp/*.log` and the output of `/tmp/posture.log` / `/tmp/duo-donnees.log` (whatever relates to the problem).
- **Code:** fork, make your change on a branch, open a pull request. Keep changes small and explain *why*.
  Scripts are POSIX `sh` or Python 3; C tools build statically with `aarch64-linux-gnu-gcc -static`.
- **Never commit** proprietary firmware, files extracted from a phone (`vendor`, `persist`, `modem`…),
  Wi-Fi passwords, phone numbers or other personal data.
- **Testing on real hardware** is the most useful thing you can do: the Duo has quirks (touch at the screen edges,
  USB link drops, bootloader refusing `fastboot flash`) that only show up on a real device.

By contributing you agree that your code is released under **GPL-3.0-or-later**.

Comments in existing scripts are in French; English or French are both fine for new code and discussions.
