/**
 * @file essorawallpaper.c
 * @brief Native EssoraWM wallpaper selector.
 *
 * agregado por josejp2424
 *
 * This module keeps the wallpaper selector inside the EssoraWM/JWM binary.
 * It scans /usr/share/backgrounds, shows thumbnails using the existing JWM
 * image loaders, applies the selected wallpaper with hsetroot/feh/xwallpaper,
 * and stores the selection in ~/.config/essorawm/wallpaper.
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
#endif

#define WALLPAPER_DIR "/usr/share/backgrounds"
#define ESSORAWM_ICON "/usr/share/jwm/essorawm.svg" /* agregado por josejp2424 */
#define CONFIG_SUBDIR ".config/essorawm"
#define CONFIG_FILE ".config/essorawm/wallpaper"
#define WINDOW_W 820
#define WINDOW_H 560
#define HEADER_H 56
#define BUTTON_H 34
#define MARGIN 14
#define THUMB_W 150
#define THUMB_H 92
#define THUMB_GAP 12
#define COLS 5
#define ROWS 4
#define SCROLL_X (WINDOW_W - 28) /* agregado por josejp2424 */
#define SCROLL_Y (HEADER_H + MARGIN) /* agregado por josejp2424 */
#define SCROLL_W 14 /* agregado por josejp2424 */
#define SCROLL_H (WINDOW_H - HEADER_H - 76) /* agregado por josejp2424 */
#define MAX_WALLPAPERS 512

typedef struct WallpaperItem {
   char *path;
   char *name;
} WallpaperItem;

typedef struct WallpaperList {
   WallpaperItem items[MAX_WALLPAPERS];
   int count;
} WallpaperList;

static Display *wpDisplay;
static Window wpWindow;
static GC wpGC;
static int wpScreen;
static Visual *wpVisual;
static Colormap wpColormap;
static int wpDepth;
static int wpWidth;
static int wpHeight;
static WallpaperList wpList;
static int wpSelected;
static int wpOffset;
static char wpOwnDisplay;
static Atom wpDeleteAtom; /* agregado por josejp2424 */

static void CloseWallpaperDisplay(void);
static void DrawWallpaperWindow(void);
static void SetWallpaperWindowIcon(void); /* agregado por josejp2424 */
static void ApplyWallpaper(const char *path, char save);
static void ScanWallpaperDir(WallpaperList *list, const char *dir);
static void AddWallpaper(WallpaperList *list, const char *path);
static char IsImageFile(const char *name);
static void FreeWallpaperList(WallpaperList *list);
static void DrawText(int x, int y, const char *text, unsigned long color);
static void DrawImageNode(Drawable d, ImageNode *image, int dx, int dy, int bg);
static void DrawImageNodeFit(Drawable d, ImageNode *image, int bx, int by, int bw, int bh, int bg);
static unsigned long RGBToPixel(unsigned char r, unsigned char g, unsigned char b);
static void DrawButton(int x, int y, int w, int h, const char *label, char active);
static int HitButton(int x, int y, int bx, int by, int bw, int bh);
static char *GetConfigPath(void);
static char *QuoteShell(const char *path);
static void EnsureConfigDir(void);
static void SaveWallpaper(const char *path);
static char *ReadWallpaperPath(void);
static void UpdatePuppyPin(const char *path); /* agregado por josejp2424 */
static int HitScrollbar(int x, int y); /* agregado por josejp2424 */
static void ScrollWallpaperList(int rows); /* agregado por josejp2424 */
static char HasCommand(const char *cmd);
static char InitWallpaperDisplay(void);

