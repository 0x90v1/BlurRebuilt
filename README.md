# Blur Rebuilt

This is the 2010 game **Blur**, by Bizarre Creations, brought up to date for modern PCs.

## What's new

The original game was a 32-bit executable using DirectX 9, and had numerous issues when framerate was allowed
to go over 60 FPS.
This project intends to solve all of that.

### DirectX 11

The original DirectX 9 renderer has been replaced with a DirectX 11 one. All shaders had to be recompiled
for the new API.

### HDR

Made possible by the switch to DX11, this version of the game supports real HDR.
On an HDR monitor with HDR turned on in Windows, the game will detect your display's peak brightness, and
headlights, neon, street lamps, power-ups and the sky will be able to shine brighter than the rest of the picture.
Menus and the HUD stay at normal brightness.

### Unlocked frame rate

The original game was designed around 60 fps, and some things (cars, cameras, audio, particles, menus, etc.)
misbehaved the farther you went above that.
This version runs with unlocked frame rate and everything behaves as expected.

### Decreased audio latency

The retail game had pretty noticeable audio latency when audio quality was set to high.
This should now be fixed as well.

### 64-bit

The game is now a native 64-bit application.

## Requirements

- An installed copy of Blur for PC
- Windows 10 or 11, 64-bit
- A DirectX 11 graphics card
- [Microsoft Visual C++ Redistributable (x64)](https://aka.ms/vs/17/release/vc_redist.x64.exe)

## Installation

1. Copy `BlurRebuilt.exe` and the `fx11` folder into your Blur install folder. That's
   the one containing `Blur.exe`, usually `C:\Program Files (x86)\Activision\Blur(TM)`.
2. Run `BlurRebuilt.exe`.

Your original `Blur.exe` isn't affected, and you can keep using it.

Before your first race, back up `%APPDATA%\Bizarre Creations\Blur`. Your save is
stored there, and both versions of the game use it.

## Optional settings

These Windows environment variables are available if you want to adjust things:

| Variable | Effect |
|---|---|
| `BLUR_DX11_HDR=off` | Play in SDR even with Windows HDR on |
| `BLUR_DX11_HDR_PEAK_NITS=1000` | Set your display's peak brightness manually |
| `BLUR_DX11_HDR_PAPER_WHITE_NITS=200` | Set the brightness of menus and everyday surfaces |
| `BLUR2PC_FRAMELIMIT=120` | Cap the frame rate |

## Intro videos

The intro videos don't play. The game goes straight to the title screen. That's expected.

## Online play

Online play through AMAX isn't supported for the time being.
AMAX's client files are built for the original `Blur.exe`, and rely on it being DX9 and 32-bit.
Local split-screen works as expected.

## Changelog

V1.0 (2026/10/21) Initial release
V1.1 (2026/10/22) Ultra widescreen fixes.
V1.2 (2026/10/22) Decreased audio latency in high quality setting.

## Comments

For comments and general discussion, go to https://www.reddit.com/r/BlurGame
