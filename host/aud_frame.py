"""Assemble AUD v1 frames. A bad frame drops the utterance."""

import re

CHUNK = 640
MAX_BYTES = 192000

_DATA = re.compile(r"(\d+) (\d+) (\d+) ([0-9a-fA-F]+)\Z")
_END = re.compile(r"(\d+) end (\d+) (\d+)\Z")
_DROP = re.compile(r"(\d+) drop ([a-z]+)\Z")


class AudError(Exception):
    pass


def assemble(lines):
    utt = None
    expect = 0
    parts = []
    total = 0
    for raw in lines:
        line = raw.strip("\r\n")
        if line == "" or not line.startswith("AUD v1 "):
            continue
        body = line[len("AUD v1 ") :]
        dropped = _DROP.fullmatch(body)
        if dropped:
            raise AudError(dropped.group(2))
        ended = _END.fullmatch(body)
        if ended:
            got_utt = int(ended.group(1))
            got_total = int(ended.group(2))
            got_count = int(ended.group(3))
            if utt is None or got_utt != utt or got_total != total or got_count != len(parts):
                raise AudError("end")
            if got_total == 0 or got_total > MAX_BYTES:
                raise AudError("end")
            return b"".join(parts)
        data = _DATA.fullmatch(body)
        if data is None:
            raise AudError("malformed")
        got_utt = int(data.group(1))
        seq = int(data.group(2))
        length = int(data.group(3))
        hex_text = data.group(4)
        if utt is None:
            utt = got_utt
        elif got_utt != utt:
            raise AudError("repress")
        if seq != expect or length == 0 or length > CHUNK or (length % 2) != 0:
            raise AudError("sequence")
        if len(hex_text) != length * 2:
            raise AudError("length")
        try:
            chunk = bytes.fromhex(hex_text)
        except ValueError as exc:
            raise AudError("hex") from exc
        if len(chunk) != length:
            raise AudError("length")
        total += length
        if total > MAX_BYTES:
            raise AudError("overflow")
        parts.append(chunk)
        expect += 1
    raise AudError("truncated")
