/*
 * "$Id$"
 *
 *   Print plug-in header file for the GIMP.
 *
 *   Copyright 1997-1999 Michael Sweet (mike@easysw.com) and
 *	Robert Krawitz (rlk@alum.mit.edu)
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Revision History:
 *
 *   See ChangeLog
 */

/*
 * Include necessary header files...
 */
#ifndef HAVE_UNISTD_H
#define HAVE_UNISTD_H
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/*
 * Constants...
 */

#define OUTPUT_GRAY		0	/* Grayscale output */
#define OUTPUT_COLOR		1	/* Color output */

#define ORIENT_AUTO		-1	/* Best orientation */
#define ORIENT_PORTRAIT		0	/* Portrait orientation */
#define ORIENT_LANDSCAPE	1	/* Landscape orientation */

#ifndef MIN
#  define MIN(a,b)		((a) < (b) ? (a) : (b))
#  define MAX(a,b)		((a) > (b) ? (a) : (b))
#endif /* !MIN */


/*
 * Printer driver control structure.  See "print.c" for the actual list...
 */

typedef struct
{
  unsigned short composite[256];
  unsigned short red[256];
  unsigned short green[256];
  unsigned short blue[256];
} lut_t;

typedef void *Image;

extern void Image_init(Image image);
extern int Image_bpp(Image image);
extern int Image_width(Image image);
extern int Image_height(Image image);
extern const char *Image_get_pluginname(Image image);
extern void Image_get_col(Image image, unsigned char *data, int column);
extern void Image_get_row(Image image, unsigned char *data, int row);
extern void Image_progress_init(Image image);
extern void Image_note_progress(Image image, double current, double total);


typedef struct
{
  char	*long_name,			/* Long name for UI */
	*short_name;			/* Short name for printrc file */
  int	color,				/* TRUE if supports color */
	model;				/* Model number */
  float	gamma,				/* Gamma correction */
	density;			/* Ink "density" or black level */
  char	**(*parameters)(int model, char *ppd_file, char *name, int *count);
					/* Parameter names */
  void	(*media_size)(int model, char *ppd_file, char *media_size,
                      int *width, int *length);
  void	(*imageable_area)(int model, char *ppd_file, char *media_size,
                          int *left, int *right, int *bottom, int *top);
  /* Print function */
  void	(*print)(int model, char *ppd_file, char *resolution,
                 char *media_size, char *media_type, char *media_source,
                 int output_type, int orientation, float scaling, int left,
                 int top, int copies, FILE *prn, Image image,
                 unsigned char *cmap, lut_t *lut, float saturation);
} printer_t;

typedef void 	(*convert_t)(unsigned char *in, unsigned short *out, int width,
			     int bpp, lut_t *lut, unsigned char *cmap,
			     float saturation);


/*
 * Prototypes...
 */

extern void	dither_black(unsigned short *, int, int, int, unsigned char *);

extern void	dither_cmyk(unsigned short *, int, int, int, unsigned char *,
			    unsigned char *, unsigned char *,
			    unsigned char *, unsigned char *,
			    unsigned char *, unsigned char *);

extern void	dither_black4(unsigned short *, int, int, int,
			      unsigned char *);

extern void	dither_cmyk4(unsigned short *, int, int, int, unsigned char *,
			     unsigned char *, unsigned char *,
			     unsigned char *);

extern void	gray_to_gray(unsigned char *, unsigned short *, int, int,
			     lut_t *, unsigned char *, float);
extern void	indexed_to_gray(unsigned char *, unsigned short *, int, int,
				  lut_t *, unsigned char *, float);
extern void	indexed_to_rgb(unsigned char *, unsigned short *, int, int,
			       lut_t *, unsigned char *, float);
extern void	rgb_to_gray(unsigned char *, unsigned short *, int, int,
			    lut_t *, unsigned char *, float);
extern void	rgb_to_rgb(unsigned char *, unsigned short *, int, int,
			   lut_t *, unsigned char *, float);

extern void	compute_lut(lut_t *lut, int icontrast,
			    float red, float green, float blue,
			    int ibrightness, float print_gamma,
			    float gimp_gamma, float user_gamma, int linear,
			    float printer_density, float user_density);



extern void	default_media_size(int model, char *ppd_file, char *media_size,
		                   int *width, int *length);


extern char	**escp2_parameters(int model, char *ppd_file, char *name,
		                   int *count);
extern void	escp2_imageable_area(int model, char *ppd_file,
				     char *media_size, int *left, int *right,
				     int *bottom, int *top);
extern void	escp2_print(int model, char *ppd_file, char *resolution,
		            char *media_size, char *media_type,
			    char *media_source, int output_type,
			    int orientation, float scaling, int left,
			    int top, int copies, FILE *prn,
			    Image image, unsigned char *cmap,
			    lut_t *lut, float saturation);


extern char	**pcl_parameters(int model, char *ppd_file, char *name,
		                 int *count);
extern void	pcl_imageable_area(int model, char *ppd_file, char *media_size,
		                   int *left, int *right, int *bottom,
				   int *top);
extern void	pcl_print(int model, char *ppd_file, char *resolution,
		          char *media_size, char *media_type,
			  char *media_source, int output_type,
			  int orientation, float scaling,
		          int left, int top, int copies, FILE *prn,
		          Image image, unsigned char *cmap,
			  lut_t *lut, float saturation);


extern char	**ps_parameters(int model, char *ppd_file, char *name,
		                int *count);
extern void	ps_media_size(int model, char *ppd_file, char *media_size,
		              int *width, int *length);
extern void	ps_imageable_area(int model, char *ppd_file, char *media_size,
		                  int *left, int *right, int *bottom,
				  int *top);
extern void	ps_print(int model, char *ppd_file, char *resolution,
		         char *media_size, char *media_type,
			 char *media_source, int output_type,
			 int orientation, float scaling, int left, int top,
			 int copies, FILE *prn, Image image,
			 unsigned char *cmap, lut_t *lut, float saturation);

#ifdef LEFTOVER_8_BIT
extern void	dither_cmyk4(unsigned char *, int, int, int, unsigned char *,
		             unsigned char *, unsigned char *,
			     unsigned char *);
extern void	dither_black4(unsigned char *, int, int, int, unsigned char *);
extern void	gray_to_gray(unsigned char *, unsigned char *, int, int,
			     lut_t *, unsigned char *, float);
extern void	indexed_to_gray(unsigned char *, unsigned char *, int, int,
				lut_t *, unsigned char *, float);
#endif

/*
 * End of "$Id$".
 */
