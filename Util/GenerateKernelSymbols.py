#!/usr/bin/env python3

import shutil
import subprocess
import sys


def escape_str(value):
    return value.replace("\\", "\\\\").replace('"', '\\"')


def normalize_path(path):
    return path.replace("\\", "/")


def make_path_relative(path, project_root):
    normalized_path = normalize_path(path)
    normalized_root = normalize_path(project_root).rstrip("/")
    if normalized_path.startswith(normalized_root + "/"):
        return normalized_path[len(normalized_root) + 1 :]
    return normalized_path


def parse_args():
    args = sys.argv[1:]
    with_source = False
    project_root = None
    if args and args[0] == "--with-source":
        with_source = True
        args = args[1:]
    while len(args) >= 2 and args[0] == "--project-root":
        project_root = args[1]
        args = args[2:]
    if len(args) != 2:
        return None
    return with_source, project_root, args[0], args[1]


def resolve_source_locations(addr2line_tool, kernel_elf, symbols):
    result = {}
    if not symbols:
        return result

    addresses = [f"0x{address:x}" for address, _ in symbols]
    try:
        proc = subprocess.run(
            [addr2line_tool, "-e", kernel_elf, *addresses],
            check=True,
            text=True,
            capture_output=True,
        )
    except subprocess.CalledProcessError:
        return result

    lines = proc.stdout.splitlines()
    for idx, line in enumerate(lines):
        if idx >= len(symbols):
            break
        text = line.strip()
        if text in {"??:0", "??:?", "??"}:
            continue
        if ":" not in text:
            continue
        file_part, line_part = text.rsplit(":", 1)
        try:
            line_no = int(line_part)
        except ValueError:
            continue
        if line_no <= 0 or not file_part or file_part == "??":
            continue
        result[symbols[idx][0]] = (file_part, line_no)

    return result


def main():
    parsed = parse_args()
    if not parsed:
        return 1

    with_source, project_root, kernel_elf, out_cpp = parsed

    nm_tool = shutil.which("llvm-nm") or shutil.which("nm")
    if not nm_tool:
        return 2

    try:
        proc = subprocess.run(
            [nm_tool, "-n", "--defined-only", "--demangle", kernel_elf],
            check=True,
            text=True,
            capture_output=True,
        )
    except subprocess.CalledProcessError as exc:
        print(exc.stderr, file=sys.stderr)
        return exc.returncode

    symbols = []
    for line in proc.stdout.splitlines():
        parts = line.strip().split(maxsplit=2)
        if len(parts) != 3:
            continue
        addr_text, sym_type, name = parts
        if sym_type.lower() not in {"t", "w"}:
            continue
        if name.startswith("."):
            continue
        try:
            address = int(addr_text, 16)
        except ValueError:
            continue
        symbols.append((address, name))

    source_locations = {}
    if with_source:
        addr2line_tool = shutil.which("llvm-addr2line") or shutil.which("addr2line")
        if addr2line_tool:
            source_locations = resolve_source_locations(addr2line_tool, kernel_elf, symbols)

    with open(out_cpp, "w", encoding="ascii") as f:
        f.write("#include <stdint.h>\n")
        f.write("#include <stddef.h>\n\n")
        f.write("namespace Katline {\n")
        f.write("struct KernelSymbol {\n")
        f.write("    uint64_t address;\n")
        f.write("    char const *name;\n")
        f.write("    char const *file;\n")
        f.write("    uint32_t line;\n")
        f.write("};\n\n")
        f.write("extern \"C\" const KernelSymbol g_kernel_symbols[] = {\n")
        for address, name in symbols:
            file_name, line_no = source_locations.get(address, ("", 0))
            if file_name and project_root:
                file_name = make_path_relative(file_name, project_root)
            f.write(
                f'    {{ 0x{address:016x}ull, "{escape_str(name)}", "{escape_str(file_name)}", {line_no}u }},\n'
            )
        f.write("};\n\n")
        f.write(
            "extern \"C\" const size_t g_kernel_symbol_count = sizeof(g_kernel_symbols) / sizeof(g_kernel_symbols[0]);\n"
        )
        f.write("}\n")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
