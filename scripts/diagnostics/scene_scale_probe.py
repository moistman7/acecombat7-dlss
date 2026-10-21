"""Configure an unreleased scene-scale experiment; this does not install DLSS.

Close AC7 first. Backups and exact rollback are provided for both changed INIs.
Use with the native-TAA pipeline probe to measure rendering dimensions.
"""
import argparse
from datetime import datetime
import hashlib
import json
from pathlib import Path
import re
import subprocess

PERCENTAGES = {"Native": "100", "Quality": "66.666667", "Balanced": "58", "Performance": "50"}


def encode_ini(raw, section, key, value):
    if raw.startswith(b"\xff\xfe"):
        encoding = "utf-16"
    elif raw.startswith(b"\xef\xbb\xbf"):
        encoding = "utf-8-sig"
    else:
        encoding = "utf-8"
    text = raw.decode(encoding)
    newline = "\r\n" if "\r\n" in text else "\n"
    pattern = re.compile(r"^\[" + re.escape(section) + r"\][^\r\n]*(?:\r?\n|$).*?(?=^\[|\Z)", re.M | re.S | re.I)
    sections = list(pattern.finditer(text))
    if len(sections) > 1:
        raise ValueError("Duplicate target section; inspect manually before probing")
    if sections:
        match = sections[0]
        body = match.group()
        keys = re.compile(r"^" + re.escape(key) + r"\s*=[^\r\n]*", re.M | re.I)
        if len(keys.findall(body)) > 1:
            raise ValueError("Duplicate target key; inspect manually before probing")
        if keys.search(body):
            body = keys.sub(lambda _: key + "=" + value, body)
        else:
            body = body.rstrip("\r\n") + newline + key + "=" + value + newline
        text = text[:match.start()] + body + text[match.end():]
    else:
        text = text.rstrip("\r\n") + newline * 2 + "[" + section + "]" + newline + key + "=" + value + newline
    return text.encode(encoding)


def ensure_closed():
    result = subprocess.run(["tasklist", "/FI", "IMAGENAME eq Ace7Game.exe", "/FO", "CSV", "/NH"],
                            capture_output=True, check=True)
    if b'"ace7game.exe"' in result.stdout.lower():
        raise RuntimeError("Close Ace Combat 7 before changing or restoring the probe")


def install(config, mode):
    ensure_closed()
    config = config.resolve(strict=True)
    if not config.is_dir():
        raise ValueError("Config directory is not a directory")
    planned = []
    for name, section, key, value in [
        ("Game.ini", "GraphicsWindowsOption.DrawScale@0", "DrawScale", PERCENTAGES[mode]),
        ("GameUserSettings.ini", "WindowsGraphicsOptions", "DrawScale", "0"),
    ]:
        path = config / name
        existed = path.exists()
        original = path.read_bytes() if existed else b""
        changed = encode_ini(original, section, key, value)
        planned.append((path, existed, original, changed))
    backup = config / ("AC7-SceneScale-Probe-" + datetime.now().strftime("%Y%m%d-%H%M%S-%f"))
    backup.mkdir()
    records = []
    for path, existed, original, changed in planned:
        (backup / path.name).write_bytes(original)
        records.append({"name": path.name, "existed": existed,
                        "original_sha256": hashlib.sha256(original).hexdigest(),
                        "installed_sha256": hashlib.sha256(changed).hexdigest()})
    manifest = {"config": str(config), "mode": mode, "percentage": PERCENTAGES[mode], "files": records}
    (backup / "manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    try:
        for path, _, _, changed in planned:
            path.write_bytes(changed)
    except Exception:
        for path, existed, original, _ in planned:
            if existed:
                path.write_bytes(original)
            elif path.exists():
                path.unlink()
        raise
    print(json.dumps({"backup": str(backup), "mode": mode, "percentage": PERCENTAGES[mode],
                      "status": "Configured experimental scene scale; verify live cvar and render targets. This does not implement a DLSS mode."}, indent=2))
    return backup


def restore(backup):
    ensure_closed()
    manifest = json.loads((backup / "manifest.json").read_text(encoding="utf-8"))
    config = Path(manifest["config"]).resolve(strict=True)
    records = manifest["files"]
    if {record["name"] for record in records} != {"Game.ini", "GameUserSettings.ini"} or len(records) != 2:
        raise ValueError("Invalid probe manifest file list")
    for record in records:
        current = (config / record["name"]).read_bytes()
        original = (backup / record["name"]).read_bytes()
        if hashlib.sha256(current).hexdigest() != record["installed_sha256"]:
            raise RuntimeError("Config changed since probe installation; refusing to overwrite later edits: " + record["name"])
        if hashlib.sha256(original).hexdigest() != record["original_sha256"]:
            raise RuntimeError("Backup checksum mismatch: " + record["name"])
    for record in records:
        target = config / record["name"]
        if record["existed"]:
            target.write_bytes((backup / record["name"]).read_bytes())
        else:
            target.unlink()
    print("Restored original scene-scale configs byte for byte; backup retained.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    actions = parser.add_mutually_exclusive_group(required=True)
    actions.add_argument("--config-directory", type=Path)
    actions.add_argument("--restore", type=Path, metavar="BACKUP_DIRECTORY")
    parser.add_argument("--mode", choices=PERCENTAGES)
    args = parser.parse_args()
    if args.restore:
        if args.mode:
            parser.error("--mode cannot accompany --restore")
        restore(args.restore)
    else:
        if not args.mode:
            parser.error("--mode is required with --config-directory")
        install(args.config_directory, args.mode)
