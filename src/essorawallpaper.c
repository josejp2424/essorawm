/**
 * @file essorawallpaper.c
 * @brief Native EssoraWM wallpaper selector.
 *
 * agregado por josejp2424
 *
 * Selector de wallpaper integrado en el binario jwm/EssoraWM.
 * - Busca imágenes en /usr/share/backgrounds.
 * - Muestra un carrusel simple con miniaturas.
 * - La ventana se centra automáticamente en la pantalla.
 * - El mouse solo selecciona/navega; la rueda y otros botones se ignoran.
 * - Aplica solo al presionar Apply o Enter.
 * - Guarda en ~/.config/essorawm/wallpaper.
 * - Si existe wbar en ejecución, se reinicia al aplicar para limpiar el marco negro.
 * - Si existe $HOME/Choices/ROX-Filer/PuppyPin, reemplaza solo la ruta del
 *   <backdrop ...> manteniendo el style, por ejemplo Stretched.
 */

#include "jwm.h"
#include "essorawallpaper.h"
#include "image.h"
#include "main.h"
#include "misc.h"
#include "error.h"

#ifndef MAKE_DEPEND
#  include <dirent.h>
#  include <sys/stat.h>
#  include <sys/types.h>
#  include <errno.h>
#  include <limits.h>
#endif

#define WALLPAPER_DIR "/usr/share/backgrounds"
#define ESSORAWM_ICON "/usr/share/jwm/essorawm.svg" /* agregado por josejp2424 */
#define CONFIG_SUBDIR ".config/essorawm"
#define CONFIG_FILE ".config/essorawm/wallpaper"
#define WINDOW_W 820
#define WINDOW_H 500
#define HEADER_H 64
#define BUTTON_H 34
#define MARGIN 18
#define PREVIEW_W 500
#define PREVIEW_H 285
#define SIDE_W 120
#define SIDE_H 78
#define MAX_WALLPAPERS 1024

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

typedef struct WallpaperItem {
   char *path;
   char *name;
} WallpaperItem;

typedef struct WallpaperList {
   WallpaperItem items[MAX_WALLPAPERS];
   int count;
} WallpaperList;

static Display *wpDisplay = NULL;
static Window wpWindow = None;
static GC wpGC = None;
static int wpScreen = 0;
static Visual *wpVisual = NULL;
static Colormap wpColormap = None;
static int wpDepth = 0;
static WallpaperList wpList;
static int wpSelected = -1;
static char wpOwnDisplay = 0;
static Atom wpDeleteAtom = None;

static char InitWallpaperDisplay(void);
static void CloseWallpaperDisplay(void);
static void DrawWallpaperWindow(void);
static void SetWallpaperWindowIcon(void);
static void DrawText(int x, int y, const char *text, unsigned long color);
static void DrawButton(int x, int y, int w, int h, const char *label, char active);
static int HitButton(int x, int y, int bx, int by, int bw, int bh);
static void DrawImageNodeFit(Drawable d, ImageNode *image, int bx, int by, int bw, int bh, int bg);
static unsigned long RGBToPixel(unsigned char r, unsigned char g, unsigned char b);
static int MaskShift(unsigned long mask);
static int MaskBits(unsigned long mask);
static unsigned long ScaleToMask(unsigned char value, unsigned long mask);
static void ScanWallpaperDir(WallpaperList *list, const char *dir);
static void AddWallpaper(WallpaperList *list, const char *path);
static char IsImageFile(const char *name);
static void FreeWallpaperList(WallpaperList *list);
static void SelectPrevious(void);
static void SelectNext(void);
static char *GetConfigPath(void);
static void EnsureConfigDir(void);
static void SaveWallpaper(const char *path);
static char *ReadWallpaperPath(void);
static char *QuoteShell(const char *path);
static char HasCommand(const char *cmd);
static void ApplyWallpaper(const char *path, char save);
static void UpdatePuppyPin(const char *path);
static void RestartWbar(void);

