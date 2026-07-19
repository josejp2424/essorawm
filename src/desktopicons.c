/**
 * @file desktopicons.c
 * @brief Native EssoraWM desktop implementation compatible with PuppyPin.
 *
 * agregado por josejp2424
 *
 * This module intentionally does not copy ROX-Filer code.  It implements a
 * small X11 desktop inside EssoraWM and keeps the PuppyPin file format so
 * existing Puppy desktops and the Essora desktop-applications manager remain
 * compatible.
 */

#include "jwm.h"
#include "desktopicons.h"
#include "main.h"
#include "icon.h"
#include "font.h"
#include "color.h"
#include "root.h"
#include "command.h"
#include "settings.h"
#include "event.h"
#include "timing.h"
#include "misc.h"
#include "error.h"
#include "debug.h"

#ifndef MAKE_DEPEND
#  include <errno.h>
#  include <fcntl.h>
#  include <sys/stat.h>
#  include <sys/types.h>
#  include <unistd.h>
#  include <strings.h>
#endif

#define DESKTOP_ICON_DEFAULT_SIZE 48
#define DESKTOP_ICON_MIN_SIZE 24
#define DESKTOP_ICON_MAX_SIZE 128
#define DESKTOP_LABEL_WIDTH 116
#define DESKTOP_LABEL_GAP 5
#define DESKTOP_MENU_WIDTH 210
#define DESKTOP_MENU_ROW_HEIGHT 30
#define DESKTOP_MENU_ICON_SIZE 20
#define DESKTOP_DRIVE_SCAN_MS 3000
#define DESKTOP_MAX_PIN_SIZE (16U * 1024U * 1024U)
#define DESKTOP_RELOAD_ATOM "_ESSORAWM_DESKTOP_RELOAD"

#define FLAG_DRIVE       0x01
#define FLAG_REMOVABLE   0x02
#define FLAG_RUNTIME     0x04
#define FLAG_HIDDEN      0x08
#define FLAG_NETWORK     0x10

typedef struct DesktopIconNode {
   char *path;
   char *label;
   char *args;
   char *iconName;
   char *execCommand;
   char *device;
   char *mountPath;
   int x;
   int y;
   int drawX;
   int drawY;
   int drawWidth;
   int drawHeight;
   unsigned flags;
   IconNode *icon;
   struct DesktopIconNode *next;
} DesktopIconNode;

typedef struct DesktopConfig {
   char enabled;
   char showDrives;
   char showInternal;
   char showRemovable;
   char showNetwork;
   char showLabels;
   char showFrame;
   char reversePack;
   char vertical;
   int iconSize;
   int spacingX;
   int spacingY;
   int xOffset;
   int yOffset;
   double xPos;
   double yPos;
   char *openCommand;
} DesktopConfig;

typedef struct DriveInfo {
   char *name;
   char *device;
   char *label;
   char *fstype;
   char *mountpoint;
   char removable;
   char network;
   struct DriveInfo *next;
} DriveInfo;

static Window desktopWindow = None;
static Window desktopMenuWindow = None;
static GC desktopGC = None;
static Atom desktopReloadAtom = None;
static DesktopIconNode *desktopIcons = NULL;
static DesktopIconNode *selectedIcon = NULL;
static DesktopIconNode *pressedIcon = NULL;
static DesktopIconNode *menuIcon = NULL;
static int desktopMenuRows = 0;
static DesktopConfig desktopConfig;
static int pressRootX = 0;
static int pressRootY = 0;
static int pressIconX = 0;
static int pressIconY = 0;
static char draggingIcon = 0;
static Time lastClickTime = 0;
static DesktopIconNode *lastClickIcon = NULL;
static char driveCallbackRegistered = 0;
static char *lastDriveSignature = NULL;
static int desktopWindowWidth = 0;
static int desktopWindowHeight = 0;
static unsigned long desktopDrawCount = 0;
static Region desktopExposeRegion = NULL;

static void FreeDesktopConfig(void);
static void LoadDesktopConfig(void);
static void ReadIniFile(const char *path);
static void ApplyIniValue(const char *key, const char *value);
static char ParseBoolean(const char *value, char fallback);
static char *TrimLocal(char *value);
static char *GetHomeDirectory(void);
static char *GetPuppyPinPath(void);
static char *GetHiddenDrivesPath(void);
static char EnsureParentDirectory(const char *path);
static char EnsurePuppyPin(void);
static char *ReadWholeFile(const char *path, size_t *lengthOut);
static char WriteWholeFileAtomic(const char *path, const char *content,
                                 size_t length);
static char CopyFileSimple(const char *source, const char *target);
static char *ReadSavedWallpaper(void);
static void FreeDesktopIcon(DesktopIconNode *icon);
static void FreeDesktopIcons(void);
static void LoadPuppyPin(void);
static char *ParseXmlAttribute(const char *start, const char *end,
                               const char *name);
static int ParseXmlInteger(const char *start, const char *end,
                           const char *name, int fallback);
static char *XmlUnescape(const char *start, size_t length);
static char *XmlEscape(const char *text);
static void AddParsedIcon(const char *tagStart, const char *tagEnd,
                          const char *bodyStart, const char *bodyEnd);
static void LoadIconMetadata(DesktopIconNode *icon);
static char *DesktopEntryValue(const char *path, const char *key);
static char *CleanDesktopExec(const char *value);
static IconNode *LoadBestIcon(const DesktopIconNode *icon);
static char EndsWith(const char *value, const char *suffix);
static char IsDirectory(const char *path);
static char IsExecutable(const char *path);
static char *QuoteShell(const char *value);
static char CommandIsAvailable(const char *command);
static void NormalizeOpenCommand(void);
static void ActivateDesktopIcon(DesktopIconNode *icon);
static DesktopIconNode *FindIconAt(int x, int y);
static DesktopIconNode *FindIconByPath(const char *path);
static void DrawDesktop(void);
static void DrawDesktopExposedRegion(Region exposed);
static void DrawDesktopIcon(DesktopIconNode *icon);
static void FillDesktopRoundedRectangle(Drawable drawable, GC gc,
                                        int x, int y, int width, int height,
                                        int radius);
static void DrawDesktopRoundedRectangle(Drawable drawable, GC gc,
                                        int x, int y, int width, int height,
                                        int radius);
static void DrawDesktopSelection(int x, int y, int width, int height);
static void DrawDesktopMenu(void);
static void DrawDesktopMenuRow(int row, const char *label, const char *iconName);
static void OpenDesktopMenu(DesktopIconNode *icon, int rootX, int rootY);
static void CloseDesktopMenu(void);
static void RemoveDesktopIcon(DesktopIconNode *icon);
static char UpdatePuppyPinIcon(const DesktopIconNode *icon, char removeIcon);
static char ReplaceIntegerAttribute(char **text, size_t *length,
                                    const char *tagStart, const char *tagEnd,
                                    const char *name, int value);
static char InsertIconBeforeClosing(char **content, size_t *length,
                                    const DesktopIconNode *icon);
static void ClampDesktopIcons(char saveChanges);
static void ResizeDesktopWindow(void);
static void SetupDesktopWindowProperties(void);
static void RefreshDriveIcons(char saveMissing);
static DriveInfo *ReadDriveList(char **signatureOut);
static void AppendNetworkMounts(DriveInfo ***tail, char **signature,
                                size_t *signatureLength);
static unsigned long HashText(const char *text);
static void FreeDriveList(DriveInfo *drives);
static char *ParseLsblkValue(const char *line, const char *key);
static char ShouldShowDrive(const DriveInfo *drive);
static char IsTechnicalDrive(const DriveInfo *drive);
static char NameLooksRemovable(const char *value);
static char IsTechnicalVolume(const char *name, const char *path,
                              const char *device, const char *fstype,
                              const char *partlabel, const char *parttype);
static char DriveIsHidden(const char *name);
static void HideDrive(const char *name);
static DesktopIconNode *CreateDriveIcon(const DriveInfo *drive, int index);
static void UpdateDriveIconMetadata(DesktopIconNode *icon,
                                    const DriveInfo *drive);
static void MountAndOpenDrive(DesktopIconNode *icon);
static void UnmountDrive(DesktopIconNode *icon);
static void RelayoutDriveIcons(void);
static void DriveDefaultPosition(int index, int *x, int *y);
static void SignalDriveRefresh(const TimeType *now, int x, int y,
                               Window w, void *data);
static void SetDesktopSelection(DesktopIconNode *icon);

void InitializeDesktopIcons(void)
{
   memset(&desktopConfig, 0, sizeof(desktopConfig));
   desktopWindow = None;
   desktopMenuWindow = None;
   desktopGC = None;
   desktopIcons = NULL;
   selectedIcon = NULL;
   pressedIcon = NULL;
   menuIcon = NULL;
   desktopMenuRows = 0;
   draggingIcon = 0;
   lastClickTime = 0;
   lastClickIcon = NULL;
   driveCallbackRegistered = 0;
   lastDriveSignature = NULL;
   desktopWindowWidth = 0;
   desktopWindowHeight = 0;
   desktopDrawCount = 0;
   desktopExposeRegion = NULL;
}

void StartupDesktopIcons(void)
{
   XSetWindowAttributes attr;
   unsigned long mask;

   LoadDesktopConfig();
   EssoraTrace("desktop", "startup requested enabled=%d root=%dx%d",
               desktopConfig.enabled, rootWidth, rootHeight);
   if(!desktopConfig.enabled) {
      EssoraTrace("desktop", "native desktop disabled by configuration");
      return;
   }

   EnsurePuppyPin();
   LoadPuppyPin();
   RefreshDriveIcons(1);
   ClampDesktopIcons(1);

   attr.override_redirect = True;
   attr.background_pixmap = ParentRelative;
   attr.event_mask = ExposureMask | ButtonPressMask | ButtonReleaseMask
      | PointerMotionMask | StructureNotifyMask;
   mask = CWOverrideRedirect | CWBackPixmap | CWEventMask;

   desktopWindow = JXCreateWindow(display, rootWindow,
                                  0, 0, (unsigned)rootWidth, (unsigned)rootHeight, 0,
                                  rootDepth, InputOutput, rootVisual,
                                  mask, &attr);
   desktopGC = JXCreateGC(display, desktopWindow, 0, NULL);
   SetupDesktopWindowProperties();
   JXMapWindow(display, desktopWindow);
   XLowerWindow(display, desktopWindow);
   desktopWindowWidth = rootWidth;
   desktopWindowHeight = rootHeight;
   EssoraTrace("desktop", "window created id=0x%lx size=%dx%d",
               (unsigned long)desktopWindow,
               desktopWindowWidth, desktopWindowHeight);

   desktopReloadAtom = JXInternAtom(display, DESKTOP_RELOAD_ATOM, False);
   RegisterCallback(DESKTOP_DRIVE_SCAN_MS, SignalDriveRefresh, NULL);
   driveCallbackRegistered = 1;
   DrawDesktop();
}

