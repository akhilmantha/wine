/*
 * X11 graphics driver initialisation functions
 *
 * Copyright 1996 Alexandre Julliard
 */

#include "config.h"

#ifndef X_DISPLAY_MISSING

#include "ts_xlib.h"

#include <string.h>

#include "bitmap.h"
#include "color.h"
#include "debug.h"
#include "ldt.h"
#include "local.h"
#include "monitor.h"
#include "winnt.h"
#include "x11drv.h"

static BOOL X11DRV_CreateDC( DC *dc, LPCSTR driver, LPCSTR device,
                               LPCSTR output, const DEVMODE16* initData );
static BOOL X11DRV_DeleteDC( DC *dc );

static INT X11DRV_Escape( DC *dc, INT nEscape, INT cbInput,
                            SEGPTR lpInData, SEGPTR lpOutData );

static const DC_FUNCTIONS X11DRV_Funcs =
{
    X11DRV_Arc,                      /* pArc */
    X11DRV_BitBlt,                   /* pBitBlt */
    X11DRV_BitmapBits,               /* pBitmapBits */
    X11DRV_Chord,                    /* pChord */
    X11DRV_CreateBitmap,             /* pCreateBitmap */
    X11DRV_CreateDC,                 /* pCreateDC */
    X11DRV_DeleteDC,                 /* pDeleteDC */
    X11DRV_DIB_CreateDIBSection,     /* pCreateDIBSection */
    X11DRV_DIB_CreateDIBSection16,   /* pCreateDIBSection16 */
    X11DRV_DeleteObject,             /* pDeleteObject */
    X11DRV_Ellipse,                  /* pEllipse */
    X11DRV_EnumDeviceFonts,          /* pEnumDeviceFonts */
    X11DRV_Escape,                   /* pEscape */
    NULL,                            /* pExcludeClipRect */
    NULL,                            /* pExcludeVisRect */
    X11DRV_ExtFloodFill,             /* pExtFloodFill */
    X11DRV_ExtTextOut,               /* pExtTextOut */
    X11DRV_GetCharWidth,             /* pGetCharWidth */
    X11DRV_GetPixel,                 /* pGetPixel */
    X11DRV_GetTextExtentPoint,       /* pGetTextExtentPoint */
    X11DRV_GetTextMetrics,           /* pGetTextMetrics */
    NULL,                            /* pIntersectClipRect */
    NULL,                            /* pIntersectVisRect */
    X11DRV_LineTo,                   /* pLineTo */
    X11DRV_LoadOEMResource,          /* pLoadOEMResource */
    X11DRV_MoveToEx,                 /* pMoveToEx */
    NULL,                            /* pOffsetClipRgn */
    NULL,                            /* pOffsetViewportOrg (optional) */
    NULL,                            /* pOffsetWindowOrg (optional) */
    X11DRV_PaintRgn,                 /* pPaintRgn */
    X11DRV_PatBlt,                   /* pPatBlt */
    X11DRV_Pie,                      /* pPie */
    X11DRV_PolyPolygon,              /* pPolyPolygon */
    X11DRV_PolyPolyline,             /* pPolyPolyline */
    X11DRV_Polygon,                  /* pPolygon */
    X11DRV_Polyline,                 /* pPolyline */
    X11DRV_PolyBezier,               /* pPolyBezier */
    NULL,                            /* pRealizePalette */
    X11DRV_Rectangle,                /* pRectangle */
    NULL,                            /* pRestoreDC */
    X11DRV_RoundRect,                /* pRoundRect */
    NULL,                            /* pSaveDC */
    NULL,                            /* pScaleViewportExt (optional) */
    NULL,                            /* pScaleWindowExt (optional) */
    NULL,                            /* pSelectClipRgn */
    X11DRV_SelectObject,             /* pSelectObject */
    NULL,                            /* pSelectPalette */
    X11DRV_SetBkColor,               /* pSetBkColor */
    NULL,                            /* pSetBkMode */
    X11DRV_SetDeviceClipping,        /* pSetDeviceClipping */
    X11DRV_SetDIBitsToDevice,        /* pSetDIBitsToDevice */
    NULL,                            /* pSetMapMode (optional) */
    NULL,                            /* pSetMapperFlags */
    X11DRV_SetPixel,                 /* pSetPixel */
    NULL,                            /* pSetPolyFillMode */
    NULL,                            /* pSetROP2 */
    NULL,                            /* pSetRelAbs */
    NULL,                            /* pSetStretchBltMode */
    NULL,                            /* pSetTextAlign */
    NULL,                            /* pSetTextCharacterExtra */
    X11DRV_SetTextColor,             /* pSetTextColor */
    NULL,                            /* pSetTextJustification */
    NULL,                            /* pSetViewportExt (optional) */
    NULL,                            /* pSetViewportOrg (optional) */
    NULL,                            /* pSetWindowExt (optional) */
    NULL,                            /* pSetWindowOrg (optional) */
    X11DRV_StretchBlt,               /* pStretchBlt */
    NULL                             /* pStretchDIBits */
};