/* agregado por josejp2424 */
void RunEssoraWallpaperSelector(void)
{
   XEvent event;
   int running = 1;
   const int leftX = 32;
   const int rightX = WINDOW_W - 118;
   const int applyX = WINDOW_W - 210;
   const int cancelX = WINDOW_W - 110;
   const int buttonY = WINDOW_H - 48;

   if(!InitWallpaperDisplay()) {
      return;
   }

   memset(&wpList, 0, sizeof(wpList));
   ScanWallpaperDir(&wpList, WALLPAPER_DIR);
   wpSelected = wpList.count > 0 ? 0 : -1;

   wpWindow = XCreateSimpleWindow(wpDisplay, RootWindow(wpDisplay, wpScreen),
                                  0, 0, WINDOW_W, WINDOW_H, 1,
                                  RGBToPixel(0x77, 0x96, 0x0A),
                                  RGBToPixel(0x20, 0x20, 0x20));

   /* agregado por josejp2424: centrar siempre el selector en la pantalla. */
   XMoveWindow(wpDisplay, wpWindow,
               MAX(0, (DisplayWidth(wpDisplay, wpScreen) - WINDOW_W) / 2),
               MAX(0, (DisplayHeight(wpDisplay, wpScreen) - WINDOW_H) / 2));

   wpDeleteAtom = XInternAtom(wpDisplay, "WM_DELETE_WINDOW", False);
   XSetWMProtocols(wpDisplay, wpWindow, &wpDeleteAtom, 1);
   XStoreName(wpDisplay, wpWindow, _("EssoraWM Wallpaper"));
   XSetIconName(wpDisplay, wpWindow, _("EssoraWM Wallpaper"));
   SetWallpaperWindowIcon();

   XSelectInput(wpDisplay, wpWindow,
                ExposureMask | ButtonPressMask | KeyPressMask | StructureNotifyMask);
   wpGC = XCreateGC(wpDisplay, wpWindow, 0, NULL);
   XMapRaised(wpDisplay, wpWindow);

   while(running) {
      XNextEvent(wpDisplay, &event);
      switch(event.type) {
      case Expose:
         if(event.xexpose.count == 0) {
            DrawWallpaperWindow();
         }
         break;
      case ButtonPress:
         /* agregado por josejp2424: ignorar rueda, botón central y clic derecho. */
         if(event.xbutton.button != Button1) {
            break;
         }
         if(HitButton(event.xbutton.x, event.xbutton.y, leftX, buttonY, 86, BUTTON_H)) {
            SelectPrevious();
            DrawWallpaperWindow();
         } else if(HitButton(event.xbutton.x, event.xbutton.y, rightX, buttonY, 86, BUTTON_H)) {
            SelectNext();
            DrawWallpaperWindow();
         } else if(HitButton(event.xbutton.x, event.xbutton.y, applyX, buttonY, 86, BUTTON_H)) {
            if(wpSelected >= 0 && wpSelected < wpList.count) {
               ApplyWallpaper(wpList.items[wpSelected].path, 1);
            }
            running = 0;
         } else if(HitButton(event.xbutton.x, event.xbutton.y, cancelX, buttonY, 86, BUTTON_H)) {
            running = 0;
         } else {
            /* agregado por josejp2424: zonas laterales seleccionan anterior/siguiente. */
            if(wpList.count > 1 && event.xbutton.y >= HEADER_H && event.xbutton.y <= HEADER_H + PREVIEW_H) {
               if(event.xbutton.x < (WINDOW_W - PREVIEW_W) / 2) {
                  SelectPrevious();
                  DrawWallpaperWindow();
               } else if(event.xbutton.x > (WINDOW_W + PREVIEW_W) / 2) {
                  SelectNext();
                  DrawWallpaperWindow();
               }
            }
         }
         break;
      case KeyPress: {
         KeySym sym = XLookupKeysym(&event.xkey, 0);
         if(sym == XK_Escape || sym == XK_q) {
            running = 0;
         } else if(sym == XK_Return || sym == XK_KP_Enter) {
            if(wpSelected >= 0 && wpSelected < wpList.count) {
               ApplyWallpaper(wpList.items[wpSelected].path, 1);
            }
            running = 0;
         } else if(sym == XK_Right || sym == XK_Down || sym == XK_n) {
            SelectNext();
            DrawWallpaperWindow();
         } else if(sym == XK_Left || sym == XK_Up || sym == XK_p) {
            SelectPrevious();
            DrawWallpaperWindow();
         }
         break;
      }
      case ClientMessage:
         if((Atom)event.xclient.data.l[0] == wpDeleteAtom) {
            running = 0;
         }
         break;
      case DestroyNotify:
         running = 0;
         break;
      }
   }

   if(wpGC) {
      XFreeGC(wpDisplay, wpGC);
      wpGC = None;
   }
   if(wpWindow) {
      XDestroyWindow(wpDisplay, wpWindow);
      XSync(wpDisplay, False);
      wpWindow = None;
   }
   FreeWallpaperList(&wpList);
   CloseWallpaperDisplay();
}

