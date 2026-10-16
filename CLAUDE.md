# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

C++17 / Qt6 Widgets desktop app (Linux) to control the BatMobile Zumo robot (`../robot_chenillette`) over classic Bluetooth (SPP/RFCOMM). Port of the PyQt6 app `../robot_chenillette_pc_python` (itself a port of the Android app `../robot_chenillette_telecommande`): same UI layout, same command protocol. Sends single ASCII characters to the robot's SoftwareSerial Bluetooth module. UI strings and comments are in French.

## Build / Run

```bash
./build.sh                 # cmake -B build, → build/batmobile + build/bt_pairing
./launchRemoteControl.sh   # builds if needed, then runs build/batmobile
./runPairing.sh [--repair] [--timeout N]
```

Dependencies: Qt6 Widgets + DBus only. No Qt Bluetooth, no libbluetooth-dev: `link.cpp` defines `sockaddr_rc` / `BTPROTO_RFCOMM` itself. Robot: name `BatMobile`, PIN `0000`, address `00:18:A1:12:16:C5`.

Offscreen smoke test (no display, no robot): `QT_QPA_PLATFORM=offscreen`, build a small program linking the sources, instantiate `Window`, use `w.link()` / `w.setConnected(true)`. Stop it with `QCoreApplication::exit()`, not `quit()`: in Qt 6 `quit()` closes the windows, which opens the modal quit confirmation and hangs.

## Architecture

- **`config.h`** — protocol / robot constants (`SpeedLevels`, `SpeedPerLevel`, `SpeedLevelDefault`, `RobotName`, `RobotPin`, `RfcommChannel = 1`, `Stop`).
- **`Link`** (`link.cpp`) — non-blocking `AF_BLUETOOTH` RFCOMM socket driven by `QSocketNotifier` (no threads). `connectTo()` connects asynchronously (Write notifier + `SO_ERROR`), sends the current speed digit, emits `connected` / `failed`. Then a Read notifier: the robot never sends anything, so readable = EOF/error → `lost`. `send()` emits `lost` on write error. `disconnectFromRobot()` sends `' '` before closing (named so to avoid hiding `QObject::disconnect`).
- **`Window`** (`window.cpp`) — header (device combo box, editable for a manual address with `NoInsert`; refresh; connect button), vertical speed `QSlider` (0–9, top = fast) left of the d-pad, claw row, head row, keyboard hint, bottom-right `✕` button calling `close()`. `closeEvent` (✕ and title-bar close alike) asks `confirmQuit()` (Quitter / Annuler, default Annuler), ignores the event on Annuler, otherwise deletes a running `Pairing` (cancels it) and sends the stop via `disconnectFromRobot()`.
  - **`dirButton(label, cmd)`** — `pressed` sends `cmd`, `released` sends `' '`; `cmd == 0` is the stop button (press only).
  - **Keyboard** — `HoldKeys` maps `Qt::Key` → command (Z/S/Q/D + arrows, V/C, W/X). Auto-repeat ignored. Space = stop, digits move the slider (which sends the digit). Stop on release only if the released key is the last pressed (`heldKey_`). `focusOutEvent` sends a stop if a key is held.
  - Every button and the slider use `Qt::NoFocus` so the window keeps keyboard focus (otherwise Space would click the focused button).
- **`bluez.cpp`** — BlueZ D-Bus helpers (system bus): `GetManagedObjects`, property get/set, `pairedDevices()` (Device1 with `Paired`, name = Alias else Name). Replaces the Python `bluetoothctl` parsing. Inside `QObject` subclasses call them as `bluez::property` / `bluez::setProperty` (QObject has members with the same names).
- **`Pairing`** (`pairing.cpp`) — port of `bt_pairing.py`, in-process (QtDBus uses the Qt event loop): powers on the adapter, finds the device named `BatMobile` (already known, else `StartDiscovery` with a `bredr` filter, watching `InterfacesAdded` / `PropertiesChanged`), exports an `org.bluez.Agent1` (`Agent`) whose `RequestPinCode` returns `0000`, calls `Pair()` async (60 s), then sets `Trusted`. Already paired → only `Trusted`. `repair` removes the device first. Emits `progress`, then exactly one of `succeeded(address)` / `failed(error)`. Destruction mid-pairing sends `CancelPairing` and unregisters the agent.
- **`bt_pairing.cpp`** — CLI around `Pairing`: progress on stdout, last line `ADDRESS=xx:…` on success, `Erreur : …` on stderr and exit code 1 on failure.

In the GUI, when no paired device is named `BatMobile`, `refreshDevices()` inserts "BatMobile — appairage automatique" with invalid data at index 0; **Connecter** on that item runs `startPairing()` (progress in `status_`), then refreshes the list and connects to the returned address.

Do not run `bt_pairing --repair` casually when testing: it deletes the existing pairing.

## Command Protocol

Same as the Android app, the Python app and the firmware:

| Char | Action |
|------|--------|
| `z` / `s` | Avance / Recule (maintenir) |
| `q` / `d` | Pivote à gauche / droite (maintenir) |
| `v` / `c` | Ferme / Ouvre la pince (maintenir) |
| `w` / `x` | Tête ▲ / ▼ (maintenir) |
| `0` … `9` | Vitesse `(n + 1) × 40` — ne stoppe pas le robot |
| ` ` (espace) | Stop |

The character is sent on press and `' '` on release, **with no repeat while held**: the firmware has no keep-alive (`COMMAND_TIMEOUT_MS = 0`). Speed constants must stay in sync with the firmware and `App.tsx`. Any change to the command flow must keep the safety stops (disconnect, close, focus loss, link loss) in place.

The robot's Bluetooth module accepts only one connection at a time: the phone app must be disconnected first.
