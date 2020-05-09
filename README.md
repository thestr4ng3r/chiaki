
![Chiaki Logo](assets/chiaki_wide.png)

# Chiaki

**Disclaimer:** This project is not endorsed or certified by Sony Interactive Entertainment LLC.

[![Travis Build Status](https://travis-ci.com/thestr4ng3r/chiaki.svg?branch=master)](https://travis-ci.com/thestr4ng3r/chiaki) [![AppVeyor Build status](https://ci.appveyor.com/api/projects/status/dnj9cmx5mmaaawer?svg=true)](https://ci.appveyor.com/project/thestr4ng3r/chiaki) [![builds.sr.ht Status](https://builds.sr.ht/~thestr4ng3r/chiaki.svg)](https://builds.sr.ht/~thestr4ng3r/chiaki?)

Chiaki is a Free and Open Source Software Client for PlayStation 4 Remote Play
for Linux, FreeBSD, OpenBSD, Android, macOS, Windows and potentially even more platforms.

![Screenshot](assets/screenshot.png)

## Features

Everything necessary for a full streaming session, including the initial
registration and wakeup of the console, is supported.
The following features however are yet to be implemented:
* Congestion Control
* H264 Error Concealment (FEC and active error recovery however are implemented)
* Touchpad support (Triggering the Touchpad Button is currently possible from the keyboard though)
* Rumble
* Accelerometer/Gyroscope

## Installing

You can either download a pre-built release (easier) or build Chiaki from source.

### Downloading a Release

Builds are provided for Linux, Android, macOS and Windows.

You can download them [here](https://github.com/thestr4ng3r/chiaki/releases).

* **Linux**: The provided file is an [AppImage](https://appimage.org/). Simply make it executable (`chmod +x <file>.AppImage`) and run it.
* **Android**: Install from [Google Play](https://play.google.com/store/apps/details?id=com.metallic.chiaki) or download the APK from GitHub.
* **macOS**: Drag the application from the `.dmg` into your Applications folder.
* **Windows**: Extract the `.zip` file and execute `chiaki.exe`.

### Building from Source

Dependencies are CMake, Qt 5 with QtMultimedia, QtOpenGL and QtSvg, FFMPEG (libavcodec with H264 is enough), libopus, OpenSSL 1.1, SDL 2,
protoc and the protobuf Python library (only used during compilation for Nanopb). Then, Chiaki builds just like any other CMake project:
```
git submodule update --init
mkdir build && cd build
cmake ..
make
```

For more detailed platform-specific instructions, see [doc/platform-build.md](doc/platform-build.md).

## Usage

If your PS4 is on your local network, is turned on or in standby mode and does not have Discovery explicitly disabled, Chiaki should find it.
Otherwise, you can add it manually.
To do so, click the "+" icon in the top right, and enter your PS4's IP address.

You will then need to register your PS4 with Chiaki. You will need two more pieces of information to do this.

### Obtaining your PSN AccountID

Starting with PS4 7.0, it is necessary to use a so-called "AccountID" as opposed to the "Online-ID" for registration (streaming itself did not change).
This ID seems to be a unique identifier for a PSN Account and it can be obtained from the PSN after logging in using OAuth.
A Python 3 script which does this is provided in [scripts/psn-account-id.py](scripts/psn-account-id.py).
Simply run it in a terminal and follow the instructions. Once you know your ID, write it down. You will likely never have to do this process again.

### Obtaining a Registration PIN

To register a PS4 with a PIN, it must be put into registration mode. To do this, on your PS4, simply go to:
Settings -> Remote Play (ensure this is ticked) -> Add Device

You can now double-click your PS4 in Chiaki's main window to start Remote Play.

## Acknowledgements

This project has only been made possible because of the following Open Source projects:
[radare2](https://github.com/radare/radare2),
[Cutter](https://cutter.re/),
[Frida](https://www.frida.re/) and
[x64dbg](https://x64dbg.com/).

Also thanks to [delroth](https://github.com/delroth) for analyzing the registration and wakeup protocol,
[grill2010](https://github.com/grill2010) for analyzing the PSN's OAuth Login,
as well as a huge thank you to [FioraAeterna](https://github.com/FioraAeterna) for giving me some
extremely helpful information about FEC and error correction.

## About

Created by Florian Märkl.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