/* agregado por josejp2424 */
void RestoreEssoraWallpaper(void)
{
   char *path = ReadWallpaperPath();
   if(path && path[0]) {
      ApplyWallpaper(path, 0);
   }
   if(path) {
      Release(path);
   }
}

static char InitWallpaperDisplay(void)
{
   if(display) {
      wpDisplay = display;
      wpScreen = rootScreen;
      wpVisual = rootVisual;
      wpColormap = rootColormap;
      wpDepth = rootDepth;
      wpOwnDisplay = 0;
      return 1;
   }

   wpDisplay = XOpenDisplay(NULL);
   if(!wpDisplay) {
      fprintf(stderr, "EssoraWM Wallpaper: could not open display\n");
      return 0;
   }
   wpScreen = DefaultScreen(wpDisplay);
   wpVisual = DefaultVisual(wpDisplay, wpScreen);
   wpColormap = DefaultColormap(wpDisplay, wpScreen);
   wpDepth = DefaultDepth(wpDisplay, wpScreen);
   wpOwnDisplay = 1;
   return 1;
}

static void CloseWallpaperDisplay(void)
{
   if(wpOwnDisplay && wpDisplay) {
      XCloseDisplay(wpDisplay);
   }
   wpDisplay = NULL;
   wpOwnDisplay = 0;
}

static void SetWallpaperWindowIcon(void)
{
   /* agregado por josejp2424: usar /usr/share/jwm/essorawm.svg como icono del panel/marco. */
   ImageNode *icon = LoadImage(ESSORAWM_ICON, 64, 64, 1);
   Atom netWmIcon;
   unsigned long *data;
   int i;

   if(!icon || !icon->data || icon->bitmap) {
      if(icon) {
         DestroyImage(icon);
      }
      return;
   }

   data = Allocate(sizeof(unsigned long) * (2 + icon->width * icon->height));
   data[0] = icon->width;
   data[1] = icon->height;
   for(i = 0; i < icon->width * icon->height; i++) {
      unsigned int a = icon->data[i * 4 + 0];
      unsigned int r = icon->data[i * 4 + 1];
      unsigned int g = icon->data[i * 4 + 2];
      unsigned int b = icon->data[i * 4 + 3];
      data[2 + i] = (a << 24) | (r << 16) | (g << 8) | b;
   }
   netWmIcon = XInternAtom(wpDisplay, "_NET_WM_ICON", False);
   XChangeProperty(wpDisplay, wpWindow, netWmIcon, XA_CARDINAL, 32,
                   PropModeReplace, (unsigned char*)data,
                   2 + icon->width * icon->height);
   Release(data);
   DestroyImage(icon);
}

