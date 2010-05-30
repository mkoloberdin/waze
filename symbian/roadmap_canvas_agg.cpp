/* roadmap_canvas_agg.cpp - manage the canvas that is used to draw the map with agg
 *
 * LICENSE:
 *
 *   Copyright 2008 Giant Steps
 *   Copyright 2008 Ehud Shabtai
 *
 *   This file is part of RoadMap.
 *
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   RoadMap is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with RoadMap; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * SYNOPSYS:
 *
 *   See roadmap_canvas.h.
 */

#include "agg_rendering_buffer.h"
#include "agg_curves.h"
#include "agg_conv_stroke.h"
#include "agg_conv_contour.h"
#include "agg_conv_transform.h"
#include "agg_rasterizer_scanline_aa.h"
#include "agg_rasterizer_outline_aa.h"
#include "agg_rasterizer_outline.h"
#include "agg_renderer_primitives.h"
#include "agg_ellipse.h"
#include "agg_renderer_scanline.h"
#include "agg_scanline_p.h"
#include "agg_renderer_outline_aa.h"
#include "agg_pixfmt_rgb_packed.h"
#include "agg_path_storage.h"
#include "util/agg_color_conv_rgb8.h"
#include "colors.h"

#include <png.h>

#include "GSConvert.h"
#include <fbs.h>
#include <e32svr.h>
#include <eikenv.h> //  for CEikonEnv

extern "C" {
#include "../roadmap.h"
#include "../roadmap_canvas.h"
#include "../roadmap_messagebox.h"
#include "../roadmap_math.h"
#include "../roadmap_config.h"
#include "../roadmap_path.h"
#include "../roadmap_screen.h"
}

extern "C" unsigned char *read_png_file(const char* file_name, int *width, int *height,
                             int *stride);

#include "roadmap_canvas_agg.h"
#include "Roadmap_NativeCanvas.h"

#ifdef _USE_ALERT_WIN
#define ALERT_WIN(x) CEikonEnv::Static()->AlertWin(_L(x));
#define ALERT_WINBUF(x) CEikonEnv::Static()->AlertWin(x);
#else
#define ALERT_WIN(x)
#define ALERT_WINBUF(x)
#endif

#ifdef _SYMBIAN32_MASKED_PNG_
#include "Roadmap_NativePNG.h"
_LIT8(PngMimeType, "image/png");  //  optional
#endif

_LIT(BufData, "x=%d y=%d mode=%d stride=%d bufSize=%d");

static CRoadmapNativeCanvas* pDSACanvas = NULL;
static unsigned char* pScreenBuf = NULL;

int roadmap_canvas_agg_to_wchar (const char *text, wchar_t *output, int size)
{
  int length = mbstowcs(output, text, size - 1);
  output[length] = 0;

  return length;
}

agg::rgba8 roadmap_canvas_agg_parse_color (const char *color) {
   int high, i, low;

   if (*color == '#') {
      int r, g, b, a;
      int count;

      count = sscanf(color, "#%2x%2x%2x%2x", &r, &g, &b, &a);

      if (count == 4) {
         return agg::rgba8(r, g, b, a);
      } else {
         return agg::rgba8(r, g, b);
      }

   } else {
      /* Do binary search on color table */
      for (low=(-1), high=sizeof(color_table)/sizeof(color_table[0]);
      high-low > 1;) {
         i = (high+low) / 2;
         if (strcmp(color, color_table[i].name) <= 0) high = i;
         else low = i;
      }

      if (!strcmp(color, color_table[high].name)) {
         return agg::rgba8(color_table[high].r, color_table[high].g,
            color_table[high].b);
      } else {
         return agg::rgba8(0, 0, 0);
      }
   }
}

void roadmap_canvas_refresh (void)
{
   if ( pDSACanvas == NULL )
   {//TODO maybe create it here?
     roadmap_log (ROADMAP_ERROR, "No DSA available for refresh");
     return;
   }
   dbg_time_start(DBG_TIME_FLIP);
   pDSACanvas->UpdateScreen();
   dbg_time_end(DBG_TIME_FLIP);
   User::ResetInactivityTime();
}


