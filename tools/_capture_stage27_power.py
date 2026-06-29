#!/usr/bin/env python3
"""One-off Stage 27 power menu screenshot helper. VM-only."""
import subprocess
import time
from pathlib import Path

import pyautogui
import pygetwindow as gw
import win32gui

pyautogui.FAILSAFE = False
pyautogui.PAUSE = 0.05

ROOT = Path(__file__).resolve().parents[1]
SHOT = ROOT / "docs" / "screenshots"
VM = "Vibio OS Dev"
VBOX = r"C:\Program Files\Oracle\VirtualBox\VBoxManage.exe"
GUEST_W, GUEST_H = 1024, 768


def vbox(*args):
    subprocess.run([VBOX, *args], check=True)


def vm_state():
    out = subprocess.check_output([VBOX, "showvminfo", VM, "--machinereadable"], text=True)
    for line in out.splitlines():
        if line.startswith("VMState="):
            return line.split('"')[1]
    return ""


def client_origin(hwnd):
    rect = win32gui.GetClientRect(hwnd)
    left_top = win32gui.ClientToScreen(hwnd, (0, 0))
    width = rect[2] - rect[0]
    height = rect[3] - rect[1]
    return left_top[0], left_top[1], width, height


def guest_mapper(hwnd):
    off_x, off_y, width, height = client_origin(hwnd)
    scale = min(width / GUEST_W, height / GUEST_H)
    disp_w = int(GUEST_W * scale)
    disp_h = int(GUEST_H * scale)
    pad_x = off_x + (width - disp_w) // 2
    pad_y = off_y + (height - disp_h) // 2
    return pad_x, pad_y, scale


def capture_and_walk(hwnd, gx, gy):
    pad_x, pad_y, scale = guest_mapper(hwnd)
    cx = int(pad_x + 512 * scale)
    cy = int(pad_y + 384 * scale)
    tx = int(pad_x + gx * scale)
    ty = int(pad_y + gy * scale)
    win32gui.SetForegroundWindow(hwnd)
    time.sleep(0.2)
    pyautogui.moveTo(cx, cy, duration=0.2)
    pyautogui.click()
    time.sleep(0.25)
    pyautogui.moveRel(int(tx - cx), int(ty - cy), duration=0.35)
    pyautogui.click()
    time.sleep(0.55)
    print(f"walk center->({gx},{gy})")


def shot(name):
    path = SHOT / name
    path.parent.mkdir(parents=True, exist_ok=True)
    vbox("controlvm", VM, "screenshotpng", str(path))
    print(f"saved {path} ({path.stat().st_size} bytes)")


def wait_window():
    for _ in range(60):
        wins = [w for w in gw.getAllWindows() if "Vibio OS Dev" in w.title and w.width > 200]
        if wins:
            w = wins[0]
            if w.isMinimized:
                w.restore()
            hwnd = w._hWnd
            win32gui.SetForegroundWindow(hwnd)
            time.sleep(0.4)
            return hwnd
        time.sleep(1)
    raise SystemExit("VM window not found")


def main():
    if vm_state() != "running":
        vbox("startvm", VM, "--type", "gui")
        time.sleep(22)
    win = wait_window()

    capture_and_walk(win, 848, 164)  # close browser
    time.sleep(0.8)
    capture_and_walk(win, 798, 748)  # power icon
    time.sleep(0.8)
    shot("2026-06-25-stage27-power-menu-fixed.png")

    capture_and_walk(win, 400, 400)  # dismiss menu
    time.sleep(0.5)
    capture_and_walk(win, 46, 119)  # terminal icon
    time.sleep(0.8)
    capture_and_walk(win, 798, 748)
    time.sleep(0.5)
    capture_and_walk(win, 700, 702)  # restart row
    time.sleep(0.8)
    shot("2026-06-25-stage27-restart-confirm-fixed.png")

    capture_and_walk(win, 586, 433)  # back
    time.sleep(0.8)
    capture_and_walk(win, 798, 748)
    time.sleep(0.5)
    capture_and_walk(win, 700, 702)
    time.sleep(0.5)
    capture_and_walk(win, 454, 433)  # confirm restart
    time.sleep(12)
    print("state after restart:", vm_state())
    if vm_state() != "running":
        vbox("startvm", VM, "--type", "gui")
        time.sleep(22)
        win = wait_window()

    capture_and_walk(win, 46, 119)
    time.sleep(0.8)
    capture_and_walk(win, 798, 748)
    time.sleep(0.5)
    capture_and_walk(win, 700, 662)  # shutdown row
    time.sleep(0.8)
    shot("2026-06-25-stage27-shutdown-confirm-fixed.png")
    capture_and_walk(win, 586, 433)
    time.sleep(0.8)
    capture_and_walk(win, 798, 748)
    time.sleep(0.5)
    capture_and_walk(win, 700, 662)
    time.sleep(0.5)
    capture_and_walk(win, 454, 433)  # confirm shutdown
    time.sleep(2.5)
    shot("2026-06-25-stage27-shutdown-result.png")
    print("final state:", vm_state())


if __name__ == "__main__":
    main()