/* agregado por josejp2424 */
void RunEssoraWallpaperSelector(void)
{
   XEvent event;
   int running = 1;
   const int upX = WINDOW_W - 405; /* agregado por josejp2424 */
   const int downX = WINDOW_W - 305; /* agregado por josejp2424 */
   const int applyX = WINDOW_W - 205;
   const int cancelX = WINDOW_W - 105;
   const int buttonY = WINDOW_H - 48;

   if(!InitWallpaperDisplay()) {
      return;
   }

   memset(&wpList, 0, sizeof(wpList));
   wpSelected = -1;
   wpOffset = 0;
   ScanWallpaperDir(&wpList, WALLPAPER_DIR);

   if(wpList.count > 0) {
      wpSelected = 0;
   }

   wpWindow = XCreateSimpleWindow(wpDisplay, RootWindow(wpDisplay, wpScreen),
                                  80, 80, WINDOW_W, WINDOW_H, 1,
                                  RGBToPixel(0x77, 0x96, 0x0A),
                                  RGBToPixel(0x20, 0x20, 0x20));
   /* agregado por josejp2424: cerrar limpio con el botón del marco, sin XIO fatal. */
   wpDeleteAtom = XInternAtom(wpDisplay, "WM_DELETE_WINDOW", False);
   XSetWMProtocols(wpDisplay, wpWindow, &wpDeleteAtom, 1);
   XStoreName(wpDisplay, wpWindow, _("EssoraWM Wallpaper"));
   XSetIconName(wpDisplay, wpWindow, _("EssoraWM Wallpaper"));
   SetWallpaperWindowIcon(); /* agregado por josejp2424 */
   XSelectInput(wpDisplay, wpWindow,
                ExposureMask | ButtonPressMask | KeyPressMask |
                StructureNotifyMask);
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
      case ConfigureNotify:
         wpWidth = event.xconfigure.width;
         wpHeight = event.xconfigure.height;
         break;
      case ButtonPress:
         /* agregado por josejp2424:
          * Ignorar completamente botón central, clic derecho y rueda del mouse.
          * La rueda no debe mover ni cambiar imágenes; la navegación se hace
          * únicamente con los botones Up/Down o con el teclado. */
         if(event.xbutton.button != Button1) {
            break;
         } else {
            int x = event.xbutton.x;
            int y = event.xbutton.y;
            int i;

            if(HitButton(x, y, upX, buttonY, 86, BUTTON_H)) {
               ScrollWallpaperList(-1);
               DrawWallpaperWindow();
               break;
            }
            if(HitButton(x, y, downX, buttonY, 86, BUTTON_H)) {
               ScrollWallpaperList(1);
               DrawWallpaperWindow();
               break;
            }
            if(HitScrollbar(x, y)) {
               int visible = COLS * ROWS;
               int maxOffset = wpList.count > visible ? wpList.count - visible : 0;
               int relative = y - SCROLL_Y;
               if(maxOffset > 0) {
                  wpOffset = (relative * maxOffset / Max(1, SCROLL_H));
                  wpOffset = (wpOffset / COLS) * COLS;
                  if(wpOffset < 0) {
                     wpOffset = 0;
                  }
                  if(wpOffset > maxOffset) {
                     wpOffset = (maxOffset / COLS) * COLS;
                  }
                  if(wpSelected < wpOffset || wpSelected >= wpOffset + visible) {
                     wpSelected = wpOffset;
                  }
               }
               DrawWallpaperWindow();
               break;
            }
            if(HitButton(x, y, applyX, buttonY, 86, BUTTON_H)) {
               if(wpSelected >= 0 && wpSelected < wpList.count) {
                  ApplyWallpaper(wpList.items[wpSelected].path, 1);
               }
               running = 0;
               break;
            }
            if(HitButton(x, y, cancelX, buttonY, 86, BUTTON_H)) {
               running = 0;
               break;
            }

            for(i = 0; i < COLS * ROWS; i++) {
               int idx = wpOffset + i;
               int col = i % COLS;
               int row = i / COLS;
               int bx = MARGIN + col * (THUMB_W + THUMB_GAP);
               int by = HEADER_H + MARGIN + row * (THUMB_H + 34);
               if(idx >= wpList.count) {
                  continue;
               }
               if(x >= bx && x <= bx + THUMB_W && y >= by && y <= by + THUMB_H + 24) {
                  wpSelected = idx;
                  DrawWallpaperWindow();
                  break;
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
         } else if(sym == XK_Down || sym == XK_Right) {
            if(wpSelected + 1 < wpList.count) {
               wpSelected++;
               if(wpSelected >= wpOffset + COLS * ROWS) {
                  wpOffset += COLS;
               }
               DrawWallpaperWindow();
            }
         } else if(sym == XK_Up || sym == XK_Left) {
            if(wpSelected > 0) {
               wpSelected--;
               if(wpSelected < wpOffset) {
                  wpOffset = Max(0, wpOffset - COLS);
               }
               DrawWallpaperWindow();
            }
         }
         break;
      }
      case ClientMessage:
         /* agregado por josejp2424: manejar WM_DELETE_WINDOW sin matar el cliente. */
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
      /* agregado por josejp2424: cerrar la ventana solo una vez y limpiar la cola. */
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
      wpWidth = rootWidth;
      wpHeight = rootHeight;
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
   wpWidth = DisplayWidth(wpDisplay, wpScreen);
   wpHeight = DisplayHeight(wpDisplay, wpScreen);
   wpOwnDisplay = 1;

   /* agregado por josejp2424: permitir que los cargadores de imagen de JWM funcionen en modo -wallpaper. */
   display = wpDisplay;
   rootScreen = wpScreen;
   rootWindow = RootWindow(wpDisplay, wpScreen);
   rootVisual = wpVisual;
   rootColormap = wpColormap;
   rootDepth = wpDepth;
   rootWidth = wpWidth;
   rootHeight = wpHeight;
   rootGC = DefaultGC(wpDisplay, wpScreen);
#ifdef USE_XRENDER
   haveRender = 0;
#endif
   return 1;
}

static void CloseWallpaperDisplay(void)
{
   if(wpOwnDisplay && wpDisplay) {
      XCloseDisplay(wpDisplay);
      display = NULL;
   }
   wpDisplay = NULL;
}


/* agregado por josejp2424
 * Define el icono de la ventana para que el panel muestre EssoraWM
 * y no una miniatura del wallpaper seleccionado.
 */
static void SetWallpaperWindowIcon(void)
{
   ImageNode *icon;
   Atom netWmIcon;
   unsigned long *data;
   int x, y, i;

   if(!wpWindow) {
      return;
   }

   icon = LoadImage(ESSORAWM_ICON, 48, 48, 1);
   if(!icon || icon->bitmap || !icon->data || icon->width <= 0 || icon->height <= 0) {
      if(icon) {
         DestroyImage(icon);
      }
      return;
   }

   data = Allocate(sizeof(unsigned long) * (2 + icon->width * icon->height));
   data[0] = (unsigned long)icon->width;
   data[1] = (unsigned long)icon->height;

   i = 2;
   for(y = 0; y < icon->height; y++) {
      for(x = 0; x < icon->width; x++) {
         int p = (y * icon->width + x) * 4;
         unsigned long a = icon->data[p + 0];
         unsigned long r = icon->data[p + 1];
         unsigned long g = icon->data[p + 2];
         unsigned long b = icon->data[p + 3];
         data[i++] = (a << 24) | (r << 16) | (g << 8) | b;
      }
   }

   netWmIcon = XInternAtom(wpDisplay, "_NET_WM_ICON", False);
   XChangeProperty(wpDisplay, wpWindow, netWmIcon, XA_CARDINAL, 32,
                   PropModeReplace, (unsigned char*)data,
                   2 + icon->width * icon->height);
   XFlush(wpDisplay);

   Release(data);
   DestroyImage(icon);
}

static void DrawWallpaperWindow(void)
{
   int i;
   unsigned long bg = RGBToPixel(0x20, 0x20, 0x20);
   unsigned long panel = RGBToPixel(0x2b, 0x2b, 0x2b);
   unsigned long green = RGBToPixel(0x77, 0x96, 0x0A);
   unsigned long white = RGBToPixel(0xff, 0xff, 0xff);
   unsigned long muted = RGBToPixel(0xb0, 0xb0, 0xb0);

   XSetForeground(wpDisplay, wpGC, bg);
   XFillRectangle(wpDisplay, wpWindow, wpGC, 0, 0, WINDOW_W, WINDOW_H);

   XSetForeground(wpDisplay, wpGC, panel);
   XFillRectangle(wpDisplay, wpWindow, wpGC, 0, 0, WINDOW_W, HEADER_H);
   XSetForeground(wpDisplay, wpGC, green);
   XFillRectangle(wpDisplay, wpWindow, wpGC, 0, HEADER_H - 3, WINDOW_W, 3);

   DrawText(MARGIN, 24, _("EssoraWM Wallpaper"), white);
   DrawText(MARGIN, 43, _("Wallpapers from /usr/share/backgrounds"), muted);

   if(wpList.count == 0) {
      DrawText(MARGIN, HEADER_H + 40, _("No images found in /usr/share/backgrounds"), white);
   }

   for(i = 0; i < COLS * ROWS; i++) {
      int idx = wpOffset + i;
      int col = i % COLS;
      int row = i / COLS;
      int bx = MARGIN + col * (THUMB_W + THUMB_GAP);
      int by = HEADER_H + MARGIN + row * (THUMB_H + 34);
      ImageNode *img;
      if(idx >= wpList.count) {
         continue;
      }

      XSetForeground(wpDisplay, wpGC, idx == wpSelected ? green : RGBToPixel(0x3a, 0x3a, 0x3a));
      XFillRectangle(wpDisplay, wpWindow, wpGC, bx - 3, by - 3, THUMB_W + 6, THUMB_H + 28);
      XSetForeground(wpDisplay, wpGC, RGBToPixel(0x10, 0x10, 0x10));
      XFillRectangle(wpDisplay, wpWindow, wpGC, bx, by, THUMB_W, THUMB_H);

      img = LoadImage(wpList.items[idx].path, THUMB_W, THUMB_H, 1);
      if(img) {
         /* agregado por josejp2424: dibujar siempre dentro de la miniatura.
          * Algunas imágenes/SVG no respetan el tamaño pedido por LoadImage;
          * por eso se escalan y se recortan de forma segura aquí. */
         DrawImageNodeFit(wpWindow, img, bx, by, THUMB_W, THUMB_H, 0x101010);
         DestroyImage(img);
      } else {
         DrawText(bx + 12, by + 48, _("No preview"), muted);
      }

      DrawText(bx + 4, by + THUMB_H + 17, wpList.items[idx].name, white);
   }

   /* agregado por josejp2424: barra vertical visual para indicar posición de la lista. */
   XSetForeground(wpDisplay, wpGC, RGBToPixel(0x10, 0x10, 0x10));
   XFillRectangle(wpDisplay, wpWindow, wpGC, SCROLL_X, SCROLL_Y, SCROLL_W, SCROLL_H);
   XSetForeground(wpDisplay, wpGC, green);
   if(wpList.count > COLS * ROWS) {
      int visible = COLS * ROWS;
      int maxOffset = wpList.count - visible;
      int knobH = Max(24, SCROLL_H * visible / wpList.count);
      int knobY = SCROLL_Y + (SCROLL_H - knobH) * wpOffset / Max(1, maxOffset);
      XFillRectangle(wpDisplay, wpWindow, wpGC, SCROLL_X + 2, knobY, SCROLL_W - 4, knobH);
   } else {
      XFillRectangle(wpDisplay, wpWindow, wpGC, SCROLL_X + 2, SCROLL_Y + 2, SCROLL_W - 4, SCROLL_H - 4);
   }

   DrawButton(WINDOW_W - 405, WINDOW_H - 48, 86, BUTTON_H, _("Up"), wpOffset > 0);
   DrawButton(WINDOW_W - 305, WINDOW_H - 48, 86, BUTTON_H, _("Down"), wpOffset + COLS * ROWS < wpList.count);
   DrawButton(WINDOW_W - 205, WINDOW_H - 48, 86, BUTTON_H, _("Apply"), wpSelected >= 0);
   DrawButton(WINDOW_W - 105, WINDOW_H - 48, 86, BUTTON_H, _("Cancel"), 1);

   if(wpSelected >= 0 && wpSelected < wpList.count) {
      DrawText(MARGIN, WINDOW_H - 27, wpList.items[wpSelected].path, muted);
   }
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
   DrawText(x + 18, y + 22, label, text);
}

static int HitButton(int x, int y, int bx, int by, int bw, int bh)
{
   return x >= bx && x <= bx + bw && y >= by && y <= by + bh;
}

/* agregado por josejp2424 */
static int HitScrollbar(int x, int y)
{
   return x >= SCROLL_X && x <= SCROLL_X + SCROLL_W
       && y >= SCROLL_Y && y <= SCROLL_Y + SCROLL_H;
}

/* agregado por josejp2424 */
static void ScrollWallpaperList(int rows)
{
   int visible = COLS * ROWS;
   int maxOffset = wpList.count > visible ? wpList.count - visible : 0;
   if(rows < 0) {
      wpOffset -= COLS;
   } else if(rows > 0) {
      wpOffset += COLS;
   }
   if(wpOffset < 0) {
      wpOffset = 0;
   }
   if(wpOffset > maxOffset) {
      wpOffset = (maxOffset / COLS) * COLS;
   }
   if(wpSelected < wpOffset) {
      wpSelected = wpOffset;
   }
   if(wpSelected >= wpOffset + visible) {
      wpSelected = wpOffset + visible - 1;
   }
   if(wpSelected >= wpList.count) {
      wpSelected = wpList.count - 1;
   }
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

static void DrawImageNode(Drawable d, ImageNode *image, int dx, int dy, int bg)
{
   XImage *xi;
   int x, y;
   unsigned char br = (bg >> 16) & 0xff;
   unsigned char bgc = (bg >> 8) & 0xff;
   unsigned char bb = bg & 0xff;

   if(!image || image->bitmap || !image->data) {
      return;
   }

   xi = XCreateImage(wpDisplay, wpVisual, wpDepth, ZPixmap, 0, NULL,
                     image->width, image->height, 32, 0);
   if(!xi) {
      return;
   }
   xi->data = calloc(1, xi->bytes_per_line * image->height);
   if(!xi->data) {
      XDestroyImage(xi);
      return;
   }

   for(y = 0; y < image->height; y++) {
      for(x = 0; x < image->width; x++) {
         int p = (y * image->width + x) * 4;
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
   XPutImage(wpDisplay, d, wpGC, xi, 0, 0, dx, dy, image->width, image->height);
   XDestroyImage(xi);
}


/* agregado por josejp2424
 * Dibuja una imagen ajustada dentro de una caja fija, preservando proporción.
 * Esto evita que SVG/JPG/PNG grandes se dibujen encima de toda la ventana
 * cuando el cargador devuelve un tamaño mayor al solicitado.
 */
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
      dh = Max(1, image->height * bw / image->width);
   } else {
      dh = bh;
      dw = Max(1, image->width * bh / image->height);
   }

   dx = bx + (bw - dw) / 2;
   dy = by + (bh - dh) / 2;

   xi = XCreateImage(wpDisplay, wpVisual, wpDepth, ZPixmap, 0, NULL,
                     dw, dh, 32, 0);
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
   snprintf(path, sizeof(path), "%s/.config", home);
   mkdir(path, 0755);
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
   char buffer[PATH_MAX];
   FILE *fp = fopen(cfg, "r");
   Release(cfg);
   if(!fp) {
      return NULL;
   }
   if(!fgets(buffer, sizeof(buffer), fp)) {
      fclose(fp);
      return NULL;
   }
   fclose(fp);
   buffer[strcspn(buffer, "\r\n")] = 0;
   return CopyString(buffer);
}

static char *QuoteShell(const char *path)
{
   char *out;
   size_t i, j = 0, len;
   len = strlen(path);
   out = Allocate(len * 4 + 3);
   out[j++] = '\'';
   for(i = 0; i < len; i++) {
      if(path[i] == '\'') {
         out[j++] = '\'';
         out[j++] = '\\';
         out[j++] = '\'';
         out[j++] = '\'';
      } else {
         out[j++] = path[i];
      }
   }
   out[j++] = '\'';
   out[j] = 0;
   return out;
}

static char HasCommand(const char *cmd)
{
   char command[256];
   snprintf(command, sizeof(command), "command -v %s >/dev/null 2>&1", cmd);
   return system(command) == 0;
}

/* agregado por josejp2424
 * Compatibilidad con Puppy/ROX-Filer:
 * si existe $HOME/Choices/ROX-Filer/PuppyPin, se reemplaza solo la ruta
 * dentro de <backdrop style="...">...</backdrop>, preservando el estilo
 * original, por ejemplo Stretched. También se conserva el archivo
 * ~/.config/essorawm/wallpaper para EssoraWM.
 */
static void UpdatePuppyPin(const char *path)
{
   const char *home = getenv("HOME");
   char pin[PATH_MAX];
   char tmp[PATH_MAX];
   FILE *in, *out;
   char line[8192];
   int changed = 0;

   if(!home || !home[0] || !path || !path[0]) {
      return;
   }

   snprintf(pin, sizeof(pin), "%s/Choices/ROX-Filer/PuppyPin", home);
   snprintf(tmp, sizeof(tmp), "%s.tmp", pin);

   in = fopen(pin, "r");
   if(!in) {
      return;
   }
   out = fopen(tmp, "w");
   if(!out) {
      fclose(in);
      return;
   }

   while(fgets(line, sizeof(line), in)) {
      char *start = strstr(line, "<backdrop");
      char *gt = start ? strchr(start, '>') : NULL;
      char *end = gt ? strstr(gt, "</backdrop>") : NULL;
      if(start && gt && end) {
         *gt = '\0';
         fprintf(out, "%s>%s</backdrop>\n", line, path);
         changed = 1;
      } else {
         fputs(line, out);
      }
   }

   fclose(in);
   fclose(out);

   if(changed) {
      rename(tmp, pin);
      if(HasCommand("rox")) {
         char *q = QuoteShell(pin);
         char command[PATH_MAX * 2];
         snprintf(command, sizeof(command), "rox -p %s >/dev/null 2>&1 &", q);
         system(command);
         Release(q);
      }
   } else {
      unlink(tmp);
   }
}

static void ApplyWallpaper(const char *path, char save)
{
   char *q;
   char command[PATH_MAX * 2];
   if(!path || access(path, R_OK) < 0) {
      return;
   }
   q = QuoteShell(path);
   if(HasCommand("hsetroot")) {
      snprintf(command, sizeof(command), "hsetroot -fill %s >/dev/null 2>&1 &", q);
   } else if(HasCommand("feh")) {
      snprintf(command, sizeof(command), "feh --bg-fill %s >/dev/null 2>&1 &", q);
   } else if(HasCommand("xwallpaper")) {
      snprintf(command, sizeof(command), "xwallpaper --zoom %s >/dev/null 2>&1 &", q);
   } else {
      snprintf(command, sizeof(command), "xsetroot -solid '#202020' >/dev/null 2>&1 &");
   }
   system(command);
   if(save) {
      SaveWallpaper(path);
      UpdatePuppyPin(path); /* agregado por josejp2424 */
   }
   Release(q);
}