void ShutdownDesktopIcons(void)
{
   if(desktopExposeRegion) {
      XDestroyRegion(desktopExposeRegion);
      desktopExposeRegion = NULL;
   }
   EssoraTrace("desktop", "shutdown window=0x%lx draws=%lu",
               (unsigned long)desktopWindow, desktopDrawCount);
   if(driveCallbackRegistered) {
      UnregisterCallback(SignalDriveRefresh, NULL);
      driveCallbackRegistered = 0;
   }
   CloseDesktopMenu();
   if(desktopGC != None) {
      JXFreeGC(display, desktopGC);
      desktopGC = None;
   }
   if(desktopWindow != None) {
      JXDestroyWindow(display, desktopWindow);
      desktopWindow = None;
   }
   FreeDesktopIcons();
   if(lastDriveSignature) {
      Release(lastDriveSignature);
      lastDriveSignature = NULL;
   }
}

void DestroyDesktopIcons(void)
{
   FreeDesktopIcons();
   FreeDesktopConfig();
   if(lastDriveSignature) {
      Release(lastDriveSignature);
      lastDriveSignature = NULL;
   }
}

void ReloadDesktopIcons(void)
{
   if(desktopWindow == None) {
      EssoraTrace("desktop", "reload ignored: no desktop window");
      return;
   }
   EssoraTrace("desktop", "reload begin window=0x%lx",
               (unsigned long)desktopWindow);
   LoadDesktopConfig();
   EnsurePuppyPin();
   LoadPuppyPin();
   RefreshDriveIcons(1);
   ClampDesktopIcons(1);
   ResizeDesktopWindow();
   DrawDesktop();
   EssoraTrace("desktop", "reload complete");
}

static void SendDesktopIconsMessage(Display *dpy, Window root, long action)
{
   XEvent event;
   Atom atom;
   if(!dpy || root == None) {
      return;
   }
   atom = XInternAtom(dpy, DESKTOP_RELOAD_ATOM, False);
   memset(&event, 0, sizeof(event));
   event.xclient.type = ClientMessage;
   event.xclient.display = dpy;
   event.xclient.window = root;
   event.xclient.message_type = atom;
   event.xclient.format = 32;
   event.xclient.data.l[0] = action;
   EssoraTrace("desktop", "send reload message action=%ld root=0x%lx",
               action, (unsigned long)root);
   XSendEvent(dpy, root, False, SubstructureNotifyMask | StructureNotifyMask,
              &event);
   XFlush(dpy);
}

void SendDesktopIconsReload(Display *dpy, Window root)
{
   SendDesktopIconsMessage(dpy, root, 1);
}

void SendDesktopIconsRelayout(Display *dpy, Window root)
{
   SendDesktopIconsMessage(dpy, root, 2);
}

char ProcessDesktopIconsEvent(XEvent *event)
{
   DesktopIconNode *icon;
   if(!event) {
      return 0;
   }

   if(event->type == ClientMessage
      && desktopReloadAtom != None
      && event->xclient.message_type == desktopReloadAtom) {
      EssoraTrace("desktop", "received reload message action=%ld window=0x%lx",
                  event->xclient.data.l[0],
                  (unsigned long)event->xclient.window);
      ReloadDesktopIcons();
      if(event->xclient.data.l[0] == 2) {
         RelayoutDriveIcons();
         ClampDesktopIcons(1);
         DrawDesktop();
      }
      return 1;
   }

   if(desktopWindow == None) {
      return 0;
   }

   if(desktopMenuWindow != None && event->xany.window == desktopMenuWindow) {
      if(event->type == Expose) {
         if(event->xexpose.count == 0) DrawDesktopMenu();
      } else if(event->type == ButtonPress) {
         const int row = event->xbutton.y / DESKTOP_MENU_ROW_HEIGHT;
         DesktopIconNode *target = menuIcon;
         CloseDesktopMenu();
         if(target) {
            if(row == 0) {
               ActivateDesktopIcon(target);
            } else if(target->flags & FLAG_DRIVE) {
               if(row == 1) {
                  if(target->mountPath && target->mountPath[0]) {
                     UnmountDrive(target);
                  } else {
                     MountAndOpenDrive(target);
                  }
               } else if(row == 2) {
                  RemoveDesktopIcon(target);
               }
            } else if(row == 1) {
               RemoveDesktopIcon(target);
            }
         }
      }
      return 1;
   }

   if(event->xany.window != desktopWindow) {
      if(desktopMenuWindow != None && event->type == ButtonPress) {
         CloseDesktopMenu();
      }
      return 0;
   }

   switch(event->type) {
   case Expose: {
      XEvent current = *event;
      int rectangles = 0;
      char sequenceComplete = 0;

      /*
       * X11 may deliver one exposed area as a sequence of rectangles.  The
       * count field says how many rectangles still follow.  The previous
       * implementation ignored every event whose count was not zero, so only
       * the final thin strip was redrawn and desktop launchers disappeared
       * until the window moved back over them.
       *
       * Accumulate every rectangle in the sequence and redraw once the final
       * count==0 event arrives.  Do not clear the area manually: the desktop
       * window uses ParentRelative as its background, and X already restores
       * that background before sending Expose.
       */
      if(!desktopExposeRegion) {
         desktopExposeRegion = XCreateRegion();
      }

      do {
         XRectangle rectangle;
         rectangle.x = (short)Max(0, current.xexpose.x);
         rectangle.y = (short)Max(0, current.xexpose.y);
         rectangle.width = (unsigned short)Max(0,
            Min(desktopWindowWidth,
                current.xexpose.x + current.xexpose.width) - rectangle.x);
         rectangle.height = (unsigned short)Max(0,
            Min(desktopWindowHeight,
                current.xexpose.y + current.xexpose.height) - rectangle.y);
         if(rectangle.width > 0 && rectangle.height > 0) {
            XUnionRectWithRegion(&rectangle, desktopExposeRegion,
                                 desktopExposeRegion);
            rectangles++;
         }
         if(current.xexpose.count == 0) {
            sequenceComplete = 1;
         }
      } while(JXCheckTypedWindowEvent(display, desktopWindow, Expose, &current));

      EssoraTrace("desktop",
                  "Expose window=0x%lx accumulated=%d complete=%d remaining=%d",
                  (unsigned long)event->xexpose.window, rectangles,
                  sequenceComplete, event->xexpose.count);

      if(sequenceComplete && desktopExposeRegion) {
         DrawDesktopExposedRegion(desktopExposeRegion);
         XDestroyRegion(desktopExposeRegion);
         desktopExposeRegion = NULL;
      }
      return 1;
   }
   case ConfigureNotify:
      if(event->xconfigure.width != desktopWindowWidth
         || event->xconfigure.height != desktopWindowHeight) {
         EssoraTrace("desktop",
                     "ConfigureNotify resize old=%dx%d new=%dx%d x=%d y=%d above=0x%lx",
                     desktopWindowWidth, desktopWindowHeight,
                     event->xconfigure.width, event->xconfigure.height,
                     event->xconfigure.x, event->xconfigure.y,
                     (unsigned long)event->xconfigure.above);
         desktopWindowWidth = event->xconfigure.width;
         desktopWindowHeight = event->xconfigure.height;
         DrawDesktop();
      } else {
         EssoraTrace("desktop",
                     "ConfigureNotify stacking-only ignored size=%dx%d above=0x%lx",
                     event->xconfigure.width, event->xconfigure.height,
                     (unsigned long)event->xconfigure.above);
      }
      return 1;
   case ButtonPress:
      CloseDesktopMenu();
      icon = FindIconAt(event->xbutton.x, event->xbutton.y);
      if(event->xbutton.button == Button1) {
         SetDesktopSelection(icon);
         pressedIcon = icon;
         draggingIcon = 0;
         pressRootX = event->xbutton.x_root;
         pressRootY = event->xbutton.y_root;
         if(icon) {
            pressIconX = icon->x;
            pressIconY = icon->y;
         }
         DrawDesktop();
      } else if(event->xbutton.button == Button3) {
         if(icon) {
            SetDesktopSelection(icon);
            OpenDesktopMenu(icon, event->xbutton.x_root,
                            event->xbutton.y_root);
            DrawDesktop();
         } else {
            ShowRootMenu(Button3, event->xbutton.x_root,
                         event->xbutton.y_root, 0);
         }
      } else if(!icon) {
         ShowRootMenu((int)event->xbutton.button, event->xbutton.x_root,
                      event->xbutton.y_root, 0);
      }
      return 1;
   case MotionNotify:
      if(pressedIcon && (event->xmotion.state & Button1Mask)) {
         const int dx = event->xmotion.x_root - pressRootX;
         const int dy = event->xmotion.y_root - pressRootY;
         if(!draggingIcon && (abs(dx) > MOVE_DELTA || abs(dy) > MOVE_DELTA)) {
            draggingIcon = 1;
         }
         if(draggingIcon) {
            pressedIcon->x = pressIconX + dx;
            pressedIcon->y = pressIconY + dy;
            ClampDesktopIcons(0);
            DrawDesktop();
         }
      }
      return 1;
   case ButtonRelease:
      if(event->xbutton.button == Button1 && pressedIcon) {
         DesktopIconNode *released = pressedIcon;
         if(draggingIcon) {
            UpdatePuppyPinIcon(released, 0);
         } else if(lastClickIcon == released
                   && event->xbutton.time != lastClickTime
                   && event->xbutton.time - lastClickTime
                      <= settings.doubleClickSpeed) {
            ActivateDesktopIcon(released);
            lastClickIcon = NULL;
            lastClickTime = 0;
         } else {
            lastClickIcon = released;
            lastClickTime = event->xbutton.time;
         }
         pressedIcon = NULL;
         draggingIcon = 0;
         DrawDesktop();
      }
      return 1;
   default:
      return 0;
   }
}

static void FreeDesktopConfig(void)
{
   if(desktopConfig.openCommand) {
      Release(desktopConfig.openCommand);
      desktopConfig.openCommand = NULL;
   }
}

static void LoadDesktopConfig(void)
{
   const char *home = getenv("HOME");
   char path[PATH_MAX];
   FreeDesktopConfig();
   memset(&desktopConfig, 0, sizeof(desktopConfig));
   desktopConfig.enabled = 1;
   desktopConfig.showDrives = 1;
   desktopConfig.showInternal = 1;
   desktopConfig.showRemovable = 1;
   desktopConfig.showNetwork = 0;
   desktopConfig.showLabels = 1;
   desktopConfig.showFrame = 0;
   desktopConfig.reversePack = 1;
   desktopConfig.vertical = 0;
   desktopConfig.iconSize = DESKTOP_ICON_DEFAULT_SIZE;
   desktopConfig.spacingX = 112;
   desktopConfig.spacingY = 126;
   desktopConfig.xOffset = 0;
   desktopConfig.yOffset = -40;
   desktopConfig.xPos = 0.0;
   desktopConfig.yPos = 1.0;
   desktopConfig.openCommand = CopyString("/usr/bin/essorawm-filemanager");

   if(home && home[0]) {
      snprintf(path, sizeof(path), "%s/.config/essorafm/config.ini", home);
      ReadIniFile(path);
      snprintf(path, sizeof(path), "%s/.config/essorawm/desktop.ini", home);
      ReadIniFile(path);
   }

   if(desktopConfig.iconSize < DESKTOP_ICON_MIN_SIZE)
      desktopConfig.iconSize = DESKTOP_ICON_MIN_SIZE;
   if(desktopConfig.iconSize > DESKTOP_ICON_MAX_SIZE)
      desktopConfig.iconSize = DESKTOP_ICON_MAX_SIZE;
   if(desktopConfig.spacingX < 48) desktopConfig.spacingX = 48;
   if(desktopConfig.spacingY < 48) desktopConfig.spacingY = 48;
   NormalizeOpenCommand();
}