GDI_DRIVER X11DRV_GDI_Driver =
{
  X11DRV_GDI_Initialize,
  X11DRV_GDI_Finalize
};

BITMAP_DRIVER X11DRV_BITMAP_Driver =
{
  X11DRV_DIB_SetDIBits,
  X11DRV_DIB_GetDIBits,
  X11DRV_DIB_DeleteDIBSection
};

PALETTE_DRIVER X11DRV_PALETTE_Driver =
{
  X11DRV_PALETTE_SetMapping,
  X11DRV_PALETTE_UpdateMapping
};

DeviceCaps X11DRV_DevCaps = {
/* version */		0, 
/* technology */	DT_RASDISPLAY,
/* size, resolution */	0, 0, 0, 0, 0, 
/* device objects */	1, 16 + 6, 16, 0, 0, 100, 0,	
/* curve caps */	CC_CIRCLES | CC_PIE | CC_CHORD | CC_ELLIPSES |
			CC_WIDE | CC_STYLED | CC_WIDESTYLED | CC_INTERIORS | CC_ROUNDRECT,
/* line caps */		LC_POLYLINE | LC_MARKER | LC_POLYMARKER | LC_WIDE |
			LC_STYLED | LC_WIDESTYLED | LC_INTERIORS,
/* polygon caps */	PC_POLYGON | PC_RECTANGLE | PC_WINDPOLYGON |
			PC_SCANLINE | PC_WIDE | PC_STYLED | PC_WIDESTYLED | PC_INTERIORS,
/* text caps */		0,
/* regions */		CP_REGION,
/* raster caps */	RC_BITBLT | RC_BANDING | RC_SCALING | RC_BITMAP64 |
			RC_DI_BITMAP | RC_DIBTODEV | RC_BIGFONT | RC_STRETCHBLT | RC_STRETCHDIB | RC_DEVBITS,
/* aspects */		36, 36, 51,
/* pad1 */		{ 0 },
/* log pixels */	0, 0, 
/* pad2 */		{ 0 },
/* palette size */	0,
/* ..etc */		0, 0 };

/**********************************************************************
 *	     X11DRV_GDI_Initialize
 */
BOOL X11DRV_GDI_Initialize(void)
{
    BITMAP_Driver = &X11DRV_BITMAP_Driver;
    PALETTE_Driver = &X11DRV_PALETTE_Driver;

    /* FIXME: colormap management should be merged with the X11DRV */

    if( !X11DRV_DIB_Init() ) return FALSE;

    if( !X11DRV_PALETTE_Init() ) return FALSE;

    if( !X11DRV_OBM_Init() ) return FALSE;

    /* Finish up device caps */

#if 0
    TRACE(x11drv, "Height = %-4i pxl, %-4i mm, Width  = %-4i pxl, %-4i mm\n",
	  HeightOfScreen(X11DRV_GetXScreen()), HeightMMOfScreen(X11DRV_GetXScreen()),
	  WidthOfScreen(X11DRV_GetXScreen()), WidthMMOfScreen(X11DRV_GetXScreen()) );
#endif

    X11DRV_DevCaps.version = 0x300;
    X11DRV_DevCaps.horzSize = WidthMMOfScreen(X11DRV_GetXScreen()) * MONITOR_GetWidth(&MONITOR_PrimaryMonitor) / WidthOfScreen(X11DRV_GetXScreen());
    X11DRV_DevCaps.vertSize = HeightMMOfScreen(X11DRV_GetXScreen()) * MONITOR_GetHeight(&MONITOR_PrimaryMonitor) / HeightOfScreen(X11DRV_GetXScreen());
    X11DRV_DevCaps.horzRes = MONITOR_GetWidth(&MONITOR_PrimaryMonitor);
    X11DRV_DevCaps.vertRes = MONITOR_GetHeight(&MONITOR_PrimaryMonitor);
    X11DRV_DevCaps.bitsPixel = MONITOR_GetDepth(&MONITOR_PrimaryMonitor);
 
    /* Resolution will be adjusted during the font init */

    X11DRV_DevCaps.logPixelsX = (int)(X11DRV_DevCaps.horzRes * 25.4 / X11DRV_DevCaps.horzSize);
    X11DRV_DevCaps.logPixelsY = (int)(X11DRV_DevCaps.vertRes * 25.4 / X11DRV_DevCaps.vertSize);

    /* Create default bitmap */

    if (!X11DRV_BITMAP_Init()) return FALSE;

    /* Initialize brush dithering */

    if (!X11DRV_BRUSH_Init()) return FALSE;

    /* Initialize fonts and text caps */

    if (!X11DRV_FONT_Init( &X11DRV_DevCaps )) return FALSE;

    return DRIVER_RegisterDriver( "DISPLAY", &X11DRV_Funcs );
}

