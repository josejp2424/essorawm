/**
 * @file essoradesktop.c
 * @brief Native EssoraWM manager for PuppyPin desktop applications.
 *
 * agregado por josejp2424
 *
 * This small X11 interface scans .desktop launchers and adds/removes them
 * from $HOME/Choices/ROX-Filer/PuppyPin. Every modification rereads the
 * current PuppyPin from disk, so positions saved by the native EssoraWM
 * desktop are never replaced by stale data kept by this window.
 */

#include "jwm.h"
#include "essoradesktop.h"
#include "main.h"
#include "desktopicons.h"

#ifndef MAKE_DEPEND
#  include <ctype.h>
#  include <dirent.h>
#  include <errno.h>
#  include <fcntl.h>
#  include <limits.h>
#  include <pwd.h>
#  include <signal.h>
#  include <strings.h>
#  include <sys/stat.h>
#  include <sys/types.h>
#  include <sys/wait.h>
#  include <unistd.h>
#  include <X11/Xutil.h>
#endif

#define DM_WINDOW_W 840
#define DM_WINDOW_H 650
#define DM_HEADER_H 66
#define DM_MARGIN 18
#define DM_TAB_Y 76
#define DM_TAB_H 34
#define DM_TAB_W 190
#define DM_SEARCH_Y 132
#define DM_SEARCH_H 34
#define DM_LIST_Y 178
#define DM_LIST_H 342
#define DM_ROW_H 26
#define DM_STATUS_Y 557
#define DM_BUTTON_Y 594
#define DM_BUTTON_H 36
#define DM_MAX_SEARCH 192
#define DM_MAX_STATUS 512
#define DM_MAX_APPS 4096
#define DM_MAX_PIN_SIZE (16U * 1024U * 1024U)

#define DM_PAGE_APPS 0
#define DM_PAGE_DRIVES 1
#define DM_DRIVE_ROW_H 38
#define DM_MIN(a,b) ((a) < (b) ? (a) : (b))
#define DM_MAX(a,b) ((a) > (b) ? (a) : (b))

typedef struct DesktopApp {
   char *name;
   char *path;
   char pinned;
} DesktopApp;

typedef struct DesktopAppList {
   DesktopApp *items;
   int count;
   int capacity;
} DesktopAppList;

typedef struct DriveUiConfig {
   char enabled;
   char showInternal;
   char showRemovable;
   char showNetwork;
   char showLabels;
   char showFrame;
   char vertical;
   char reversePack;
   int iconSize;
   int spacingX;
   int spacingY;
   int xOffset;
   int yOffset;
   int horizontalAlign;
   int verticalAlign;
   char openCommand[PATH_MAX];
} DriveUiConfig;

static Display *dmDisplay = NULL;
static Window dmWindow = None;
static GC dmGC = None;
static XFontStruct *dmFont = NULL;
static XFontSet dmFontSet = NULL;
static Visual *dmVisual = NULL;
static Colormap dmColormap = None;
static int dmScreen = 0;
static Atom dmDeleteAtom = None;
static DesktopAppList dmApps;
static int dmFiltered[DM_MAX_APPS];
static int dmFilteredCount = 0;
static int dmSelected = 0;
static int dmTop = 0;
static char dmSearch[DM_MAX_SEARCH];
static char dmStatus[DM_MAX_STATUS];
static int dmPage = DM_PAGE_APPS;
static DriveUiConfig dmDriveConfig;
static char dmDriveDirty = 0;

static char *DupString(const char *value);
static void FreeApps(void);
static void ScanApplications(void);
static void ScanApplicationDirectory(const char *directory);
static char ParseDesktopFile(const char *path, char **name_out);
static void GetLocaleKeys(char *exact, size_t exact_size,
                          char *language, size_t language_size);
static void AddApplication(const char *name, const char *path);
static int CompareApplications(const void *a, const void *b);
static char EndsWithDesktop(const char *name);
static char IsTrueValue(const char *value);
static void TrimLine(char *line);
static void UpdateFilter(void);
static char ContainsAsciiCaseInsensitive(const char *text, const char *needle);
static void RefreshPinnedStates(void);
static char *GetHomeDirectory(void);
static char *GetPuppyPinPath(void);
static char EnsurePuppyPinDirectory(const char *pin_path);
static char *ReadWholeFile(const char *path, size_t *size_out);
static char *CreateInitialPuppyPin(void);
static char *ReadSavedWallpaper(void);
static char *XmlEscape(const char *text);
static char *XmlUnescapeRange(const char *start, size_t length);
static char PuppyPinContainsPath(const char *content, const char *path);
static char FindMatchingIcon(const char *content, const char *path,
                             const char **start_out, const char **end_out);
static void FindFreePosition(const char *content, int *x_out, int *y_out);
static char PositionOccupied(const char *content, int x, int y);
static int ParseAttributeInteger(const char *start, const char *end,
                                 const char *name, int *value_out);
static char WriteFileAtomic(const char *path, const char *content, size_t length);
static char CopyFile(const char *source, const char *target);
static int AddSelectedApplication(void);
static int RemoveSelectedApplication(void);
static void ReloadRoxPinboard(const char *pin_path);
static char *JoinPath(const char *left, const char *right);
static void SetStatus(const char *text);
static void EnsureSelectionVisible(void);
static void DrawWindow(void);
static void DrawTextAt(int x, int y, const char *text, unsigned long color);
static void DrawButton(int x, int y, int width, int height,
                       const char *label, char active);
static char HitRect(int x, int y, int bx, int by, int bw, int bh);
static unsigned long RGBToPixel(unsigned char r, unsigned char g, unsigned char b);
static int MaskShift(unsigned long mask);
static int MaskBits(unsigned long mask);
static unsigned long ScaleToMask(unsigned char value, unsigned long mask);
static void LoadDriveUiConfig(void);
static void SetDriveUiDefaults(void);
static void ReadDriveIni(const char *path);
static char SaveDriveUiConfig(void);
static char EnsureDirectoryTree(const char *path);
static char ParseBoolValue(const char *value, char fallback);
static void ApplyDriveUiConfig(void);
static void ResetHiddenDrives(void);
static void DrawApplicationsPage(void);
static void DrawDrivePage(void);
static void DrawTab(int x, const char *label, char active);
static void DrawToggle(int x, int y, int width, const char *label, char active);
static void DrawChoice(int x, int y, int width, const char *label, char active);
static void DrawStepper(int y, const char *label, int value, int minimum,
                        int maximum, int step, int control);
static void HandleDriveClick(int x, int y);
static int TextWidth(const char *text);
static void CloseDisplay(void);

