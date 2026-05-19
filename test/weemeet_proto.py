"""WeMeet binary protocol pack/unpack helpers.

Verified struct sizes (MinGW 8.1 64-bit, default alignment):
  LOGIN_RQ=64  LOGIN_RS=44  MEETING_CREATE_RQ=40  MEETING_CREATE_RS=12
  MEETING_JOIN_RQ=44  MEETING_JOIN_RS=12  MEETING_EXIT_RQ=12
  MEETING_JOIN_NOTIFY=44  MEETING_CHAT_RQ=8204  MEETING_CHAT_NOTIFY=8236
"""

import struct
import socket
import time

# --- Protocol constants from Net/def.h ---
SRV_IP = "192.168.74.135"
SRV_PORT = 8080
MAX_LENGTH = 30
CONTENT_LENGTH = 8192

BASE = 1000
LOGIN_RQ = BASE + 3
LOGIN_RS = BASE + 4
MEETING_CREATE_RQ = BASE + 12
MEETING_CREATE_RS = BASE + 13
MEETING_JOIN_RQ = BASE + 14
MEETING_JOIN_RS = BASE + 15
MEETING_EXIT_RQ = BASE + 16
MEETING_JOIN_NOTIFY = BASE + 17
MEETING_CHAT_RQ = BASE + 19
MEETING_CHAT_NOTIFY = BASE + 20
MEETING_END_RQ = BASE + 27

# Additional types that may arrive between expected packets
FRIEND_INFO = BASE + 10
FRIEND_REQUEST_NOTIFY = BASE + 11
MEETING_JOIN_NOTIFY = BASE + 17
MEETING_EXIT_NOTIFY = BASE + 18
MEETING_MEMBER_INFO = BASE + 21
MEETING_AUDIO_RQ = BASE + 22
MEETING_VIDEO_RQ = BASE + 23
DELETE_FRIEND_RQ = BASE + 24
DELETE_FRIEND_RS = BASE + 25
FRIEND_STATUS_NOTIFY = BASE + 26

# Server push types to consume (don't error on these)
SERVER_PUSH_TYPES = {
    FRIEND_INFO,
    FRIEND_REQUEST_NOTIFY,
    MEETING_JOIN_NOTIFY,
    MEETING_EXIT_NOTIFY,
    MEETING_MEMBER_INFO,
    FRIEND_STATUS_NOTIFY,
    DELETE_FRIEND_RS,
}

LOGIN_SUCCESS = 0
SIZES = {
    LOGIN_RQ: 64,
    LOGIN_RS: 44,
    MEETING_CREATE_RQ: 40,
    MEETING_CREATE_RS: 12,
    MEETING_JOIN_RQ: 44,
    MEETING_JOIN_RS: 12,
    MEETING_EXIT_RQ: 12,
    MEETING_JOIN_NOTIFY: 44,
    MEETING_CHAT_RQ: 8204,
    MEETING_CHAT_NOTIFY: 8236,
}


def _pad_str(s, size):
    """Encode UTF-8 string, truncate to size-1, null-pad to exactly size bytes."""
    b = s.encode("utf-8")[: size - 1]
    return b + b"\x00" * (size - len(b))


def _unpad_str(b):
    """Strip trailing nulls, decode UTF-8."""
    return b.rstrip(b"\x00").decode("utf-8", errors="replace")


def pack_login_rq(tel, password):
    buf = bytearray(64)
    struct.pack_into("<i", buf, 0, LOGIN_RQ)
    buf[4:34] = _pad_str(tel, 30)
    buf[34:64] = _pad_str(password, 30)
    return bytes(buf)


def unpack_login_rs(data):
    return {
        "type": struct.unpack_from("<i", data, 0)[0],
        "userId": struct.unpack_from("<i", data, 4)[0],
        "userName": _unpad_str(data[8:38]),
        "result": struct.unpack_from("<i", data, 40)[0],
    }


def pack_meeting_create_rq(user_id, user_name):
    buf = bytearray(40)
    struct.pack_into("<i", buf, 0, MEETING_CREATE_RQ)
    struct.pack_into("<i", buf, 4, user_id)
    buf[8:38] = _pad_str(user_name, 30)
    return bytes(buf)


def unpack_meeting_create_rs(data):
    return {
        "type": struct.unpack_from("<i", data, 0)[0],
        "meetingId": struct.unpack_from("<i", data, 4)[0],
        "result": struct.unpack_from("<i", data, 8)[0],
    }


def pack_meeting_join_rq(user_id, user_name, meeting_id):
    buf = bytearray(44)
    struct.pack_into("<i", buf, 0, MEETING_JOIN_RQ)
    struct.pack_into("<i", buf, 4, user_id)
    buf[8:38] = _pad_str(user_name, 30)
    struct.pack_into("<i", buf, 40, meeting_id)
    return bytes(buf)


def unpack_meeting_join_rs(data):
    return {
        "type": struct.unpack_from("<i", data, 0)[0],
        "meetingId": struct.unpack_from("<i", data, 4)[0],
        "result": struct.unpack_from("<i", data, 8)[0],
    }


