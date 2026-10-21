"""Read AC7's effective resolution scale and DrawScale table. No process writes.

Windows, Python 3, no third-party packages. Run while AC7 is open:
    python read_scene_scale.py GAME_PID
Offsets are restricted to the explicitly verified executable hash below.
"""
import argparse
import ctypes as ct
from ctypes import wintypes as wt
import hashlib
import json
from pathlib import Path
import struct

EXE_SHA256 = "c7da97f5f8a807d4f1264adbb074146fcffe9bdc2ffa98791b822cd28e558f4f"


class ModuleEntry(ct.Structure):
    _fields_ = [("size", wt.DWORD), ("module_id", wt.DWORD),
                ("pid", wt.DWORD), ("global_usage", wt.DWORD),
                ("process_usage", wt.DWORD), ("base", ct.c_void_p),
                ("image_size", wt.DWORD), ("module", wt.HMODULE),
                ("name", wt.WCHAR * 256), ("path", wt.WCHAR * 260)]


def inspect(pid):
    kernel = ct.WinDLL("kernel32", use_last_error=True)
    kernel.CreateToolhelp32Snapshot.argtypes = [wt.DWORD, wt.DWORD]
    kernel.CreateToolhelp32Snapshot.restype = wt.HANDLE
    kernel.Module32FirstW.argtypes = [wt.HANDLE, ct.POINTER(ModuleEntry)]
    kernel.Module32NextW.argtypes = [wt.HANDLE, ct.POINTER(ModuleEntry)]
    kernel.OpenProcess.argtypes = [wt.DWORD, wt.BOOL, wt.DWORD]
    kernel.OpenProcess.restype = wt.HANDLE
    kernel.ReadProcessMemory.argtypes = [wt.HANDLE, ct.c_void_p,
                                        ct.c_void_p, ct.c_size_t,
                                        ct.POINTER(ct.c_size_t)]
    kernel.CloseHandle.argtypes = [wt.HANDLE]
    snapshot = kernel.CreateToolhelp32Snapshot(0x18, pid)
    if snapshot == ct.c_void_p(-1).value:
        raise ct.WinError(ct.get_last_error())
    module = ModuleEntry()
    module.size = ct.sizeof(module)
    try:
        valid = kernel.Module32FirstW(snapshot, ct.byref(module))
        while valid and module.name.lower() != "ace7game.exe":
            valid = kernel.Module32NextW(snapshot, ct.byref(module))
        if not valid:
            raise RuntimeError("Ace7Game.exe not found in selected process")
        base, path = module.base, Path(module.path)
    finally:
        kernel.CloseHandle(snapshot)
    if hashlib.sha256(path.read_bytes()).hexdigest() != EXE_SHA256:
        raise RuntimeError("Unsupported executable hash; refusing fixed-offset reads")
    process = kernel.OpenProcess(0x410, False, pid)  # QUERY_INFORMATION | VM_READ
    if not process:
        raise ct.WinError(ct.get_last_error())
    try:
        def read(address, size):
            if not 0x10000 <= address < 0x7FFFFFFFFFFF or not 0 < size <= 1048576:
                raise ValueError("Invalid read bounds")
            buffer = ct.create_string_buffer(size)
            received = ct.c_size_t()
            if not kernel.ReadProcessMemory(process, address, buffer, size, ct.byref(received)):
                raise ct.WinError(ct.get_last_error())
            if received.value != size:
                raise RuntimeError("Short process read")
            return buffer.raw

        def u64(address):
            return struct.unpack("<Q", read(address, 8))[0]

        def i32(address):
            return struct.unpack("<i", read(address, 4))[0]

        def string(address):
            ptr, count, capacity = struct.unpack("<Qii", read(address, 16))
            if count == 0:
                return ""
            if not 0 < count <= capacity <= 32768:
                raise RuntimeError("Invalid FString; game state may have changed")
            return read(ptr, count * 2).decode("utf-16-le").rstrip("\0")

        def elements(address, stride):
            data, count, capacity = struct.unpack("<Qii", read(address, 16))
            if not 0 <= count <= capacity <= 32768:
                raise RuntimeError("Invalid sparse-array bounds")
            bits = u64(address + 0x20) or address + 0x10
            for index in range(count):
                word = struct.unpack("<I", read(bits + (index // 32) * 4, 4))[0]
                if word & (1 << (index % 32)):
                    yield data + index * stride

        # Validate the getter's instructions before accessing its singleton.
        if read(base + 0x4650E0, 3) != b"\x48\x8b\x05":
            raise RuntimeError("Loaded-code validation failed")
        graphics = u64(base + 0x3A56808)
        draw_scale = i32(graphics + 0x34)
        game_ini = string(base + 0x3AC5868)
        config = u64(base + 0x3AB94B8)
        game_file = next((entry + 16 for entry in elements(config + 8, 0x130)
                          if string(entry).casefold() == game_ini.casefold()), None)
        if game_file is None:
            raise RuntimeError("Game.ini absent from live config cache")
        mapping = {}
        for section in elements(game_file, 0xB8):
            name = string(section)
            if not name.startswith("GraphicsWindowsOption.DrawScale@"):
                continue
            values = list(elements(section + 16, 48))
            if len(values) != 1:
                raise RuntimeError("Unexpected DrawScale section layout")
            mapping[name.rsplit("@", 1)[1]] = string(values[0] + 8)
        values = u64(base + 0x3A944E8)
        game_value, render_value = struct.unpack("<ff", read(values, 8))
        return {"pid": pid, "exe_sha256": EXE_SHA256, "draw_scale": draw_scale,
                "configured_percentage": mapping.get(str(draw_scale)),
                "screen_percentage_game_thread": game_value,
                "screen_percentage_render_thread": render_value,
                "draw_scale_mapping": mapping,
                "note": "Cvar values do not prove reduced render-target dimensions."}
    finally:
        kernel.CloseHandle(process)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pid", type=int)
    args = parser.parse_args()
    print(json.dumps(inspect(args.pid), indent=2))