void roadmap_canvas_start (void)
{
   if ( pDSACanvas == NULL )
   {
     roadmap_log( ROADMAP_ERROR, "No DSA available" );
     return;
   }
   pDSACanvas->Start();
}

void roadmap_canvas_stop (void)
{
   if ( pDSACanvas == NULL )
   {
     roadmap_log( ROADMAP_ERROR, "No DSA available" );
     return;
   }
   pDSACanvas->Stop();
}


int translate_native_bpp(TDisplayMode displayMode)
{
  int bpp = 0;
  switch (displayMode)
  {
  //* No display mode */
  case ENone:
    bpp = 0;
    break;
  //* Monochrome display mode (1 bpp) */
  case EGray2:
    bpp = 1;
    break;
  //* Four grayscales display mode (2 bpp) */
  case EGray4:
    bpp = 2;
    break;
  //* 16 grayscales display mode (4 bpp) */
  case EGray16:  //  fallthrough
  //* Low colour EGA 16 colour display mode (4 bpp) */
  case EColor16:
    bpp = 4;
    break;
  //* 256 grayscales display mode (8 bpp) */
  case EGray256: //  fallthrough
  //* 256 colour display mode (8 bpp) */
  case EColor256:
    bpp = 8;
    break;
  //* 64,000 colour display mode (16 bpp) */
  case EColor64K:
    bpp = 16;
    break;
  //* True colour display mode (24 bpp) */
  case EColor16M:
    bpp = 24;
    break;
  //* (Not an actual display mode used for moving buffers containing bitmaps) */
  //ERgb:
  //* 4096 colour display (12 bpp). */
  case EColor4K:
    bpp = 12;
    break;
  //* True colour display mode (32 bpp, but top byte is unused and unspecified) */
  case EColor16MU: //  fallthrough
  //* Display mode with alpha (24bpp colour plus 8bpp alpha) */
  case EColor16MA:
    bpp = 32;
    break;
  default:
    bpp = 0;
    break;
  }

  return bpp;
}

BOOL roadmap_canvas_agg_is_landscape()
{

   CWsScreenDevice* pScreenDevice = CEikonEnv::Static()->ScreenDevice();

   TSize screenSize = pScreenDevice->SizeInPixels();

    return ( screenSize.iWidth > screenSize.iHeight );
}

void roadmap_canvas_new (RWindow& aWindow, int initWidth, int initHeight)
{
  CWsScreenDevice* pScreenDevice = CEikonEnv::Static()->ScreenDevice();
  if ( pScreenDevice == NULL )
  {
    ALERT_WIN("Could not open screen device!");
    roadmap_log (ROADMAP_FATAL, "Could not open screen device!");
    return;
  }

  TBuf<100> logBuf;

  TSize screenSize = pScreenDevice->SizeInPixels();
  TDisplayMode displayMode = pScreenDevice->DisplayMode();
  //CFbsBitmap* pBitmap = new(ELeave)CFbsBitmap();
  //pBitmap->Create(screenSize, displayMode);
  //pScreenDevice->CopyScreenToBitmap(pBitmap); // no need for all this

  int stride = screenSize.iWidth * DEFAULT_BPP;//translate_native_bpp(displayMode) / 8;
  int bufSizeInBytes = screenSize.iHeight * stride;
  logBuf.Format(BufData,
      screenSize.iWidth,
      screenSize.iHeight,
      (TInt)displayMode,
      stride,
      bufSizeInBytes);
  ALERT_WINBUF(logBuf);
  if ( bufSizeInBytes == 0)
  {
    ALERT_WIN("Screen device returned invalid data!");
    roadmap_log (ROADMAP_ERROR, "Screen device returned invalid data!");
  }
  ALERT_WIN("alloc buf");
  if ( pScreenBuf == NULL )
  {
    pScreenBuf = (unsigned char*)malloc(bufSizeInBytes);
  }
  else if ( initWidth*initHeight != bufSizeInBytes )
  {// screen dimensions were changed ; can also use realloc here
    free(pScreenBuf);
    pScreenBuf = (unsigned char*)malloc(bufSizeInBytes);
  }
  ALERT_WIN("agg_configure");
  roadmap_canvas_agg_configure(pScreenBuf,
     screenSize.iWidth,
     screenSize.iHeight,
     stride);

  //  init DSA
  if ( pDSACanvas == NULL )
  {
    delete pDSACanvas;
    pDSACanvas = NULL;
  }

  TRAPD (err, pDSACanvas = CRoadmapNativeCanvas::NewL(CEikonEnv::Static()->WsSession(),
                                                      *pScreenDevice,
                                                      aWindow));
  if ( err != KErrNone || pDSACanvas == NULL )
  {
    ALERT_WIN("Could not init direct screen access!");
    roadmap_log (ROADMAP_ERROR, "Could not init direct screen access!");
    return;
  }
  pDSACanvas->SetBuffer(pScreenBuf, bufSizeInBytes);
  err = pDSACanvas->Start();
  if ( err != KErrNone )
  {
    ALERT_WIN("Could not start direct screen access!");
    roadmap_log (ROADMAP_ERROR, "Could not start direct screen access!");
    return;
  }
  ALERT_WIN("screen configure done");

  (*RoadMapCanvasConfigure) ();
}

