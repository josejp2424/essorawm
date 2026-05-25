<p align="center">
<img src="assets/essorawm.png"/>
</p>

<h1 align="center">EssoraWM</h1>

<p align="center">
Fast, lightweight and customizable window manager based on JWM.
</p>

<p align="center">
EssoraWM 0.1<br>
Based on JWM 2.4.7
</p>

---

## About

EssoraWM is a fork of JWM (Joe's Window Manager) focused on preserving JWM's lightweight structure while adding Essora improvements and modern behavior.

EssoraWM keeps the original `jwm` binary name for compatibility:

```bash
jwm -restart
jwm -reload
jwm -version
```

Version output:

```text
EssoraWM 0.1
Based on JWM 2.4.7
```

EssoraWM preserves the original JWM architecture and configuration system, but replaces the original menu workflow.

The traditional JWM menu is no longer the primary launcher.

Instead, EssoraWM uses:

```bash
/usr/local/bin/pymenu
```

---

## Screenshots

### Task preview / thumbnail support

EssoraWM includes window previews directly in the taskbar.

Window text labels were removed for a cleaner panel appearance.

Task buttons now prioritize:

* Window previews
* Icons
* Cleaner panel behavior

Screenshot:

<p align="center">
<img src="assets/essorawm-miniatura.png"/>
</p>

---

### Pymenu launcher

Pymenu is used as the application launcher for EssoraWM.

Screenshot:

<p align="center">
<img src="assets/essorawm-pymenu.png"/>
</p>

---

## Important

Pymenu is distributed separately.

EssoraWM expects:

```bash
/usr/local/bin/pymenu
```

to exist.

Package example:

```text
essora-pymenu
```

EssoraWM does not embed Python/GTK menu code directly into the window manager.

This keeps the WM:

* lightweight
* modular
* independent
* easy to maintain

---

## EssoraWM changes

Current modifications:

* Window task preview support
* Taskbar thumbnails
* Removed task text labels from open windows
* Pymenu integration
* Essora branding
* Updated panel behavior
* Improved icon handling
* Optional custom window buttons
* User JWM configuration support
* Custom build system
* Automatic Debian package generation

Essora specific changes are marked in source code:

```c
/* agregado por josejp2424 */
```

---

## Building EssoraWM

EssoraWM includes its own build system.

The build script:

* compiles EssoraWM
* builds NLS translations
* creates package structure
* includes Essora assets
* generates Debian package automatically

Requirements:

```bash
sudo apt install build-essential \
libx11-dev \
libxext-dev \
libxrender-dev \
libxmu-dev \
libxinerama-dev \
libxpm-dev \
libjpeg-dev \
libpng-dev \
libcairo2-dev \
librsvg2-dev \
libpango1.0-dev \
gettext \
autoconf \
automake
```

Build:

```bash
chmod +x build-essorawm-deb.sh
./build-essorawm-deb.sh
```

Generated structure:

```text
build-deb/
└── essorawm_0.1_amd64/
```

Generated package:

```text
essorawm_0.1_amd64.deb
```

---

## Original project

EssoraWM is based on:

JWM (Joe's Window Manager)

Original author: Joe Wingbermuehle

Original project: https://joewing.net/projects/jwm/

License:

MIT

---

## EssoraWM

Modifications and Essora integration:

josejp2424
