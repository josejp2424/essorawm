# EssoraWM changelog

## 0.1.6

- Cleaned the GitHub source tree by removing generated Autotools files, compiled translations, local Makefiles and package artifacts.
- Updated the Debian builder so all configuration, compilation and staging work happens in a temporary directory and only the final package is written to `deb-output/`.
- Reworked the main README from the original repository documentation to cover the native desktop, drive manager, portable file-manager launcher, SVG handling, translations and clean build workflow.
- Fixed SVG wallpapers appearing as **No preview** in the native wallpaper manager.
- Reused the safe SVG-to-PNG fallback already used by menu and desktop icons.
- SVG previews now work for the main image, neighboring carousel images and the selector window icon, even when EssoraWM is compiled without librsvg development headers.
- Kept native librsvg loading as the first choice and uses `rsvg-convert`, Inkscape or ImageMagick only as a fallback.

## 0.1.5

- Packaged the supplied EssoraWM wallpaper/configuration launchers, desktop entries and SVG icons.
- Replaced `example.jwmrc` with the supplied Essora configuration and corrected its malformed multimedia-key prefixes and Pymenu key actions.
- Added `/usr/bin/essorawm-filemanager`, which uses Puppy's `/usr/local/bin/defaultfilemanager` first and falls back to Debian alternatives, `xdg-open`, `gio` or an installed file manager.
- The Debian package creates `/usr/local/bin/defaultfilemanager` only when Puppy/the distribution has not already supplied it.
- Drive activation now mounts natively and always opens the resulting directory through the portable file-manager launcher.
- Replaced the square desktop selection blocks with one rounded icon-and-label frame, a small shadow and a clean outline.
- Opening an application, folder or drive clears the desktop selection immediately.
- Restored menu and submenu icons by searching the standard `apps`, `devices`, `places`, `actions`, `categories`, `mimetypes`, `status` and `emblems` theme directories.
- Added a safe SVG-to-PNG fallback when JWM is compiled without native librsvg support.
- Added native drive icons with themed images and Open/Mount/Unmount/Remove contextual actions.
- Added the **Drive icons** tab to `jwm -desktop-icons`.
- Added horizontal/vertical layout, left/center/right and top/center/bottom placement, icon size, spacing, offsets, labels, frames, reverse order and visibility controls.
- Applying a layout repositions only drive icons; applications keep their manually chosen coordinates.
- Drive icons remain freely draggable and their positions are stored in PuppyPin.
- Added translations for every language already shipped by JWM, plus Arabic, Catalan and Japanese.
- Added network-mount enumeration and the option to restore hidden drives.

## 0.1.3

- Added a native X11 desktop inside the `jwm` process.
- Added PuppyPin-compatible reading, creation and atomic writing.
- Added application/file/folder icon rendering and double-click activation.
- Added drag movement with persistent coordinates.
- Added contextual open/remove menu and root menu on empty desktop space.
- Added internal `_ESSORAWM_DESKTOP_RELOAD` handling for `jwm -desktop-icons` and wallpaper changes.
- Added screen-bound correction after resolution changes.
- Integrated EssoraFM drive placement and visibility settings.
- Added native block-device detection with Puppy layer, swap and EFI filtering.
- Added Puppy event-handler activation, `udisksctl` mounting and `/bin/mount` fallback.
- Removed the external GTK2 `desktop_drive_icons` component.
- Removed every runtime call to `rox -p`.
