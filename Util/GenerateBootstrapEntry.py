#!/usr/bin/env python3

import shutil
import subprocess
import sys


def parse_args():
    args = sys.argv[1:]
    if len(args) != 2:
        return None
    return args[0], args[1]


def main():
    parsed = parse_args()
    if not parsed:
        return 1

    out_h, bootstrap_elf = parsed

    nm_tool = shutil.which("llvm-nm") or shutil.which("nm")
    if not nm_tool:
        return 2

    try:
        proc = subprocess.run(
            [nm_tool, "-n", "--defined-only", bootstrap_elf],
            check=True,
            text=True,
            capture_output=True,
        )
    except subprocess.CalledProcessError as exc:
        print(exc.stderr, file=sys.stderr)
        return exc.returncode

    start_addr = None
    for line in proc.stdout.splitlines():
        parts = line.strip().split(maxsplit=2)
        if len(parts) != 3:
            continue
        addr_text, sym_type, name = parts
        if name != "_start":
            continue
        if sym_type.lower() not in {"t", "w"}:
            continue
        try:
            start_addr = int(addr_text, 16)
        except ValueError:
            continue
        break

    if start_addr is None:
        print("failed to find _start in Bootstrap ELF", file=sys.stderr)
        return 3

    with open(out_h, "w", encoding="ascii") as f:
        f.write("#pragma once\n")
        f.write("inline constexpr unsigned long long KATLINE_BOOTSTRAP_ENTRY_OFFSET = ")
        f.write(f"0x{start_addr:x}ull;\n")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
