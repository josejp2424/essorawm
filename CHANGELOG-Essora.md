## 0.1.7 persistent wallpaper window and real wbar restart

- Store the complete wallpaper selector as the X11 background pixmap of its own window.
- Restore every exposed rectangle server-side with `XClearArea`, avoiding blank selector windows under XLibre.
- Fix the selector at 820x500 and request backing store while it is mapped.
- Discover live wbar processes directly through `/proc` instead of relying on `ps -C`.
- Stop wbar with `kill(2)`, verify that every old PID disappeared, reapply the wallpaper, and start a genuinely new process.
- Log the old and new wbar PIDs so a failed restart is immediately visible.

## 0.1.7 wbar and desktop Expose correction

- Accumulate every X11 Expose rectangle, including events whose count is greater than zero.
- Redraw desktop launchers only after the complete Expose sequence has arrived.
- Stop manually clearing exposed desktop strips; ParentRelative restores the background.
- Restart wbar with verified live PIDs, ignoring zombie processes.
- Reapply the wallpaper after the old wbar exits and before the final fresh instance starts.

## 0.1.7 desktop repaint and Cancel follow-up

- Preserve the exact XLibre Expose rectangles instead of clearing their bounding box.
- Prevent desktop icons from disappearing while another window moves over them.
- Move the wallpaper Right button so it no longer overlaps Cancel.
- Use non-overlapping button hit boxes and trace Cancel clicks.

# EssoraWM changelog

## 0.1.7

- Fixed continuous flashing in the native wallpaper selector when another window passes over it.
- Coalesced pending X11 `Expose` events instead of repainting repeatedly for every queued event.
- Changed the native desktop to restore only the exposed region rather than redrawing the complete screen.
- Added a double-buffered wallpaper selector so already rendered previews are restored with `XCopyArea`.
- Prevented wallpaper thumbnails from being reopened and rescaled from disk during ordinary window exposure.
- Kept the desktop diagnostics and duplicate-`wbar` protection introduced during the 0.1.6 investigation.

## 0.1.6 diagnostic desktop fix

- Added an optional per-user runtime trace controlled by `essorawm-debug`.
- Logged EssoraWM restarts, wallpaper expose/configure events, native desktop
  reloads/redraws, drive changes and wbar process counts.
- Ignored desktop `ConfigureNotify` events that only change stacking order.
- Coalesced pending wallpaper `Expose` events before repainting.
- Made the wbar refresh wait for the old process and avoid launching a second
  instance when an autostart or supervisor has already restarted it.

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
