/*
 * GDI bitmap definitions
 *
 * Copyright 1993, 1994  Alexandre Julliard
 */

#ifndef __WINE_BITMAP_H
#define __WINE_BITMAP_H

#include "gdi.h"

struct tagGDI_BITMAP_DRIVER;

/* Flags used for BitmapBits. We only use the first two at the moment */

#define DDB_SET			1
#define DDB_GET			2
#define DDB_COPY		4
#define DDB_SETWITHFILLER	8

typedef struct {
    const struct tagDC_FUNCS *funcs; /* DC function table */
    void	 *physBitmap; /* ptr to device specific data */
} DDBITMAP;

  /* GDI logical bitmap object */
typedef struct tagBITMAPOBJ
{
    GDIOBJHDR   header;
    BITMAP      bitmap;
    SIZE        size;   /* For SetBitmapDimension() */

    DDBITMAP   *DDBitmap;

    /* For device-independent bitmaps: */
    DIBSECTION *dib;
} BITMAPOBJ;

typedef struct tagBITMAP_DRIVER
{
  INT  (*pSetDIBits)(struct tagBITMAPOBJ *,struct tagDC *,UINT,UINT,LPCVOID,const BITMAPINFO *,UINT,HBITMAP);
  INT  (*pGetDIBits)(struct tagBITMAPOBJ *,struct tagDC *,UINT,UINT,LPVOID,BITMAPINFO *,UINT,HBITMAP);
  VOID (*pDeleteDIBSection)(struct tagBITMAPOBJ *);
} BITMAP_DRIVER;

extern BITMAP_DRIVER *BITMAP_Driver;

  /* objects/bitmap.c */
extern INT16   BITMAP_GetObject16( BITMAPOBJ * bmp, INT16 count, LPVOID buffer );
extern INT   BITMAP_GetObject( BITMAPOBJ * bmp, INT count, LPVOID buffer );
extern BOOL  BITMAP_DeleteObject( HBITMAP16 hbitmap, BITMAPOBJ * bitmap );
extern INT   BITMAP_GetWidthBytes( INT width, INT depth );
extern HBITMAP BITMAP_LoadBitmapW(HINSTANCE instance,LPCWSTR name,
  UINT loadflags);
extern HBITMAP BITMAP_CopyBitmap( HBITMAP hbitmap );

  /* objects/dib.c */
extern int DIB_GetDIBWidthBytes( int width, int depth );
extern int DIB_BitmapInfoSize( BITMAPINFO * info, WORD coloruse );
extern int DIB_GetBitmapInfo( const BITMAPINFOHEADER *header, DWORD *width,
                              int *height, WORD *bpp, WORD *compr );
extern void DIB_UpdateDIBSection( DC *dc, BOOL toDIB );
extern void DIB_DeleteDIBSection( BITMAPOBJ *bmp );
extern void DIB_SelectDIBSection( DC *dc, BITMAPOBJ *bmp );
extern void DIB_FixColorsToLoadflags(BITMAPINFO * bmi, UINT loadflags,
  BYTE pix);

#endif  /* __WINE_BITMAP_H */