static void ReadIniFile(const char *path)
{
   FILE *file;
   char line[4096];
   char active = 1;
   if(!path) return;
   file = fopen(path, "r");
   if(!file) return;
   while(fgets(line, sizeof(line), file)) {
      char *key;
      char *value;
      char *eq;
      key = TrimLocal(line);
      if(!key[0] || key[0] == '#' || key[0] == ';') continue;
      if(key[0] == '[') {
         char *close = strchr(key, ']');
         if(close) *close = 0;
         active = !strcasecmp(key + 1, "Main")
            || !strcasecmp(key + 1, "Desktop")
            || !strcasecmp(key + 1, "EssoraWM");
         continue;
      }
      if(!active) continue;
      eq = strchr(key, '=');
      if(!eq) continue;
      *eq = 0;
      value = TrimLocal(eq + 1);
      key = TrimLocal(key);
      ApplyIniValue(key, value);
   }
   fclose(file);
}

static void ApplyIniValue(const char *key, const char *value)
{
   if(!strcasecmp(key, "Enabled"))
      desktopConfig.enabled = ParseBoolean(value, desktopConfig.enabled);
   else if(!strcasecmp(key, "desktop_drive_icons"))
      desktopConfig.showDrives = ParseBoolean(value, desktopConfig.showDrives);
   else if(!strcasecmp(key, "desktop_drive_show_internal"))
      desktopConfig.showInternal = ParseBoolean(value, desktopConfig.showInternal);
   else if(!strcasecmp(key, "desktop_drive_show_removable"))
      desktopConfig.showRemovable = ParseBoolean(value, desktopConfig.showRemovable);
   else if(!strcasecmp(key, "desktop_drive_show_network"))
      desktopConfig.showNetwork = ParseBoolean(value, desktopConfig.showNetwork);
   else if(!strcasecmp(key, "desktop_drive_icon_size"))
      desktopConfig.iconSize = atoi(value);
   else if(!strcasecmp(key, "ShowLabels"))
      desktopConfig.showLabels = ParseBoolean(value, desktopConfig.showLabels);
   else if(!strcasecmp(key, "ShowFrame"))
      desktopConfig.showFrame = ParseBoolean(value, desktopConfig.showFrame);
   else if(!strcasecmp(key, "ReversePack"))
      desktopConfig.reversePack = ParseBoolean(value, desktopConfig.reversePack);
   else if(!strcasecmp(key, "Vertical"))
      desktopConfig.vertical = ParseBoolean(value, desktopConfig.vertical);
   else if(!strcasecmp(key, "SpacingX"))
      desktopConfig.spacingX = atoi(value);
   else if(!strcasecmp(key, "SpacingY"))
      desktopConfig.spacingY = atoi(value);
   else if(!strcasecmp(key, "XOffset"))
      desktopConfig.xOffset = atoi(value);
   else if(!strcasecmp(key, "YOffset"))
      desktopConfig.yOffset = atoi(value);
   else if(!strcasecmp(key, "XPos"))
      desktopConfig.xPos = atof(value);
   else if(!strcasecmp(key, "YPos"))
      desktopConfig.yPos = atof(value);
   else if(!strcasecmp(key, "OpenCommand") && value && value[0]) {
      if(desktopConfig.openCommand) Release(desktopConfig.openCommand);
      desktopConfig.openCommand = CopyString(value);
   }
}

static char ParseBoolean(const char *value, char fallback)
{
   if(!value) return fallback;
   if(!strcasecmp(value, "true") || !strcasecmp(value, "yes")
      || !strcasecmp(value, "on") || !strcmp(value, "1")) return 1;
   if(!strcasecmp(value, "false") || !strcasecmp(value, "no")
      || !strcasecmp(value, "off") || !strcmp(value, "0")) return 0;
   return fallback;
}

static char *TrimLocal(char *value)
{
   char *end;
   while(*value && isspace((unsigned char)*value)) value++;
   end = value + strlen(value);
   while(end > value && isspace((unsigned char)end[-1])) end--;
   *end = 0;
   return value;
}

static char *GetHomeDirectory(void)
{
   const char *home = getenv("HOME");
   return CopyString(home && home[0] ? home : "/root");
}

static char *GetPuppyPinPath(void)
{
   char *home = GetHomeDirectory();
   char *path = Allocate(strlen(home) + 40);
   sprintf(path, "%s/Choices/ROX-Filer/PuppyPin", home);
   Release(home);
   return path;
}

static char *GetHiddenDrivesPath(void)
{
   char *home = GetHomeDirectory();
   char *path = Allocate(strlen(home) + 48);
   sprintf(path, "%s/.config/essorawm/hidden-drives", home);
   Release(home);
   return path;
}

static char EnsureParentDirectory(const char *path)
{
   char copy[PATH_MAX];
   char *slash;
   char *p;
   if(!path || strlen(path) >= sizeof(copy)) return 0;
   strcpy(copy, path);
   slash = strrchr(copy, '/');
   if(!slash) return 1;
   *slash = 0;
   for(p = copy + 1; *p; p++) {
      if(*p == '/') {
         *p = 0;
         if(mkdir(copy, 0755) < 0 && errno != EEXIST) return 0;
         *p = '/';
      }
   }
   return mkdir(copy, 0755) == 0 || errno == EEXIST;
}

static char EnsurePuppyPin(void)
{
   char *path = GetPuppyPinPath();
   char *wallpaper;
   char *escaped;
   char *content;
   size_t length;
   char result;
   if(access(path, F_OK) == 0) {
      Release(path);
      return 1;
   }
   if(!EnsureParentDirectory(path)) {
      Release(path);
      return 0;
   }
   wallpaper = ReadSavedWallpaper();
   escaped = XmlEscape(wallpaper ? wallpaper : "");
   length = strlen(escaped) + 128;
   content = Allocate(length);
   snprintf(content, length,
            "<?xml version=\"1.0\"?>\n<pinboard>\n"
            "  <backdrop style=\"Stretched\">%s</backdrop>\n"
            "</pinboard>\n", escaped);
   result = WriteWholeFileAtomic(path, content, strlen(content));
   Release(content);
   Release(escaped);
   if(wallpaper) Release(wallpaper);
   Release(path);
   return result;
}

static char *ReadSavedWallpaper(void)
{
   char *home = GetHomeDirectory();
   char path[PATH_MAX];
   FILE *file;
   char line[PATH_MAX];
   char *result = NULL;
   snprintf(path, sizeof(path), "%s/.config/essorawm/wallpaper", home);
   Release(home);
   file = fopen(path, "r");
   if(file && fgets(line, sizeof(line), file)) {
      char *value = TrimLocal(line);
      if(value[0]) result = CopyString(value);
   }
   if(file) fclose(file);
   if(!result) result = CopyString("/usr/share/backgrounds/essora-1.1.svg");
   return result;
}

static char *ReadWholeFile(const char *path, size_t *lengthOut)
{
   FILE *file;
   long size;
   char *content;
   size_t got;
   if(lengthOut) *lengthOut = 0;
   file = fopen(path, "rb");
   if(!file) return NULL;
   if(fseek(file, 0, SEEK_END) < 0 || (size = ftell(file)) < 0
      || (unsigned long)size > DESKTOP_MAX_PIN_SIZE
      || fseek(file, 0, SEEK_SET) < 0) {
      fclose(file);
      return NULL;
   }
   content = Allocate((size_t)size + 1);
   got = fread(content, 1, (size_t)size, file);
   fclose(file);
   if(got != (size_t)size) {
      Release(content);
      return NULL;
   }
   content[got] = 0;
   if(lengthOut) *lengthOut = got;
   return content;
}

static char WriteWholeFileAtomic(const char *path, const char *content,
                                 size_t length)
{
   char temporary[PATH_MAX];
   char backup[PATH_MAX];
   struct stat st;
   mode_t mode = 0644;
   int fd;
   size_t written = 0;
   if(!path || !content || strlen(path) + 32 >= sizeof(temporary)) return 0;
   if(stat(path, &st) == 0) {
      mode = st.st_mode & 0777;
      snprintf(backup, sizeof(backup), "%s.essorawm-backup", path);
      CopyFileSimple(path, backup);
   }
   snprintf(temporary, sizeof(temporary), "%s.essorawm-XXXXXX", path);
   fd = mkstemp(temporary);
   if(fd < 0) return 0;
   fchmod(fd, mode);
   while(written < length) {
      ssize_t count = write(fd, content + written, length - written);
      if(count < 0) {
         if(errno == EINTR) continue;
         close(fd); unlink(temporary); return 0;
      }
      written += (size_t)count;
   }
   if(fsync(fd) < 0 || close(fd) < 0 || rename(temporary, path) < 0) {
      unlink(temporary);
      return 0;
   }
   return 1;
}

static char CopyFileSimple(const char *source, const char *target)
{
   int in = open(source, O_RDONLY);
   int out;
   char buffer[8192];
   ssize_t count;
   if(in < 0) return 0;
   out = open(target, O_WRONLY | O_CREAT | O_TRUNC, 0600);
   if(out < 0) { close(in); return 0; }
   while((count = read(in, buffer, sizeof(buffer))) > 0) {
      ssize_t offset = 0;
      while(offset < count) {
         ssize_t done = write(out, buffer + offset, (size_t)(count - offset));
         if(done < 0) {
            if(errno == EINTR) continue;
            close(in); close(out); return 0;
         }
         offset += done;
      }
   }
   close(in); close(out);
   return count == 0;
}

static void FreeDesktopIcon(DesktopIconNode *icon)
{
   if(!icon) return;
   if(icon->path) Release(icon->path);
   if(icon->label) Release(icon->label);
   if(icon->args) Release(icon->args);
   if(icon->iconName) Release(icon->iconName);
   if(icon->execCommand) Release(icon->execCommand);
   if(icon->device) Release(icon->device);
   if(icon->mountPath) Release(icon->mountPath);
   Release(icon);
}

static void FreeDesktopIcons(void)
{
   DesktopIconNode *icon;
   while(desktopIcons) {
      icon = desktopIcons->next;
      FreeDesktopIcon(desktopIcons);
      desktopIcons = icon;
   }
   selectedIcon = NULL;
   pressedIcon = NULL;
   menuIcon = NULL;
   lastClickIcon = NULL;
}

static void LoadPuppyPin(void)
{
   char *path = GetPuppyPinPath();
   char *content;
   size_t length;
   const char *cursor;
   FreeDesktopIcons();
   content = ReadWholeFile(path, &length);
   Release(path);
   if(!content) return;
   cursor = content;
   while((cursor = strstr(cursor, "<icon")) != NULL) {
      const char *tagEnd = strchr(cursor, '>');
      const char *bodyEnd;
      if(!tagEnd) break;
      bodyEnd = strstr(tagEnd + 1, "</icon>");
      if(!bodyEnd) break;
      AddParsedIcon(cursor, tagEnd, tagEnd + 1, bodyEnd);
      cursor = bodyEnd + 7;
   }
   Release(content);
}

