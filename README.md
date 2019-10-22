QEMU 3Dfx Glide Pass-through
Copyright (c) 2018-2019
by KJ Liew <liewkj@yahoo.com>

Content
-------
qemu-0/hw/3dfx          - Overlay for QEMU source tree to add 3Dfx Glide pass-through device model
wrapper                 - wrappers for supported guest OS/environment. (DOS/Windows/DJGPP/Linux)
00-qemu410-3dfx.patch   - Patch for QEMU version 4.xx
00-qemu311-3dfx.patch   - Patch for QEMU version 3.xx
99-3dfx.patch           - Patch for QEMU version 1.6.x to 2.12.1 (deprecated)
99-oldqemu.patch        - Addition patch for QEMU version <= 2.10 (deprecated)

Simple guide to apply the patch:

$ mkdir myqemu && cd myqemu
$ git clone <this>
$ cd qemu-3dfx
$ wget https://download.qemu.org/qemu-4.1.0.tar.xz
$ tar xf qemu-4.1.0.tar.xz
$ cd qemu-4.1.0
$ rsync -r ../qemu-0/hw ./
$ patch -p0 -i ../qemu-0/00-qemu410-3dfx.patch
$ mkdir ../build && cd ../buid
$ ../qemu-4.1.0/configure && make