static void DrawWallpaperWindow(void)
{
   const unsigned long bg = RGBToPixel(0x20, 0x20, 0x20);
   const unsigned long panel = RGBToPixel(0x2b, 0x2b, 0x2b);
   const unsigned long green = RGBToPixel(0x77, 0x96, 0x0A);
   const unsigned long white = RGBToPixel(0xff, 0xff, 0xff);
   const unsigned long muted = RGBToPixel(0x9f, 0xb0, 0xc0);
   const int centerX = (WINDOW_W - PREVIEW_W) / 2;
   const int previewY = HEADER_H + 18;
   char counter[64];
   ImageNode *img;

   XSetForeground(wpDisplay, wpGC, bg);
   XFillRectangle(wpDisplay, wpWindow, wpGC, 0, 0, WINDOW_W, WINDOW_H);

   XSetForeground(wpDisplay, wpGC, panel);
   XFillRectangle(wpDisplay, wpWindow, wpGC, 0, 0, WINDOW_W, HEADER_H);

   img = LoadImage(ESSORAWM_ICON, 42, 42, 1);
   if(img) {
      DrawImageNodeFit(wpWindow, img, MARGIN, 10, 42, 42, 0x2b2b2b);
      DestroyImage(img);
   }

   DrawText(72, 28, _("EssoraWM Wallpaper"), white);
   DrawText(72, 48, _("Select a wallpaper from /usr/share/backgrounds"), muted);

   if(wpList.count <= 0) {
      DrawText(MARGIN, HEADER_H + 80, _("No wallpapers found"), white);
      DrawButton(WINDOW_W - 110, WINDOW_H - 48, 86, BUTTON_H, _("Cancel"), 1);
      XFlush(wpDisplay);
      return;
   }

   /* agregado por josejp2424: carrusel simple, similar al selector de EssoraFM. */
   if(wpList.count > 1) {
      int prev = (wpSelected - 1 + wpList.count) % wpList.count;
      int next = (wpSelected + 1) % wpList.count;
      ImageNode *left = LoadImage(wpList.items[prev].path, SIDE_W, SIDE_H, 1);
      ImageNode *right = LoadImage(wpList.items[next].path, SIDE_W, SIDE_H, 1);
      if(left) {
         XSetForeground(wpDisplay, wpGC, RGBToPixel(0x10, 0x10, 0x10));
         XFillRectangle(wpDisplay, wpWindow, wpGC, MARGIN, previewY + 90, SIDE_W, SIDE_H);
         DrawImageNodeFit(wpWindow, left, MARGIN, previewY + 90, SIDE_W, SIDE_H, 0x101010);
         DestroyImage(left);
      }
      if(right) {
         XSetForeground(wpDisplay, wpGC, RGBToPixel(0x10, 0x10, 0x10));
         XFillRectangle(wpDisplay, wpWindow, wpGC, WINDOW_W - MARGIN - SIDE_W, previewY + 90, SIDE_W, SIDE_H);
         DrawImageNodeFit(wpWindow, right, WINDOW_W - MARGIN - SIDE_W, previewY + 90, SIDE_W, SIDE_H, 0x101010);
         DestroyImage(right);
      }
   }

   XSetForeground(wpDisplay, wpGC, green);
   XFillRectangle(wpDisplay, wpWindow, wpGC, centerX - 4, previewY - 4, PREVIEW_W + 8, PREVIEW_H + 8);
   XSetForeground(wpDisplay, wpGC, RGBToPixel(0x10, 0x10, 0x10));
   XFillRectangle(wpDisplay, wpWindow, wpGC, centerX, previewY, PREVIEW_W, PREVIEW_H);

   img = LoadImage(wpList.items[wpSelected].path, PREVIEW_W, PREVIEW_H, 1);
   if(img) {
      DrawImageNodeFit(wpWindow, img, centerX, previewY, PREVIEW_W, PREVIEW_H, 0x101010);
      DestroyImage(img);
   } else {
      DrawText(centerX + 20, previewY + 80, _("No preview"), muted);
   }

   snprintf(counter, sizeof(counter), "%d / %d", wpSelected + 1, wpList.count);
   DrawText(centerX, previewY + PREVIEW_H + 26, wpList.items[wpSelected].name, white);
   DrawText(WINDOW_W - MARGIN - 70, previewY + PREVIEW_H + 26, counter, muted);

   DrawButton(32, WINDOW_H - 48, 86, BUTTON_H, _("Left"), wpList.count > 1);
   DrawButton(WINDOW_W - 118, WINDOW_H - 48, 86, BUTTON_H, _("Right"), wpList.count > 1);
   DrawButton(WINDOW_W - 210, WINDOW_H - 48, 86, BUTTON_H, _("Apply"), wpSelected >= 0);
   DrawButton(WINDOW_W - 110, WINDOW_H - 48, 86, BUTTON_H, _("Cancel"), 1);

   DrawText(MARGIN, WINDOW_H - 66, wpList.items[wpSelected].path, muted);
   XFlush(wpDisplay);
}