unsigned char* BmpWithMaskToBuffer( CFbsBitmap* apFrame,
                                    CFbsBitmap* apFrameMask,
                                    int width,
                                    int height,
                                    int stride)
{
  unsigned char* pBuf = (unsigned char*)malloc(height*stride);
  if ( pBuf == NULL ) { return pBuf;  }

  apFrame->LockHeap();
  apFrameMask->LockHeap();

  //BGR->RGB conversion
  TUint8* src = (TUint8*)apFrame->DataAddress();
  TInt maskSizeInPixels = apFrameMask->SizeInPixels().iHeight * apFrameMask->SizeInPixels().iWidth;
  TUint8* srcMask = (TUint8*)apFrameMask->DataAddress();
  TUint8* dst = (TUint8*)pBuf;

  TInt bpp = translate_native_bpp(apFrame->DisplayMode()) / 8;
//no need, it's 1 and we don't use it (see below) -->
  //TInt bppMask = translate_native_bpp(apFrameMask->DisplayMode()) / 8;
  //

  TInt paddedWidth = (apFrame->DataStride()/bpp);
  if ( paddedWidth > width )
  {
    int z = 7;
    z++;
  }
  for (TInt y = 0; y < height; y++ )
  {
    for (TInt x = 0; x < paddedWidth; x++ )
    {
      if ( x>=width )
      {// skip the padding in the source, we want contiguous data
        src += bpp;
        ++srcMask;
        continue;
      }
      dst[0] = src[2];
      dst[1] = src[1];
      dst[2] = src[0];
      if ( maskSizeInPixels != 0 )
      {//TODO ideally a for loop here equal to bppMask,
        //but actually mask is EGray256 which equals 1 anyway
        dst[3] = *srcMask;
        ++dst;
        ++srcMask;
      }
      dst += bpp; // depth = 3 or 4;
      src += bpp;
    }
  }

  apFrameMask->UnlockHeap();
  apFrame->UnlockHeap();
  return pBuf;
}

