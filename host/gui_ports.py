"""Serial port discovery split by platform.

Windows reads names from the registry and does not open a handle.
Mac and Linux adapters do not scan devices. A Windows run must not
report them as measured.
"""

import sys


def windows_port_names():
    import winreg

    names = []
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
    return names


def windows_ports():
    try:
        names = windows_port_names()
        error = ""
    except OSError as exc:
        names = []
        error = str(exc)
    return {
        "platform": "windows",
        "measured": True,
        "status": "listed",
        "ports": names,
        "connect": "disabled",
        "opened": False,
        "error": error,
    }


def mac_ports():
    return {
        "platform": "mac",
        "measured": False,
        "status": "not_measured",
        "ports": [],
        "connect": "not_measured",
        "opened": False,
        "error": "",
    }


def linux_ports():
    return {
        "platform": "linux",
        "measured": False,
        "status": "not_measured",
        "ports": [],
        "connect": "not_measured",
        "opened": False,
        "error": "",
    }


def current_ports():
    if sys.platform == "win32":
        return windows_ports()
    if sys.platform == "darwin":
        return mac_ports()
    if sys.platform.startswith("linux"):
        return linux_ports()
    return {
        "platform": sys.platform,
        "measured": False,
        "status": "not_measured",
        "ports": [],
        "connect": "not_measured",
        "opened": False,
        "error": "",
    }


def adapter_report():
    """Run every adapter that does not touch a device.

    On Windows the Mac and Linux functions still return not_measured.
    """
    import gui_serial

    return {
        "host": current_ports(),
        "windows": windows_ports() if sys.platform == "win32" else {
            "platform": "windows",
            "measured": False,
            "status": "not_measured",
            "ports": [],
            "connect": "not_measured",
            "opened": False,
            "error": "",
        },
        "mac": mac_ports(),
        "linux": linux_ports(),
        "serial": gui_serial.boundary_report(),
    }
