"""Serial enumerate/open/read/write/close split by operating system.

Windows enumeration reads the registry and does not open a handle.
The Win32 open path is mvp_host.open_serial. Tests pass a kernel stand-in.

POSIX open/read/write/close is implemented for a real POSIX process.
On Windows those calls are not run, and this module does not report
macOS or Linux devices as measured. A socket pair exercises the
read/write/close contract when a PTY is not available.
"""

import os
import socket
import sys


def windows_enumerate():
    import winreg

    names = []
    error = ""
    if sys.platform != "win32":
        return {
            "platform": "windows",
            "measured": False,
            "status": "not_measured",
            "ports": [],
            "opened": False,
            "error": "",
            "ops": ["enumerate", "open", "read", "write", "close"],
        }
    try:
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r"HARDWARE\DEVICEMAP\SERIALCOMM") as key:
            index = 0
            while True:
                try:
                    _value, data, _kind = winreg.EnumValue(key, index)
                except OSError:
                    break
                index += 1
                if isinstance(data, str) and data.startswith("COM"):
                    names.append(data)
        names.sort(key=lambda name: (len(name), name))
    except OSError as exc:
        names = []
        error = str(exc)
    return {
        "platform": "windows",
        "measured": sys.platform == "win32",
        "status": "listed" if sys.platform == "win32" else "not_measured",
        "ports": names if sys.platform == "win32" else [],
        "opened": False,
        "error": error,
        "ops": ["enumerate", "open", "read", "write", "close"],
    }


def posix_device_names(platform):
    """Return device names only when this process is that platform.

    A Windows process returns None and does not look at a device node.
    """
    if platform == "linux" and sys.platform.startswith("linux"):
        root = __import__("pathlib").Path("/dev")
        found = []
        for pattern in ("ttyUSB*", "ttyACM*"):
            found.extend(sorted(path.name for path in root.glob(pattern)))
        return found
    if platform == "mac" and sys.platform == "darwin":
        root = __import__("pathlib").Path("/dev")
        return sorted(path.name for path in root.glob("cu.*") if "Bluetooth" not in path.name)
    return None


def posix_open(path):
    """Open one POSIX tty. On Windows this returns without opening the path."""
    if os.name != "posix":
        return {"opened": False, "measured": False, "status": "not_measured", "error": "posix open was not run", "fd": None}
    import termios

    fd = os.open(path, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    attrs = termios.tcgetattr(fd)
    attrs[4] = attrs[5] = termios.B115200
    attrs[2] = attrs[2] | termios.CLOCAL | termios.CREAD
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    return {"opened": True, "measured": True, "status": "opened", "error": "", "fd": fd}


def posix_read(fd, size):
    return os.read(fd, size)


def posix_write(fd, data):
    view = bytes(data)
    sent = 0
    while sent < len(view):
        wrote = os.write(fd, view[sent:])
        if wrote <= 0:
            raise OSError("posix write failed")
        sent += wrote
    return sent


def posix_close(fd):
    os.close(fd)


def parameter_transfer():
    """Exercise read, write, and close on a local pair.

    This is not a Mac or Linux device measurement. When os.openpty exists,
    the pair is a PTY. Otherwise it is a socket pair.
    """
    kind = "socketpair"
    left = None
    right = None
    if hasattr(os, "openpty"):
        left, right = os.openpty()
        kind = "pty"
        try:
            posix_write(right, b"pty-byte")
            got = posix_read(left, 16)
            posix_close(left)
            posix_close(right)
            blocked = False
            try:
                posix_read(left, 1)
                blocked = False
            except OSError:
                blocked = True
            return {"kind": kind, "payload": got.decode("ascii"), "closed": blocked, "measuredDevice": False}
        except OSError as exc:
            for fd in (left, right):
                if fd is not None:
                    try:
                        os.close(fd)
                    except OSError:
                        pass
            return {"kind": kind, "error": str(exc), "measuredDevice": False}
    pair = socket.socketpair()
    try:
        pair[0].settimeout(0.2)
        pair[1].sendall(b"sock-byte")
        got = pair[0].recv(16)
        pair[0].close()
        pair[1].close()
        refused = False
        try:
            pair[0].recv(1)
        except OSError:
            refused = True
        return {"kind": kind, "payload": got.decode("ascii"), "closed": refused, "measuredDevice": False}
    except OSError as exc:
        for end in pair:
            try:
                end.close()
            except OSError:
                pass
        return {"kind": kind, "error": str(exc), "measuredDevice": False}


def windows_parameter_open():
    """Call the product Win32 open with a kernel stand-in. No COM handle is created."""
    import mvp_host

    class Recording(mvp_host.KernelMock):
        def __init__(self):
            super().__init__()
            self.baud = None
            self.byte = None
            self.parity = None
            self.stop = None
            self.flags = None

        def SetCommState(self, _handle, dcb):
            import ctypes
            from ctypes import wintypes

            class Head(ctypes.Structure):
                _fields_ = [
                    ("DCBlength", wintypes.DWORD),
                    ("BaudRate", wintypes.DWORD),
                    ("flags", wintypes.DWORD),
                    ("wReserved", wintypes.WORD),
                    ("XonLim", wintypes.WORD),
                    ("XoffLim", wintypes.WORD),
                    ("ByteSize", wintypes.BYTE),
                    ("Parity", wintypes.BYTE),
                    ("StopBits", wintypes.BYTE),
                ]

            addr = ctypes.cast(dcb, ctypes.c_void_p).value
            fields = ctypes.cast(addr, ctypes.POINTER(Head)).contents
            self.baud = int(fields.BaudRate)
            self.byte = int(fields.ByteSize)
            self.parity = int(fields.Parity)
            self.stop = int(fields.StopBits)
            self.flags = int(fields.flags)
            return 1 if self.set_state else 0

    kernel = Recording()
    link = mvp_host.open_serial("COM99", kernel=kernel)
    try:
        link.write(b"CUE v1 1 prepare\n")
        kernel.pending = b"ok"
        chunk = link.read(8)
    finally:
        link.close()
    return {
        "name": "COM99",
        "baud": kernel.baud,
        "byte": kernel.byte,
        "parity": kernel.parity,
        "stop": kernel.stop,
        "flags": kernel.flags,
        "wrote": bytes(link.sent).decode("ascii"),
        "read": chunk.decode("ascii"),
        "closed": kernel.closed == [kernel.handle],
        "created": kernel.created,
        "deviceOpened": False,
    }


def boundary_report():
    pty = "available" if hasattr(os, "openpty") else "unavailable"
    return {
        "windowsOps": ["enumerate", "open", "read", "write", "close"],
        "posixOps": ["enumerate", "open", "read", "write", "close"],
        "pty": pty,
        "posixExecutedHere": os.name == "posix",
        "macMeasured": False,
        "linuxMeasured": False,
    }
