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
 * - Actualiza el backdrop de PuppyPin y recarga el escritorio nativo.
 */

#include "jwm.h"
#include "essorawallpaper.h"
#include "image.h"
#include "icon.h"
#include "main.h"
#include "misc.h"
#include "error.h"
#include "debug.h"
#include "desktopicons.h"

#ifndef MAKE_DEPEND
#  include <dirent.h>
#  include <sys/stat.h>
#  include <sys/types.h>
#  include <errno.h>
#  include <limits.h>
#  include <signal.h>
#  include <fcntl.h>
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
static Pixmap wpBuffer = None;
static Drawable wpDrawTarget = None;
static char wpBufferDirty = 1;
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
static char *EscapeXmlText(const char *text);
static void RestartWbar(const char *setterCommand);
static int CountWbarProcesses(void);
static void GetWbarPids(char *buffer, size_t size);
static void StopWbarProcesses(void);
static int GetWbarProcesses(pid_t *pids, int maxCount);
static char IsWbarProcess(pid_t pid);
static int StartWbarProcess(void);

/* agregado por josejp2424 */
void RunEssoraWallpaperSelector(void)
{
   XEvent event;
   int running = 1;
   const int leftX = 32;
   const int rightX = 128;
   const int applyX = WINDOW_W - 210;
   const int cancelX = WINDOW_W - 110;
   const int buttonY = WINDOW_H - 48;

   EssoraTrace("wallpaper", "selector requested");
   if(!InitWallpaperDisplay()) {
      EssoraTrace("wallpaper", "cannot initialize X display");
      return;
   }

   memset(&wpList, 0, sizeof(wpList));
   ScanWallpaperDir(&wpList, WALLPAPER_DIR);
   wpSelected = wpList.count > 0 ? 0 : -1;
   wpBufferDirty = 1;
   EssoraTrace("wallpaper", "scan complete: count=%d selected=%d",
               wpList.count, wpSelected);

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

   {
      XSizeHints hints;
      memset(&hints, 0, sizeof(hints));
      hints.flags = PMinSize | PMaxSize;
      hints.min_width = hints.max_width = WINDOW_W;
      hints.min_height = hints.max_height = WINDOW_H;
      XSetWMNormalHints(wpDisplay, wpWindow, &hints);
   }

   {
      XSetWindowAttributes attrs;
      attrs.backing_store = WhenMapped;
      XChangeWindowAttributes(wpDisplay, wpWindow, CWBackingStore, &attrs);
   }

   XSelectInput(wpDisplay, wpWindow,
                ExposureMask | ButtonPressMask | KeyPressMask | StructureNotifyMask);
   wpGC = XCreateGC(wpDisplay, wpWindow, 0, NULL);
   XMapRaised(wpDisplay, wpWindow);
   EssoraTrace("wallpaper", "window created id=0x%lx geometry=%dx%d",
               (unsigned long)wpWindow, WINDOW_W, WINDOW_H);

   while(running) {
      XNextEvent(wpDisplay, &event);
      switch(event.type) {
      case Expose:
         EssoraTrace("wallpaper",
                     "Expose window=0x%lx region=%d,%d %dx%d count=%d dirty=%d buffer=0x%lx",
                     (unsigned long)event.xexpose.window,
                     event.xexpose.x, event.xexpose.y,
                     event.xexpose.width, event.xexpose.height,
                     event.xexpose.count, wpBufferDirty,
                     (unsigned long)wpBuffer);
         if(wpBuffer == None || wpBufferDirty) {
            DrawWallpaperWindow();
         } else {
            /*
             * The complete UI is installed as the X11 background pixmap.
             * Clear only the exposed rectangle so the server restores it
             * atomically, even while XLibre is moving another window.
             */
            XClearArea(wpDisplay, wpWindow,
                       event.xexpose.x, event.xexpose.y,
                       (unsigned)event.xexpose.width,
                       (unsigned)event.xexpose.height, False);
            if(event.xexpose.count == 0) {
               XFlush(wpDisplay);
            }
         }
         break;
      case ConfigureNotify:
         EssoraTrace("wallpaper",
                     "ConfigureNotify window=0x%lx x=%d y=%d w=%d h=%d above=0x%lx",
                     (unsigned long)event.xconfigure.window,
                     event.xconfigure.x, event.xconfigure.y,
                     event.xconfigure.width, event.xconfigure.height,
                     (unsigned long)event.xconfigure.above);
         break;
      case MapNotify:
         EssoraTrace("wallpaper", "MapNotify window=0x%lx",
                     (unsigned long)event.xmap.window);
         break;
      case UnmapNotify:
         EssoraTrace("wallpaper", "UnmapNotify window=0x%lx",
                     (unsigned long)event.xunmap.window);
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
               EssoraTrace("wallpaper", "Apply clicked: %s",
                           wpList.items[wpSelected].path);
               ApplyWallpaper(wpList.items[wpSelected].path, 1);
            }
            running = 0;
         } else if(HitButton(event.xbutton.x, event.xbutton.y, cancelX, buttonY, 86, BUTTON_H)) {
            EssoraTrace("wallpaper", "Cancel clicked");
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
               EssoraTrace("wallpaper", "Apply by keyboard: %s",
                           wpList.items[wpSelected].path);
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
   if(wpBuffer != None) {
      if(wpWindow != None) {
         XSetWindowBackgroundPixmap(wpDisplay, wpWindow, None);
      }
      XFreePixmap(wpDisplay, wpBuffer);
      wpBuffer = None;
   }
   wpDrawTarget = None;
   if(wpWindow) {
      XDestroyWindow(wpDisplay, wpWindow);
      XSync(wpDisplay, False);
      wpWindow = None;
   }
   FreeWallpaperList(&wpList);
   CloseWallpaperDisplay();
   EssoraTrace("wallpaper", "selector closed");
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
   ImageNode *icon = LoadImageWithFallback(ESSORAWM_ICON, 64, 64, 1, NULL);
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
   Pixmap newBuffer = None;
   Pixmap oldBuffer;

   if(wpWindow == None || !wpDisplay || !wpGC) {
      return;
   }

   if(wpBuffer != None && !wpBufferDirty) {
      XClearWindow(wpDisplay, wpWindow);
      XFlush(wpDisplay);
      return;
   }

   newBuffer = XCreatePixmap(wpDisplay, wpWindow,
                             WINDOW_W, WINDOW_H, (unsigned)wpDepth);
   if(newBuffer == None) {
      EssoraTrace("wallpaper", "cannot create persistent background pixmap");
      wpDrawTarget = wpWindow;
   } else {
      wpDrawTarget = newBuffer;
   }

   XSetForeground(wpDisplay, wpGC, bg);
   XFillRectangle(wpDisplay, wpDrawTarget, wpGC, 0, 0, WINDOW_W, WINDOW_H);

   XSetForeground(wpDisplay, wpGC, panel);
   XFillRectangle(wpDisplay, wpDrawTarget, wpGC, 0, 0, WINDOW_W, HEADER_H);

   img = LoadImageWithFallback(ESSORAWM_ICON, 42, 42, 1, NULL);
   if(img) {
      DrawImageNodeFit(wpDrawTarget, img, MARGIN, 10, 42, 42, 0x2b2b2b);
      DestroyImage(img);
   }

   DrawText(72, 28, _("EssoraWM Wallpaper"), white);
   DrawText(72, 48, _("Select a wallpaper from /usr/share/backgrounds"), muted);

   if(wpList.count <= 0) {
      DrawText(MARGIN, HEADER_H + 80, _("No wallpapers found"), white);
      DrawButton(WINDOW_W - 110, WINDOW_H - 48, 86, BUTTON_H, _("Cancel"), 1);
   } else {
      if(wpList.count > 1) {
         int prev = (wpSelected - 1 + wpList.count) % wpList.count;
         int next = (wpSelected + 1) % wpList.count;
         ImageNode *left = LoadImageWithFallback(wpList.items[prev].path, SIDE_W, SIDE_H, 1, NULL);
         ImageNode *right = LoadImageWithFallback(wpList.items[next].path, SIDE_W, SIDE_H, 1, NULL);
         if(left) {
            XSetForeground(wpDisplay, wpGC, RGBToPixel(0x10, 0x10, 0x10));
            XFillRectangle(wpDisplay, wpDrawTarget, wpGC, MARGIN, previewY + 90, SIDE_W, SIDE_H);
            DrawImageNodeFit(wpDrawTarget, left, MARGIN, previewY + 90, SIDE_W, SIDE_H, 0x101010);
            DestroyImage(left);
         }
         if(right) {
            XSetForeground(wpDisplay, wpGC, RGBToPixel(0x10, 0x10, 0x10));
            XFillRectangle(wpDisplay, wpDrawTarget, wpGC, WINDOW_W - MARGIN - SIDE_W, previewY + 90, SIDE_W, SIDE_H);
            DrawImageNodeFit(wpDrawTarget, right, WINDOW_W - MARGIN - SIDE_W, previewY + 90, SIDE_W, SIDE_H, 0x101010);
            DestroyImage(right);
         }
      }

      XSetForeground(wpDisplay, wpGC, green);
      XFillRectangle(wpDisplay, wpDrawTarget, wpGC, centerX - 4, previewY - 4, PREVIEW_W + 8, PREVIEW_H + 8);
      XSetForeground(wpDisplay, wpGC, RGBToPixel(0x10, 0x10, 0x10));
      XFillRectangle(wpDisplay, wpDrawTarget, wpGC, centerX, previewY, PREVIEW_W, PREVIEW_H);

      img = LoadImageWithFallback(wpList.items[wpSelected].path, PREVIEW_W, PREVIEW_H, 1, NULL);
      if(img) {
         DrawImageNodeFit(wpDrawTarget, img, centerX, previewY, PREVIEW_W, PREVIEW_H, 0x101010);
         DestroyImage(img);
      } else {
         DrawText(centerX + 20, previewY + 80, _("No preview"), muted);
      }

      snprintf(counter, sizeof(counter), "%d / %d", wpSelected + 1, wpList.count);
      DrawText(centerX, previewY + PREVIEW_H + 26, wpList.items[wpSelected].name, white);
      DrawText(WINDOW_W - MARGIN - 70, previewY + PREVIEW_H + 26, counter, muted);

      DrawButton(32, WINDOW_H - 48, 86, BUTTON_H, _("Left"), wpList.count > 1);
      DrawButton(128, WINDOW_H - 48, 86, BUTTON_H, _("Right"), wpList.count > 1);
      DrawButton(WINDOW_W - 210, WINDOW_H - 48, 86, BUTTON_H, _("Apply"), wpSelected >= 0);
      DrawButton(WINDOW_W - 110, WINDOW_H - 48, 86, BUTTON_H, _("Cancel"), 1);

      DrawText(MARGIN, WINDOW_H - 66, wpList.items[wpSelected].path, muted);
   }

   if(newBuffer != None) {
      oldBuffer = wpBuffer;
      wpBuffer = newBuffer;
      wpBufferDirty = 0;
      wpDrawTarget = wpWindow;

      /* Keep the entire rendered selector inside the X server. */
      XSetWindowBackgroundPixmap(wpDisplay, wpWindow, wpBuffer);
      XClearWindow(wpDisplay, wpWindow);
      XSync(wpDisplay, False);

      if(oldBuffer != None) {
         XFreePixmap(wpDisplay, oldBuffer);
      }
      EssoraTrace("wallpaper", "persistent background pixmap installed id=0x%lx",
                  (unsigned long)wpBuffer);
   } else {
      wpBufferDirty = 1;
      wpDrawTarget = wpWindow;
      XFlush(wpDisplay);
   }
}

static void DrawText(int x, int y, const char *text, unsigned long color)
{
   if(!text) {
      return;
   }
   XSetForeground(wpDisplay, wpGC, color);
   XDrawString(wpDisplay, wpDrawTarget, wpGC, x, y, text, strlen(text));
}

static void DrawButton(int x, int y, int w, int h, const char *label, char active)
{
   unsigned long border = RGBToPixel(0x77, 0x96, 0x0A);
   unsigned long fill = active ? RGBToPixel(0x2b, 0x2b, 0x2b) : RGBToPixel(0x1a, 0x1a, 0x1a);
   unsigned long text = active ? RGBToPixel(0xff, 0xff, 0xff) : RGBToPixel(0x77, 0x77, 0x77);
   XSetForeground(wpDisplay, wpGC, border);
   XFillRectangle(wpDisplay, wpDrawTarget, wpGC, x, y, w, h);
   XSetForeground(wpDisplay, wpGC, fill);
   XFillRectangle(wpDisplay, wpDrawTarget, wpGC, x + 1, y + 1, w - 2, h - 2);
   DrawText(x + 16, y + 22, label, text);
}

static int HitButton(int x, int y, int bx, int by, int bw, int bh)
{
   return x >= bx && x < bx + bw && y >= by && y < by + bh;
}

static void SelectPrevious(void)
{
   if(wpList.count <= 0) {
      return;
   }
   wpSelected = (wpSelected - 1 + wpList.count) % wpList.count;
   wpBufferDirty = 1;
}

static void SelectNext(void)
{
   if(wpList.count <= 0) {
      return;
   }
   wpSelected = (wpSelected + 1) % wpList.count;
   wpBufferDirty = 1;
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
   command[0] = 0;
   if(HasCommand("hsetroot")) {
      snprintf(command, sizeof(command), "hsetroot -fill %s", quoted);
   } else if(HasCommand("feh")) {
      snprintf(command, sizeof(command), "feh --bg-fill %s", quoted);
   } else if(HasCommand("xwallpaper")) {
      snprintf(command, sizeof(command), "xwallpaper --zoom %s", quoted);
   }
   if(command[0]) {
      int status;
      EssoraTrace("wallpaper", "setter command: %s", command);
      status = system(command);
      EssoraTrace("wallpaper", "setter exit status=%d", status);
   } else {
      EssoraTrace("wallpaper", "no supported wallpaper setter found");
   }
   Release(quoted);

   UpdatePuppyPin(path);
   if(save) {
      SaveWallpaper(path);
      RestartWbar(command);
   }
}

/* agregado por josejp2424 */
static char IsWbarProcess(pid_t pid)
{
   char path[64];
   char buffer[256];
   FILE *fp;
   char *closeParen;
   char state = 0;
   size_t length;

   if(pid <= 1 || pid == getpid()) {
      return 0;
   }

   snprintf(path, sizeof(path), "/proc/%ld/comm", (long)pid);
   fp = fopen(path, "r");
   if(!fp) {
      return 0;
   }
   if(!fgets(buffer, sizeof(buffer), fp)) {
      fclose(fp);
      return 0;
   }
   fclose(fp);
   length = strlen(buffer);
   while(length > 0 && isspace((unsigned char)buffer[length - 1])) {
      buffer[--length] = 0;
   }
   if(strcmp(buffer, "wbar") != 0) {
      return 0;
   }

   snprintf(path, sizeof(path), "/proc/%ld/stat", (long)pid);
   fp = fopen(path, "r");
   if(!fp) {
      return 0;
   }
   if(!fgets(buffer, sizeof(buffer), fp)) {
      fclose(fp);
      return 0;
   }
   fclose(fp);
   closeParen = strrchr(buffer, ')');
   if(closeParen && closeParen[1] == ' ' && closeParen[2]) {
      state = closeParen[2];
   }
   return state != 'Z';
}

static int GetWbarProcesses(pid_t *pids, int maxCount)
{
   DIR *dir;
   struct dirent *entry;
   int count = 0;

   dir = opendir("/proc");
   if(!dir) {
      return 0;
   }
   while((entry = readdir(dir)) != NULL) {
      const char *p = entry->d_name;
      pid_t pid;
      if(!*p) {
         continue;
      }
      while(*p && isdigit((unsigned char)*p)) {
         p++;
      }
      if(*p) {
         continue;
      }
      pid = (pid_t)strtol(entry->d_name, NULL, 10);
      if(IsWbarProcess(pid)) {
         if(pids && count < maxCount) {
            pids[count] = pid;
         }
         count++;
      }
   }
   closedir(dir);
   return count;
}

static int CountWbarProcesses(void)
{
   return GetWbarProcesses(NULL, 0);
}

static void GetWbarPids(char *buffer, size_t size)
{
   pid_t pids[64];
   int count;
   int i;
   size_t used = 0;

   if(!buffer || size == 0) {
      return;
   }
   buffer[0] = 0;
   count = GetWbarProcesses(pids, (int)(sizeof(pids) / sizeof(pids[0])));
   for(i = 0; i < count && i < (int)(sizeof(pids) / sizeof(pids[0])); i++) {
      int written = snprintf(buffer + used, size - used, "%s%ld",
                             used ? "," : "", (long)pids[i]);
      if(written < 0 || (size_t)written >= size - used) {
         break;
      }
      used += (size_t)written;
   }
}

static void StopWbarProcesses(void)
{
   pid_t pids[64];
   int round;
   int count;
   int i;
   int attempt;

   for(round = 0; round < 4; round++) {
      count = GetWbarProcesses(pids, (int)(sizeof(pids) / sizeof(pids[0])));
      if(count <= 0) {
         break;
      }

      EssoraTrace("wbar", "stop round=%d live=%d", round + 1, count);
      for(i = 0; i < count && i < (int)(sizeof(pids) / sizeof(pids[0])); i++) {
         if(kill(pids[i], SIGTERM) != 0 && errno != ESRCH) {
            EssoraTrace("wbar", "SIGTERM pid=%ld failed errno=%d",
                        (long)pids[i], errno);
         }
      }
      for(attempt = 0; attempt < 15 && CountWbarProcesses() > 0; attempt++) {
         usleep(100000);
      }

      count = GetWbarProcesses(pids, (int)(sizeof(pids) / sizeof(pids[0])));
      if(count <= 0) {
         break;
      }
      for(i = 0; i < count && i < (int)(sizeof(pids) / sizeof(pids[0])); i++) {
         if(kill(pids[i], SIGKILL) != 0 && errno != ESRCH) {
            EssoraTrace("wbar", "SIGKILL pid=%ld failed errno=%d",
                        (long)pids[i], errno);
         }
      }
      for(attempt = 0; attempt < 15 && CountWbarProcesses() > 0; attempt++) {
         usleep(100000);
      }
   }

   EssoraTrace("wbar", "stop complete live=%d", CountWbarProcesses());
}

static int StartWbarProcess(void)
{
   pid_t pid;
   int nullfd;

   pid = fork();
   if(pid < 0) {
      EssoraTrace("wbar", "fork failed errno=%d", errno);
      return -1;
   }
   if(pid == 0) {
      setsid();
      nullfd = open("/dev/null", O_RDWR);
      if(nullfd >= 0) {
         dup2(nullfd, STDIN_FILENO);
         dup2(nullfd, STDOUT_FILENO);
         dup2(nullfd, STDERR_FILENO);
         if(nullfd > STDERR_FILENO) {
            close(nullfd);
         }
      }
      execlp("wbar", "wbar", (char*)NULL);
      _exit(127);
   }
   EssoraTrace("wbar", "started child pid=%ld", (long)pid);
   return 0;
}

static void RestartWbar(const char *setterCommand)
{
   char beforePids[256];
   char afterPids[256];
   int before;
   int status = 0;
   int attempt;

   before = CountWbarProcesses();
   GetWbarPids(beforePids, sizeof(beforePids));
   EssoraTrace("wbar",
               "restart requested; live before=%d pids=%s",
               before, beforePids[0] ? beforePids : "none");
   if(before <= 0) {
      EssoraTrace("wbar", "not running; no restart needed");
      return;
   }

   StopWbarProcesses();

   /* A valid restart requires the old PID to disappear completely. */
   if(CountWbarProcesses() > 0) {
      GetWbarPids(afterPids, sizeof(afterPids));
      EssoraTrace("wbar", "unable to stop old process pids=%s",
                  afterPids[0] ? afterPids : "unknown");
      return;
   }

   if(setterCommand && setterCommand[0]) {
      EssoraTrace("wbar", "reapplying wallpaper with wbar fully stopped");
      status = system(setterCommand);
      EssoraTrace("wbar", "wallpaper reapply status=%d", status);
      if(wpDisplay) {
         XSync(wpDisplay, False);
      }
   }
   usleep(250000);

   /* Remove an unexpected early instance, then restore the root once more. */
   if(CountWbarProcesses() > 0) {
      EssoraTrace("wbar", "unexpected early instance detected");
      StopWbarProcesses();
      if(setterCommand && setterCommand[0]) {
         status = system(setterCommand);
         EssoraTrace("wbar", "second wallpaper reapply status=%d", status);
      }
   }

   status = StartWbarProcess();
   for(attempt = 0; attempt < 30 && CountWbarProcesses() == 0; attempt++) {
      usleep(100000);
   }

   GetWbarPids(afterPids, sizeof(afterPids));
   EssoraTrace("wbar",
               "restart complete status=%d live after=%d pids=%s old=%s",
               status, CountWbarProcesses(),
               afterPids[0] ? afterPids : "none",
               beforePids[0] ? beforePids : "none");
}

/* agregado por josejp2424 */
static void UpdatePuppyPin(const char *path)
{
   const char *home = getenv("HOME");
   char pin[PATH_MAX];
   char tmp[PATH_MAX];
   char *escaped;
   FILE *in, *out;
   int changed = 0;

   if(!home || !home[0] || !path || !path[0]) {
      return;
   }

   snprintf(pin, sizeof(pin), "%s/Choices/ROX-Filer/PuppyPin", home);
   if(access(pin, R_OK | W_OK) != 0) {
      return;
   }
   if(strlen(pin) + 4 >= sizeof(tmp)) {
      return;
   }
   snprintf(tmp, sizeof(tmp), "%s.tmp", pin);
   escaped = EscapeXmlText(path);
   if(!escaped) {
      return;
   }

   in = fopen(pin, "r");
   out = fopen(tmp, "w");
   if(!in || !out) {
      if(in) fclose(in);
      if(out) fclose(out);
      Release(escaped);
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
            fprintf(out, "%s%s%s", line, escaped, end);
            changed = 1;
            continue;
         }
      }
      if(!changed && strstr(line, "</pinboard>")) {
         fprintf(out, "  <backdrop style=\"Stretched\">%s</backdrop>\n", escaped);
         changed = 1;
      }
      fputs(line, out);
   }
   fclose(in);
   if(fclose(out) != 0) {
      changed = 0;
   }
   Release(escaped);

   if(changed && rename(tmp, pin) == 0) {
      EssoraTrace("wallpaper", "PuppyPin backdrop updated: %s", pin);
      if(wpDisplay) {
         EssoraTrace("wallpaper", "sending native desktop reload");
         SendDesktopIconsReload(wpDisplay, RootWindow(wpDisplay, wpScreen));
      }
   } else {
      EssoraTrace("wallpaper", "PuppyPin was not changed: %s", pin);
      unlink(tmp);
   }
}

static char *EscapeXmlText(const char *text)
{
   const char *p;
   size_t length = 1;
   char *result;
   char *out;

   if(!text) {
      return NULL;
   }
   for(p = text; *p; p++) {
      switch(*p) {
      case '&': length += 5; break;
      case '<': case '>': length += 4; break;
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
      default: *out++ = *p; break;
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