void RunEssoraDesktopManager(void)
{
   XEvent event;
   XSizeHints size_hints;
   XClassHint class_hint;
   char **missing = NULL;
   int missingCount = 0;
   char *defaultString = NULL;
   int running = 1;

   memset(&dmApps, 0, sizeof(dmApps));
   memset(dmSearch, 0, sizeof(dmSearch));
   memset(dmStatus, 0, sizeof(dmStatus));
   dmPage = DM_PAGE_APPS;
   dmDriveDirty = 0;

   dmDisplay = XOpenDisplay(NULL);
   if(!dmDisplay) {
      fprintf(stderr, "%s\n", _("Could not open the X display"));
      return;
   }

   dmScreen = DefaultScreen(dmDisplay);
   dmVisual = DefaultVisual(dmDisplay, dmScreen);
   dmColormap = DefaultColormap(dmDisplay, dmScreen);

   dmWindow = XCreateSimpleWindow(dmDisplay,
                                  RootWindow(dmDisplay, dmScreen),
                                  0, 0, DM_WINDOW_W, DM_WINDOW_H, 1,
                                  RGBToPixel(0x77, 0x96, 0x0a),
                                  RGBToPixel(0x20, 0x20, 0x20));
   XMoveWindow(dmDisplay, dmWindow,
               (DisplayWidth(dmDisplay, dmScreen) - DM_WINDOW_W) / 2,
               (DisplayHeight(dmDisplay, dmScreen) - DM_WINDOW_H) / 2);

   XStoreName(dmDisplay, dmWindow, _("EssoraWM Desktop Manager"));
   class_hint.res_name = (char*)"essorawm-desktop";
   class_hint.res_class = (char*)"EssoraWM";
   XSetClassHint(dmDisplay, dmWindow, &class_hint);

   size_hints.flags = PMinSize | PMaxSize;
   size_hints.min_width = DM_WINDOW_W;
   size_hints.max_width = DM_WINDOW_W;
   size_hints.min_height = DM_WINDOW_H;
   size_hints.max_height = DM_WINDOW_H;
   XSetWMNormalHints(dmDisplay, dmWindow, &size_hints);

   dmDeleteAtom = XInternAtom(dmDisplay, "WM_DELETE_WINDOW", False);
   XSetWMProtocols(dmDisplay, dmWindow, &dmDeleteAtom, 1);
   XSelectInput(dmDisplay, dmWindow,
                ExposureMask | KeyPressMask | ButtonPressMask |
                StructureNotifyMask);

   dmGC = XCreateGC(dmDisplay, dmWindow, 0, NULL);
   dmFontSet = XCreateFontSet(dmDisplay,
      "-*-sans-medium-r-normal--14-*-*-*-*-*-*-*,fixed",
      &missing, &missingCount, &defaultString);
   if(missing) {
      XFreeStringList(missing);
   }
   if(!dmFontSet) {
      dmFont = XLoadQueryFont(dmDisplay, "fixed");
      if(dmFont) {
         XSetFont(dmDisplay, dmGC, dmFont->fid);
      }
   }

   ScanApplications();
   RefreshPinnedStates();
   UpdateFilter();
   LoadDriveUiConfig();
   if(dmApps.count == 0) {
      SetStatus(_("No application launchers were found"));
   } else {
      SetStatus(_("Select an application to add it to the desktop"));
   }

   XMapRaised(dmDisplay, dmWindow);
   DrawWindow();

   while(running && XNextEvent(dmDisplay, &event) == 0) {
      if(event.type == Expose) {
         if(event.xexpose.count == 0) {
            DrawWindow();
         }
      } else if(event.type == ClientMessage) {
         if((Atom)event.xclient.data.l[0] == dmDeleteAtom) {
            running = 0;
         }
      } else if(event.type == ButtonPress) {
         const int mx = event.xbutton.x;
         const int my = event.xbutton.y;
         if(event.xbutton.button == Button1
            && HitRect(mx, my, DM_MARGIN, DM_TAB_Y, DM_TAB_W, DM_TAB_H)) {
            dmPage = DM_PAGE_APPS;
            SetStatus(_("Select an application to add it to the desktop"));
            DrawWindow();
            continue;
         }
         if(event.xbutton.button == Button1
            && HitRect(mx, my, DM_MARGIN + DM_TAB_W + 8, DM_TAB_Y,
                       DM_TAB_W, DM_TAB_H)) {
            dmPage = DM_PAGE_DRIVES;
            SetStatus(_("Move drive icons freely on the desktop or arrange them here"));
            DrawWindow();
            continue;
         }
         if(dmPage == DM_PAGE_APPS) {
            const int addX = DM_WINDOW_W - 322;
            const int removeX = DM_WINDOW_W - 216;
            const int closeX = DM_WINDOW_W - 110;
            if(event.xbutton.button == Button4) {
               if(dmTop > 0) {
                  dmTop--;
                  DrawWindow();
               }
            } else if(event.xbutton.button == Button5) {
               const int visibleRows = DM_LIST_H / DM_ROW_H;
               if(dmTop + visibleRows < dmFilteredCount) {
                  dmTop++;
                  DrawWindow();
               }
            } else if(event.xbutton.button == Button1) {
               if(my >= DM_LIST_Y && my < DM_LIST_Y + DM_LIST_H) {
                  int row = (my - DM_LIST_Y) / DM_ROW_H;
                  int selected = dmTop + row;
                  if(selected >= 0 && selected < dmFilteredCount) {
                     dmSelected = selected;
                     DrawWindow();
                  }
               } else if(HitRect(mx, my, addX, DM_BUTTON_Y, 96, DM_BUTTON_H)) {
                  AddSelectedApplication();
                  DrawWindow();
               } else if(HitRect(mx, my, removeX, DM_BUTTON_Y, 96, DM_BUTTON_H)) {
                  RemoveSelectedApplication();
                  DrawWindow();
               } else if(HitRect(mx, my, closeX, DM_BUTTON_Y, 86, DM_BUTTON_H)) {
                  running = 0;
               }
            }
         } else if(event.xbutton.button == Button1) {
            const int applyX = DM_WINDOW_W - 350;
            const int resetX = DM_WINDOW_W - 238;
            const int closeX = DM_WINDOW_W - 110;
            if(HitRect(mx, my, DM_MARGIN, DM_BUTTON_Y, 190, DM_BUTTON_H)) {
               ResetHiddenDrives();
               DrawWindow();
            } else if(HitRect(mx, my, applyX, DM_BUTTON_Y, 102, DM_BUTTON_H)) {
               ApplyDriveUiConfig();
               DrawWindow();
            } else if(HitRect(mx, my, resetX, DM_BUTTON_Y, 118, DM_BUTTON_H)) {
               SetDriveUiDefaults();
               dmDriveDirty = 1;
               SetStatus(_("Default drive icon settings restored; press Apply"));
               DrawWindow();
            } else if(HitRect(mx, my, closeX, DM_BUTTON_Y, 86, DM_BUTTON_H)) {
               running = 0;
            } else {
               HandleDriveClick(mx, my);
               DrawWindow();
            }
         }
      } else if(event.type == KeyPress) {
         char buffer[64];
         KeySym key = NoSymbol;
         int length = XLookupString(&event.xkey, buffer, sizeof(buffer) - 1,
                                    &key, NULL);
         buffer[length > 0 ? length : 0] = 0;

         if(key == XK_Escape) {
            running = 0;
         } else if(dmPage == DM_PAGE_DRIVES) {
            if(key == XK_Return || key == XK_KP_Enter) {
               ApplyDriveUiConfig();
               DrawWindow();
            }
         } else if(key == XK_Up) {
            if(dmSelected > 0) {
               dmSelected--;
               EnsureSelectionVisible();
               DrawWindow();
            }
         } else if(key == XK_Down) {
            if(dmSelected + 1 < dmFilteredCount) {
               dmSelected++;
               EnsureSelectionVisible();
               DrawWindow();
            }
         } else if(key == XK_Page_Up) {
            dmSelected -= DM_LIST_H / DM_ROW_H;
            if(dmSelected < 0) dmSelected = 0;
            EnsureSelectionVisible();
            DrawWindow();
         } else if(key == XK_Page_Down) {
            dmSelected += DM_LIST_H / DM_ROW_H;
            if(dmSelected >= dmFilteredCount) dmSelected = dmFilteredCount - 1;
            if(dmSelected < 0) dmSelected = 0;
            EnsureSelectionVisible();
            DrawWindow();
         } else if(key == XK_Return || key == XK_KP_Enter) {
            AddSelectedApplication();
            DrawWindow();
         } else if(key == XK_Delete) {
            RemoveSelectedApplication();
            DrawWindow();
         } else if(key == XK_BackSpace) {
            size_t searchLength = strlen(dmSearch);
            if(searchLength > 0) {
               dmSearch[searchLength - 1] = 0;
               UpdateFilter();
               DrawWindow();
            }
         } else if(length > 0 && !iscntrl((unsigned char)buffer[0])) {
            size_t used = strlen(dmSearch);
            size_t available = sizeof(dmSearch) - used - 1;
            if((size_t)length <= available) {
               memcpy(dmSearch + used, buffer, (size_t)length);
               dmSearch[used + (size_t)length] = 0;
               UpdateFilter();
               DrawWindow();
            }
         }
      }
   }

   CloseDisplay();
   FreeApps();
}

static char *DupString(const char *value)
{
   size_t length;
   char *copy;
   if(!value) {
      return NULL;
   }
   length = strlen(value) + 1;
   copy = malloc(length);
   if(copy) {
      memcpy(copy, value, length);
   }
   return copy;
}

static void FreeApps(void)
{
   int i;
   for(i = 0; i < dmApps.count; i++) {
      free(dmApps.items[i].name);
      free(dmApps.items[i].path);
   }
   free(dmApps.items);
   memset(&dmApps, 0, sizeof(dmApps));
}

static void ScanApplications(void)
{
   const char *home = getenv("HOME");
   char *local = NULL;

   ScanApplicationDirectory("/usr/share/applications");
   ScanApplicationDirectory("/usr/local/share/applications");
   if(home && home[0]) {
      local = JoinPath(home, ".local/share/applications");
      if(local) {
         ScanApplicationDirectory(local);
         free(local);
      }
   }

   if(dmApps.count > 1) {
      qsort(dmApps.items, (size_t)dmApps.count, sizeof(dmApps.items[0]),
            CompareApplications);
   }
}

static void ScanApplicationDirectory(const char *directory)
{
   DIR *dir;
   struct dirent *entry;
   if(!directory || !(dir = opendir(directory))) {
      return;
   }

   while((entry = readdir(dir)) != NULL) {
      char *path;
      char *name = NULL;
      if(dmApps.count >= DM_MAX_APPS || !EndsWithDesktop(entry->d_name)) {
         continue;
      }
      path = JoinPath(directory, entry->d_name);
      if(!path) {
         continue;
      }
      if(ParseDesktopFile(path, &name)) {
         AddApplication(name, path);
      }
      free(name);
      free(path);
   }
   closedir(dir);
}