static void DrawText(int x, int y, const char *text, unsigned long color)
{
   if(!text) {
      return;
   }
   XSetForeground(wpDisplay, wpGC, color);
   XDrawString(wpDisplay, wpWindow, wpGC, x, y, text, strlen(text));
}

static void DrawButton(int x, int y, int w, int h, const char *label, char active)
{
   unsigned long border = RGBToPixel(0x77, 0x96, 0x0A);
   unsigned long fill = active ? RGBToPixel(0x2b, 0x2b, 0x2b) : RGBToPixel(0x1a, 0x1a, 0x1a);
   unsigned long text = active ? RGBToPixel(0xff, 0xff, 0xff) : RGBToPixel(0x77, 0x77, 0x77);
   XSetForeground(wpDisplay, wpGC, border);
   XFillRectangle(wpDisplay, wpWindow, wpGC, x, y, w, h);
   XSetForeground(wpDisplay, wpGC, fill);
   XFillRectangle(wpDisplay, wpWindow, wpGC, x + 1, y + 1, w - 2, h - 2);
   DrawText(x + 16, y + 22, label, text);
}

static int HitButton(int x, int y, int bx, int by, int bw, int bh)
{
   return x >= bx && x <= bx + bw && y >= by && y <= by + bh;
}

static void SelectPrevious(void)
{
   if(wpList.count <= 0) {
      return;
   }
   wpSelected = (wpSelected - 1 + wpList.count) % wpList.count;
}

static void SelectNext(void)
{
   if(wpList.count <= 0) {
      return;
   }
   wpSelected = (wpSelected + 1) % wpList.count;
}

static int MaskShift(unsigned long mask)
{
   int shift = 0;
   if(!mask) {
      return 0;
   }
   while((mask & 1) == 0) {
      mask >>= 1;
      shift++;
   }
   return shift;
}

static int MaskBits(unsigned long mask)
{
   int bits = 0;
   while(mask) {
      if(mask & 1) {
         bits++;
      }
      mask >>= 1;
   }
   return bits;
}

static unsigned long ScaleToMask(unsigned char value, unsigned long mask)
{
   int bits = MaskBits(mask);
   int shift = MaskShift(mask);
   unsigned long maxv;
   if(bits <= 0) {
      return 0;
   }
   maxv = (1UL << bits) - 1;
   return ((unsigned long)value * maxv / 255) << shift;
}

static unsigned long RGBToPixel(unsigned char r, unsigned char g, unsigned char b)
{
   if(wpVisual && wpVisual->class == TrueColor) {
      return ScaleToMask(r, wpVisual->red_mask)
           | ScaleToMask(g, wpVisual->green_mask)
           | ScaleToMask(b, wpVisual->blue_mask);
   } else {
      XColor c;
      c.red = r << 8;
      c.green = g << 8;
      c.blue = b << 8;
      c.flags = DoRed | DoGreen | DoBlue;
      XAllocColor(wpDisplay, wpColormap, &c);
      return c.pixel;
   }
}

