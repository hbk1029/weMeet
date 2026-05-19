"""CI smoke test: start server, login 2 users, send meeting chat, verify delivery.

Usage:
  python smoke_test.py              # restart server + protocol test
  python smoke_test.py --build      # rebuild server (make clean && make) + restart + test
  python smoke_test.py --test-only  # only protocol test, no server restart
"""

import socket
import subprocess
import time
import sys
import os

from weemeet_proto import (
    SRV_IP, SRV_PORT,
    pack_login_rq, pack_meeting_create_rq, pack_meeting_join_rq,
    pack_meeting_chat_rq, pack_meeting_end_rq, pack_meeting_exit_rq,
    send_packet, recv_until_type,
    LOGIN_RS, MEETING_CREATE_RS, MEETING_JOIN_RS,
    MEETING_CHAT_NOTIFY, LOGIN_SUCCESS,
)

VM_IP = "192.168.74.135"
VM_USER = "hbk"
SRV_DIR = "/home/hbk/Desktop/QtSave/WeMeetServer"
SRV_BIN = f"{SRV_DIR}/WeMeetServer"
SRV_LOG = "/tmp/meet_server.log"
SSH_ENV = {
    "SSH_ASKPASS": "/tmp/askpass.sh",
    "DISPLAY": ":0",
}
SSH_BASE = [
    "ssh", "-o", "StrictHostKeyChecking=no",
    "-o", "ConnectTimeout=10",
    f"{VM_USER}@{VM_IP}",
]
TEST_USER1 = "11111111111"
TEST_USER2 = "22222222222"
TEST_PASS1 = "11111111111"
TEST_PASS2 = "22222222222"
TEST_CHAT_MSG = "SMOKE_TEST_HELLO"


def fail(msg):
    print(f"\n=== [FAIL] {msg} ===")
    sys.exit(1)


def rprint(msg):
    print(f"  {msg}")


def ssh(cmd):
    """Run command on VM via SSH_ASKPASS."""
    env = {**os.environ, **SSH_ENV}
    return subprocess.run(
        SSH_BASE + [cmd],
        capture_output=True, text=True, timeout=120, env=env,
    )


def step_server_build():
    print("[Build] Building server on VM (make clean && make)...")
    r = ssh(f"cd {SRV_DIR} && make clean && make 2>&1")
    if r.returncode != 0:
        fail(f"Build failed:\n{r.stdout}\n{r.stderr}")
    rprint("Build OK")


def step_server_start():
    print("[Start] Restarting server...")
    ssh(f"pkill -9 -f WeMeetServer 2>/dev/null; sleep 1")
    r = ssh(f"nohup {SRV_BIN} > {SRV_LOG} 2>&1 &")
    if r.returncode != 0:
        fail(f"Start failed:\n{r.stdout}\n{r.stderr}")
    rprint("Start command sent")


def step_server_wait():
    print("[Wait]  Polling server port 8080...")
    for i in range(30):
        try:
            probe = socket.create_connection((SRV_IP, SRV_PORT), timeout=2)
            probe.close()
            rprint(f"Ready after {i + 1}s")
            return
        except OSError:
            time.sleep(1)
    fail("Server did not open port within 30s")


# --- Protocol test steps ---

def step_login(sock, number, password, label):
    """Login and return user info dict."""
    rprint(f"{label}: connecting...")
    sock.connect((SRV_IP, SRV_PORT))
    send_packet(sock, pack_login_rq(number, password))
    result = recv_until_type(sock, LOGIN_RS)
    if result is None:
        fail(f"{label} login: no response")
    if result["result"] != LOGIN_SUCCESS:
        fail(f"{label} login: result={result['result']}")
    rprint(f"{label}: userId={result['userId']} name={result['userName']}")
    return result


def step_create_meeting(sock, info):
    rprint("User1: creating meeting...")
    send_packet(sock, pack_meeting_create_rq(info["userId"], info["userName"]))
    result = recv_until_type(sock, MEETING_CREATE_RS)
    if result is None or result["result"] != 0:
        fail(f"Meeting create failed: {result}")
    rprint(f"meetingId={result['meetingId']}")
    return result["meetingId"]


def step_join_meeting(sock, info, meeting_id):
    rprint("User2: joining meeting...")
    send_packet(sock, pack_meeting_join_rq(info["userId"], info["userName"], meeting_id))
    result = recv_until_type(sock, MEETING_JOIN_RS)
    if result is None or result["result"] != 0:
        fail(f"Meeting join failed: {result}")
    rprint(f"Joined meetingId={result['meetingId']}")


def step_send_and_verify(sock_sender, sock_receiver, info, meeting_id):
    rprint("User1: sending chat...")
    send_packet(sock_sender, pack_meeting_chat_rq(info["userId"], meeting_id, TEST_CHAT_MSG))

    rprint("User2: waiting for chat notify...")
    result = recv_until_type(sock_receiver, MEETING_CHAT_NOTIFY, timeout=10)
    if result is None:
        fail("Chat notify: no response")
    received = result["content"]
    if received != TEST_CHAT_MSG:
        fail(f"Content mismatch: expected '{TEST_CHAT_MSG}', got '{received}'")
    rprint(f"Message verified: '{received}' from {result['userName']}")


def step_cleanup(sock_sender, sock_receiver, info1, info2, meeting_id):
    rprint("Cleanup...")
    try:
        send_packet(sock_sender, pack_meeting_end_rq(info1["userId"], meeting_id))
        time.sleep(0.3)
    except Exception:
        pass
    for s in [sock_sender, sock_receiver]:
        try:
            s.close()
        except Exception:
            pass


def main():
    do_build = "--build" in sys.argv
    test_only = "--test-only" in sys.argv

    print("=== WeMeet Smoke Test ===")
    if not test_only:
        if do_build:
            step_server_build()
        step_server_start()
        step_server_wait()

    sock1 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock2 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    info1 = info2 = None
    meeting_id = None

    try:
        info1 = step_login(sock1, TEST_USER1, TEST_PASS1, "User1")
        info2 = step_login(sock2, TEST_USER2, TEST_PASS2, "User2")
        meeting_id = step_create_meeting(sock1, info1)
        step_join_meeting(sock2, info2, meeting_id)
        step_send_and_verify(sock1, sock2, info1, meeting_id)
        print("\n=== [PASS] All checks OK ===")
        sys.exit(0)
    except Exception as e:
        import traceback
        traceback.print_exc()
        fail(str(e))
    finally:
        if info1 and info2 and meeting_id:
            step_cleanup(sock1, sock2, info1, info2, meeting_id)


if __name__ == "__main__":
    main()