static char ParseDesktopFile(const char *path, char **name_out)
{
   FILE *file;
   char line[4096];
   char exact[64];
   char language[32];
   char exact_key[96];
   char language_key[64];
   char *base_name = NULL;
   char *exact_name = NULL;
   char *language_name = NULL;
   char type_application = 0;
   char hidden = 0;
   char no_display = 0;
   char in_desktop_entry = 0;
   char valid = 0;

   *name_out = NULL;
   file = fopen(path, "r");
   if(!file) {
      return 0;
   }

   GetLocaleKeys(exact, sizeof(exact), language, sizeof(language));
   exact_key[0] = 0;
   language_key[0] = 0;
   if(exact[0]) {
      snprintf(exact_key, sizeof(exact_key), "Name[%s]", exact);
   }
   if(language[0]) {
      snprintf(language_key, sizeof(language_key), "Name[%s]", language);
   }

   while(fgets(line, sizeof(line), file)) {
      char *equals;
      char *key;
      char *value;
      TrimLine(line);
      if(line[0] == '[') {
         in_desktop_entry = !strcmp(line, "[Desktop Entry]");
         continue;
      }
      if(!in_desktop_entry || line[0] == '#' || line[0] == 0) {
         continue;
      }
      equals = strchr(line, '=');
      if(!equals) {
         continue;
      }
      *equals = 0;
      key = line;
      value = equals + 1;
      if(!strcmp(key, "Type")) {
         type_application = !strcasecmp(value, "Application");
      } else if(!strcmp(key, "Hidden")) {
         hidden = IsTrueValue(value);
      } else if(!strcmp(key, "NoDisplay")) {
         no_display = IsTrueValue(value);
      } else if(!strcmp(key, "Name")) {
         free(base_name);
         base_name = DupString(value);
      } else if(exact_key[0] && !strcmp(key, exact_key)) {
         free(exact_name);
         exact_name = DupString(value);
      } else if(language_key[0] && !strcmp(key, language_key)) {
         free(language_name);
         language_name = DupString(value);
      }
   }
   fclose(file);

   if(type_application && !hidden && !no_display) {
      const char *chosen = exact_name ? exact_name
                         : language_name ? language_name
                         : base_name;
      if(chosen && chosen[0]) {
         *name_out = DupString(chosen);
         valid = *name_out != NULL;
      }
   }

   free(base_name);
   free(exact_name);
   free(language_name);
   return valid;
}

static void GetLocaleKeys(char *exact, size_t exact_size,
                          char *language, size_t language_size)
{
   const char *locale = getenv("LC_ALL");
   char buffer[64];
   char *separator;

   if(!locale || !locale[0]) locale = getenv("LC_MESSAGES");
   if(!locale || !locale[0]) locale = getenv("LANG");
   if(!locale || !locale[0] || !strcmp(locale, "C") || !strcmp(locale, "POSIX")) {
      exact[0] = 0;
      language[0] = 0;
      return;
   }

   snprintf(buffer, sizeof(buffer), "%s", locale);
   separator = strpbrk(buffer, ".@");
   if(separator) *separator = 0;
   snprintf(exact, exact_size, "%.*s", (int)exact_size - 1, buffer);
   separator = strchr(buffer, '_');
   if(separator) *separator = 0;
   snprintf(language, language_size, "%.*s", (int)language_size - 1, buffer);
}

static void AddApplication(const char *name, const char *path)
{
   DesktopApp *new_items;
   int i;
   if(!name || !path) {
      return;
   }
   for(i = 0; i < dmApps.count; i++) {
      if(!strcmp(dmApps.items[i].path, path)) {
         return;
      }
   }
   if(dmApps.count == dmApps.capacity) {
      int new_capacity = dmApps.capacity ? dmApps.capacity * 2 : 128;
      new_items = realloc(dmApps.items,
                          (size_t)new_capacity * sizeof(dmApps.items[0]));
      if(!new_items) {
         return;
      }
      dmApps.items = new_items;
      dmApps.capacity = new_capacity;
   }
   dmApps.items[dmApps.count].name = DupString(name);
   dmApps.items[dmApps.count].path = DupString(path);
   dmApps.items[dmApps.count].pinned = 0;
   if(dmApps.items[dmApps.count].name && dmApps.items[dmApps.count].path) {
      dmApps.count++;
   } else {
      free(dmApps.items[dmApps.count].name);
      free(dmApps.items[dmApps.count].path);
   }
}

static int CompareApplications(const void *a, const void *b)
{
   const DesktopApp *left = a;
   const DesktopApp *right = b;
   int result = strcasecmp(left->name, right->name);
   if(result == 0) {
      result = strcmp(left->path, right->path);
   }
   return result;
}

static char EndsWithDesktop(const char *name)
{
   const size_t suffix_length = strlen(".desktop");
   const size_t length = name ? strlen(name) : 0;
   return length > suffix_length
       && !strcasecmp(name + length - suffix_length, ".desktop");
}

static char IsTrueValue(const char *value)
{
   return value && (!strcasecmp(value, "true") || !strcmp(value, "1")
                  || !strcasecmp(value, "yes"));
}

static void TrimLine(char *line)
{
   size_t length;
   if(!line) return;
   length = strlen(line);
   while(length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r')) {
      line[--length] = 0;
   }
}

static void UpdateFilter(void)
{
   int i;
   dmFilteredCount = 0;
   for(i = 0; i < dmApps.count && dmFilteredCount < DM_MAX_APPS; i++) {
      if(!dmSearch[0]
         || ContainsAsciiCaseInsensitive(dmApps.items[i].name, dmSearch)
         || ContainsAsciiCaseInsensitive(dmApps.items[i].path, dmSearch)) {
         dmFiltered[dmFilteredCount++] = i;
      }
   }
   dmSelected = 0;
   dmTop = 0;
   if(dmFilteredCount == 0) {
      SetStatus(_("No applications match the search"));
   } else {
      SetStatus(_("Select an application to add it to the desktop"));
   }
}

static char ContainsAsciiCaseInsensitive(const char *text, const char *needle)
{
   const unsigned char *start;
   size_t needle_length;
   if(!text || !needle || !needle[0]) {
      return 1;
   }
   needle_length = strlen(needle);
   for(start = (const unsigned char*)text; *start; start++) {
      size_t i;
      for(i = 0; i < needle_length; i++) {
         unsigned char a = start[i];
         unsigned char b = (unsigned char)needle[i];
         if(!a || tolower(a) != tolower(b)) {
            break;
         }
      }
      if(i == needle_length) {
         return 1;
      }
   }
   return 0;
}

static void RefreshPinnedStates(void)
{
   char *pin_path = GetPuppyPinPath();
   char *content = NULL;
   int i;
   if(pin_path) {
      content = ReadWholeFile(pin_path, NULL);
   }
   for(i = 0; i < dmApps.count; i++) {
      dmApps.items[i].pinned = content
                            ? PuppyPinContainsPath(content, dmApps.items[i].path)
                            : 0;
   }
   free(content);
   free(pin_path);
}

static char *GetHomeDirectory(void)
{
   const char *home = getenv("HOME");
   struct passwd *pw;
   if(home && home[0]) {
      return DupString(home);
   }
   pw = getpwuid(getuid());
   if(pw && pw->pw_dir && pw->pw_dir[0]) {
      return DupString(pw->pw_dir);
   }
   return NULL;
}

static char *GetPuppyPinPath(void)
{
   char *home = GetHomeDirectory();
   char *path;
   if(!home) {
      return NULL;
   }
   path = JoinPath(home, "Choices/ROX-Filer/PuppyPin");
   free(home);
   return path;
}

static char EnsurePuppyPinDirectory(const char *pin_path)
{
   char buffer[PATH_MAX];
   char *p;
   size_t length;
   if(!pin_path) {
      return 0;
   }
   length = strlen(pin_path);
   if(length >= sizeof(buffer)) {
      return 0;
   }
   memcpy(buffer, pin_path, length + 1);
   p = strrchr(buffer, '/');
   if(!p) {
      return 0;
   }
   *p = 0;

   for(p = buffer + 1; *p; p++) {
      if(*p == '/') {
         *p = 0;
         if(mkdir(buffer, 0755) < 0 && errno != EEXIST) {
            return 0;
         }
         *p = '/';
      }
   }
   if(mkdir(buffer, 0755) < 0 && errno != EEXIST) {
      return 0;
   }
   return 1;
}