/* agregado por josejp2424 */
static void DrawImageNodeFit(Drawable d, ImageNode *image, int bx, int by, int bw, int bh, int bg)
{
   XImage *xi;
   int x, y;
   int dw, dh, dx, dy;
   unsigned char br = (bg >> 16) & 0xff;
   unsigned char bgc = (bg >> 8) & 0xff;
   unsigned char bb = bg & 0xff;

   if(!image || image->bitmap || !image->data || image->width <= 0 || image->height <= 0) {
      return;
   }

   if(image->width * bh > image->height * bw) {
      dw = bw;
      dh = MAX(1, image->height * bw / image->width);
   } else {
      dh = bh;
      dw = MAX(1, image->width * bh / image->height);
   }

   dx = bx + (bw - dw) / 2;
   dy = by + (bh - dh) / 2;

   xi = XCreateImage(wpDisplay, wpVisual, wpDepth, ZPixmap, 0, NULL, dw, dh, 32, 0);
   if(!xi) {
      return;
   }
   xi->data = calloc(1, xi->bytes_per_line * dh);
   if(!xi->data) {
      XDestroyImage(xi);
      return;
   }

   for(y = 0; y < dh; y++) {
      int sy = y * image->height / dh;
      for(x = 0; x < dw; x++) {
         int sx = x * image->width / dw;
         int p = (sy * image->width + sx) * 4;
         unsigned int a = image->data[p + 0];
         unsigned int r = image->data[p + 1];
         unsigned int g = image->data[p + 2];
         unsigned int b = image->data[p + 3];
         unsigned char rr = (r * a + br * (255 - a)) / 255;
         unsigned char gg = (g * a + bgc * (255 - a)) / 255;
         unsigned char bb2 = (b * a + bb * (255 - a)) / 255;
         XPutPixel(xi, x, y, RGBToPixel(rr, gg, bb2));
      }
   }

   XPutImage(wpDisplay, d, wpGC, xi, 0, 0, dx, dy, dw, dh);
   XDestroyImage(xi);
}

static char IsImageFile(const char *name)
{
   const char *dot = strrchr(name, '.');
   if(!dot) {
      return 0;
   }
   return !StrCmpNoCase(dot, ".png")
       || !StrCmpNoCase(dot, ".jpg")
       || !StrCmpNoCase(dot, ".jpeg")
       || !StrCmpNoCase(dot, ".svg")
       || !StrCmpNoCase(dot, ".xpm")
       || !StrCmpNoCase(dot, ".xbm");
}

static void AddWallpaper(WallpaperList *list, const char *path)
{
   const char *base;
   if(list->count >= MAX_WALLPAPERS || !path) {
      return;
   }
   base = strrchr(path, '/');
   base = base ? base + 1 : path;
   list->items[list->count].path = CopyString(path);
   list->items[list->count].name = CopyString(base);
   list->count++;
}

static void ScanWallpaperDir(WallpaperList *list, const char *dir)
{
   DIR *dp;
   struct dirent *de;
   dp = opendir(dir);
   if(!dp) {
      return;
   }
   while((de = readdir(dp)) != NULL) {
      char path[PATH_MAX];
      struct stat st;
      if(!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) {
         continue;
      }
      snprintf(path, sizeof(path), "%s/%s", dir, de->d_name);
      if(stat(path, &st) < 0) {
         continue;
      }
      if(S_ISDIR(st.st_mode)) {
         ScanWallpaperDir(list, path);
      } else if(S_ISREG(st.st_mode) && IsImageFile(path)) {
         AddWallpaper(list, path);
      }
   }
   closedir(dp);
}

static void FreeWallpaperList(WallpaperList *list)
{
   int i;
   for(i = 0; i < list->count; i++) {
      if(list->items[i].path) {
         Release(list->items[i].path);
      }
      if(list->items[i].name) {
         Release(list->items[i].name);
      }
   }
   memset(list, 0, sizeof(*list));
}

static char *GetConfigPath(void)
{
   const char *home = getenv("HOME");
   char *path;
   if(!home || !home[0]) {
      home = "/tmp";
   }
   path = Allocate(strlen(home) + strlen("/" CONFIG_FILE) + 1);
   sprintf(path, "%s/%s", home, CONFIG_FILE);
   return path;
}

static void EnsureConfigDir(void)
{
   const char *home = getenv("HOME");
   char path[PATH_MAX];
   if(!home || !home[0]) {
      return;
   }
   snprintf(path, sizeof(path), "%s/%s", home, CONFIG_SUBDIR);
   mkdir(path, 0755);
}

static void SaveWallpaper(const char *path)
{
   char *cfg;
   FILE *fp;
   EnsureConfigDir();
   cfg = GetConfigPath();
   fp = fopen(cfg, "w");
   if(fp) {
      fprintf(fp, "%s\n", path);
      fclose(fp);
   }
   Release(cfg);
}

