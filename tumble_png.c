/*
 * tumble: build a PDF file from image files
 *
 * Copyright 2004 Daniel Gloeckner
 * 
 * Derived from tumble_jpeg.c written 2003 by Eric Smith <spacewar@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.  Note that permission is
 * not granted to redistribute this program under the terms of any
 * other version of the General Public License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111 USA
 *
 *  2009-03-13 [JDB] New module to add PNG image support.
 */


#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>  /* strcasecmp() is a BSDism */


#include "semantics.h"
#include "tumble.h"
#include "bitblt.h"
#include "pdf.h"
#include "tumble_input.h"


static FILE *png_f;

static struct {
  uint32_t palent;
  uint8_t bpp;
  uint8_t color;
  char pal[256*3];
} cinfo;


static bool match_png_suffix (char *suffix)
{
  return (strcasecmp (suffix, ".png") == 0);
}

static bool close_png_input_file (void)
{
  return (1);
}

#define BENUM(p) (((p)[0]<<24)+((p)[1]<<16)+((p)[2]<<8)+(p)[3])

static bool open_png_input_file (FILE *f, char *name)
{
  const char sig [8]="\211PNG\r\n\032\n";
  uint8_t buf [8];
  int l;

  l = fread (buf, 1, sizeof (sig), f);
  if (l != sizeof (sig) || memcmp(buf,sig,sizeof(sig))) {
    rewind(f);
    return 0;
  }

  png_f = f;
  
  return 1;
}


static bool last_png_input_page (void)
{
  return 1;
}


static bool get_png_image_info (int image,
				 input_attributes_t input_attributes,
				 image_info_t *image_info)
{
  uint8_t buf [20], unit;
  uint32_t width,height,xppu,yppu;
  size_t l;
  bool seen_IHDR,seen_PLTE,seen_pHYs;

  seen_IHDR=seen_PLTE=seen_pHYs=false;
  memset(&cinfo,0,sizeof(cinfo));
  unit=0;
  xppu=yppu=1;
  
  for(;;)
  {
    l = fread (buf, 1, 8, png_f);
    if(l != 8)
      return 0;
    l=BENUM(buf);
    if(!memcmp(buf+4,"IHDR",4)) {
      if(seen_IHDR || l!=13)
	return 0;
      seen_IHDR=true;
      l = fread (buf, 1, 17, png_f);
      if(l!=17)
	return 0;
      width=BENUM(buf);
      height=BENUM(buf+4);
      cinfo.bpp=buf[8];
      cinfo.color=buf[9];
      if(buf[8]>8 || buf[10] || buf[11] || buf[12])
	return 0;
      continue;
    }
    if(!seen_IHDR)
      return 0;
    if(!memcmp(buf+4,"PLTE",4)) {
      size_t i;
      if(seen_PLTE || l>256*3 || l%3 || !cinfo.color)
	return 0;
      seen_PLTE=true;
      i = fread (cinfo.pal, 1, l, png_f);
      if(i != l)
	return 0;
      cinfo.palent=l/3;
      fseek(png_f,4,SEEK_CUR);
    } else if(!memcmp(buf+4,"pHYs",4)) {
      if(seen_pHYs || l!=9)
	return 0;
      seen_pHYs=true;
      l = fread (buf, 1, 13, png_f);
      if(l != 13)
	return 0;
      xppu=BENUM(buf);
      yppu=BENUM(buf+4);
      unit=buf[8];
    } else if(!memcmp(buf+4,"IDAT",4)) {
      fseek(png_f,-8,SEEK_CUR);
      break;
    } else {
      fseek(png_f,l+4,SEEK_CUR);
    }
  }
  if(cinfo.color==3 && !seen_PLTE)
    return 0;

#ifdef DEBUG_JPEG
  printf ("color type: %d\n", cinfo.color);
  printf ("bit depth: %d\n", cinfo.bpp);
  printf ("density unit: %d\n", unit);
  printf ("x density: %d\n", xppu);
  printf ("y density: %d\n", yppu);
  printf ("width: %d\n", width);
  printf ("height: %d\n", height);
#endif

  switch (cinfo.color)
    {
    case 0:
      image_info->color = 0;
      break;
    case 2:
    case 3:
      image_info->color = 1;
      break;
    default:
      fprintf (stderr, "PNG color type %d not supported\n", cinfo.color);
      return (0);
    }
  image_info->width_samples = width;
  image_info->height_samples = height;

  switch (unit==1)
  {
    case 1:
      image_info->width_points = ((image_info->width_samples * POINTS_PER_INCH) /
				  (xppu * 0.0254));
      image_info->height_points = ((image_info->height_samples * POINTS_PER_INCH) /
				   (yppu * 0.0254));
      break;
    case 0:
      /* assume 300 DPI - not great, but what else can we do? */
      image_info->width_points = (image_info->width_samples * POINTS_PER_INCH) / 300.0;
      image_info->height_points = ((double) yppu * image_info->height_samples * POINTS_PER_INCH) / ( 300.0 * xppu);
      break;
    default:
      fprintf (stderr, "PNG pHYs unit %d not supported\n", unit);
  }

  return 1;
}


static bool process_png_image (int image,  /* range 1 .. n */
			       input_attributes_t input_attributes,
			       image_info_t *image_info,
			       pdf_page_handle page,
			       position_t position)
{
  pdf_write_png_image (page,
		       position.x, position.y,
		       image_info->width_points,
		       image_info->height_points,
		       cinfo.color,
		       cinfo.color==3?cinfo.pal:NULL,
		       cinfo.palent,
		       cinfo.bpp,
		       image_info->width_samples,
		       image_info->height_samples,
		       input_attributes.transparency,
		       png_f);

  return (1);
}


input_handler_t png_handler =
  {
    match_png_suffix,
    open_png_input_file,
    close_png_input_file,
    last_png_input_page,
    get_png_image_info,
    process_png_image
  };


void init_png_handler (void)
{
  install_input_handler (& png_handler);
}