#ifndef _SYMBIAN32_MASKED_PNG_
unsigned char *read_png_file(const char* file_name, int *width, int *height,
                             int *stride) {

   png_byte color_type;
   png_byte bit_depth;

   png_structp png_ptr;
   png_infop info_ptr;
   int number_of_passes;
   png_bytep* row_pointers;
   int y;

   png_byte header[8];   // 8 is the maximum size that can be checked
   unsigned char *buf;
   FILE *fp;

   /* open file and test for it being a png */
   fp = fopen(file_name, "rb");
   if (!fp) return NULL;

   fread(header, 1, 8, fp);
   if (png_sig_cmp(header, 0, 8)) {
      roadmap_log (ROADMAP_ERROR,
         "[read_png_file] File %s is not recognized as a PNG file",
         file_name);
      fclose(fp);
      return NULL;
   }

   /* initialize stuff */
   png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

   if (!png_ptr) {
      roadmap_log (ROADMAP_ERROR,
                  "[read_png_file] png_create_read_struct failed");
      fclose(fp);
      return NULL;
   }

   info_ptr = png_create_info_struct(png_ptr);

   if (!info_ptr) {
      roadmap_log (ROADMAP_ERROR,
            "[read_png_file] png_create_info_struct failed");
      fclose(fp);
      return NULL;
   }

   if (setjmp(png_jmpbuf(png_ptr))) {
      roadmap_log (ROADMAP_ERROR, "[read_png_file] Error during init_io");
      fclose(fp);
      return NULL;
   }

   png_init_io(png_ptr, fp);
   png_set_sig_bytes(png_ptr, 8);

   png_read_info(png_ptr, info_ptr);

   *width = info_ptr->width;
   *height = info_ptr->height;
   *stride = info_ptr->rowbytes;
   color_type = info_ptr->color_type;
   bit_depth = info_ptr->bit_depth;

   number_of_passes = png_set_interlace_handling(png_ptr);
   png_read_update_info(png_ptr, info_ptr);

   /* read file */
   if (setjmp(png_jmpbuf(png_ptr))) {
      roadmap_log (ROADMAP_ERROR, "[read_png_file] Error during read_image");
      fclose(fp);
      return NULL;
   }

   row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * *height);
   buf = (unsigned char *)malloc (*height * info_ptr->rowbytes);
   for (y=0; y<*height; y++) {
      row_pointers[y] = (png_byte*) (buf + y * info_ptr->rowbytes);
   }

   png_read_image(png_ptr, row_pointers);
   png_read_end(png_ptr, NULL);
   png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
   free (row_pointers);

   fclose(fp);

   return buf;
}

#else
unsigned char *read_png_file(const char* file_name,
                              int &width,
                              int &height,
                              int &stride)
{
  TFileName filename;
  TBuf8<256> mimeType;
  GSConvert::CharPtrToTDes16(file_name, filename);
  CImageDecoder::GetMimeTypeFileL(CEikonEnv::Static()->FsSession(), filename, mimeType);
  CImageDecoder* pPngDecoder = NULL;
  TRAPD(err, pPngDecoder = CImageDecoder::FileNewL(
                                   CEikonEnv::Static()->FsSession(),
                                   filename,
                                   mimeType,
                                   CImageDecoder::EAllowGeneratedMask)); //PngMimeType; use is optional
  if ( err != KErrNone )
  {
    roadmap_log (ROADMAP_ERROR, "Could not read PNG file : %s, reason %d", file_name, err);
    return NULL;
  }

  CFbsBitmap* pFrame = new(ELeave)CFbsBitmap();
  pFrame->Create( pPngDecoder->FrameInfo(0).iOverallSizeInPixels,
                  pPngDecoder->FrameInfo(0).iFrameDisplayMode);
  CRoadmapNativePNG::Decode(pPngDecoder, pFrame, NULL);

  TSize bmpSize = pFrame->SizeInPixels();
  height = bmpSize.iHeight;
  width = bmpSize.iWidth;
  stride = pFrame->DataStride();
  //stride = CFbsBitmap::ScanLineLength(width, pFrame->DisplayMode());
  //stride = bmpSize.iWidth * translate_native_bpp(pFrame->DisplayMode()) / 8;
  int bufSizeInBytes = height * stride;
  if ( bufSizeInBytes == 0)
  {
    roadmap_log (ROADMAP_ERROR, "Screen device returned invalid data!");
  }
//  roadmap_log(ROADMAP_ERROR, "h=%d w=%d mode=%d stride=%d bufSize=%d",
//      height, width, translate_native_bpp(pPngDecoder->FrameInfo(0).iFrameDisplayMode), stride, bufSizeInBytes);

  unsigned char* buf = (unsigned char*)malloc(bufSizeInBytes);
  pFrame->LockHeap();
  memcpy(buf, pFrame->DataAddress(), bufSizeInBytes);
  pFrame->UnlockHeap();

  delete pFrame;
  delete pPngDecoder;
  return buf;
}
#endif