static char *ReadWallpaperPath(void)
{
   char *cfg = GetConfigPath();
   FILE *fp = fopen(cfg, "r");
   char buffer[PATH_MAX];
   char *result = NULL;
   Release(cfg);
   if(!fp) {
      return NULL;
   }
   if(fgets(buffer, sizeof(buffer), fp)) {
      size_t len = strlen(buffer);
      while(len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r')) {
         buffer[--len] = 0;
      }
      result = CopyString(buffer);
   }
   fclose(fp);
   return result;
}

static char *QuoteShell(const char *path)
{
   size_t len = 3;
   const char *p;
   char *out, *q;
   for(p = path; *p; p++) {
      len += (*p == '\'') ? 4 : 1;
   }
   out = Allocate(len);
   q = out;
   *q++ = '\'';
   for(p = path; *p; p++) {
      if(*p == '\'') {
         strcpy(q, "'\\''");
         q += 4;
      } else {
         *q++ = *p;
      }
   }
   *q++ = '\'';
   *q = 0;
   return out;
}

static char HasCommand(const char *cmd)
{
   char buffer[256];
   snprintf(buffer, sizeof(buffer), "command -v %s >/dev/null 2>&1", cmd);
   return system(buffer) == 0;
}

static void ApplyWallpaper(const char *path, char save)
{
   char *quoted;
   char command[PATH_MAX * 2];
   if(!path || !path[0]) {
      return;
   }

   quoted = QuoteShell(path);
   if(HasCommand("hsetroot")) {
      snprintf(command, sizeof(command), "hsetroot -fill %s", quoted);
      system(command);
   } else if(HasCommand("feh")) {
      snprintf(command, sizeof(command), "feh --bg-fill %s", quoted);
      system(command);
   } else if(HasCommand("xwallpaper")) {
      snprintf(command, sizeof(command), "xwallpaper --zoom %s", quoted);
      system(command);
   }
   Release(quoted);

   UpdatePuppyPin(path);
   if(save) {
      SaveWallpaper(path);
      RestartWbar();
   }
}

/* agregado por josejp2424 */
static void RestartWbar(void)
{
   /*
    * Reinicia wbar si está activa para evitar que quede el marco negro
    * después de cambiar el wallpaper. No inicia wbar si no estaba corriendo.
    */
   if(system("pgrep -x wbar >/dev/null 2>&1") == 0) {
      system("pkill -x wbar >/dev/null 2>&1");
      system("sh -c 'sleep 0.35; wbar >/dev/null 2>&1 &' ");
   }
}

/* agregado por josejp2424 */
static void UpdatePuppyPin(const char *path)
{
   const char *home = getenv("HOME");
   char pin[PATH_MAX];
   char tmp[PATH_MAX];
   FILE *in, *out;
   int changed = 0;

   if(!home || !home[0] || !path || !path[0]) {
      return;
   }

   snprintf(pin, sizeof(pin), "%s/Choices/ROX-Filer/PuppyPin", home);
   if(access(pin, R_OK | W_OK) != 0) {
      return;
   }
   snprintf(tmp, sizeof(tmp), "%s.tmp", pin);

   in = fopen(pin, "r");
   out = fopen(tmp, "w");
   if(!in || !out) {
      if(in) fclose(in);
      if(out) fclose(out);
      return;
   }

   while(!feof(in)) {
      char line[8192];
      char *start, *end;
      if(!fgets(line, sizeof(line), in)) {
         break;
      }
      start = strstr(line, "<backdrop");
      end = strstr(line, "</backdrop>");
      if(start && end && !changed) {
         char *gt = strchr(start, '>');
         if(gt && gt < end) {
            *++gt = 0;
            fprintf(out, "%s%s%s", line, path, end);
            changed = 1;
            continue;
         }
      }
      fputs(line, out);
   }
   fclose(in);
   fclose(out);

   if(changed) {
      char *quoted = QuoteShell(pin);
      char command[PATH_MAX * 2];
      rename(tmp, pin);
      if(HasCommand("rox")) {
         snprintf(command, sizeof(command), "rox -p %s >/dev/null 2>&1 &", quoted);
         system(command);
      }
      Release(quoted);
   } else {
      unlink(tmp);
   }
}