def pack_meeting_exit_rq(user_id, meeting_id):
    buf = bytearray(12)
    struct.pack_into("<i", buf, 0, MEETING_EXIT_RQ)
    struct.pack_into("<i", buf, 4, user_id)
    struct.pack_into("<i", buf, 8, meeting_id)
    return bytes(buf)


def pack_meeting_end_rq(user_id, meeting_id):
    buf = bytearray(12)
    struct.pack_into("<i", buf, 0, MEETING_END_RQ)
    struct.pack_into("<i", buf, 4, user_id)
    struct.pack_into("<i", buf, 8, meeting_id)
    return bytes(buf)


def unpack_meeting_join_notify(data):
    return {
        "type": struct.unpack_from("<i", data, 0)[0],
        "meetingId": struct.unpack_from("<i", data, 4)[0],
        "userId": struct.unpack_from("<i", data, 8)[0],
        "userName": _unpad_str(data[12:42]),
    }


def pack_meeting_chat_rq(user_id, meeting_id, content):
    buf = bytearray(8204)
    struct.pack_into("<i", buf, 0, MEETING_CHAT_RQ)
    struct.pack_into("<i", buf, 4, user_id)
    struct.pack_into("<i", buf, 8, meeting_id)
    buf[12:8204] = _pad_str(content, 8192)
    return bytes(buf)


def unpack_meeting_chat_notify(data):
    return {
        "type": struct.unpack_from("<i", data, 0)[0],
        "userId": struct.unpack_from("<i", data, 4)[0],
        "userName": _unpad_str(data[8:38]),
        "meetingId": struct.unpack_from("<i", data, 40)[0],
        "content": _unpad_str(data[44:8236]),
    }


def peek_type(data):
    """Read packType from raw bytes without knowing struct."""
    return struct.unpack_from("<i", data, 0)[0]


def send_packet(sock, payload):
    """4-byte LE length prefix + payload, matching TCPClient::sendData."""
    header = struct.pack("<i", len(payload))
    sock.sendall(header + payload)


def recv_packet(sock, timeout=5.0):
    """Read length-prefixed packet, matching TCPClient::recvData loop."""
    sock.settimeout(timeout)
    header = _recv_exact(sock, 4)
    if header is None:
        return None
    pack_len = struct.unpack("<i", header)[0]
    if pack_len <= 0 or pack_len > 100 * 1024 * 1024:
        raise ValueError(f"Bad packet length: {pack_len}")
    data = _recv_exact(sock, pack_len)
    return data


def _recv_exact(sock, n):
    """Receive exactly n bytes, return None on timeout/EOF."""
    buf = bytearray()
    while len(buf) < n:
        try:
            chunk = sock.recv(n - len(buf))
            if not chunk:
                return None
            buf.extend(chunk)
        except socket.timeout:
            return None
    return bytes(buf)


def recv_packet_type(sock, expected_type, timeout=5.0):
    """Receive next packet and verify type. Returns parsed dict or None."""
    data = recv_packet(sock, timeout)
    if data is None:
        return None
    handlers = {
        LOGIN_RS: unpack_login_rs,
        MEETING_CREATE_RS: unpack_meeting_create_rs,
        MEETING_JOIN_RS: unpack_meeting_join_rs,
        MEETING_JOIN_NOTIFY: unpack_meeting_join_notify,
        MEETING_CHAT_NOTIFY: unpack_meeting_chat_notify,
    }
    pt = peek_type(data)
    if pt != expected_type:
        raise ValueError(f"Expected type {expected_type}, got {pt}")
    if pt in handlers:
        return handlers[pt](data)
    return {"type": pt, "raw": data}


def recv_until_type(sock, expected_type, timeout=5.0):
    """Receive packets, consuming server pushes, until expected type arrives.

    Server may push FRIEND_INFO, FRIEND_STATUS_NOTIFY, etc. between
    request and response.  This function swallows those and returns
    only the matching packet.
    """
    deadline = time.time() + timeout
    handlers = {
        LOGIN_RS: unpack_login_rs,
        MEETING_CREATE_RS: unpack_meeting_create_rs,
        MEETING_JOIN_RS: unpack_meeting_join_rs,
        MEETING_JOIN_NOTIFY: unpack_meeting_join_notify,
        MEETING_CHAT_NOTIFY: unpack_meeting_chat_notify,
    }
    pushes = []  # record pushed packets for diagnostics
    while time.time() < deadline:
        remaining = deadline - time.time()
        data = recv_packet(sock, max(remaining, 0.5))
        if data is None:
            return None
        pt = peek_type(data)
        if pt in SERVER_PUSH_TYPES:
            pushes.append(pt)
            continue
        if pt == expected_type:
            if len(pushes) > 0:
                import logging
                logging.debug(f"Consumed pushes: {pushes}")
            if pt in handlers:
                return handlers[pt](data)
            return {"type": pt, "raw": data}
        raise ValueError(
            f"Expected type {expected_type}, got unexpected {pt}"
        )
    raise socket.timeout(
        f"Timeout waiting for type {expected_type}"
    )
