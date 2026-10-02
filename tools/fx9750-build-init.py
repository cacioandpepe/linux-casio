#!/usr/bin/env python3
"""Build a freestanding SH4AL big-endian PID 1 and a minimal newc initramfs."""
from pathlib import Path
import argparse, os, stat, struct, subprocess, tempfile


def entry(name, mode, contents=b"", inode=1, major=0, minor=0):
    encoded = name.encode() + b"\0"
    values = [inode, mode, 0, 0, 1, 0, len(contents), 0, 0,
              major, minor, len(encoded), 0]
    header = b"070701" + b"".join(f"{x:08x}".encode() for x in values)
    out = header + encoded
    out += bytes((-len(out)) % 4)
    out += contents
    return out + bytes((-len(contents)) % 4)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--cc", required=True)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parent.parent
    board = root / "arch/sh/boards/mach-fx9750giii"
    cpu = None
    for flag in ("-m4a-nofpu", "-m4al", "-m4a"):
        probe = subprocess.run([args.cc, "-mb", flag, "-x", "c", "-c",
            "-o", os.devnull, "-"], input="int probe(void) { return 0; }\n",
            text=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        if probe.returncode == 0:
            cpu = flag
            break
    if cpu is None:
        raise SystemExit("Compiler supports none of the SH4A CPU flags")
    print("PID 1 CPU flag: " + cpu, flush=True)
    with tempfile.TemporaryDirectory(prefix="fx9750-init-") as temp:
        elf = Path(temp) / "init.elf"
        obj = Path(temp) / "init.o"
        subprocess.run([args.cc, "-mb", cpu, "-mno-fdpic", "-Os",
            "-ffreestanding", "-fno-builtin", "-fno-stack-protector",
            "-fno-pic", "-fno-pie", "-fno-asynchronous-unwind-tables",
            "-c",
            "-I" + str(root / "arch/sh/include/uapi"),
            "-I" + str(args.build_dir / "arch/sh/include/generated/uapi"),
            "-o", str(obj), str(board / "bootinit.c")], check=True)
        ld = subprocess.check_output([args.cc, "-print-prog-name=ld"], text=True).strip()
        subprocess.run([ld, "-EB", "--build-id=none", "-s",
            "-T", str(board / "bootinit.lds"), "-o", str(elf), str(obj)], check=True)
        data = elf.read_bytes()
    if data[:7] != b"\x7fELF\x01\x02\x01" or struct.unpack_from(">H", data, 18)[0] != 42:
        raise SystemExit("PID 1 is not a big-endian ELF32 SH executable")
    kind = struct.unpack_from(">H", data, 16)[0]
    entrypoint, phoff = struct.unpack_from(">II", data, 24)
    phnum = struct.unpack_from(">H", data, 44)[0]
    valid = False
    for i in range(phnum):
        kind_ph, off, va, pa, file_size, memory_size, flags, align = struct.unpack_from(">8I", data, phoff + 32*i)
        if kind_ph == 3: raise SystemExit("PID 1 unexpectedly needs an ELF interpreter")
        if kind_ph == 1 and va <= entrypoint < va + memory_size and flags & 1:
            valid = True
    if kind != 2 or not valid or len(data) > 4096:
        raise SystemExit("Invalid or oversized PID 1 image")
    archive = b"".join((
        entry("dev", stat.S_IFDIR | 0o755, inode=1),
        entry("dev/kmsg", stat.S_IFCHR | 0o600, inode=2, major=1, minor=11),
        entry("init", stat.S_IFREG | 0o755, data, inode=3),
        entry("TRAILER!!!", 0, inode=4)))
    archive += bytes((-len(archive)) % 512)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    if not args.output.exists() or args.output.read_bytes() != archive:
        temporary = args.output.with_suffix(".tmp")
        temporary.write_bytes(archive)
        temporary.replace(args.output)
    print(f"Built native PID 1 ({len(data)} bytes); initramfs {len(archive)} bytes", flush=True)


if __name__ == "__main__": main()