void roadmap_canvas_button_pressed(const TPoint *data) {
   RoadMapGuiPoint point;

   point.x = data->iX;
   point.y = data->iY;

   (*RoadMapCanvasMouseButtonPressed) (&point);

}


void roadmap_canvas_button_released(const TPoint *data) {
   RoadMapGuiPoint point;

   point.x = data->iX;
   point.y = data->iY;

   (*RoadMapCanvasMouseButtonReleased) (&point);

}


void roadmap_canvas_mouse_moved(const TPoint *data) {
   RoadMapGuiPoint point;

   point.x = data->iX;
   point.y = data->iY;

   (*RoadMapCanvasMouseMoved) (&point);

}

/*
static void roadmap_canvas_convert_points (POINT *winpoints,
         RoadMapGuiPoint *points, int count)
{
    RoadMapGuiPoint *end = points + count;

    while (points < end) {
        winpoints->x = points->x;
        winpoints->y = points->y;
        winpoints += 1;
        points += 1;
    }
}

void select_native_color (COLORREF color, int thickness) {
   static HBRUSH oldBrush;
   static HPEN oldPen;
   static COLORREF last_color;
   static int last_thickness;
   static int init;

   if (!init) {
      oldBrush = (HBRUSH) SelectObject(RoadMapDrawingBuffer,
            CreateSolidBrush(color));

      oldPen = (HPEN) SelectObject(RoadMapDrawingBuffer, CreatePen(PS_SOLID,
               thickness, color));

      last_color = color;
      last_thickness = thickness;
      init = 1;
      return;
   }

   if (last_color != color) {
      DeleteObject(
         SelectObject(RoadMapDrawingBuffer, CreatePen(PS_SOLID,
               thickness, color)));
      DeleteObject(SelectObject(RoadMapDrawingBuffer,
            CreateSolidBrush(color)));
      last_color = color;
      last_thickness = thickness;
   } else if (last_thickness != thickness) {
      DeleteObject(
         SelectObject(RoadMapDrawingBuffer, CreatePen(PS_SOLID,
               thickness, color)));
      last_thickness = thickness;
   }
}


void roadmap_canvas_native_draw_multiple_lines (int count, int *lines,
            RoadMapGuiPoint *points, int r, int g, int b, int thickness)
{
   int i;
   int count_of_points;

   POINT winpoints[1024];

   static int init;

   select_native_color (RGB(r, g, b), thickness);

   for (i = 0; i < count; ++i) {

      RoadMapGuiPoint end_points[2];
      int first = 1;

      count_of_points = *lines;

      if (count_of_points < 2) continue;

      while (count_of_points > 1024) {

         if (first) {
            first = 0;
            end_points[0] = *points;
         }
         roadmap_canvas_convert_points (winpoints, points, 1024);
         Polyline(RoadMapDrawingBuffer, winpoints, 1024);
         // We shift by 1023 only, because we must link the lines.
         points += 1023;
         count_of_points -= 1023;
      }

      if (first) {
         first = 0;
         end_points[0] = *points;
      }

      end_points[1] = points[count_of_points - 1];
      roadmap_canvas_convert_points (winpoints, points, count_of_points);
      Polyline(RoadMapDrawingBuffer, winpoints, count_of_points);

#if 0
      if (CurrentPen->thinkness > 5) {

         HPEN oldPen = SelectObject(RoadMapDrawingBuffer,
            GetStockObject(NULL_PEN));

         int radius = CurrentPen->thinkness / 2;

         Ellipse(RoadMapDrawingBuffer,
            end_points[0].x - radius, end_points[0].y - radius,
            radius + end_points[0].x + 1,
            radius + end_points[0].y + 1);

         Ellipse(RoadMapDrawingBuffer,
            end_points[1].x - radius, end_points[1].y - radius,
            radius + end_points[1].x + 1,
            radius + end_points[1].y + 1);

         SelectObject(RoadMapDrawingBuffer, oldPen);
      }
#endif

      points += count_of_points;
      lines += 1;
   }

//   DeleteObject(SelectObject(RoadMapDrawingBuffer, oldPen));
//   DeleteObject(SelectObject(RoadMapDrawingBuffer, oldBrush));
}
*/
