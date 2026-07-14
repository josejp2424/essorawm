/**
 * @file desktopicons.h
 * @brief Native EssoraWM desktop and PuppyPin support.
 * agregado por josejp2424
 */

#ifndef DESKTOPICONS_H
#define DESKTOPICONS_H

void InitializeDesktopIcons(void);
void StartupDesktopIcons(void);
void ShutdownDesktopIcons(void);
void DestroyDesktopIcons(void);

/** Process an X event belonging to the native desktop. */
char ProcessDesktopIconsEvent(XEvent *event);

/** Reload PuppyPin, drive state, and repaint the native desktop. */
void ReloadDesktopIcons(void);

/** Tell a running EssoraWM process to reload its native desktop. */
void SendDesktopIconsReload(Display *dpy, Window root);

/** Reload configuration and arrange all drive icons using the selected layout. */
void SendDesktopIconsRelayout(Display *dpy, Window root);

#endif /* DESKTOPICONS_H */