static char *ReadWholeFile(const char *path, size_t *size_out)
{
   FILE *file;
   struct stat st;
   char *content;
   size_t used;
   if(size_out) *size_out = 0;
   if(!path || stat(path, &st) < 0 || st.st_size < 0
      || (unsigned long long)st.st_size > DM_MAX_PIN_SIZE) {
      return NULL;
   }
   file = fopen(path, "rb");
   if(!file) {
      return NULL;
   }
   content = malloc((size_t)st.st_size + 1);
   if(!content) {
      fclose(file);
      return NULL;
   }
   used = fread(content, 1, (size_t)st.st_size, file);
   if(ferror(file)) {
      free(content);
      fclose(file);
      return NULL;
   }
   content[used] = 0;
   fclose(file);
   if(size_out) *size_out = used;
   return content;
}

static char *CreateInitialPuppyPin(void)
{
   char *wallpaper = ReadSavedWallpaper();
   char *escaped = wallpaper ? XmlEscape(wallpaper) : NULL;
   char *content;
   size_t length;

   if(escaped && escaped[0]) {
      length = strlen(escaped) + 128;
      content = malloc(length);
      if(content) {
         snprintf(content, length,
                  "<?xml version=\"1.0\"?>\n"
                  "<pinboard>\n"
                  "  <backdrop style=\"Stretched\">%s</backdrop>\n"
                  "</pinboard>\n", escaped);
      }
   } else {
      content = DupString(
         "<?xml version=\"1.0\"?>\n"
         "<pinboard>\n"
         "</pinboard>\n");
   }
   free(wallpaper);
   free(escaped);
   return content;
}

static char *ReadSavedWallpaper(void)
{
   char *home = GetHomeDirectory();
   char *path;
   FILE *file;
   char line[PATH_MAX];
   char *result = NULL;
   if(!home) return NULL;
   path = JoinPath(home, ".config/essorawm/wallpaper");
   free(home);
   if(!path) return NULL;
   file = fopen(path, "r");
   free(path);
   if(!file) return NULL;
   if(fgets(line, sizeof(line), file)) {
      TrimLine(line);
      if(line[0]) result = DupString(line);
   }
   fclose(file);
   return result;
}

static char *XmlEscape(const char *text)
{
   const unsigned char *p;
   size_t length = 1;
   char *result;
   char *out;
   if(!text) return DupString("");
   for(p = (const unsigned char*)text; *p; p++) {
      switch(*p) {
      case '&': length += 5; break;
      case '<': case '>': length += 4; break;
      case '"': case '\'': length += 6; break;
      default: length++; break;
      }
   }
   result = malloc(length);
   if(!result) return NULL;
   out = result;
   for(p = (const unsigned char*)text; *p; p++) {
      const char *replacement = NULL;
      switch(*p) {
      case '&': replacement = "&amp;"; break;
      case '<': replacement = "&lt;"; break;
      case '>': replacement = "&gt;"; break;
      case '"': replacement = "&quot;"; break;
      case '\'': replacement = "&apos;"; break;
      default: *out++ = (char)*p; break;
      }
      if(replacement) {
         size_t n = strlen(replacement);
         memcpy(out, replacement, n);
         out += n;
      }
   }
   *out = 0;
   return result;
}

static char *XmlUnescapeRange(const char *start, size_t length)
{
   char *result = malloc(length + 1);
   size_t i = 0;
   size_t out = 0;
   if(!result) return NULL;
   while(i < length) {
      if(start[i] == '&') {
         if(i + 5 <= length && !memcmp(start + i, "&amp;", 5)) {
            result[out++] = '&'; i += 5; continue;
         }
         if(i + 4 <= length && !memcmp(start + i, "&lt;", 4)) {
            result[out++] = '<'; i += 4; continue;
         }
         if(i + 4 <= length && !memcmp(start + i, "&gt;", 4)) {
            result[out++] = '>'; i += 4; continue;
         }
         if(i + 6 <= length && !memcmp(start + i, "&quot;", 6)) {
            result[out++] = '"'; i += 6; continue;
         }
         if(i + 6 <= length && !memcmp(start + i, "&apos;", 6)) {
            result[out++] = '\''; i += 6; continue;
         }
      }
      result[out++] = start[i++];
   }
   result[out] = 0;
   return result;
}

static char PuppyPinContainsPath(const char *content, const char *path)
{
   return FindMatchingIcon(content, path, NULL, NULL);
}

static char FindMatchingIcon(const char *content, const char *path,
                             const char **start_out, const char **end_out)
{
   const char *cursor = content;
   if(!content || !path) return 0;
   while((cursor = strstr(cursor, "<icon")) != NULL) {
      const char *gt = strchr(cursor, '>');
      const char *close;
      char *decoded;
      if(!gt) break;
      close = strstr(gt + 1, "</icon>");
      if(!close) break;
      decoded = XmlUnescapeRange(gt + 1, (size_t)(close - (gt + 1)));
      if(decoded && !strcmp(decoded, path)) {
         const char *start = cursor;
         const char *end = close + strlen("</icon>");
         const char *line = cursor;
         while(line > content && line[-1] != '\n') line--;
         {
            const char *q = line;
            while(q < cursor && (*q == ' ' || *q == '\t')) q++;
            if(q == cursor) start = line;
         }
         while(*end == ' ' || *end == '\t' || *end == '\r') end++;
         if(*end == '\n') end++;
         if(start_out) *start_out = start;
         if(end_out) *end_out = end;
         free(decoded);
         return 1;
      }
      free(decoded);
      cursor = close + strlen("</icon>");
   }
   return 0;
}

static void FindFreePosition(const char *content, int *x_out, int *y_out)
{
   int width = dmDisplay ? DisplayWidth(dmDisplay, dmScreen) : 1024;
   int height = dmDisplay ? DisplayHeight(dmDisplay, dmScreen) : 768;
   int x;
   int y;
   const int max_y = height > 220 ? height - 150 : height;

   for(y = 48; y < max_y; y += 92) {
      for(x = 48; x < width - 48; x += 96) {
         if(!PositionOccupied(content, x, y)) {
            *x_out = x;
            *y_out = y;
            return;
         }
      }
   }
   *x_out = 48;
   *y_out = 48;
}

static char PositionOccupied(const char *content, int x, int y)
{
   const char *cursor = content;
   while(content && (cursor = strstr(cursor, "<icon")) != NULL) {
      const char *gt = strchr(cursor, '>');
      int existing_x;
      int existing_y;
      if(!gt) break;
      if(ParseAttributeInteger(cursor, gt, "x", &existing_x)
         && ParseAttributeInteger(cursor, gt, "y", &existing_y)
         && abs(existing_x - x) < 72 && abs(existing_y - y) < 72) {
         return 1;
      }
      cursor = gt + 1;
   }
   return 0;
}

static int ParseAttributeInteger(const char *start, const char *end,
                                 const char *name, int *value_out)
{
   char pattern[32];
   const char *found;
   const char *value;
   char *number_end;
   long parsed;
   snprintf(pattern, sizeof(pattern), "%s=\"", name);
   found = start;
   while((found = strstr(found, pattern)) != NULL && found < end) {
      if(found == start || isspace((unsigned char)found[-1])) {
         value = found + strlen(pattern);
         errno = 0;
         parsed = strtol(value, &number_end, 10);
         if(errno == 0 && number_end > value && number_end < end
            && *number_end == '"') {
            *value_out = (int)parsed;
            return 1;
         }
      }
      found += strlen(pattern);
   }
   return 0;
}

static char WriteFileAtomic(const char *path, const char *content, size_t length)
{
   char temporary[PATH_MAX];
   char backup[PATH_MAX];
   struct stat st;
   mode_t mode = 0644;
   int fd;
   size_t written = 0;

   if(!path || !content || strlen(path) + 24 >= sizeof(temporary)) {
      return 0;
   }
   if(stat(path, &st) == 0) {
      mode = st.st_mode & 0777;
      snprintf(backup, sizeof(backup), "%s.essorawm-backup", path);
      CopyFile(path, backup);
   }
   snprintf(temporary, sizeof(temporary), "%s.essorawm-XXXXXX", path);
   fd = mkstemp(temporary);
   if(fd < 0) {
      return 0;
   }
   if(fchmod(fd, mode) < 0) {
      close(fd);
      unlink(temporary);
      return 0;
   }
   while(written < length) {
      ssize_t result = write(fd, content + written, length - written);
      if(result < 0) {
         if(errno == EINTR) continue;
         close(fd);
         unlink(temporary);
         return 0;
      }
      written += (size_t)result;
   }
   if(fsync(fd) < 0 || close(fd) < 0) {
      unlink(temporary);
      return 0;
   }
   if(rename(temporary, path) < 0) {
      unlink(temporary);
      return 0;
   }
   return 1;
}

