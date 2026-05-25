/**
 * @file taskpreview.c
 * @author Joe Wingbermuehle
 * @author josejp2424
 * @date 2026
 *
 * @brief Internal TaskList thumbnail previews for JWM.
 *
 * Agregado por josejp2424: preview/thumbnail interno para TaskList.
 * This keeps thumbnail pixmaps inside JWM instead of using an external helper.
 */

#include "jwm.h"
#include "taskpreview.h"
#include "main.h"
#include "error.h"
#include "misc.h"

#define PREVIEW_MAX_WIDTH    280
#define PREVIEW_MAX_HEIGHT   170
#define PREVIEW_BORDER       2
#define PREVIEW_PAD          4
#define PREVIEW_MARGIN       12

#define ESSORA_GREEN_PIXEL   0x7D9B37
#define ESSORA_BG_PIXEL      0x20242a

typedef struct PreviewCacheNode {
   Window window;
   Pixmap pixmap;
   int width;
   int height;
   struct PreviewCacheNode *next;
} PreviewCacheNode;

static PreviewCacheNode *previewCache;
static Window previewPopup;
static Pixmap previewBuffer;
static int previewWidth;
static int previewHeight;
static Window activePreviewWindow;

static PreviewCacheNode *FindCache(Window window);
static PreviewCacheNode *CreateCache(Window window);
static void FreeCacheNode(PreviewCacheNode *node);
static Pixmap CaptureClientPixmap(ClientNode *np, int *thumbWidth, int *thumbHeight);
static void DrawScaledImage(Pixmap dest, XImage *image,
                            int srcWidth, int srcHeight,
                            int dstX, int dstY,
                            int dstWidth, int dstHeight);
static void RenderPreview(Pixmap pixmap, int width, int height);

/** Initialize the preview subsystem. */
void InitializeTaskPreview(void)
{
   previewCache = NULL;
   previewPopup = None;
   previewBuffer = None;
   previewWidth = 0;
   previewHeight = 0;
   activePreviewWindow = None;
}

/** Shutdown the preview subsystem. */
void ShutdownTaskPreview(void)
{
   HideTaskPreview();
}

/** Destroy all cached thumbnails. */
void DestroyTaskPreview(void)
{
   PreviewCacheNode *node = previewCache;
   while(node) {
      PreviewCacheNode *next = node->next;
      FreeCacheNode(node);
      node = next;
   }
   previewCache = NULL;
}

/** Update a cached thumbnail for a visible client. */
void UpdateTaskPreviewCache(ClientNode *np)
{
   PreviewCacheNode *node;
   Pixmap pixmap;
   int width, height;

   if(!np || np->window == None) {
      return;
   }

   /* Agregado por josejp2424: do not replace a good cache with a minimized
    * or hidden capture. This keeps the last real image for minimized windows.
    */
   if(np->state.status & (STAT_MINIMIZED | STAT_HIDDEN | STAT_SHADED)) {
      return;
   }

   pixmap = CaptureClientPixmap(np, &width, &height);
   if(pixmap == None) {
      return;
   }

   node = CreateCache(np->window);
   if(!node) {
      JXFreePixmap(display, pixmap);
      return;
   }

   if(node->pixmap != None) {
      JXFreePixmap(display, node->pixmap);
   }
   node->pixmap = pixmap;
   node->width = width;
   node->height = height;
}

/** Show the cached preview for a client. */
void ShowTaskPreview(ClientNode *np, int x, int y)
{
   PreviewCacheNode *node;
   int totalWidth, totalHeight;
   int px, py;
   XSetWindowAttributes attr;

   if(!np || np->window == None) {
      HideTaskPreview();
      return;
   }

   /* Agregado por josejp2424: refresh the cache before showing when possible. */
   UpdateTaskPreviewCache(np);

   node = FindCache(np->window);
   if(!node || node->pixmap == None || node->width <= 0 || node->height <= 0) {
      HideTaskPreview();
      return;
   }

   totalWidth = node->width + 2 * (PREVIEW_BORDER + PREVIEW_PAD);
   totalHeight = node->height + 2 * (PREVIEW_BORDER + PREVIEW_PAD);

   if(previewPopup == None) {
      attr.override_redirect = True;
      attr.background_pixel = ESSORA_BG_PIXEL;
      attr.border_pixel = ESSORA_GREEN_PIXEL;
      attr.save_under = True;
      previewPopup = JXCreateWindow(display, rootWindow, 0, 0,
                                    totalWidth, totalHeight,
                                    0, rootDepth, InputOutput, rootVisual,
                                    CWOverrideRedirect | CWBackPixel |
                                    CWBorderPixel | CWSaveUnder, &attr);
      JXStoreName(display, previewPopup, "Essora JWM Preview");
   }

   if(previewBuffer == None || previewWidth != totalWidth || previewHeight != totalHeight) {
      if(previewBuffer != None) {
         JXFreePixmap(display, previewBuffer);
      }
      previewBuffer = JXCreatePixmap(display, rootWindow,
                                     totalWidth, totalHeight, rootDepth);
      previewWidth = totalWidth;
      previewHeight = totalHeight;
   }

   RenderPreview(node->pixmap, node->width, node->height);

   px = x + PREVIEW_MARGIN;
   py = y - totalHeight - PREVIEW_MARGIN;
   if(px + totalWidth > rootWidth) {
      px = rootWidth - totalWidth - PREVIEW_MARGIN;
   }
   if(px < 0) {
      px = PREVIEW_MARGIN;
   }
   if(py < 0) {
      py = y + PREVIEW_MARGIN;
   }
   if(py + totalHeight > rootHeight) {
      py = rootHeight - totalHeight - PREVIEW_MARGIN;
   }
   if(py < 0) {
      py = PREVIEW_MARGIN;
   }

   JXMoveResizeWindow(display, previewPopup, px, py, totalWidth, totalHeight);
   JXMapWindow(display, previewPopup);
   JXRaiseWindow(display, previewPopup);
   JXCopyArea(display, previewBuffer, previewPopup, rootGC,
              0, 0, totalWidth, totalHeight, 0, 0);

   activePreviewWindow = np->window;
}

