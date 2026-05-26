<p align="center">
<img src="contrib/essorawm.svg"/>
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

EssoraWM is a fork of **JWM (Joe's Window Manager)**.

It keeps the original JWM structure and the `jwm` binary name for compatibility, but adds Essora-specific improvements for the panel, task previews, Pymenu launching, branding and wallpaper handling.

EssoraWM keeps compatibility with regular JWM commands:

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

EssoraWM preserves the original JWM architecture, configuration style and lightweight behavior, but it does **not** use the original JWM menu as the main application launcher.

Instead, the menu button and menu hotkeys are intended to call:

```bash
/usr/local/bin/pymenu
```

Pymenu is distributed as a separate package.

---

## Screenshots

### Task preview / thumbnail support

EssoraWM includes task previews directly in the panel.

The open-window task list was changed to avoid showing application names under the preview. The panel now focuses on icons and thumbnails for a cleaner look.

<p align="center">
<img src="assets/essorawm-miniatura.png"/>
</p>

---

### Pymenu launcher

EssoraWM is designed to launch Pymenu from `/usr/local/bin/pymenu`.

Pymenu is **not embedded** inside the EssoraWM source. It is expected to come as a separate package, for example:

```text
essora-pymenu
```

<p align="center">
<img src="assets/essorawm-pymenu.png"/>
</p>

---

### Wallpaper selector

EssoraWM includes a native wallpaper selector that can be launched with:

```bash
jwm -wallpaper
```

The wallpaper selector searches images in:

```text
/usr/share/backgrounds
```

It saves the selected wallpaper in:

```text
~/.config/essorawm/wallpaper
```

It can apply wallpapers using available tools such as:

- `hsetroot`
- `feh`
- `xwallpaper`

It also includes Puppy/ROX compatibility. If this file exists:

```text
$HOME/Choices/ROX-Filer/PuppyPin
```

EssoraWM updates the `<backdrop>` entry while preserving the current backdrop style, for example `Stretched`.

The wallpaper can be restored with:

```bash
jwm -wallpaper-restore
```

<p align="center">
<img src="assets/essorawm-wallpaper.png"/>
</p>

---

## EssoraWM changes

Current Essora-specific changes include:

- EssoraWM branding
- `jwm -version` identifies the fork as EssoraWM 0.1
- Task preview / thumbnail support for open windows
- Removed task text labels from open windows
- Cleaner task list behavior
- Pymenu launcher integration using `/usr/local/bin/pymenu`
- Wallpaper selector integrated into the `jwm` binary
- `jwm -wallpaper` command
- `jwm -wallpaper-restore` command
- Wallpaper selection from `/usr/share/backgrounds`
- Wallpaper state stored in `~/.config/essorawm/wallpaper`
- Puppy/ROX `PuppyPin` backdrop compatibility
- Optional custom window buttons and Essora theme assets
- Custom build script with automatic Debian package generation
- NLS/translations kept inside the main package build

Essora-specific source changes are marked with:

```c
/* agregado por josejp2424 */
```

or equivalent comments in scripts/configuration files.

---

## Building EssoraWM

EssoraWM includes a custom build script that compiles the window manager and creates a Debian package automatically.

### Build dependencies

On Debian/Devuan-based systems:

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

### Build and generate the package

```bash
chmod +x build-essora-jwm-deb.sh
./build-essora-jwm-deb.sh
```

The build script creates the package tree in:

```text
build-deb/essorawm_0.1_amd64/
```

The generated Debian package is placed in:

```text
deb-output/essorawm_0.1_amd64.deb
```

A copy is also left in the source directory:

```text
essorawm_0.1_amd64.deb
```

The build system automatically:

- runs `autogen.sh` when needed
- configures the source
- compiles EssoraWM
- builds NLS translations
- installs into a temporary package tree
- creates a `.deb` package
- keeps generated files inside the source build directory

---

## Runtime notes

EssoraWM keeps `/usr/bin/jwm` as the main binary for compatibility.

Pymenu is expected at:

```bash
/usr/local/bin/pymenu
```

Wallpaper tools are optional but recommended:

```bash
sudo apt install hsetroot feh xwallpaper
```

EssoraWM will use whichever supported wallpaper setter is available.

---

## Original project

EssoraWM is based on:

**JWM (Joe's Window Manager)**

Original author:

**Joe Wingbermuehle**

Original project:

https://joewing.net/projects/jwm/

License:

**MIT**

---

## EssoraWM

Modifications and Essora integration:

**josejp2424**