static char CopyFile(const char *source, const char *target)
{
   int in;
   int out;
   char buffer[8192];
   ssize_t amount;
   in = open(source, O_RDONLY);
   if(in < 0) return 0;
   out = open(target, O_WRONLY | O_CREAT | O_TRUNC, 0600);
   if(out < 0) {
      close(in);
      return 0;
   }
   while((amount = read(in, buffer, sizeof(buffer))) > 0) {
      ssize_t done = 0;
      while(done < amount) {
         ssize_t result = write(out, buffer + done, (size_t)(amount - done));
         if(result < 0) {
            if(errno == EINTR) continue;
            close(in);
            close(out);
            return 0;
         }
         done += result;
      }
   }
   close(in);
   close(out);
   return amount == 0;
}

static int AddSelectedApplication(void)
{
   DesktopApp *app;
   char *pin_path;
   char *content;
   char *escaped_name;
   char *escaped_path;
   char *new_content;
   const char *closing;
   size_t content_length;
   size_t prefix_length;
   size_t icon_length;
   size_t suffix_length;
   int x;
   int y;
   char icon[PATH_MAX * 3];

   if(dmFilteredCount <= 0 || dmSelected < 0 || dmSelected >= dmFilteredCount) {
      return 0;
   }
   app = &dmApps.items[dmFiltered[dmSelected]];
   pin_path = GetPuppyPinPath();
   if(!pin_path || !EnsurePuppyPinDirectory(pin_path)) {
      free(pin_path);
      SetStatus(_("Could not create the PuppyPin configuration directory"));
      return 0;
   }

   content = ReadWholeFile(pin_path, &content_length);
   if(content && content_length == 0) {
      free(content);
      content = NULL;
   }
   if(!content) {
      content = CreateInitialPuppyPin();
      content_length = content ? strlen(content) : 0;
   }
   if(!content) {
      free(pin_path);
      SetStatus(_("Could not create PuppyPin"));
      return 0;
   }
   if(PuppyPinContainsPath(content, app->path)) {
      free(content);
      free(pin_path);
      app->pinned = 1;
      SetStatus(_("This application is already on the desktop"));
      return 0;
   }
   closing = strstr(content, "</pinboard>");
   if(!closing) {
      free(content);
      free(pin_path);
      SetStatus(_("PuppyPin is invalid: </pinboard> was not found"));
      return 0;
   }

   FindFreePosition(content, &x, &y);
   escaped_name = XmlEscape(app->name);
   escaped_path = XmlEscape(app->path);
   if(!escaped_name || !escaped_path) {
      free(escaped_name);
      free(escaped_path);
      free(content);
      free(pin_path);
      SetStatus(_("Not enough memory to update PuppyPin"));
      return 0;
   }
   snprintf(icon, sizeof(icon),
            "  <icon x=\"%d\" y=\"%d\" label=\"%s\">%s</icon>\n",
            x, y, escaped_name, escaped_path);
   free(escaped_name);
   free(escaped_path);

   prefix_length = (size_t)(closing - content);
   icon_length = strlen(icon);
   suffix_length = content_length - prefix_length;
   new_content = malloc(prefix_length + icon_length + suffix_length + 1);
   if(!new_content) {
      free(content);
      free(pin_path);
      SetStatus(_("Not enough memory to update PuppyPin"));
      return 0;
   }
   memcpy(new_content, content, prefix_length);
   memcpy(new_content + prefix_length, icon, icon_length);
   memcpy(new_content + prefix_length + icon_length, closing, suffix_length);
   new_content[prefix_length + icon_length + suffix_length] = 0;

   if(!WriteFileAtomic(pin_path, new_content,
                       prefix_length + icon_length + suffix_length)) {
      SetStatus(_("Could not write PuppyPin"));
      free(new_content);
      free(content);
      free(pin_path);
      return 0;
   }

   ReloadRoxPinboard(pin_path);
   free(new_content);
   free(content);
   free(pin_path);
   RefreshPinnedStates();
   SetStatus(_("Added to the desktop. EssoraWM will save the new position when you move it"));
   return 1;
}

static int RemoveSelectedApplication(void)
{
   DesktopApp *app;
   char *pin_path;
   char *content;
   size_t content_length;
   const char *start;
   const char *end;
   char *new_content;
   size_t prefix_length;
   size_t suffix_length;

   if(dmFilteredCount <= 0 || dmSelected < 0 || dmSelected >= dmFilteredCount) {
      return 0;
   }
   app = &dmApps.items[dmFiltered[dmSelected]];
   pin_path = GetPuppyPinPath();
   content = pin_path ? ReadWholeFile(pin_path, &content_length) : NULL;
   if(!content || !FindMatchingIcon(content, app->path, &start, &end)) {
      free(content);
      free(pin_path);
      app->pinned = 0;
      SetStatus(_("This application is not on the desktop"));
      return 0;
   }

   prefix_length = (size_t)(start - content);
   suffix_length = content_length - (size_t)(end - content);
   new_content = malloc(prefix_length + suffix_length + 1);
   if(!new_content) {
      free(content);
      free(pin_path);
      SetStatus(_("Not enough memory to update PuppyPin"));
      return 0;
   }
   memcpy(new_content, content, prefix_length);
   memcpy(new_content + prefix_length, end, suffix_length);
   new_content[prefix_length + suffix_length] = 0;

   if(!WriteFileAtomic(pin_path, new_content, prefix_length + suffix_length)) {
      SetStatus(_("Could not write PuppyPin"));
      free(new_content);
      free(content);
      free(pin_path);
      return 0;
   }

   ReloadRoxPinboard(pin_path);
   free(new_content);
   free(content);
   free(pin_path);
   RefreshPinnedStates();
   SetStatus(_("Removed from the desktop"));
   return 1;
}

static void ReloadRoxPinboard(const char *pin_path)
{
   (void)pin_path;
   /* El escritorio vive dentro de EssoraWM: pedir recarga por X11. */
   if(dmDisplay) {
      SendDesktopIconsReload(dmDisplay, RootWindow(dmDisplay, dmScreen));
   }
}


static char *JoinPath(const char *left, const char *right)
{
   size_t left_length;
   size_t right_length;
   char *result;
   char slash;
   if(!left || !right) return NULL;
   left_length = strlen(left);
   right_length = strlen(right);
   slash = left_length > 0 && left[left_length - 1] != '/';
   result = malloc(left_length + (slash ? 1U : 0U) + right_length + 1);
   if(!result) return NULL;
   memcpy(result, left, left_length);
   if(slash) result[left_length++] = '/';
   memcpy(result + left_length, right, right_length + 1);
   return result;
}

static void SetStatus(const char *text)
{
   snprintf(dmStatus, sizeof(dmStatus), "%s", text ? text : "");
}

static void EnsureSelectionVisible(void)
{
   const int visible_rows = DM_LIST_H / DM_ROW_H;
   if(dmSelected < dmTop) {
      dmTop = dmSelected;
   } else if(dmSelected >= dmTop + visible_rows) {
      dmTop = dmSelected - visible_rows + 1;
   }
   if(dmTop < 0) dmTop = 0;
}

static void SetDriveUiDefaults(void)
{
   memset(&dmDriveConfig, 0, sizeof(dmDriveConfig));
   dmDriveConfig.enabled = 1;
   dmDriveConfig.showInternal = 1;
   dmDriveConfig.showRemovable = 1;
   dmDriveConfig.showNetwork = 0;
   dmDriveConfig.showLabels = 1;
   dmDriveConfig.showFrame = 0;
   dmDriveConfig.vertical = 0;
   dmDriveConfig.reversePack = 1;
   dmDriveConfig.iconSize = 48;
   dmDriveConfig.spacingX = 112;
   dmDriveConfig.spacingY = 126;
   dmDriveConfig.xOffset = 0;
   dmDriveConfig.yOffset = -40;
   dmDriveConfig.horizontalAlign = 0;
   dmDriveConfig.verticalAlign = 2;
   snprintf(dmDriveConfig.openCommand, sizeof(dmDriveConfig.openCommand),
            "%s", "/usr/bin/essorawm-filemanager");
}

