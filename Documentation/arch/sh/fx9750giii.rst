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
* fx9750giii kernel configuration

Configuration
=============

The calculator configuration is:

::

   arch/sh/configs/fx9750giii_defconfig

TuxForge
========

This repository contains the Linux kernel port only.

TuxForge is the complete Linux system/distribution built around this
kernel port. The first TuxForge release for the fx-9750GIII will be
TuxForge v0.1.

Author
======

Artem Novak