/** Hide the preview. */
void HideTaskPreview(void)
{
   if(previewPopup != None) {
      JXUnmapWindow(display, previewPopup);
   }
   activePreviewWindow = None;
}

/** Find a cache node. */
PreviewCacheNode *FindCache(Window window)
{
   PreviewCacheNode *node;
   for(node = previewCache; node; node = node->next) {
      if(node->window == window) {
         return node;
      }
   }
   return NULL;
}

/** Create a cache node. */
PreviewCacheNode *CreateCache(Window window)
{
   PreviewCacheNode *node = FindCache(window);
   if(node) {
      return node;
   }
   node = Allocate(sizeof(PreviewCacheNode));
   node->window = window;
   node->pixmap = None;
   node->width = 0;
   node->height = 0;
   node->next = previewCache;
   previewCache = node;
   return node;
}

/** Free a cache node. */
void FreeCacheNode(PreviewCacheNode *node)
{
   if(!node) {
      return;
   }
   if(node->pixmap != None) {
      JXFreePixmap(display, node->pixmap);
   }
   Release(node);
}

/** Capture a scaled thumbnail pixmap from a client window. */
Pixmap CaptureClientPixmap(ClientNode *np, int *thumbWidth, int *thumbHeight)
{
   XWindowAttributes attr;
   XImage *image;
   Pixmap pixmap;
   int srcWidth, srcHeight;
   int dstWidth, dstHeight;
   double scaleX, scaleY, scale;

   if(!np || np->window == None) {
      return None;
   }

   if(!JXGetWindowAttributes(display, np->window, &attr)) {
      return None;
   }
   if(attr.map_state != IsViewable || attr.width <= 1 || attr.height <= 1) {
      return None;
   }

   srcWidth = attr.width;
   srcHeight = attr.height;
   scaleX = (double)PREVIEW_MAX_WIDTH / (double)srcWidth;
   scaleY = (double)PREVIEW_MAX_HEIGHT / (double)srcHeight;
   scale = Min(scaleX, scaleY);
   if(scale > 1.0) {
      scale = 1.0;
   }
   dstWidth = Max(1, (int)((double)srcWidth * scale));
   dstHeight = Max(1, (int)((double)srcHeight * scale));

   image = JXGetImage(display, np->window, 0, 0,
                      srcWidth, srcHeight, AllPlanes, ZPixmap);
   if(!image) {
      return None;
   }

   pixmap = JXCreatePixmap(display, rootWindow, dstWidth, dstHeight, rootDepth);
   DrawScaledImage(pixmap, image, srcWidth, srcHeight, 0, 0, dstWidth, dstHeight);
   JXDestroyImage(image);

   *thumbWidth = dstWidth;
   *thumbHeight = dstHeight;
   return pixmap;
}

/** Draw a scaled XImage into a pixmap using nearest-neighbor scaling. */
void DrawScaledImage(Pixmap dest, XImage *image,
                     int srcWidth, int srcHeight,
                     int dstX, int dstY,
                     int dstWidth, int dstHeight)
{
   XImage *scaled;
   char *data;
   int x, y;

   data = Allocate(dstWidth * dstHeight * 4);
   scaled = JXCreateImage(display, rootVisual, rootDepth, ZPixmap, 0, data,
                          dstWidth, dstHeight, 32, 0);
   if(!scaled) {
      Release(data);
      return;
   }

   for(y = 0; y < dstHeight; y++) {
      const int sy = (y * srcHeight) / dstHeight;
      for(x = 0; x < dstWidth; x++) {
         const int sx = (x * srcWidth) / dstWidth;
         const unsigned long pixel = XGetPixel(image, sx, sy);
         XPutPixel(scaled, x, y, pixel);
      }
   }

   JXPutImage(display, dest, rootGC, scaled, 0, 0, dstX, dstY,
              dstWidth, dstHeight);
   JXDestroyImage(scaled);
}

/** Render the preview border/background and thumbnail. */
void RenderPreview(Pixmap pixmap, int width, int height)
{
   const int totalWidth = width + 2 * (PREVIEW_BORDER + PREVIEW_PAD);
   const int totalHeight = height + 2 * (PREVIEW_BORDER + PREVIEW_PAD);
   const int imageX = PREVIEW_BORDER + PREVIEW_PAD;
   const int imageY = PREVIEW_BORDER + PREVIEW_PAD;

   JXSetForeground(display, rootGC, ESSORA_BG_PIXEL);
   JXFillRectangle(display, previewBuffer, rootGC, 0, 0, totalWidth, totalHeight);
   JXSetForeground(display, rootGC, ESSORA_GREEN_PIXEL);
   JXDrawRectangle(display, previewBuffer, rootGC, 0, 0,
                   totalWidth - 1, totalHeight - 1);
   JXDrawRectangle(display, previewBuffer, rootGC, 1, 1,
                   totalWidth - 3, totalHeight - 3);
   JXCopyArea(display, pixmap, previewBuffer, rootGC,
              0, 0, width, height, imageX, imageY);
}
