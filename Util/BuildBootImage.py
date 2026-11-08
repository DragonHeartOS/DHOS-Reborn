import argparse
import io
import pathlib
import tarfile


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("archive", type=pathlib.Path)
    parser.add_argument("entries", nargs="+")
    args = parser.parse_args()

    entries = []
    names = set()
    for entry in args.entries:
        target, separator, path = entry.partition("=")
        if not separator or not target:
            parser.error(f"invalid boot image entry: {entry}")

        executable = pathlib.Path(path)
        name = executable.name
        if name in names:
            parser.error(f"duplicate boot image member name: {name}")
        if not executable.is_file():
            parser.error(f"boot image target {target} does not exist: {executable}")

        names.add(name)
        entries.append((executable, name))

    args.archive.parent.mkdir(parents=True, exist_ok=True)
    order = "".join(f"{name}\n" for _, name in entries)

    with tarfile.open(args.archive, "w", format=tarfile.USTAR_FORMAT) as archive:
        for executable, name in entries:
            archive.add(executable, arcname=name, recursive=False)
        info = tarfile.TarInfo("load-order.txt")
        data = order.encode("utf-8")
        info.size = len(data)
        info.mode = 0o644
        info.mtime = 0
        archive.addfile(info, io.BytesIO(data))


if __name__ == "__main__":
    main()