static void LoadDriveUiConfig(void)
{
   const char *home = getenv("HOME");
   char path[PATH_MAX];
   SetDriveUiDefaults();
   if(home && home[0]) {
      snprintf(path, sizeof(path), "%s/.config/essorafm/config.ini", home);
      ReadDriveIni(path);
      snprintf(path, sizeof(path), "%s/.config/essorawm/desktop.ini", home);
      ReadDriveIni(path);
   }
   dmDriveConfig.iconSize = DM_MAX(16, DM_MIN(128, dmDriveConfig.iconSize));
   dmDriveConfig.spacingX = DM_MAX(48, DM_MIN(320, dmDriveConfig.spacingX));
   dmDriveConfig.spacingY = DM_MAX(48, DM_MIN(320, dmDriveConfig.spacingY));
   dmDriveConfig.xOffset = DM_MAX(-500, DM_MIN(500, dmDriveConfig.xOffset));
   dmDriveConfig.yOffset = DM_MAX(-500, DM_MIN(500, dmDriveConfig.yOffset));
}

static char *TrimIniValue(char *value)
{
   char *end;
   while(*value && isspace((unsigned char)*value)) value++;
   end = value + strlen(value);
   while(end > value && isspace((unsigned char)end[-1])) *--end = 0;
   return value;
}

static void ReadDriveIni(const char *path)
{
   FILE *file = fopen(path, "r");
   char line[4096];
   char inMain = 0;
   if(!file) return;
   while(fgets(line, sizeof(line), file)) {
      char *text = TrimIniValue(line);
      char *equals;
      char *key;
      char *value;
      if(!text[0] || text[0] == '#' || text[0] == ';') continue;
      if(text[0] == '[') {
         inMain = !strcasecmp(text, "[Main]");
         continue;
      }
      if(!inMain) continue;
      equals = strchr(text, '=');
      if(!equals) continue;
      *equals = 0;
      key = TrimIniValue(text);
      value = TrimIniValue(equals + 1);
      if(!strcasecmp(key, "desktop_drive_icons"))
         dmDriveConfig.enabled = ParseBoolValue(value, dmDriveConfig.enabled);
      else if(!strcasecmp(key, "desktop_drive_show_internal"))
         dmDriveConfig.showInternal = ParseBoolValue(value, dmDriveConfig.showInternal);
      else if(!strcasecmp(key, "desktop_drive_show_removable"))
         dmDriveConfig.showRemovable = ParseBoolValue(value, dmDriveConfig.showRemovable);
      else if(!strcasecmp(key, "desktop_drive_show_network"))
         dmDriveConfig.showNetwork = ParseBoolValue(value, dmDriveConfig.showNetwork);
      else if(!strcasecmp(key, "ShowLabels"))
         dmDriveConfig.showLabels = ParseBoolValue(value, dmDriveConfig.showLabels);
      else if(!strcasecmp(key, "ShowFrame"))
         dmDriveConfig.showFrame = ParseBoolValue(value, dmDriveConfig.showFrame);
      else if(!strcasecmp(key, "Vertical"))
         dmDriveConfig.vertical = ParseBoolValue(value, dmDriveConfig.vertical);
      else if(!strcasecmp(key, "ReversePack"))
         dmDriveConfig.reversePack = ParseBoolValue(value, dmDriveConfig.reversePack);
      else if(!strcasecmp(key, "desktop_drive_icon_size"))
         dmDriveConfig.iconSize = atoi(value);
      else if(!strcasecmp(key, "SpacingX"))
         dmDriveConfig.spacingX = atoi(value);
      else if(!strcasecmp(key, "SpacingY"))
         dmDriveConfig.spacingY = atoi(value);
      else if(!strcasecmp(key, "XOffset"))
         dmDriveConfig.xOffset = atoi(value);
      else if(!strcasecmp(key, "YOffset"))
         dmDriveConfig.yOffset = atoi(value);
      else if(!strcasecmp(key, "XPos")) {
         double pos = atof(value);
         dmDriveConfig.horizontalAlign = pos < 0.25 ? 0 : pos > 0.75 ? 2 : 1;
      } else if(!strcasecmp(key, "YPos")) {
         double pos = atof(value);
         dmDriveConfig.verticalAlign = pos < 0.25 ? 0 : pos > 0.75 ? 2 : 1;
      } else if(!strcasecmp(key, "OpenCommand") && value[0]) {
         snprintf(dmDriveConfig.openCommand, sizeof(dmDriveConfig.openCommand),
                  "%s", value);
      }
   }
   fclose(file);
}

static char ParseBoolValue(const char *value, char fallback)
{
   if(!value) return fallback;
   if(!strcasecmp(value, "true") || !strcasecmp(value, "yes")
      || !strcasecmp(value, "on") || !strcmp(value, "1")) return 1;
   if(!strcasecmp(value, "false") || !strcasecmp(value, "no")
      || !strcasecmp(value, "off") || !strcmp(value, "0")) return 0;
   return fallback;
}

static char EnsureDirectoryTree(const char *path)
{
   char copy[PATH_MAX];
   char *p;
   if(!path || snprintf(copy, sizeof(copy), "%s", path) >= (int)sizeof(copy))
      return 0;
   for(p = copy + 1; *p; p++) {
      if(*p == '/') {
         *p = 0;
         if(mkdir(copy, 0700) != 0 && errno != EEXIST) return 0;
         *p = '/';
      }
   }
   if(mkdir(copy, 0700) != 0 && errno != EEXIST) return 0;
   return 1;
}

static char SaveDriveUiConfig(void)
{
   const char *home = getenv("HOME");
   char directory[PATH_MAX];
   char path[PATH_MAX];
   char content[4096];
   double xpos = dmDriveConfig.horizontalAlign == 0 ? 0.0
                : dmDriveConfig.horizontalAlign == 2 ? 1.0 : 0.5;
   double ypos = dmDriveConfig.verticalAlign == 0 ? 0.0
                : dmDriveConfig.verticalAlign == 2 ? 1.0 : 0.5;
   int length;
   if(!home || !home[0]) return 0;
   if(snprintf(directory, sizeof(directory), "%s/.config/essorawm", home)
      >= (int)sizeof(directory) || !EnsureDirectoryTree(directory)) return 0;
   if(snprintf(path, sizeof(path), "%s/desktop.ini", directory)
      >= (int)sizeof(path)) return 0;
   length = snprintf(content, sizeof(content),
      "[Main]\n"
      "desktop_drive_icons=%s\n"
      "desktop_drive_show_internal=%s\n"
      "desktop_drive_show_removable=%s\n"
      "desktop_drive_show_network=%s\n"
      "desktop_drive_icon_size=%d\n"
      "ShowLabels=%s\n"
      "ShowFrame=%s\n"
      "Vertical=%s\n"
      "ReversePack=%s\n"
      "SpacingX=%d\n"
      "SpacingY=%d\n"
      "XOffset=%d\n"
      "YOffset=%d\n"
      "XPos=%.1f\n"
      "YPos=%.1f\n"
      "OpenCommand=%s\n",
      dmDriveConfig.enabled ? "true" : "false",
      dmDriveConfig.showInternal ? "true" : "false",
      dmDriveConfig.showRemovable ? "true" : "false",
      dmDriveConfig.showNetwork ? "true" : "false",
      dmDriveConfig.iconSize,
      dmDriveConfig.showLabels ? "true" : "false",
      dmDriveConfig.showFrame ? "true" : "false",
      dmDriveConfig.vertical ? "true" : "false",
      dmDriveConfig.reversePack ? "true" : "false",
      dmDriveConfig.spacingX, dmDriveConfig.spacingY,
      dmDriveConfig.xOffset, dmDriveConfig.yOffset,
      xpos, ypos, dmDriveConfig.openCommand);
   if(length < 0 || length >= (int)sizeof(content)) return 0;
   return WriteFileAtomic(path, content, (size_t)length);
}

static void ApplyDriveUiConfig(void)
{
   if(!SaveDriveUiConfig()) {
      SetStatus(_("Could not save drive icon settings"));
      return;
   }
   SendDesktopIconsRelayout(dmDisplay, RootWindow(dmDisplay, dmScreen));
   dmDriveDirty = 0;
   SetStatus(_("Drive icon settings applied"));
}

static void ResetHiddenDrives(void)
{
   const char *home = getenv("HOME");
   char path[PATH_MAX];
   if(home && home[0]) {
      snprintf(path, sizeof(path), "%s/.config/essorawm/hidden-drives", home);
      unlink(path);
   }
   SendDesktopIconsReload(dmDisplay, RootWindow(dmDisplay, dmScreen));
   SetStatus(_("Hidden drives are visible again"));
}