/**********************************************************************
 *	     X11DRV_GDI_Finalize
 */
void X11DRV_GDI_Finalize(void)
{
  X11DRV_PALETTE_Cleanup();
}

/**********************************************************************
 *	     X11DRV_CreateDC
 */
static BOOL X11DRV_CreateDC( DC *dc, LPCSTR driver, LPCSTR device,
                               LPCSTR output, const DEVMODE16* initData )
{
    X11DRV_PDEVICE *physDev;

    dc->physDev = physDev = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
				       sizeof(*physDev) );
    if(!physDev) {
        ERR(x11drv, "Can't allocate physDev\n");
	return FALSE;
    }

    dc->w.devCaps      = &X11DRV_DevCaps;
    if (dc->w.flags & DC_MEMORY)
    {
        X11DRV_PHYSBITMAP *pbitmap;
        BITMAPOBJ *bmp = (BITMAPOBJ *) GDI_GetObjPtr( dc->w.hBitmap,
                                                      BITMAP_MAGIC );
	X11DRV_CreateBitmap( dc->w.hBitmap );
	pbitmap            = bmp->DDBitmap->physBitmap;
        physDev->drawable  = pbitmap->pixmap;
        physDev->gc        = TSXCreateGC(display, physDev->drawable, 0, NULL);
        dc->w.bitsPerPixel = bmp->bitmap.bmBitsPixel;

        dc->w.totalExtent.left   = 0;
        dc->w.totalExtent.top    = 0;
        dc->w.totalExtent.right  = bmp->bitmap.bmWidth;
        dc->w.totalExtent.bottom = bmp->bitmap.bmHeight;
        dc->w.hVisRgn            = CreateRectRgnIndirect( &dc->w.totalExtent );

	GDI_HEAP_UNLOCK( dc->w.hBitmap );
    }
    else
    {
        physDev->drawable  = X11DRV_GetXRootWindow();
        physDev->gc        = TSXCreateGC( display, physDev->drawable, 0, NULL );
        dc->w.bitsPerPixel = MONITOR_GetDepth(&MONITOR_PrimaryMonitor);

        dc->w.totalExtent.left   = 0;
        dc->w.totalExtent.top    = 0;
        dc->w.totalExtent.right  = MONITOR_GetWidth(&MONITOR_PrimaryMonitor);
        dc->w.totalExtent.bottom = MONITOR_GetHeight(&MONITOR_PrimaryMonitor);
        dc->w.hVisRgn            = CreateRectRgnIndirect( &dc->w.totalExtent );
    }

    if (!dc->w.hVisRgn)
    {
        TSXFreeGC( display, physDev->gc );
        return FALSE;
    }

    TSXSetGraphicsExposures( display, physDev->gc, False );
    TSXSetSubwindowMode( display, physDev->gc, IncludeInferiors );

    return TRUE;
}


/**********************************************************************
 *	     X11DRV_DeleteDC
 */
static BOOL X11DRV_DeleteDC( DC *dc )
{
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;
    TSXFreeGC( display, physDev->gc );
    HeapFree( GetProcessHeap(), 0, physDev );
    dc->physDev = NULL;
    return TRUE;
}

/**********************************************************************
 *           X11DRV_Escape
 */
static INT X11DRV_Escape( DC *dc, INT nEscape, INT cbInput,
                            SEGPTR lpInData, SEGPTR lpOutData )
{
    switch( nEscape )
    {
	case GETSCALINGFACTOR:
	     if( lpOutData )
	     {
		 LPPOINT16 lppt = (LPPOINT16)PTR_SEG_TO_LIN(lpOutData);
		 lppt->x = lppt->y = 0;	/* no device scaling */
		 return 1;
	     }
	     break;
    }
    return 0;
}

#endif /* !defined(X_DISPLAY_MISSING) */