static char *ParseXmlAttribute(const char *start, const char *end,
                               const char *name)
{
   char pattern[64];
   const char *found = start;
   const char *value;
   const char *close;
   snprintf(pattern, sizeof(pattern), "%s=\"", name);
   while((found = strstr(found, pattern)) != NULL && found < end) {
      if(found == start || isspace((unsigned char)found[-1])) {
         value = found + strlen(pattern);
         close = strchr(value, '"');
         if(close && close <= end) return XmlUnescape(value, (size_t)(close-value));
      }
      found += strlen(pattern);
   }
   return NULL;
}

static int ParseXmlInteger(const char *start, const char *end,
                           const char *name, int fallback)
{
   char *value = ParseXmlAttribute(start, end, name);
   int result = value ? atoi(value) : fallback;
   if(value) Release(value);
   return result;
}

static char *XmlUnescape(const char *start, size_t length)
{
   char *result = Allocate(length + 1);
   size_t in = 0, out = 0;
   while(in < length) {
      if(start[in] == '&') {
         if(in + 5 <= length && !memcmp(start + in, "&amp;", 5)) {
            result[out++] = '&'; in += 5; continue;
         }
         if(in + 4 <= length && !memcmp(start + in, "&lt;", 4)) {
            result[out++] = '<'; in += 4; continue;
         }
         if(in + 4 <= length && !memcmp(start + in, "&gt;", 4)) {
            result[out++] = '>'; in += 4; continue;
         }
         if(in + 6 <= length && !memcmp(start + in, "&quot;", 6)) {
            result[out++] = '"'; in += 6; continue;
         }
         if(in + 6 <= length && !memcmp(start + in, "&apos;", 6)) {
            result[out++] = '\''; in += 6; continue;
         }
      }
      result[out++] = start[in++];
   }
   result[out] = 0;
   return result;
}

static char *XmlEscape(const char *text)
{
   const char *p;
   size_t length = 1;
   char *result;
   char *out;
   if(!text) return CopyString("");
   for(p = text; *p; p++) {
      switch(*p) {
      case '&': length += 5; break;
      case '<': case '>': length += 4; break;
      case '"': case '\'': length += 6; break;
      default: length++; break;
      }
   }
   result = Allocate(length);
   out = result;
   for(p = text; *p; p++) {
      const char *replacement = NULL;
      switch(*p) {
      case '&': replacement = "&amp;"; break;
      case '<': replacement = "&lt;"; break;
      case '>': replacement = "&gt;"; break;
      case '"': replacement = "&quot;"; break;
      case '\'': replacement = "&apos;"; break;
      default: *out++ = *p; break;
      }
      if(replacement) {
         size_t count = strlen(replacement);
         memcpy(out, replacement, count);
         out += count;
      }
   }
   *out = 0;
   return result;
}

static void AddParsedIcon(const char *tagStart, const char *tagEnd,
                          const char *bodyStart, const char *bodyEnd)
{
   DesktopIconNode *icon = Allocate(sizeof(DesktopIconNode));
   memset(icon, 0, sizeof(DesktopIconNode));
   icon->x = ParseXmlInteger(tagStart, tagEnd, "x", 48);
   icon->y = ParseXmlInteger(tagStart, tagEnd, "y", 48);
   icon->label = ParseXmlAttribute(tagStart, tagEnd, "label");
   icon->args = ParseXmlAttribute(tagStart, tagEnd, "args");
   icon->path = XmlUnescape(bodyStart, (size_t)(bodyEnd - bodyStart));
   {
      char *trimmed = TrimLocal(icon->path);
      if(trimmed != icon->path) memmove(icon->path, trimmed, strlen(trimmed)+1);
   }
   if(!icon->label || !icon->label[0]) {
      const char *base = strrchr(icon->path, '/');
      if(icon->label) Release(icon->label);
      icon->label = CopyString(base ? base + 1 : icon->path);
   }
   if(strstr(icon->path, "/tmp/pup_event_frontend/drive_")
      || (icon->args && (!strncmp(icon->args, "drive", 5)
                        || !strncmp(icon->args, "usbdrv", 6)))) {
      icon->flags |= FLAG_DRIVE;
      if(icon->args && !strncmp(icon->args, "usbdrv", 6))
         icon->flags |= FLAG_REMOVABLE;
   }
   LoadIconMetadata(icon);
   icon->next = desktopIcons;
   desktopIcons = icon;
}

static void LoadIconMetadata(DesktopIconNode *icon)
{
   char *value;
   if(!icon || !icon->path) return;
   if(EndsWith(icon->path, ".desktop")) {
      value = DesktopEntryValue(icon->path, "Icon");
      if(value) icon->iconName = value;
      value = DesktopEntryValue(icon->path, "Exec");
      if(value) {
         icon->execCommand = CleanDesktopExec(value);
         Release(value);
      }
      if((!icon->label || !icon->label[0])) {
         value = DesktopEntryValue(icon->path, "Name");
         if(value) {
            if(icon->label) Release(icon->label);
            icon->label = value;
         }
      }
   }
   icon->icon = LoadBestIcon(icon);
}

static char *DesktopEntryValue(const char *path, const char *key)
{
   FILE *file = fopen(path, "r");
   char line[8192];
   char section = 0;
   size_t keyLength = strlen(key);
   if(!file) return NULL;
   while(fgets(line, sizeof(line), file)) {
      char *value = TrimLocal(line);
      if(value[0] == '[') {
         section = !strncmp(value, "[Desktop Entry]", 15);
         continue;
      }
      if(section && !strncmp(value, key, keyLength) && value[keyLength] == '=') {
         char *result = CopyString(TrimLocal(value + keyLength + 1));
         fclose(file);
         return result;
      }
   }
   fclose(file);
   return NULL;
}

static char *CleanDesktopExec(const char *value)
{
   size_t length = strlen(value);
   char *result = Allocate(length + 1);
   size_t in = 0, out = 0;
   while(in < length) {
      if(value[in] == '%' && in + 1 < length) {
         if(value[in + 1] == '%') result[out++] = '%';
         in += 2;
         continue;
      }
      result[out++] = value[in++];
   }
   result[out] = 0;
   return result;
}

static IconNode *LoadBestIcon(const DesktopIconNode *icon)
{
   IconNode *result = NULL;
   const char *fallback = "application-x-executable";
   if(icon->iconName && icon->iconName[0])
      result = LoadNamedIcon(icon->iconName, 1, 1);
   if(!result || result == &emptyIcon) {
      if(icon->flags & FLAG_NETWORK) {
         fallback = "folder-remote";
      } else if(icon->flags & FLAG_DRIVE) {
         fallback = (icon->flags & FLAG_REMOVABLE)
            ? "drive-removable-media-usb" : "drive-harddisk";
      } else if(IsDirectory(icon->path)) {
         fallback = "folder";
      } else if(!EndsWith(icon->path, ".desktop")) {
         fallback = IsExecutable(icon->path)
            ? "application-x-executable" : "text-x-generic";
      }
      result = LoadNamedIcon(fallback, 1, 1);
   }
   return result;
}

static char EndsWith(const char *value, const char *suffix)
{
   size_t valueLength, suffixLength;
   if(!value || !suffix) return 0;
   valueLength = strlen(value);
   suffixLength = strlen(suffix);
   return valueLength >= suffixLength
      && !strcasecmp(value + valueLength - suffixLength, suffix);
}