static void HandleDriveClick(int x, int y)
{
   int changed = 1;
   if(HitRect(x, y, 35, 154, 350, 30)) dmDriveConfig.enabled ^= 1;
   else if(HitRect(x, y, 35, 194, 170, 30)) dmDriveConfig.showInternal ^= 1;
   else if(HitRect(x, y, 215, 194, 170, 30)) dmDriveConfig.showRemovable ^= 1;
   else if(HitRect(x, y, 35, 234, 170, 30)) dmDriveConfig.showNetwork ^= 1;
   else if(HitRect(x, y, 215, 234, 170, 30)) dmDriveConfig.showLabels ^= 1;
   else if(HitRect(x, y, 35, 274, 170, 30)) dmDriveConfig.showFrame ^= 1;
   else if(HitRect(x, y, 215, 274, 170, 30)) dmDriveConfig.reversePack ^= 1;
   else if(HitRect(x, y, 430, 154, 175, 30)) dmDriveConfig.vertical = 0;
   else if(HitRect(x, y, 615, 154, 175, 30)) dmDriveConfig.vertical = 1;
   else if(HitRect(x, y, 430, 220, 115, 30)) dmDriveConfig.horizontalAlign = 0;
   else if(HitRect(x, y, 552, 220, 115, 30)) dmDriveConfig.horizontalAlign = 1;
   else if(HitRect(x, y, 674, 220, 115, 30)) dmDriveConfig.horizontalAlign = 2;
   else if(HitRect(x, y, 430, 276, 115, 30)) dmDriveConfig.verticalAlign = 0;
   else if(HitRect(x, y, 552, 276, 115, 30)) dmDriveConfig.verticalAlign = 1;
   else if(HitRect(x, y, 674, 276, 115, 30)) dmDriveConfig.verticalAlign = 2;
   else {
      static const int rows[] = {338, 380, 422, 464, 506};
      int control;
      changed = 0;
      for(control = 0; control < 5; control++) {
         int *value = control == 0 ? &dmDriveConfig.iconSize
                    : control == 1 ? &dmDriveConfig.spacingX
                    : control == 2 ? &dmDriveConfig.spacingY
                    : control == 3 ? &dmDriveConfig.xOffset
                    : &dmDriveConfig.yOffset;
         int minimum = control <= 2 ? (control == 0 ? 16 : 48) : -500;
         int maximum = control == 0 ? 128 : control <= 2 ? 320 : 500;
         int step = control == 0 ? 8 : control <= 2 ? 8 : 10;
         if(HitRect(x, y, 690, rows[control], 34, 30)) {
            *value = DM_MAX(minimum, *value - step);
            changed = 1;
            break;
         }
         if(HitRect(x, y, 792, rows[control], 34, 30)) {
            *value = DM_MIN(maximum, *value + step);
            changed = 1;
            break;
         }
      }
   }
   if(changed) {
      dmDriveDirty = 1;
      SetStatus(_("Drive icon settings changed; press Apply"));
   }
}

static void DrawWindow(void)
{
   const unsigned long bg = RGBToPixel(0x20, 0x20, 0x20);
   const unsigned long panel = RGBToPixel(0x2b, 0x2b, 0x2b);
   const unsigned long white = RGBToPixel(0xff, 0xff, 0xff);
   const unsigned long muted = RGBToPixel(0xa8, 0xb4, 0xbd);
   XSetForeground(dmDisplay, dmGC, bg);
   XFillRectangle(dmDisplay, dmWindow, dmGC, 0, 0, DM_WINDOW_W, DM_WINDOW_H);
   XSetForeground(dmDisplay, dmGC, panel);
   XFillRectangle(dmDisplay, dmWindow, dmGC, 0, 0, DM_WINDOW_W, DM_HEADER_H);
   DrawTextAt(DM_MARGIN, 27, _("EssoraWM Desktop Manager"), white);
   DrawTextAt(DM_MARGIN, 49,
              _("Applications and native drive icons"), muted);
   DrawTab(DM_MARGIN, _("Applications"), dmPage == DM_PAGE_APPS);
   DrawTab(DM_MARGIN + DM_TAB_W + 8, _("Drive icons"),
           dmPage == DM_PAGE_DRIVES);
   if(dmPage == DM_PAGE_APPS) DrawApplicationsPage();
   else DrawDrivePage();
   DrawTextAt(DM_MARGIN, DM_STATUS_Y, dmStatus, muted);
   XFlush(dmDisplay);
}

static void DrawApplicationsPage(void)
{
   const unsigned long panel = RGBToPixel(0x2b, 0x2b, 0x2b);
   const unsigned long green = RGBToPixel(0x77, 0x96, 0x0a);
   const unsigned long white = RGBToPixel(0xff, 0xff, 0xff);
   const unsigned long muted = RGBToPixel(0xa8, 0xb4, 0xbd);
   const unsigned long selectedBg = RGBToPixel(0x36, 0x46, 0x1a);
   const int visibleRows = DM_LIST_H / DM_ROW_H;
   const int addX = DM_WINDOW_W - 322;
   const int removeX = DM_WINDOW_W - 216;
   const int closeX = DM_WINDOW_W - 110;
   int row;
   char line[1024];
   DesktopApp *selectedApp = NULL;

   DrawTextAt(DM_MARGIN, DM_SEARCH_Y - 8, _("Search"), muted);
   XSetForeground(dmDisplay, dmGC, green);
   XFillRectangle(dmDisplay, dmWindow, dmGC,
                  DM_MARGIN, DM_SEARCH_Y, DM_WINDOW_W - 2 * DM_MARGIN, DM_SEARCH_H);
   XSetForeground(dmDisplay, dmGC, panel);
   XFillRectangle(dmDisplay, dmWindow, dmGC,
                  DM_MARGIN + 1, DM_SEARCH_Y + 1,
                  DM_WINDOW_W - 2 * DM_MARGIN - 2, DM_SEARCH_H - 2);
   snprintf(line, sizeof(line), "%s%s", dmSearch,
            dmSearch[0] ? "_" : _("Type to filter applications"));
   DrawTextAt(DM_MARGIN + 10, DM_SEARCH_Y + 22, line,
              dmSearch[0] ? white : muted);

   XSetForeground(dmDisplay, dmGC, panel);
   XFillRectangle(dmDisplay, dmWindow, dmGC,
                  DM_MARGIN, DM_LIST_Y, DM_WINDOW_W - 2 * DM_MARGIN, DM_LIST_H);
   for(row = 0; row < visibleRows; row++) {
      int filteredIndex = dmTop + row;
      int appIndex;
      DesktopApp *app;
      int y = DM_LIST_Y + row * DM_ROW_H;
      if(filteredIndex >= dmFilteredCount) break;
      appIndex = dmFiltered[filteredIndex];
      app = &dmApps.items[appIndex];
      if(filteredIndex == dmSelected) {
         XSetForeground(dmDisplay, dmGC, selectedBg);
         XFillRectangle(dmDisplay, dmWindow, dmGC,
                        DM_MARGIN + 2, y + 1,
                        DM_WINDOW_W - 2 * DM_MARGIN - 4, DM_ROW_H - 2);
      }
      snprintf(line, sizeof(line), "%s  %s",
               app->pinned ? "[x]" : "[ ]", app->name);
      DrawTextAt(DM_MARGIN + 10, y + 18, line,
                 filteredIndex == dmSelected ? white : muted);
   }
   if(dmFilteredCount > 0 && dmSelected >= 0 && dmSelected < dmFilteredCount)
      selectedApp = &dmApps.items[dmFiltered[dmSelected]];
   snprintf(line, sizeof(line), "%d / %d", dmFilteredCount, dmApps.count);
   DrawTextAt(DM_MARGIN, DM_LIST_Y + DM_LIST_H + 20, line, muted);
   DrawButton(addX, DM_BUTTON_Y, 96, DM_BUTTON_H,
              _("Add"), selectedApp && !selectedApp->pinned);
   DrawButton(removeX, DM_BUTTON_Y, 96, DM_BUTTON_H,
              _("Remove"), selectedApp && selectedApp->pinned);
   DrawButton(closeX, DM_BUTTON_Y, 86, DM_BUTTON_H, _("Close"), 1);
}

