=========================
Casio fx-9750GIII Linux
=========================

Linux kernel support for the Casio fx-9750GIII and SH7305,
developed by Artem Novak.

Status
======

Available for development.

The Linux kernel port source is public and under active development.
Native Linux boot on the fx-9750GIII is currently being brought up.

Current support includes:

* SH7305 CPU identification
* SH7305 clock/CPG support
* interrupt controller support
* TMU timer support
* fx-9750GIII machine support
* P1/P3 split kernel memory layout
* boot-time MMU/TLB support
* fx9750giii configuration for 512 KiB of RAM with SLUB_TINY
* allocation-free polled LCD printk console from RAM entry through panic
* immutable map validation and dynamic reservation of used ROM repair pages
* an 8-16 KiB allocator emergency reserve for this 512 KiB platform
* a freestanding native big-endian PID 1 embedded in a minimal initramfs

Configuration
=============

The calculator configuration is:

::

   arch/sh/configs/fx9750giii_defconfig

=

This repository contains the Linux kernel port only.

TuxForge is the complete Linux system/distribution built around this
kernel port. The first TuxForge release for the fx-9750GIII will be
TuxForge v0.1.
Boot bring-up
=============

Use ``sh4aeb-buildroot-linux-gnu-gcc`` (or its ``.br_real`` executable) for the
kernel and native PID 1. Configure ``fx9750giii_defconfig``, run ``make prepare``,
and generate the archive with::

   python3 tools/fx9750-build-init.py --cc sh4aeb-buildroot-linux-gnu-gcc \
       --build-dir . --output arch/sh/boards/mach-fx9750giii/bootinit.cpio

For an out-of-tree build, give the builder the output directory and set
``CONFIG_INITRAMFS_SOURCE`` to the absolute generated archive path. Regenerate
both BIN files and the add-in from the same linked kernel.

The loader must use boot ABI 5. The reserved first word in the boot descriptor
selects LCD protocol 1 or 2, as detected by gint. The console inherits the
loader's bus and display configuration and writes directly to the controller.

A hardware boot is confirmed when ``Linux PID 1 started`` appears and repeated
``Timer/syscalls OK`` messages follow. The bundled PID 1 proves native userspace
execution and timer wakeups; it does not provide a shell or keyboard support.
A build or source review alone is not proof of a successful calculator boot.
Author
======

Artem Novak
