#!/usr/bin/env python3
"""Talk to the Duo's boot shell over the USB network link (172.16.42.1, port 2323).

  duo_link.py wait                     wait until the Duo answers
  duo_link.py run SCRIPT [SECONDS]     run a local shell script on the Duo, print its output
  duo_link.py push LOCAL REMOTE        copy a file in verified 4 MiB chunks (the USB gadget may drop on big transfers)
  duo_link.py stream LOCAL PORT        send a file to a waiting `nc -l -p PORT` on the Duo
"""
import hashlib
import os
import socket
import sys
import time

HOST = os.environ.get('DUO_HOST', '172.16.42.1')


def shell(script, wait=4.0):
    s = socket.create_connection((HOST, 2323), timeout=10)
    time.sleep(0.5)
    s.sendall(b"cat > /tmp/r.sh <<'FIN_R'\n" + script.encode() + b"\nFIN_R\nsh /tmp/r.sh 2>&1; exit\n")
    time.sleep(wait)
    s.settimeout(3)
    out = b''
    try:
        while True:
            c = s.recv(65536)
            if not c:
                break
            out += c
    except OSError:
        pass
    t = out.decode('utf-8', 'replace')
    return t[t.find('\n', t.rfind('FIN_R')) + 1:] if 'FIN_R' in t else t


def wait(timeout=300):
    end = time.time() + timeout
    while time.time() < end:
        try:
            socket.create_connection((HOST, 2323), timeout=3).close()
            return True
        except OSError:
            time.sleep(3)
    return False


def send_raw(data, remote):
    for attempt in range(8):
        try:
            wait(60)
            s = socket.create_connection((HOST, 2323), timeout=10)
            time.sleep(0.4)
            s.sendall(f"nc -l -p 5558 > {remote}\n".encode())
            time.sleep(1.2)
            d = socket.create_connection((HOST, 5558), timeout=10)
            d.settimeout(60)
            d.sendall(data)
            d.shutdown(socket.SHUT_WR)
            d.close()
            time.sleep(0.5)
            s.close()
            if hashlib.md5(data).hexdigest() in shell(f"md5sum {remote}", 2):
                return
        except OSError:
            time.sleep(3)
    sys.exit(f'transfer failed: {remote}')


def push(local, remote):
    shell('mkdir -p /tmp/parts; rm -f /tmp/parts/*', 2)
    with open(local, 'rb') as f:
        i = 0
        while True:
            chunk = f.read(4 << 20)
            if not chunk:
                break
            send_raw(chunk, f'/tmp/parts/p.{i:04d}')
            i += 1
            print(f'\r{i * 4} MiB', end='', flush=True)
    print()
    out = shell(f'cat /tmp/parts/p.* > {remote} && rm -rf /tmp/parts && md5sum {remote}', 8)
    want = hashlib.md5(open(local, 'rb').read()).hexdigest()
    if want not in out:
        sys.exit('final checksum mismatch')
    print(f'{remote}: OK ({want})')


def stream(local, port):
    d = socket.create_connection((HOST, int(port)), timeout=15)
    d.settimeout(300)
    with open(local, 'rb') as f:
        while True:
            b = f.read(1 << 20)
            if not b:
                break
            d.sendall(b)
    d.shutdown(socket.SHUT_WR)
    d.close()


if __name__ == '__main__':
    cmd = sys.argv[1]
    if cmd == 'wait':
        sys.exit(0 if wait() else 1)
    elif cmd == 'run':
        print(shell(open(sys.argv[2]).read(), float(sys.argv[3]) if len(sys.argv) > 3 else 4))
    elif cmd == 'push':
        push(sys.argv[2], sys.argv[3])
    elif cmd == 'stream':
        stream(sys.argv[2], sys.argv[3])