static void DrawDrivePage(void)
{
   const unsigned long panel = RGBToPixel(0x2b, 0x2b, 0x2b);
   const unsigned long muted = RGBToPixel(0xa8, 0xb4, 0xbd);
   const unsigned long white = RGBToPixel(0xff, 0xff, 0xff);
   const int applyX = DM_WINDOW_W - 350;
   const int resetX = DM_WINDOW_W - 238;
   const int closeX = DM_WINDOW_W - 110;

   XSetForeground(dmDisplay, dmGC, panel);
   XFillRectangle(dmDisplay, dmWindow, dmGC, 18, 124, 382, 420);
   XFillRectangle(dmDisplay, dmWindow, dmGC, 415, 124, 407, 420);
   DrawTextAt(35, 145, _("Visibility"), white);
   DrawToggle(35, 154, 350, _("Show drive icons"), dmDriveConfig.enabled);
   DrawToggle(35, 194, 170, _("Internal drives"), dmDriveConfig.showInternal);
   DrawToggle(215, 194, 170, _("Removable drives"), dmDriveConfig.showRemovable);
   DrawToggle(35, 234, 170, _("Network drives"), dmDriveConfig.showNetwork);
   DrawToggle(215, 234, 170, _("Show labels"), dmDriveConfig.showLabels);
   DrawToggle(35, 274, 170, _("Label frame"), dmDriveConfig.showFrame);
   DrawToggle(215, 274, 170, _("Reverse order"), dmDriveConfig.reversePack);
   DrawTextAt(35, 330,
      _("You can also drag each drive icon freely on the desktop."), muted);
   DrawTextAt(35, 354,
      _("Apply arranges all drives using the selected layout."), muted);

   DrawTextAt(430, 145, _("Layout"), white);
   DrawTextAt(430, 151, "", muted);
   DrawChoice(430, 154, 175, _("Horizontal"), !dmDriveConfig.vertical);
   DrawChoice(615, 154, 175, _("Vertical"), dmDriveConfig.vertical);
   DrawTextAt(430, 211, _("Horizontal position"), muted);
   DrawChoice(430, 220, 115, _("Left"), dmDriveConfig.horizontalAlign == 0);
   DrawChoice(552, 220, 115, _("Center"), dmDriveConfig.horizontalAlign == 1);
   DrawChoice(674, 220, 115, _("Right"), dmDriveConfig.horizontalAlign == 2);
   DrawTextAt(430, 267, _("Vertical position"), muted);
   DrawChoice(430, 276, 115, _("Top"), dmDriveConfig.verticalAlign == 0);
   DrawChoice(552, 276, 115, _("Center"), dmDriveConfig.verticalAlign == 1);
   DrawChoice(674, 276, 115, _("Bottom"), dmDriveConfig.verticalAlign == 2);
   DrawStepper(338, _("Icon size"), dmDriveConfig.iconSize, 16, 128, 8, 0);
   DrawStepper(380, _("Horizontal spacing"), dmDriveConfig.spacingX, 48, 320, 8, 1);
   DrawStepper(422, _("Vertical spacing"), dmDriveConfig.spacingY, 48, 320, 8, 2);
   DrawStepper(464, _("Horizontal offset"), dmDriveConfig.xOffset, -500, 500, 10, 3);
   DrawStepper(506, _("Vertical offset"), dmDriveConfig.yOffset, -500, 500, 10, 4);

   DrawButton(DM_MARGIN, DM_BUTTON_Y, 190, DM_BUTTON_H,
              _("Show hidden drives"), 1);
   DrawButton(applyX, DM_BUTTON_Y, 102, DM_BUTTON_H,
              dmDriveDirty ? _("Apply") : _("Applied"), 1);
   DrawButton(resetX, DM_BUTTON_Y, 118, DM_BUTTON_H,
              _("Defaults"), 1);
   DrawButton(closeX, DM_BUTTON_Y, 86, DM_BUTTON_H, _("Close"), 1);
}

static void DrawTab(int x, const char *label, char active)
{
   DrawButton(x, DM_TAB_Y, DM_TAB_W, DM_TAB_H, label, active);
}

static void DrawToggle(int x, int y, int width, const char *label, char active)
{
   char text[512];
   snprintf(text, sizeof(text), "%s %s", active ? "[x]" : "[ ]", label);
   DrawButton(x, y, width, 30, text, 1);
}

static void DrawChoice(int x, int y, int width, const char *label, char active)
{
   const unsigned long green = RGBToPixel(0x77, 0x96, 0x0a);
   const unsigned long panel = RGBToPixel(active ? 0x36 : 0x24,
                                          active ? 0x46 : 0x24,
                                          active ? 0x1a : 0x24);
   const unsigned long white = RGBToPixel(0xff, 0xff, 0xff);
   XSetForeground(dmDisplay, dmGC, active ? green : RGBToPixel(0x55,0x55,0x55));
   XFillRectangle(dmDisplay, dmWindow, dmGC, x, y, width, 30);
   XSetForeground(dmDisplay, dmGC, panel);
   XFillRectangle(dmDisplay, dmWindow, dmGC, x + 1, y + 1, width - 2, 28);
   DrawTextAt(x + (width - TextWidth(label)) / 2, y + 20, label, white);
}

static void DrawStepper(int y, const char *label, int value, int minimum,
                        int maximum, int step, int control)
{
   char number[64];
   (void)minimum; (void)maximum; (void)step; (void)control;
   DrawTextAt(430, y + 20, label, RGBToPixel(0xa8, 0xb4, 0xbd));
   DrawButton(690, y, 34, 30, "-", 1);
   snprintf(number, sizeof(number), "%d", value);
   DrawChoice(730, y, 58, number, 1);
   DrawButton(792, y, 34, 30, "+", 1);
}

static int TextWidth(const char *text)
{
   if(!text) return 0;
   if(dmFontSet) return XmbTextEscapement(dmFontSet, text, (int)strlen(text));
   if(dmFont) return XTextWidth(dmFont, text, (int)strlen(text));
   return (int)strlen(text) * 8;
}

static void DrawTextAt(int x, int y, const char *text, unsigned long color)
{
   if(!text) return;
   XSetForeground(dmDisplay, dmGC, color);
   if(dmFontSet) {
      XmbDrawString(dmDisplay, dmWindow, dmFontSet, dmGC,
                    x, y, text, (int)strlen(text));
   } else {
      XDrawString(dmDisplay, dmWindow, dmGC, x, y, text, (int)strlen(text));
   }
}

static void DrawButton(int x, int y, int width, int height,
                       const char *label, char active)
{
   const unsigned long green = RGBToPixel(0x77, 0x96, 0x0a);
   const unsigned long border = active ? green : RGBToPixel(0x4a,0x4a,0x4a);
   const unsigned long fill = active
                            ? RGBToPixel(0x2b, 0x2b, 0x2b)
                            : RGBToPixel(0x18, 0x18, 0x18);
   const unsigned long text = active
                            ? RGBToPixel(0xff, 0xff, 0xff)
                            : RGBToPixel(0x70, 0x70, 0x70);
   int textX = x + 12;
   if(label) textX = x + (width - TextWidth(label)) / 2;
   XSetForeground(dmDisplay, dmGC, border);
   XFillRectangle(dmDisplay, dmWindow, dmGC, x, y, width, height);
   XSetForeground(dmDisplay, dmGC, fill);
   XFillRectangle(dmDisplay, dmWindow, dmGC,
                  x + 1, y + 1, width - 2, height - 2);
   DrawTextAt(textX, y + (height + 10) / 2, label, text);
}

static char HitRect(int x, int y, int bx, int by, int bw, int bh)
{
   return x >= bx && x <= bx + bw && y >= by && y <= by + bh;
}

static int MaskShift(unsigned long mask)
{
   int shift = 0;
   if(!mask) return 0;
   while((mask & 1UL) == 0) {
      mask >>= 1;
      shift++;
   }
   return shift;
}

static int MaskBits(unsigned long mask)
{
   int bits = 0;
   while(mask) {
      if(mask & 1UL) bits++;
      mask >>= 1;
   }
   return bits;
}

static unsigned long ScaleToMask(unsigned char value, unsigned long mask)
{
   const int bits = MaskBits(mask);
   const int shift = MaskShift(mask);
   unsigned long maximum;
   if(bits <= 0) return 0;
   maximum = (1UL << bits) - 1UL;
   return ((unsigned long)value * maximum / 255UL) << shift;
}

static unsigned long RGBToPixel(unsigned char r, unsigned char g, unsigned char b)
{
   if(dmVisual && dmVisual->class == TrueColor) {
      return ScaleToMask(r, dmVisual->red_mask)
           | ScaleToMask(g, dmVisual->green_mask)
           | ScaleToMask(b, dmVisual->blue_mask);
   } else {
      XColor color;
      color.red = (unsigned short)r << 8;
      color.green = (unsigned short)g << 8;
      color.blue = (unsigned short)b << 8;
      color.flags = DoRed | DoGreen | DoBlue;
      XAllocColor(dmDisplay, dmColormap, &color);
      return color.pixel;
   }
}

static void CloseDisplay(void)
{
   if(dmFontSet) {
      XFreeFontSet(dmDisplay, dmFontSet);
      dmFontSet = NULL;
   }
   if(dmFont) {
      XFreeFont(dmDisplay, dmFont);
      dmFont = NULL;
   }
   if(dmGC) {
      XFreeGC(dmDisplay, dmGC);
      dmGC = None;
   }
   if(dmWindow != None) {
      XDestroyWindow(dmDisplay, dmWindow);
      dmWindow = None;
   }
   if(dmDisplay) {
      XCloseDisplay(dmDisplay);
      dmDisplay = NULL;
   }
}