static char IsDirectory(const char *path)
{
   struct stat st;
   return path && stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static char IsExecutable(const char *path)
{
   return path && access(path, X_OK) == 0;
}

static char *QuoteShell(const char *value)
{
   const char *p;
   size_t length = 3;
   char *result;
   char *out;
   for(p = value; *p; p++) length += *p == '\'' ? 4 : 1;
   result = Allocate(length);
   out = result;
   *out++ = '\'';
   for(p = value; *p; p++) {
      if(*p == '\'') {
         memcpy(out, "'\\''", 4);
         out += 4;
      } else {
         *out++ = *p;
      }
   }
   *out++ = '\'';
   *out = 0;
   return result;
}

static char CommandIsAvailable(const char *command)
{
   const char *start;
   const char *end;
   char executable[PATH_MAX];
   size_t length;
   const char *path;
   const char *segment;
   if(!command) return 0;
   start = command;
   while(*start && isspace((unsigned char)*start)) start++;
   if(!*start) return 0;
   if(*start == '\'' || *start == '"') {
      const char quote = *start++;
      end = strchr(start, quote);
      if(!end) return 0;
   } else {
      end = start;
      while(*end && !isspace((unsigned char)*end)) end++;
   }
   length = (size_t)(end - start);
   if(length == 0 || length >= sizeof(executable)) return 0;
   memcpy(executable, start, length);
   executable[length] = 0;
   if(strchr(executable, '/')) return access(executable, X_OK) == 0;
   path = getenv("PATH");
   if(!path || !path[0]) path = "/usr/local/bin:/usr/bin:/bin";
   segment = path;
   while(segment) {
      const char *colon = strchr(segment, ':');
      const size_t dirLength = colon ? (size_t)(colon - segment) : strlen(segment);
      char candidate[PATH_MAX];
      if(dirLength == 0) {
         snprintf(candidate, sizeof(candidate), "./%s", executable);
      } else if(dirLength + 1 + length < sizeof(candidate)) {
         memcpy(candidate, segment, dirLength);
         candidate[dirLength] = '/';
         memcpy(candidate + dirLength + 1, executable, length + 1);
      } else {
         candidate[0] = 0;
      }
      if(candidate[0] && access(candidate, X_OK) == 0) return 1;
      segment = colon ? colon + 1 : NULL;
   }
   return 0;
}

static void NormalizeOpenCommand(void)
{
   if(CommandIsAvailable(desktopConfig.openCommand)) return;
   if(desktopConfig.openCommand) Release(desktopConfig.openCommand);
   desktopConfig.openCommand = CopyString("/usr/bin/essorawm-filemanager");
}

static void ActivateDesktopIcon(DesktopIconNode *icon)
{
   char *quoted;
   char *command;
   size_t length;
   if(!icon || !icon->path) return;
   if(icon->flags & FLAG_DRIVE) {
      MountAndOpenDrive(icon);
      return;
   }
   SetDesktopSelection(NULL);
   lastClickIcon = NULL;
   lastClickTime = 0;
   DrawDesktop();
   if(icon->execCommand && icon->execCommand[0]) {
      RunCommand(icon->execCommand);
      return;
   }
   quoted = QuoteShell(icon->path);
   if(IsDirectory(icon->path)) {
      length = strlen(desktopConfig.openCommand) + strlen(quoted) + 4;
      command = Allocate(length);
      snprintf(command, length, "%s %s", desktopConfig.openCommand, quoted);
   } else if(IsExecutable(icon->path) && !EndsWith(icon->path, ".desktop")) {
      length = strlen(quoted) + 1;
      command = Allocate(length);
      strcpy(command, quoted);
   } else {
      length = strlen(desktopConfig.openCommand) + strlen(quoted) + 4;
      command = Allocate(length);
      snprintf(command, length, "%s %s", desktopConfig.openCommand, quoted);
   }
   RunCommand(command);
   Release(command);
   Release(quoted);
}

static DesktopIconNode *FindIconAt(int x, int y)
{
   DesktopIconNode *icon;
   for(icon = desktopIcons; icon; icon = icon->next) {
      if(icon->flags & FLAG_HIDDEN) continue;
      if(x >= icon->drawX && x <= icon->drawX + icon->drawWidth
         && y >= icon->drawY && y <= icon->drawY + icon->drawHeight) {
         return icon;
      }
   }
   return NULL;
}

static DesktopIconNode *FindIconByPath(const char *path)
{
   DesktopIconNode *icon;
   for(icon = desktopIcons; icon; icon = icon->next) {
      if(icon->path && path && !strcmp(icon->path, path)) return icon;
   }
   return NULL;
}

static void DrawDesktop(void)
{
   DesktopIconNode *icon;
   if(desktopWindow == None) return;
   desktopDrawCount++;
   EssoraTrace("desktop", "draw #%lu window=0x%lx",
               desktopDrawCount, (unsigned long)desktopWindow);
   JXClearWindow(display, desktopWindow);
   for(icon = desktopIcons; icon; icon = icon->next) {
      if(!(icon->flags & FLAG_HIDDEN)) DrawDesktopIcon(icon);
   }
   JXFlush(display);
}

/*
 * Redraw icons that intersect the exact X expose region.  The background is
 * cleared per individual Expose rectangle in ProcessDesktopIconsEvent; this
 * function deliberately never clears the bounding box of the whole region.
 */
static void DrawDesktopExposedRegion(Region exposed)
{
   DesktopIconNode *icon;
   const int padding = 8;

   if(desktopWindow == None || !exposed) {
      return;
   }

   desktopDrawCount++;
   EssoraTrace("desktop", "draw-exposed #%lu window=0x%lx",
               desktopDrawCount, (unsigned long)desktopWindow);

   for(icon = desktopIcons; icon; icon = icon->next) {
      int iconLeft;
      int iconTop;
      unsigned int iconWidth;
      unsigned int iconHeight;

      if(icon->flags & FLAG_HIDDEN) {
         continue;
      }

      iconLeft = icon->drawX - padding;
      iconTop = icon->drawY - padding;
      iconWidth = (unsigned)Max(1, icon->drawWidth + padding * 2);
      iconHeight = (unsigned)Max(1, icon->drawHeight + padding * 2);

      if(XRectInRegion(exposed, iconLeft, iconTop,
                       iconWidth, iconHeight) != RectangleOut) {
         DrawDesktopIcon(icon);
      }
   }
   JXFlush(display);
}

static void FillDesktopRoundedRectangle(Drawable drawable, GC gc,
                                        int x, int y, int width, int height,
                                        int radius)
{
   XRectangle rectangles[3];
   XArc arcs[4];
   if(width <= 0 || height <= 0) return;
   radius = Max(0, Min(radius, Min(width, height) / 2));
   if(radius == 0) {
      JXFillRectangle(display, drawable, gc, x, y,
                      (unsigned)width, (unsigned)height);
      return;
   }
   rectangles[0].x = (short)(x + radius);
   rectangles[0].y = (short)y;
   rectangles[0].width = (unsigned short)Max(1, width - radius * 2);
   rectangles[0].height = (unsigned short)radius;
   rectangles[1].x = (short)x;
   rectangles[1].y = (short)(y + radius);
   rectangles[1].width = (unsigned short)width;
   rectangles[1].height = (unsigned short)Max(1, height - radius * 2);
   rectangles[2].x = (short)(x + radius);
   rectangles[2].y = (short)(y + height - radius);
   rectangles[2].width = (unsigned short)Max(1, width - radius * 2);
   rectangles[2].height = (unsigned short)radius;
   JXFillRectangles(display, drawable, gc, rectangles, 3);

   arcs[0].x = (short)x;
   arcs[0].y = (short)y;
   arcs[0].width = (unsigned short)(radius * 2);
   arcs[0].height = (unsigned short)(radius * 2);
   arcs[0].angle1 = 90 * 64;
   arcs[0].angle2 = 90 * 64;
   arcs[1].x = (short)(x + width - radius * 2 - 1);
   arcs[1].y = (short)y;
   arcs[1].width = (unsigned short)(radius * 2);
   arcs[1].height = (unsigned short)(radius * 2);
   arcs[1].angle1 = 0;
   arcs[1].angle2 = 90 * 64;
   arcs[2].x = (short)x;
   arcs[2].y = (short)(y + height - radius * 2 - 1);
   arcs[2].width = (unsigned short)(radius * 2);
   arcs[2].height = (unsigned short)(radius * 2);
   arcs[2].angle1 = 180 * 64;
   arcs[2].angle2 = 90 * 64;
   arcs[3].x = (short)(x + width - radius * 2 - 1);
   arcs[3].y = (short)(y + height - radius * 2 - 1);
   arcs[3].width = (unsigned short)(radius * 2);
   arcs[3].height = (unsigned short)(radius * 2);
   arcs[3].angle1 = 270 * 64;
   arcs[3].angle2 = 90 * 64;
   JXFillArcs(display, drawable, gc, arcs, 4);
}

static void DrawDesktopRoundedRectangle(Drawable drawable, GC gc,
                                        int x, int y, int width, int height,
                                        int radius)
{
   XSegment segments[4];
   XArc arcs[4];
   if(width <= 0 || height <= 0) return;
   radius = Max(0, Min(radius, Min(width, height) / 2));
   if(radius == 0) {
      JXDrawRectangle(display, drawable, gc, x, y,
                      (unsigned)width, (unsigned)height);
      return;
   }
   segments[0].x1 = (short)(x + radius); segments[0].y1 = (short)y;
   segments[0].x2 = (short)(x + width - radius); segments[0].y2 = (short)y;
   segments[1].x1 = (short)(x + radius); segments[1].y1 = (short)(y + height);
   segments[1].x2 = (short)(x + width - radius); segments[1].y2 = (short)(y + height);
   segments[2].x1 = (short)x; segments[2].y1 = (short)(y + radius);
   segments[2].x2 = (short)x; segments[2].y2 = (short)(y + height - radius);
   segments[3].x1 = (short)(x + width); segments[3].y1 = (short)(y + radius);
   segments[3].x2 = (short)(x + width); segments[3].y2 = (short)(y + height - radius);
   JXDrawSegments(display, drawable, gc, segments, 4);
   arcs[0].x = (short)x; arcs[0].y = (short)y;
   arcs[0].width = arcs[0].height = (unsigned short)(radius * 2);
   arcs[0].angle1 = 90 * 64; arcs[0].angle2 = 90 * 64;
   arcs[1].x = (short)(x + width - radius * 2);
   arcs[1].y = (short)y;
   arcs[1].width = arcs[1].height = (unsigned short)(radius * 2);
   arcs[1].angle1 = 0; arcs[1].angle2 = 90 * 64;
   arcs[2].x = (short)x;
   arcs[2].y = (short)(y + height - radius * 2);
   arcs[2].width = arcs[2].height = (unsigned short)(radius * 2);
   arcs[2].angle1 = 180 * 64; arcs[2].angle2 = 90 * 64;
   arcs[3].x = (short)(x + width - radius * 2);
   arcs[3].y = (short)(y + height - radius * 2);
   arcs[3].width = arcs[3].height = (unsigned short)(radius * 2);
   arcs[3].angle1 = 270 * 64; arcs[3].angle2 = 90 * 64;
   JXDrawArcs(display, drawable, gc, arcs, 4);
}

static void DrawDesktopSelection(int x, int y, int width, int height)
{
   /* A small offset shadow and a rounded unified selection are much cleaner
    * than separate square blocks around the icon and its label. */
   JXSetForeground(display, desktopGC, colors[COLOR_MENU_DOWN]);
   FillDesktopRoundedRectangle(desktopWindow, desktopGC,
                               x + 3, y + 4, width, height, 8);
   JXSetForeground(display, desktopGC, colors[COLOR_MENU_ACTIVE_BG1]);
   FillDesktopRoundedRectangle(desktopWindow, desktopGC,
                               x, y, width, height, 8);
   JXSetForeground(display, desktopGC, colors[COLOR_MENU_ACTIVE_FG]);
   DrawDesktopRoundedRectangle(desktopWindow, desktopGC,
                               x, y, width - 1, height - 1, 8);
}

static void DrawDesktopIcon(DesktopIconNode *icon)
{
   int size;
   const int textHeight = GetStringHeight(FONT_MENU);
   const int labelHeight = desktopConfig.showLabels ? textHeight + 6 : 0;
   int iconX;
   int iconY;
   int labelWidth = 0;
   int labelX;
   int labelY;
   int contentLeft;
   int contentRight;
   int contentBottom;
   if(!icon) return;
   size = (icon->flags & FLAG_DRIVE)
      ? desktopConfig.iconSize : DESKTOP_ICON_DEFAULT_SIZE;
   iconX = icon->x - size / 2;
   iconY = icon->y;
   labelX = iconX;
   labelY = iconY + size + DESKTOP_LABEL_GAP;
   contentLeft = iconX;
   contentRight = iconX + size;
   contentBottom = iconY + size;

   if(desktopConfig.showLabels && icon->label && icon->label[0]) {
      labelWidth = Min(DESKTOP_LABEL_WIDTH,
                       GetStringWidth(FONT_MENU, icon->label) + 10);
      labelWidth = Max(labelWidth, 30);
      labelX = icon->x - labelWidth / 2;
      contentLeft = Min(contentLeft, labelX);
      contentRight = Max(contentRight, labelX + labelWidth);
      contentBottom = labelY + labelHeight;
   }

   icon->drawX = contentLeft;
   icon->drawY = iconY;
   icon->drawWidth = contentRight - contentLeft;
   icon->drawHeight = contentBottom - iconY;

   if(selectedIcon == icon) {
      const int padding = 5;
      DrawDesktopSelection(contentLeft - padding, iconY - padding,
                           contentRight - contentLeft + padding * 2,
                           contentBottom - iconY + padding * 2);
   }
   if(icon->icon) {
      PutIcon(icon->icon, desktopWindow, (long)colors[COLOR_TRAY_FG],
              iconX, iconY, size, size);
   }
   if(desktopConfig.showLabels && icon->label && icon->label[0]) {
      if(desktopConfig.showFrame && selectedIcon != icon) {
         JXSetForeground(display, desktopGC, colors[COLOR_MENU_BG]);
         FillDesktopRoundedRectangle(desktopWindow, desktopGC,
                                     labelX, labelY, labelWidth,
                                     textHeight + 5, 4);
      }
      RenderString(desktopWindow, FONT_MENU,
                   selectedIcon == icon ? COLOR_MENU_ACTIVE_FG : COLOR_TRAY_FG,
                   labelX + 4, labelY + 2, labelWidth - 8, icon->label);
   }
}

static void OpenDesktopMenu(DesktopIconNode *icon, int rootX, int rootY)
{
   XSetWindowAttributes attr;
   unsigned long mask;
   int x = rootX;
   int y = rootY;
   int height;
   CloseDesktopMenu();
   menuIcon = icon;
   desktopMenuRows = icon && (icon->flags & FLAG_DRIVE) ? 3 : 2;
   height = desktopMenuRows * DESKTOP_MENU_ROW_HEIGHT;
   if(x + DESKTOP_MENU_WIDTH > rootWidth) x = rootWidth - DESKTOP_MENU_WIDTH;
   if(y + height > rootHeight) y = rootHeight - height;
   x = Max(0, x); y = Max(0, y);
   attr.override_redirect = True;
   attr.background_pixel = colors[COLOR_MENU_BG];
   attr.border_pixel = colors[COLOR_MENU_UP];
   attr.event_mask = ExposureMask | ButtonPressMask;
   mask = CWOverrideRedirect | CWBackPixel | CWBorderPixel | CWEventMask;
   desktopMenuWindow = JXCreateWindow(display, rootWindow, x, y,
                                      DESKTOP_MENU_WIDTH, height, 1,
                                      rootDepth, InputOutput, rootVisual,
                                      mask, &attr);
   JXMapRaised(display, desktopMenuWindow);
   DrawDesktopMenu();
}

static void DrawDesktopMenu(void)
{
   int row;
   if(desktopMenuWindow == None || !menuIcon) return;
   JXSetForeground(display, desktopGC, colors[COLOR_MENU_BG]);
   JXFillRectangle(display, desktopMenuWindow, desktopGC, 0, 0,
                   DESKTOP_MENU_WIDTH,
                   (unsigned)(desktopMenuRows * DESKTOP_MENU_ROW_HEIGHT));
   DrawDesktopMenuRow(0, _("Open"), "document-open");
   if(menuIcon->flags & FLAG_DRIVE) {
      if(menuIcon->mountPath && menuIcon->mountPath[0]) {
         DrawDesktopMenuRow(1, _("Unmount"), "media-eject");
      } else {
         DrawDesktopMenuRow(1, _("Mount"), "drive-harddisk");
      }
      DrawDesktopMenuRow(2, _("Remove from desktop"), "list-remove");
   } else {
      DrawDesktopMenuRow(1, _("Remove from desktop"), "list-remove");
   }
   JXSetForeground(display, desktopGC, colors[COLOR_MENU_DOWN]);
   for(row = 1; row < desktopMenuRows; row++) {
      JXDrawLine(display, desktopMenuWindow, desktopGC, 0,
                 row * DESKTOP_MENU_ROW_HEIGHT, DESKTOP_MENU_WIDTH,
                 row * DESKTOP_MENU_ROW_HEIGHT);
   }
   JXFlush(display);
}

static void DrawDesktopMenuRow(int row, const char *label, const char *iconName)
{
   IconNode *rowIcon = LoadNamedIcon(iconName, 1, 1);
   const int y = row * DESKTOP_MENU_ROW_HEIGHT;
   if(rowIcon && rowIcon != &emptyIcon) {
      PutIcon(rowIcon, desktopMenuWindow, (long)colors[COLOR_MENU_FG],
              7, y + (DESKTOP_MENU_ROW_HEIGHT - DESKTOP_MENU_ICON_SIZE) / 2,
              DESKTOP_MENU_ICON_SIZE, DESKTOP_MENU_ICON_SIZE);
   }
   RenderString(desktopMenuWindow, FONT_MENU, COLOR_MENU_FG,
                34, y + 7, DESKTOP_MENU_WIDTH - 42, label);
}

static void CloseDesktopMenu(void)
{
   if(desktopMenuWindow != None) {
      JXDestroyWindow(display, desktopMenuWindow);
      desktopMenuWindow = None;
   }
   menuIcon = NULL;
   desktopMenuRows = 0;
}

static void RemoveDesktopIcon(DesktopIconNode *icon)
{
   if(!icon) return;
   if(icon->flags & FLAG_DRIVE) {
      const char *base = strrchr(icon->path ? icon->path : "", '_');
      if(base && base[1]) HideDrive(base + 1);
   }
   if(UpdatePuppyPinIcon(icon, 1)) {
      ReloadDesktopIcons();
   }
}

static char UpdatePuppyPinIcon(const DesktopIconNode *icon, char removeIcon)
{
   char *pinPath = GetPuppyPinPath();
   char *content;
   size_t length;
   char *cursor;
   char found = 0;
   char result = 0;
   content = ReadWholeFile(pinPath, &length);
   if(!content) {
      Release(pinPath);
      return 0;
   }
   cursor = content;
   while((cursor = strstr(cursor, "<icon")) != NULL) {
      char *tagEnd = strchr(cursor, '>');
      char *bodyEnd;
      char *decoded;
      if(!tagEnd) break;
      bodyEnd = strstr(tagEnd + 1, "</icon>");
      if(!bodyEnd) break;
      decoded = XmlUnescape(tagEnd + 1, (size_t)(bodyEnd - (tagEnd + 1)));
      if(decoded) {
         char *trimmed = TrimLocal(decoded);
         if(!strcmp(trimmed, icon->path)) {
            found = 1;
            if(removeIcon) {
               char *start = cursor;
               char *end = bodyEnd + 7;
               while(start > content && start[-1] != '\n') start--;
               while(*end == ' ' || *end == '\t' || *end == '\r') end++;
               if(*end == '\n') end++;
               memmove(start, end, length - (size_t)(end - content) + 1);
               length -= (size_t)(end - start);
            } else {
               const ptrdiff_t startOffset = cursor - content;
               const ptrdiff_t endOffset = tagEnd - content;
               if(!ReplaceIntegerAttribute(&content, &length,
                                           content + startOffset,
                                           content + endOffset,
                                           "x", icon->x)) break;
               cursor = content + startOffset;
               tagEnd = strchr(cursor, '>');
               if(!tagEnd || !ReplaceIntegerAttribute(&content, &length,
                                                       cursor, tagEnd,
                                                       "y", icon->y)) break;
            }
            Release(decoded);
            break;
         }
         Release(decoded);
      }
      cursor = bodyEnd + 7;
   }
   if(!found && !removeIcon) {
      if(InsertIconBeforeClosing(&content, &length, icon)) found = 1;
   }
   if(found) result = WriteWholeFileAtomic(pinPath, content, length);
   Release(content);
   Release(pinPath);
   return result;
}

static char ReplaceIntegerAttribute(char **text, size_t *length,
                                    const char *tagStart, const char *tagEnd,
                                    const char *name, int value)
{
   char pattern[32];
   const char *found;
   const char *numberStart;
   const char *numberEnd;
   char number[32];
   size_t prefix;
   size_t suffix;
   size_t oldLength;
   size_t newLength;
   char *result;
   snprintf(pattern, sizeof(pattern), "%s=\"", name);
   found = strstr(tagStart, pattern);
   if(found && found < tagEnd) {
      numberStart = found + strlen(pattern);
      numberEnd = strchr(numberStart, '"');
      if(!numberEnd || numberEnd > tagEnd) return 0;
      snprintf(number, sizeof(number), "%d", value);
      prefix = (size_t)(numberStart - *text);
      suffix = *length - (size_t)(numberEnd - *text);
      oldLength = (size_t)(numberEnd - numberStart);
      newLength = strlen(number);
      result = Allocate(*length - oldLength + newLength + 1);
      memcpy(result, *text, prefix);
      memcpy(result + prefix, number, newLength);
      memcpy(result + prefix + newLength, numberEnd, suffix + 1);
      Release(*text);
      *text = result;
      *length = *length - oldLength + newLength;
      return 1;
   }
   return 0;
}

static char InsertIconBeforeClosing(char **content, size_t *length,
                                    const DesktopIconNode *icon)
{
   char *closing = strstr(*content, "</pinboard>");
   char *escapedPath;
   char *escapedLabel;
   char *escapedArgs;
   char *line;
   char *result;
   size_t lineLength;
   size_t prefix;
   size_t suffix;
   if(!closing) return 0;
   escapedPath = XmlEscape(icon->path);
   escapedLabel = XmlEscape(icon->label ? icon->label : "");
   escapedArgs = XmlEscape(icon->args ? icon->args : "");
   lineLength = strlen(escapedPath) + strlen(escapedLabel)
      + strlen(escapedArgs) + 160;
   line = Allocate(lineLength);
   if(icon->args && icon->args[0]) {
      snprintf(line, lineLength,
               "  <icon x=\"%d\" y=\"%d\" label=\"%s\" args=\"%s\">%s</icon>\n",
               icon->x, icon->y, escapedLabel, escapedArgs, escapedPath);
   } else {
      snprintf(line, lineLength,
               "  <icon x=\"%d\" y=\"%d\" label=\"%s\">%s</icon>\n",
               icon->x, icon->y, escapedLabel, escapedPath);
   }
   prefix = (size_t)(closing - *content);
   suffix = *length - prefix;
   result = Allocate(*length + strlen(line) + 1);
   memcpy(result, *content, prefix);
   memcpy(result + prefix, line, strlen(line));
   memcpy(result + prefix + strlen(line), closing, suffix + 1);
   *length += strlen(line);
   Release(*content);
   *content = result;
   Release(line); Release(escapedPath); Release(escapedLabel); Release(escapedArgs);
   return 1;
}

static void ClampDesktopIcons(char saveChanges)
{
   DesktopIconNode *icon;
   for(icon = desktopIcons; icon; icon = icon->next) {
      const int size = (icon->flags & FLAG_DRIVE)
         ? desktopConfig.iconSize : DESKTOP_ICON_DEFAULT_SIZE;
      const int margin = size / 2 + 4;
      const int maxX = Max(margin, rootWidth - margin);
      const int maxY = Max(4, rootHeight - size
                           - GetStringHeight(FONT_MENU) - 12);
      int oldX = icon->x;
      int oldY = icon->y;
      icon->x = Max(margin, Min(maxX, icon->x));
      icon->y = Max(4, Min(maxY, icon->y));
      if(saveChanges && (oldX != icon->x || oldY != icon->y))
         UpdatePuppyPinIcon(icon, 0);
   }
}

static void ResizeDesktopWindow(void)
{
   if(desktopWindow != None) {
      EssoraTrace("desktop", "resize/lower requested root=%dx%d",
                  rootWidth, rootHeight);
      desktopWindowWidth = rootWidth;
      desktopWindowHeight = rootHeight;
      JXMoveResizeWindow(display, desktopWindow, 0, 0,
                         (unsigned)rootWidth, (unsigned)rootHeight);
      XLowerWindow(display, desktopWindow);
   }
}

static void SetupDesktopWindowProperties(void)
{
   Atom type = JXInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
   Atom desktopType = JXInternAtom(display, "_NET_WM_WINDOW_TYPE_DESKTOP", False);
   Atom state = JXInternAtom(display, "_NET_WM_STATE", False);
   Atom stateValues[3];
   XClassHint hint;
   stateValues[0] = JXInternAtom(display, "_NET_WM_STATE_BELOW", False);
   stateValues[1] = JXInternAtom(display, "_NET_WM_STATE_STICKY", False);
   stateValues[2] = JXInternAtom(display, "_NET_WM_STATE_SKIP_TASKBAR", False);
   JXChangeProperty(display, desktopWindow, type, XA_ATOM, 32,
                    PropModeReplace, (unsigned char*)&desktopType, 1);
   JXChangeProperty(display, desktopWindow, state, XA_ATOM, 32,
                    PropModeReplace, (unsigned char*)stateValues, 3);
   hint.res_name = (char*)"essorawm-desktop";
   hint.res_class = (char*)"EssoraWMDesktop";
   XSetClassHint(display, desktopWindow, &hint);
   XStoreName(display, desktopWindow, "EssoraWM Desktop");
}

static void RefreshDriveIcons(char saveMissing)
{
   DriveInfo *drives;
   DriveInfo *drive;
   DesktopIconNode *icon;
   char *signature = NULL;
   int index = 0;
   if(!desktopConfig.showDrives) {
      for(icon = desktopIcons; icon; icon = icon->next)
         if(icon->flags & FLAG_DRIVE) icon->flags |= FLAG_HIDDEN;
      return;
   }
   drives = ReadDriveList(&signature);
   for(icon = desktopIcons; icon; icon = icon->next)
      if(icon->flags & FLAG_DRIVE) icon->flags |= FLAG_HIDDEN;

   for(drive = drives; drive; drive = drive->next) {
      char runtimePath[PATH_MAX];
      if(!ShouldShowDrive(drive) || DriveIsHidden(drive->name)) continue;
      if(drive->network) {
         snprintf(runtimePath, sizeof(runtimePath),
                  "/tmp/essorawm_network_%08lx",
                  HashText(drive->mountpoint ? drive->mountpoint : drive->name));
      } else {
         snprintf(runtimePath, sizeof(runtimePath),
                  "/tmp/pup_event_frontend/drive_%s", drive->name);
      }
      icon = FindIconByPath(runtimePath);
      if(!icon) {
         icon = CreateDriveIcon(drive, index);
         icon->next = desktopIcons;
         desktopIcons = icon;
         if(saveMissing) UpdatePuppyPinIcon(icon, 0);
      } else {
         UpdateDriveIconMetadata(icon, drive);
      }
      icon->flags &= (unsigned)~FLAG_HIDDEN;
      if(drive->removable) icon->flags |= FLAG_REMOVABLE;
      else icon->flags &= (unsigned)~FLAG_REMOVABLE;
      if(drive->network) icon->flags |= FLAG_NETWORK;
      else icon->flags &= (unsigned)~FLAG_NETWORK;
      index++;
   }
   if(lastDriveSignature) Release(lastDriveSignature);
   lastDriveSignature = signature ? signature : CopyString("");
   FreeDriveList(drives);
}

static DriveInfo *ReadDriveList(char **signatureOut)
{
   FILE *pipe;
   char line[8192];
   DriveInfo *head = NULL;
   DriveInfo **tail = &head;
   char *signature = CopyString("");
   size_t signatureLength = 0;
   if(signatureOut) *signatureOut = NULL;
   pipe = popen("sh -c 'lsblk -P -p -o NAME,PATH,LABEL,FSTYPE,MOUNTPOINT,RM,TYPE,HOTPLUG,TRAN,MODEL,PARTLABEL,PARTTYPE,SIZE 2>/dev/null || lsblk -P -p -o NAME,LABEL,FSTYPE,MOUNTPOINT,RM,TYPE,TRAN,MODEL,SIZE 2>/dev/null'", "r");
   if(!pipe) return NULL;
   while(fgets(line, sizeof(line), pipe)) {
      DriveInfo *drive = Allocate(sizeof(DriveInfo));
      char *rm;
      char *hotplug;
      char *type;
      char *tran;
      char *model;
      char *partlabel;
      char *parttype;
      char *size;
      char *newSignature;
      char usefulType;
      char useful;
      size_t lineLength = strlen(line);
      memset(drive, 0, sizeof(DriveInfo));
      drive->name = ParseLsblkValue(line, "NAME");
      drive->device = ParseLsblkValue(line, "PATH");
      if(!drive->device && drive->name && drive->name[0] == '/')
         drive->device = CopyString(drive->name);
      if(drive->name) {
         const char *baseName = strrchr(drive->name, '/');
         if(baseName && baseName[1]) {
            char *shortName = CopyString(baseName + 1);
            Release(drive->name);
            drive->name = shortName;
         }
      }
      drive->label = ParseLsblkValue(line, "LABEL");
      drive->fstype = ParseLsblkValue(line, "FSTYPE");
      drive->mountpoint = ParseLsblkValue(line, "MOUNTPOINT");
      rm = ParseLsblkValue(line, "RM");
      hotplug = ParseLsblkValue(line, "HOTPLUG");
      type = ParseLsblkValue(line, "TYPE");
      tran = ParseLsblkValue(line, "TRAN");
      model = ParseLsblkValue(line, "MODEL");
      partlabel = ParseLsblkValue(line, "PARTLABEL");
      parttype = ParseLsblkValue(line, "PARTTYPE");
      size = ParseLsblkValue(line, "SIZE");

      drive->removable = (rm && atoi(rm) != 0)
         || (hotplug && atoi(hotplug) != 0)
         || (tran && (!strcasecmp(tran, "usb")
                      || !strcasecmp(tran, "mmc")
                      || !strcasecmp(tran, "sd")))
         || NameLooksRemovable(drive->label)
         || NameLooksRemovable(partlabel)
         || NameLooksRemovable(model);

      usefulType = type && (!strcmp(type, "part") || !strcmp(type, "crypt")
                            || !strcmp(type, "lvm") || !strcmp(type, "rom"));
      useful = drive->name && drive->device
         && !strncmp(drive->device, "/dev/", 5)
         && (usefulType || (drive->fstype && drive->fstype[0])
             || (drive->mountpoint && drive->mountpoint[0]))
         && ((drive->fstype && drive->fstype[0])
             || (drive->mountpoint && drive->mountpoint[0])
             || drive->removable)
         && !IsTechnicalDrive(drive)
         && !IsTechnicalVolume(drive->label, drive->mountpoint,
                               drive->device, drive->fstype,
                               partlabel, parttype);

      if(useful) {
         if((!drive->label || !drive->label[0])
            && partlabel && partlabel[0]) {
            if(drive->label) Release(drive->label);
            drive->label = CopyString(partlabel);
         } else if((!drive->label || !drive->label[0])
                   && size && size[0]) {
            char displayName[128];
            snprintf(displayName, sizeof(displayName), _("Volume %s"), size);
            if(drive->label) Release(drive->label);
            drive->label = CopyString(displayName);
         }
         *tail = drive;
         tail = &drive->next;
      } else {
         FreeDriveList(drive);
      }

      if(rm) Release(rm);
      if(hotplug) Release(hotplug);
      if(type) Release(type);
      if(tran) Release(tran);
      if(model) Release(model);
      if(partlabel) Release(partlabel);
      if(parttype) Release(parttype);
      if(size) Release(size);
      newSignature = Allocate(signatureLength + lineLength + 1);
      memcpy(newSignature, signature, signatureLength);
      memcpy(newSignature + signatureLength, line, lineLength + 1);
      Release(signature);
      signature = newSignature;
      signatureLength += lineLength;
   }
   pclose(pipe);
   AppendNetworkMounts(&tail, &signature, &signatureLength);
   if(signatureOut) *signatureOut = signature;
   else Release(signature);
   return head;
}

static void AppendNetworkMounts(DriveInfo ***tailPtr, char **signature,
                                size_t *signatureLength)
{
   FILE *pipe;
   char line[8192];
   DriveInfo **tail;
   if(!tailPtr || !*tailPtr || !signature || !*signature || !signatureLength)
      return;
   tail = *tailPtr;
   pipe = popen("findmnt -P -rn -o SOURCE,TARGET,FSTYPE 2>/dev/null", "r");
   if(!pipe) return;
   while(fgets(line, sizeof(line), pipe)) {
      char *source = ParseLsblkValue(line, "SOURCE");
      char *target = ParseLsblkValue(line, "TARGET");
      char *fstype = ParseLsblkValue(line, "FSTYPE");
      char network = fstype && (!strcasecmp(fstype, "nfs")
         || !strcasecmp(fstype, "nfs4") || !strcasecmp(fstype, "cifs")
         || !strcasecmp(fstype, "smb3") || strstr(fstype, "sshfs")
         || strstr(fstype, "davfs") || strstr(fstype, "rclone")
         || strstr(fstype, "fuse.gvfsd"));
      if(network && target && target[0] && strcmp(target, "/")) {
         DriveInfo *drive = Allocate(sizeof(DriveInfo));
         const char *base = strrchr(target, '/');
         char *newSignature;
         size_t lineLength = strlen(line);
         memset(drive, 0, sizeof(DriveInfo));
         drive->name = CopyString(base && base[1] ? base + 1 : "network");
         drive->device = CopyString(source ? source : "network");
         drive->label = CopyString(source && source[0] ? source : drive->name);
         drive->fstype = CopyString(fstype ? fstype : "network");
         drive->mountpoint = CopyString(target);
         drive->network = 1;
         drive->removable = 0;
         *tail = drive;
         tail = &drive->next;
         newSignature = Allocate(*signatureLength + lineLength + 1);
         memcpy(newSignature, *signature, *signatureLength);
         memcpy(newSignature + *signatureLength, line, lineLength + 1);
         Release(*signature);
         *signature = newSignature;
         *signatureLength += lineLength;
      }
      if(source) Release(source);
      if(target) Release(target);
      if(fstype) Release(fstype);
   }
   pclose(pipe);
   *tailPtr = tail;
}

static unsigned long HashText(const char *text)
{
   const unsigned char *p = (const unsigned char*)(text ? text : "");
   unsigned long value = 2166136261UL;
   while(*p) {
      value ^= (unsigned long)*p++;
      value *= 16777619UL;
   }
   return value;
}

static void FreeDriveList(DriveInfo *drives)
{
   DriveInfo *next;
   while(drives) {
      next = drives->next;
      if(drives->name) Release(drives->name);
      if(drives->device) Release(drives->device);
      if(drives->label) Release(drives->label);
      if(drives->fstype) Release(drives->fstype);
      if(drives->mountpoint) Release(drives->mountpoint);
      Release(drives);
      drives = next;
   }
}

static char *ParseLsblkValue(const char *line, const char *key)
{
   char pattern[32];
   const char *start;
   const char *end;
   char *result;
   char *out;
   snprintf(pattern, sizeof(pattern), "%s=\"", key);
   start = strstr(line, pattern);
   if(!start) return NULL;
   start += strlen(pattern);
   end = start;
   while(*end && !(*end == '"' && (end == start || end[-1] != '\\'))) end++;
   result = Allocate((size_t)(end - start) + 1);
   out = result;
   while(start < end) {
      if(*start == '\\' && start + 1 < end) start++;
      *out++ = *start++;
   }
   *out = 0;
   return result;
}

static char ShouldShowDrive(const DriveInfo *drive)
{
   if(!drive) return 0;
   if(drive->network) return desktopConfig.showNetwork;
   if(drive->removable) return desktopConfig.showRemovable;
   return desktopConfig.showInternal;
}

static char IsTechnicalDrive(const DriveInfo *drive)
{
   const char *base;
   if(!drive || !drive->name) return 1;
   base = strrchr(drive->name, '/');
   base = base ? base + 1 : drive->name;
   if(!strncasecmp(base, "loop", 4)
      || !strncasecmp(base, "zram", 4)
      || !strncasecmp(base, "ram", 3)
      || !strncasecmp(base, "dm-", 3)) return 1;
   if(drive->mountpoint
      && (!strncmp(drive->mountpoint, "/initrd/pup_", 12)
          || !strncmp(drive->mountpoint, "/pup_", 5))) return 1;
   if(drive->fstype && (!strcasecmp(drive->fstype, "squashfs")
      || !strcasecmp(drive->fstype, "overlay")
      || !strcasecmp(drive->fstype, "aufs"))) return 1;
   return 0;
}

static char NameLooksRemovable(const char *value)
{
   char lower[512];
   size_t i;
   if(!value || !value[0]) return 0;
   for(i = 0; i + 1 < sizeof(lower) && value[i]; i++)
      lower[i] = (char)tolower((unsigned char)value[i]);
   lower[i] = 0;
   return strstr(lower, "usb") || strstr(lower, "ventoy")
      || strstr(lower, "pendrive") || strstr(lower, "flash")
      || strstr(lower, "removable") || strstr(lower, "sd card")
      || strstr(lower, "memory card");
}

static char IsTechnicalVolume(const char *name, const char *path,
                              const char *device, const char *fstype,
                              const char *partlabel, const char *parttype)
{
   char joined[2048];
   char lower[2048];
   char compact[2048];
   size_t i, j = 0;
   if(fstype && !strcasecmp(fstype, "swap")) return 1;
   if(parttype && (!strcasecmp(parttype,
                  "c12a7328-f81f-11d2-ba4b-00a0c93ec93b")
                   || !strcasecmp(parttype, "ef00"))) return 1;
   snprintf(joined, sizeof(joined), "%s %s %s %s %s",
            name ? name : "", path ? path : "", device ? device : "",
            partlabel ? partlabel : "", parttype ? parttype : "");
   for(i = 0; i + 1 < sizeof(lower) && joined[i]; i++) {
      lower[i] = (char)tolower((unsigned char)joined[i]);
      if(isalnum((unsigned char)joined[i]) && j + 1 < sizeof(compact))
         compact[j++] = lower[i];
   }
   lower[i] = 0;
   compact[j] = 0;
   if(strstr(compact, "pupro") || strstr(compact, "puprw")
      || !strcmp(compact, "pupa") || !strcmp(compact, "pupb")
      || !strcmp(compact, "pupf") || !strcmp(compact, "pupk")
      || !strcmp(compact, "pupz") || strstr(compact, "vtoyefi")
      || strstr(lower, "/boot/efi") || strstr(lower, "efi system")
      || !strcmp(compact, "efi") || !strcmp(compact, "esp")
      || !strcmp(compact, "efisystempartition")
      || !strcmp(compact, "swap")) return 1;
   return 0;
}

static char DriveIsHidden(const char *name)
{
   char *path = GetHiddenDrivesPath();
   FILE *file = fopen(path, "r");
   char line[256];
   char hidden = 0;
   Release(path);
   if(!file) return 0;
   while(fgets(line, sizeof(line), file)) {
      if(!strcmp(TrimLocal(line), name)) { hidden = 1; break; }
   }
   fclose(file);
   return hidden;
}

static void HideDrive(const char *name)
{
   char *path;
   FILE *file;
   if(!name || DriveIsHidden(name)) return;
   path = GetHiddenDrivesPath();
   if(EnsureParentDirectory(path)) {
      file = fopen(path, "a");
      if(file) { fprintf(file, "%s\n", name); fclose(file); }
   }
   Release(path);
}

static DesktopIconNode *CreateDriveIcon(const DriveInfo *drive, int index)
{
   DesktopIconNode *icon = Allocate(sizeof(DesktopIconNode));
   char path[PATH_MAX];
   char args[256];
   memset(icon, 0, sizeof(DesktopIconNode));
   if(drive->network) {
      snprintf(path, sizeof(path), "/tmp/essorawm_network_%08lx",
               HashText(drive->mountpoint ? drive->mountpoint : drive->name));
   } else {
      snprintf(path, sizeof(path), "/tmp/pup_event_frontend/drive_%s",
               drive->name);
   }
   snprintf(args, sizeof(args), "%s %s",
            drive->network ? "network"
               : drive->removable ? "usbdrv" : "drive",
            drive->fstype && drive->fstype[0] ? drive->fstype : "unknown");
   icon->path = CopyString(path);
   icon->label = CopyString(drive->label && drive->label[0]
                            ? drive->label : drive->name);
   icon->args = CopyString(args);
   icon->flags = FLAG_DRIVE | FLAG_RUNTIME;
   if(drive->removable) icon->flags |= FLAG_REMOVABLE;
   if(drive->network) icon->flags |= FLAG_NETWORK;
   UpdateDriveIconMetadata(icon, drive);
   DriveDefaultPosition(index, &icon->x, &icon->y);
   icon->icon = LoadBestIcon(icon);
   return icon;
}

static void UpdateDriveIconMetadata(DesktopIconNode *icon,
                                    const DriveInfo *drive)
{
   if(!icon || !drive) return;
   if(icon->device) Release(icon->device);
   if(icon->mountPath) Release(icon->mountPath);
   icon->device = CopyString(drive->device ? drive->device : "");
   icon->mountPath = CopyString(drive->mountpoint ? drive->mountpoint : "");
}

static void MountAndOpenDrive(DesktopIconNode *icon)
{
   char inferred[PATH_MAX];
   const char *device;
   char *quotedDevice;
   char *quotedOpen;
   char *quotedMount;
   char *command;
   size_t length;
   const char *base;
   const char *driveOpenCommand;
   if(!icon) return;
   SetDesktopSelection(NULL);
   lastClickIcon = NULL;
   lastClickTime = 0;
   DrawDesktop();
   NormalizeOpenCommand();
   driveOpenCommand = access("/usr/bin/essorawm-filemanager", X_OK) == 0
      ? "/usr/bin/essorawm-filemanager" : desktopConfig.openCommand;
   device = icon->device && icon->device[0] ? icon->device : NULL;
   if(!device && icon->path) {
      base = strstr(icon->path, "drive_");
      if(base && base[6]) {
         snprintf(inferred, sizeof(inferred), "/dev/%s", base + 6);
         device = inferred;
      }
   }
   if(icon->mountPath && icon->mountPath[0]
      && IsDirectory(icon->mountPath)) {
      quotedMount = QuoteShell(icon->mountPath);
      length = strlen(driveOpenCommand) + strlen(quotedMount) + 4;
      command = Allocate(length);
      snprintf(command, length, "%s %s", driveOpenCommand, quotedMount);
      RunCommand(command);
      Release(command);
      Release(quotedMount);
      return;
   }
   if(!device || strncmp(device, "/dev/", 5)) return;

   quotedDevice = QuoteShell(device);
   quotedOpen = QuoteShell(driveOpenCommand);
   /* Root/Puppy uses mount directly. A regular Essora user uses udisksctl.
    * Both paths finish by opening the resulting directory through the
    * portable file-manager launcher. */
   length = strlen(quotedDevice) + strlen(quotedOpen) + 1600;
   command = Allocate(length);
   snprintf(command, length,
      "sh -c 'dev=$1; fm=$2; "
      "mp=$(findmnt -rn -S \"$dev\" -o TARGET 2>/dev/null | head -n1); "
      "if [ -z \"$mp\" ]; then "
      "if [ \"$(id -u)\" -eq 0 ]; then "
      "mp=/mnt/${dev##*/}; mkdir -p \"$mp\" || exit 1; "
      "/bin/mount \"$dev\" \"$mp\" >/dev/null 2>&1 || { "
      "rmdir \"$mp\" 2>/dev/null || true; mp=; "
      "command -v udisksctl >/dev/null 2>&1 && "
      "udisksctl mount -b \"$dev\" >/dev/null 2>&1; }; "
      "else command -v udisksctl >/dev/null 2>&1 && "
      "udisksctl mount -b \"$dev\" >/dev/null 2>&1; fi; "
      "mp=$(findmnt -rn -S \"$dev\" -o TARGET 2>/dev/null | head -n1); "
      "[ -n \"$mp\" ] || mp=/mnt/${dev##*/}; fi; "
      "[ -d \"$mp\" ] || exit 1; exec \"$fm\" \"$mp\"' sh %s %s",
      quotedDevice, quotedOpen);
   RunCommand(command);
   Release(command);
   Release(quotedOpen);
   Release(quotedDevice);
}

static void UnmountDrive(DesktopIconNode *icon)
{
   const char *device;
   char *quotedDevice;
   char *quotedMount = NULL;
   char *command;
   size_t length;
   if(!icon) return;
   SetDesktopSelection(NULL);
   lastClickIcon = NULL;
   lastClickTime = 0;
   DrawDesktop();
   device = icon->device && icon->device[0] ? icon->device : NULL;
   if((!device || !device[0]) && (!icon->mountPath || !icon->mountPath[0])) return;
   quotedDevice = QuoteShell(device ? device : "");
   if(icon->mountPath && icon->mountPath[0]) {
      quotedMount = QuoteShell(icon->mountPath);
   }
   length = strlen(quotedDevice) + (quotedMount ? strlen(quotedMount) : 0) + 512;
   command = Allocate(length);
   snprintf(command, length,
      "sh -c 'dev=$1; mp=$2; "
      "case \"$dev\" in /dev/*) if command -v udisksctl >/dev/null 2>&1; then "
      "udisksctl unmount -b \"$dev\" >/dev/null 2>&1 && exit 0; fi;; esac; "
      "[ -n \"$mp\" ] && /bin/umount \"$mp\"' sh %s %s",
      quotedDevice, quotedMount ? quotedMount : "''");
   RunCommand(command);
   Release(command);
   if(quotedMount) Release(quotedMount);
   Release(quotedDevice);
}

static void RelayoutDriveIcons(void)
{
   DesktopIconNode *icon;
   int index = 0;
   for(icon = desktopIcons; icon; icon = icon->next) {
      if((icon->flags & FLAG_DRIVE) && !(icon->flags & FLAG_HIDDEN)) {
         DriveDefaultPosition(index++, &icon->x, &icon->y);
         UpdatePuppyPinIcon(icon, 0);
      }
   }
}

static void DriveDefaultPosition(int index, int *x, int *y)
{
   const int baseX = (int)(desktopConfig.xPos * rootWidth)
      + desktopConfig.xOffset;
   const int baseY = (int)(desktopConfig.yPos * rootHeight)
      + desktopConfig.yOffset;
   const int direction = desktopConfig.reversePack ? -1 : 1;
   int px = baseX;
   int py = baseY;
   if(desktopConfig.xPos < 0.25) px += desktopConfig.iconSize / 2 + 8;
   else if(desktopConfig.xPos > 0.75) px -= desktopConfig.iconSize / 2 + 8;
   if(desktopConfig.yPos < 0.25) py += 8;
   else if(desktopConfig.yPos > 0.75)
      py -= desktopConfig.iconSize + GetStringHeight(FONT_MENU) + 12;
   if(desktopConfig.vertical) py += direction * index * desktopConfig.spacingY;
   else px += (desktopConfig.xPos > 0.75 ? -1 : 1)
      * index * desktopConfig.spacingX;
   *x = px;
   *y = py;
}

static void SignalDriveRefresh(const TimeType *now, int x, int y,
                               Window w, void *data)
{
   DriveInfo *drives;
   char *signature = NULL;
   (void)now; (void)x; (void)y; (void)w; (void)data;
   drives = ReadDriveList(&signature);
   FreeDriveList(drives);
   if(!signature) signature = CopyString("");
   if(!lastDriveSignature || strcmp(lastDriveSignature, signature)) {
      EssoraTrace("desktop", "drive signature changed; reloading icons");
      Release(signature);
      LoadPuppyPin();
      RefreshDriveIcons(1);
      ClampDesktopIcons(1);
      DrawDesktop();
   } else {
      Release(signature);
   }
}

static void SetDesktopSelection(DesktopIconNode *icon)
{
   selectedIcon = icon;
}
