/*
 * "$Id$"
 *
 *   Print plug-in HP PCL driver for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com),
 *	Robert Krawitz (rlk@alum.mit.edu) and
 *      Dave Hill (dave@minnie.demon.co.uk)
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
 */

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gtk, etc.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gimp-print.h>
#include <gimp-print-internal.h>
#include <gimp-print-intl-internal.h>
#include <stdio.h>
#include <string.h>

/* #define DEBUG */
/* #define PCL_DEBUG_DISABLE_COMPRESSION */
/* #define PCL_DEBUG_DISABLE_BLANKLINE_REMOVAL */

/*
 * Local functions...
 */
static void	pcl_mode0(const stp_vars_t, unsigned char *, int, int);
static void	pcl_mode2(const stp_vars_t, unsigned char *, int, int);

/*
 * Generic define for a name/value set
 */

typedef struct
{
  const char  *pcl_name;
  int   pcl_code;
} pcl_t;

/*
 * Media size to PCL media size code table
 *
 * Note, you can force the list of papersizes given in the GUI to be only those
 * supported by defining PCL_NO_CUSTOM_PAPERSIZES
 */

/* #define PCL_NO_CUSTOM_PAPERSIZES */

#define PCL_PAPERSIZE_EXECUTIVE		1
#define PCL_PAPERSIZE_LETTER		2
#define PCL_PAPERSIZE_LEGAL		3
#define PCL_PAPERSIZE_TABLOID		6	/* "Ledger" */
#define PCL_PAPERSIZE_STATEMENT		15	/* Called "Manual" in print-util */
#define PCL_PAPERSIZE_SUPER_B		16	/* Called "13x19" in print-util */
#define PCL_PAPERSIZE_A5		25
#define PCL_PAPERSIZE_A4		26
#define PCL_PAPERSIZE_A3		27
#define PCL_PAPERSIZE_JIS_B5		45
#define PCL_PAPERSIZE_JIS_B4		46
#define PCL_PAPERSIZE_HAGAKI_CARD	71
#define PCL_PAPERSIZE_OUFUKU_CARD	72
#define PCL_PAPERSIZE_A6_CARD		73
#define PCL_PAPERSIZE_4x6		74
#define PCL_PAPERSIZE_5x8		75
#define PCL_PAPERSIZE_3x5		78
#define PCL_PAPERSIZE_MONARCH_ENV	80
#define PCL_PAPERSIZE_COMMERCIAL10_ENV	81
#define PCL_PAPERSIZE_DL_ENV		90
#define PCL_PAPERSIZE_C5_ENV		91
#define PCL_PAPERSIZE_C6_ENV		92
#define PCL_PAPERSIZE_CUSTOM		101	/* Custom size */
#define PCL_PAPERSIZE_INVITATION_ENV	109
#define PCL_PAPERSIZE_JAPANESE_3_ENV	110
#define PCL_PAPERSIZE_JAPANESE_4_ENV	111
#define PCL_PAPERSIZE_KAKU_ENV		113
#define PCL_PAPERSIZE_HP_CARD		114	/* HP Greeting card!?? */

/*
 * This data comes from the HP documentation "Deskjet 1220C and 1120C
 * PCL reference guide 2.0, Nov 1999". NOTE: The names *must* match
 * those in print-util.c for the lookups to work properly!
 */

static const pcl_t pcl_media_sizes[] =
{
    {N_ ("Executive"), PCL_PAPERSIZE_EXECUTIVE},		/* US Exec (7.25 x 10.5 in) */
    {N_ ("Letter"), PCL_PAPERSIZE_LETTER},			/* US Letter (8.5 x 11 in) */
    {N_ ("Legal"), PCL_PAPERSIZE_LEGAL},			/* US Legal (8.5 x 14 in) */
    {N_ ("Tabloid"), PCL_PAPERSIZE_TABLOID},			/* US Tabloid (11 x 17 in) */
    {N_ ("Manual"), PCL_PAPERSIZE_STATEMENT},			/* US Manual/Statement (5.5 x 8.5 in) */
    {N_ ("13x19"), PCL_PAPERSIZE_SUPER_B},			/* US 13x19/Super B (13 x 19 in) */
    {N_ ("A5"), PCL_PAPERSIZE_A5},				/* ISO/JIS A5 (148 x 210 mm) */
    {N_ ("A4"), PCL_PAPERSIZE_A4},				/* ISO/JIS A4 (210 x 297 mm) */
    {N_ ("A3"), PCL_PAPERSIZE_A3},				/* ISO/JIS A3 (297 x 420 mm) */
    {N_ ("B5 JIS"), PCL_PAPERSIZE_JIS_B5},			/* JIS B5 (182 x 257 mm). */
    {N_ ("B4 JIS"), PCL_PAPERSIZE_JIS_B4},			/* JIS B4 (257 x 364 mm). */
    {N_ ("Hagaki Card"), PCL_PAPERSIZE_HAGAKI_CARD},		/* Japanese Hagaki Card (100 x 148 mm) */
    {N_ ("Oufuku Card"), PCL_PAPERSIZE_OUFUKU_CARD},		/* Japanese Oufuku Card (148 x 200 mm) */
    {N_ ("A6"), PCL_PAPERSIZE_A6_CARD},				/* ISO/JIS A6 card */
    {N_ ("4x6"), PCL_PAPERSIZE_4x6},				/* US Index card (4 x 6 in) */
    {N_ ("5x8"), PCL_PAPERSIZE_5x8},				/* US Index card (5 x 8 in) */
    {N_ ("3x5"), PCL_PAPERSIZE_3x5},				/* US Index card (3 x 5 in) */
    {N_ ("Monarch"), PCL_PAPERSIZE_MONARCH_ENV},		/* Monarch Envelope (3 7/8 x 7 1/2 in) */
    {N_ ("Commercial 10"), PCL_PAPERSIZE_COMMERCIAL10_ENV},	/* US Commercial 10 Envelope (4.125 x 9.5 in) Portrait */
    {N_ ("DL"), PCL_PAPERSIZE_DL_ENV},				/* DL envelope (110 x 220 mm) Portrait */
    {N_ ("C5"), PCL_PAPERSIZE_C5_ENV},				/* C5 envelope (162 x 229 mm) */
    {N_ ("C6"), PCL_PAPERSIZE_C6_ENV},				/* C6 envelope (114 x 162 mm) */
    {N_ ("A2 Invitation"), PCL_PAPERSIZE_INVITATION_ENV},	/* US A2 Invitation envelope (4 3/8 x 5 3/4 in) */
    {N_ ("Long 3"), PCL_PAPERSIZE_JAPANESE_3_ENV},		/* Japanese Long Envelope #3 (120 x 235 mm) */
    {N_ ("Long 4"), PCL_PAPERSIZE_JAPANESE_4_ENV},		/* Japanese Long Envelope #4 (90 x 205 mm) */
    {N_ ("Kaku"), PCL_PAPERSIZE_KAKU_ENV},			/* Japanese Kaku Envelope (240 x 332.1 mm) */
    {N_ ("HP Greeting Card"), PCL_PAPERSIZE_HP_CARD}, 	/* Hp greeting card (size?? */
};
#define NUM_PRINTER_PAPER_SIZES	(sizeof(pcl_media_sizes) / sizeof(pcl_t))

/*
 * Media type to code table
 */

#define PCL_PAPERTYPE_PLAIN	0
#define PCL_PAPERTYPE_BOND	1
#define PCL_PAPERTYPE_PREMIUM	2
#define PCL_PAPERTYPE_GLOSSY	3	/* or photo */
#define PCL_PAPERTYPE_TRANS	4
#define PCL_PAPERTYPE_QPHOTO	5	/* Quick dry photo (2000 only) */
#define PCL_PAPERTYPE_QTRANS	6	/* Quick dry transparency (2000 only) */

static const pcl_t pcl_media_types[] =
{
    {N_ ("Plain"), PCL_PAPERTYPE_PLAIN},
    {N_ ("Bond"), PCL_PAPERTYPE_BOND},
    {N_ ("Premium"), PCL_PAPERTYPE_PREMIUM},
    {N_ ("Glossy/Photo"), PCL_PAPERTYPE_GLOSSY},
    {N_ ("Transparency"), PCL_PAPERTYPE_TRANS},
    {N_ ("Quick-dry Photo"), PCL_PAPERTYPE_QPHOTO},
    {N_ ("Quick-dry Transparency"), PCL_PAPERTYPE_QTRANS},
};
#define NUM_PRINTER_PAPER_TYPES	(sizeof(pcl_media_types) / sizeof(pcl_t))

/*
 * Media feed to code table. There are different names for the same code,
 * so we encode them by adding "lumps" of "PAPERSOURCE_MOD".
 * This is removed later to get back to the main codes.
 */

#define PAPERSOURCE_MOD			16

#define PCL_PAPERSOURCE_STANDARD	0	/* Don't output code */
#define PCL_PAPERSOURCE_MANUAL		2
#define PCL_PAPERSOURCE_ENVELOPE	3	/* Not used */

/* LaserJet types */
#define PCL_PAPERSOURCE_LJ_TRAY2	1
#define PCL_PAPERSOURCE_LJ_TRAY3	4
#define PCL_PAPERSOURCE_LJ_TRAY4	5
#define PCL_PAPERSOURCE_LJ_TRAY1	8

/* Deskjet 340 types */
#define PCL_PAPERSOURCE_340_PCSF	1 + PAPERSOURCE_MOD
						/* Portable sheet feeder for 340 */
#define PCL_PAPERSOURCE_340_DCSF	4 + PAPERSOURCE_MOD
						/* Desktop sheet feeder for 340 */

/* Other Deskjet types */
#define PCL_PAPERSOURCE_DJ_TRAY		1 + PAPERSOURCE_MOD + PAPERSOURCE_MOD
#define PCL_PAPERSOURCE_DJ_TRAY2	4 + PAPERSOURCE_MOD + PAPERSOURCE_MOD
						/* Tray 2 for 2500 */
#define PCL_PAPERSOURCE_DJ_OPTIONAL	5 + PAPERSOURCE_MOD + PAPERSOURCE_MOD
						/* Optional source for 2500 */
#define PCL_PAPERSOURCE_DJ_AUTO		7 + PAPERSOURCE_MOD + PAPERSOURCE_MOD
						/* Autoselect for 2500 */

static const pcl_t pcl_media_sources[] =
{
    {N_ ("Standard"), PCL_PAPERSOURCE_STANDARD},
    {N_ ("Manual"), PCL_PAPERSOURCE_MANUAL},
/*  {"Envelope", PCL_PAPERSOURCE_ENVELOPE}, */
    {N_ ("Tray 1"), PCL_PAPERSOURCE_LJ_TRAY1},
    {N_ ("Tray 2"), PCL_PAPERSOURCE_LJ_TRAY2},
    {N_ ("Tray 3"), PCL_PAPERSOURCE_LJ_TRAY3},
    {N_ ("Tray 4"), PCL_PAPERSOURCE_LJ_TRAY4},
    {N_ ("Portable Sheet Feeder"), PCL_PAPERSOURCE_340_PCSF},
    {N_ ("Desktop Sheet Feeder"), PCL_PAPERSOURCE_340_DCSF},
    {N_ ("Tray"), PCL_PAPERSOURCE_DJ_TRAY},
    {N_ ("Tray 2"), PCL_PAPERSOURCE_DJ_TRAY2},
    {N_ ("Optional Source"), PCL_PAPERSOURCE_DJ_OPTIONAL},
    {N_ ("Autoselect"), PCL_PAPERSOURCE_DJ_AUTO},
};
#define NUM_PRINTER_PAPER_SOURCES	(sizeof(pcl_media_sources) / sizeof(pcl_t))

#define PCL_RES_150_150		1
#define PCL_RES_300_300		2
#define PCL_RES_600_300		4	/* DJ 600 series */
#define PCL_RES_600_600_MONO	8	/* DJ 600/800/1100/2000 b/w only */
#define PCL_RES_600_600		16	/* DJ 9xx/1220C/PhotoSmart */
#define PCL_RES_1200_600	32	/* DJ 9xx/1220C/PhotoSmart */
#define PCL_RES_2400_600	64	/* DJ 9xx/1220C/PhotoSmart */

static const pcl_t pcl_resolutions[] =
{
    {N_ ("150x150 DPI"), PCL_RES_150_150},
    {N_ ("300x300 DPI"), PCL_RES_300_300},
    {N_ ("600x300 DPI"), PCL_RES_600_300},
    {N_ ("600x600 DPI monochrome"), PCL_RES_600_600_MONO},
    {N_ ("600x600 DPI"), PCL_RES_600_600},
    {N_ ("1200x600 DPI"), PCL_RES_1200_600},
    {N_ ("2400x600 DPI"), PCL_RES_2400_600},
};
#define NUM_RESOLUTIONS		(sizeof(pcl_resolutions) / sizeof (pcl_t))

static void
pcl_describe_resolution(const stp_printer_t printer,
			const char *resolution, int *x, int *y)
{
  int i;
  for (i = 0; i < NUM_RESOLUTIONS; i++)
    {
      if (!strcmp(resolution, _(pcl_resolutions[i].pcl_name)))
	{
	  sscanf(resolution, "%dx%d", x, y);
	  return;
	}
    }
  *x = -1;
  *y = -1;
}

/*
 * Printer capability data
 */

typedef struct {
  int model;
  int max_width;
  int max_height;
  int resolutions;
  int top_margin;
  int bottom_margin;
  int left_margin;
  int right_margin;
  int color_type;		/* 2 print head or one, 2 level or 4 */
  int stp_printer_type;		/* Deskjet/Laserjet and quirks */
/* The paper size, paper type and paper source codes cannot be combined */
  const int paper_sizes[NUM_PRINTER_PAPER_SIZES + 1];
				/* Paper sizes */
  const int paper_types[NUM_PRINTER_PAPER_TYPES + 1];
				/* Paper types */
  const int paper_sources[NUM_PRINTER_PAPER_SOURCES + 1];
				/* Paper sources */
  } pcl_cap_t;

#define PCL_COLOR_NONE		0
#define PCL_COLOR_CMY		1	/* One print head */
#define PCL_COLOR_CMYK		2	/* Two print heads */
#define PCL_COLOR_CMYK4		4	/* CRet printing */
#define PCL_COLOR_CMYKcm	8	/* CMY + Photo Cart */
#define PCL_COLOR_CMYK4b	16	/* CRet for HP840c */

#define PCL_PRINTER_LJ		1
#define PCL_PRINTER_DJ		2
#define PCL_PRINTER_NEW_ERG	4	/* use "\033*rC" to end raster graphics,
					   instead of "\033*rB" */
#define PCL_PRINTER_TIFF	8	/* Use TIFF compression */
#define PCL_PRINTER_MEDIATYPE	16	/* Use media type & print quality */
#define PCL_PRINTER_CUSTOM_SIZE	32	/* Custom sizes supported */
#define PCL_PRINTER_BLANKLINE	64	/* Blank line removal supported */

/*
 * FIXME - the 520 shouldn't be lumped in with the 500 as it supports
 * more paper sizes.
 *
 * The following models use depletion, raster quality and shingling:-
 * 500, 500c, 510, 520, 550c, 560c.
 * The rest use Media Type and Print Quality.
 *
 * This data comes from the HP documentation "Deskjet 1220C and 1120C
 * PCL reference guide 2.0, Nov 1999".
 */

static const pcl_cap_t pcl_model_capabilities[] =
{
  /* Default/unknown printer - assume laserjet */
  { 0,
    17 * 72 / 2, 14 * 72,		/* Max paper size */
    PCL_RES_150_150 | PCL_RES_300_300,	/* Resolutions */
    12, 12, 18, 18,			/* Margins */
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_STATEMENT,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A4,
      0,
    },
    { -1,			/* No selectable paper types */
    },
    { -1,			/* No selectable paper sources */
    },
  },
  /* Deskjet 340 */
  { 340,
    17 * 72 / 2, 14 * 72,
    PCL_RES_150_150 | PCL_RES_300_300,
    7, 41, 18, 18,
    PCL_COLOR_CMY,
    PCL_PRINTER_DJ | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A4,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_340_PCSF,
      PCL_PAPERSOURCE_340_DCSF,
      -1,
    },
  },
  /* Deskjet 400 */
  { 400,
    17 * 72 / 2, 14 * 72,
    PCL_RES_150_150 | PCL_RES_300_300,
    7, 41, 18, 18,
    PCL_COLOR_CMY,
    PCL_PRINTER_DJ | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_JIS_B5,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    { -1,			/* No selectable paper sources */
    },
  },
  /* Deskjet 500, 520 */
  { 500,
    17 * 72 / 2, 14 * 72,
    PCL_RES_150_150 | PCL_RES_300_300,
    7, 41, 18, 18,
    PCL_COLOR_NONE,
    PCL_PRINTER_DJ | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE,
    {
/*    PCL_PAPERSIZE_EXECUTIVE,	The 500 doesn't support this, the 520 does */
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
/*    PCL_PAPERSIZE_DL_ENV,	The 500 doesn't support this, the 520 does */
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_DJ_TRAY,
      -1,
    },
  },
  /* Deskjet 500C */
  { 501,
    17 * 72 / 2, 14 * 72,
    PCL_RES_150_150 | PCL_RES_300_300,
    7, 33, 18, 18,
    PCL_COLOR_CMY,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_DJ_TRAY,
      -1,
    },
  },
  /* Deskjet 540C */
  { 540,
    17 * 72 / 2, 14 * 72,
    PCL_RES_150_150 | PCL_RES_300_300,
    7, 33, 18, 18,
    PCL_COLOR_CMY,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_JIS_B5,
      PCL_PAPERSIZE_HAGAKI_CARD,
      PCL_PAPERSIZE_A6_CARD,
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C6_ENV,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_DJ_TRAY,
      -1,
    },
  },
  /* Deskjet 550C, 560C */
  { 550,
    17 * 72 / 2, 14 * 72,
    PCL_RES_150_150 | PCL_RES_300_300,
    3, 33, 18, 18,
    PCL_COLOR_CMYK,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A4,
/* The 550/560 support COM10 and DL envelope, but the control codes
   are negative, indicating landscape mode. This needs thinking about! */
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_DJ_TRAY,
      -1,
    },
  },
  /* Deskjet 600/600C */
  { 600,
    17 * 72 / 2, 14 * 72,
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_300 | PCL_RES_600_600_MONO,
    0, 33, 18, 18,
    PCL_COLOR_CMY,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_HAGAKI_CARD,
      PCL_PAPERSIZE_A6_CARD,
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C6_ENV,
      PCL_PAPERSIZE_INVITATION_ENV,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    { -1,			/* No selectable paper sources */
    },
  },
  /* Deskjet 6xx series, plus 810/812/842/895 */
  { 601,
    17 * 72 / 2, 14 * 72,
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_300 | PCL_RES_600_600_MONO,
    0, 33, 18, 18,
    PCL_COLOR_CMYK,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_HAGAKI_CARD,
      PCL_PAPERSIZE_A6_CARD,
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C6_ENV,
      PCL_PAPERSIZE_INVITATION_ENV,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    {
      -1,
    },
  },
  /* Deskjet 69x series (Photo Capable) */
  { 690,
    17 * 72 / 2, 14 * 72,
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_300 | PCL_RES_600_600,
    0, 33, 18, 18,
    PCL_COLOR_CMYK | PCL_COLOR_CMYKcm,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_HAGAKI_CARD,
      PCL_PAPERSIZE_A6_CARD,
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C6_ENV,
      PCL_PAPERSIZE_INVITATION_ENV,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    {
      -1,
    },
  },
  /* Deskjet 850/855/870/890 (C-RET) */
  { 800,
    17 * 72 / 2, 14 * 72,
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600_MONO,
    3, 33, 18, 18,
    PCL_COLOR_CMYK | PCL_COLOR_CMYK4,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_HAGAKI_CARD,
      PCL_PAPERSIZE_A6_CARD,
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C6_ENV,
      PCL_PAPERSIZE_INVITATION_ENV,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    {
      -1,
    },
  },
  /* Deskjet 840 (C-RET) */
  { 840,
    17 * 72 / 2, 14 * 72,
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_300 | PCL_RES_600_600,
    0, 33, 18, 18,
    PCL_COLOR_CMYK | PCL_COLOR_CMYK4b,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_HAGAKI_CARD,
      PCL_PAPERSIZE_A6_CARD,
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C6_ENV,
      PCL_PAPERSIZE_INVITATION_ENV,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    {
      -1,
    },
  },
  /* Deskjet 900 series, 1220C, PhotoSmart P1000/P1100 */
  { 900,
    17 * 72 / 2, 14 * 72,
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600 /* | PCL_RES_1200_600 | PCL_RES_2400_600 */,
    3, 33, 18, 18,
    PCL_COLOR_CMYK,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_HAGAKI_CARD,
      PCL_PAPERSIZE_A6_CARD,
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C6_ENV,
      PCL_PAPERSIZE_INVITATION_ENV,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    { -1,			/* No selectable paper sources */
    },
  },
  /* Deskjet 1220C (or other large format 900) */
  { 901,
    13 * 72, 19 * 72,
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600 /* | PCL_RES_1200_600 | PCL_RES_2400_600 */,
    3, 33, 18, 18,
    PCL_COLOR_CMYK,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_TABLOID,
      PCL_PAPERSIZE_STATEMENT,
      PCL_PAPERSIZE_SUPER_B,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_A3,
      PCL_PAPERSIZE_JIS_B5,
      PCL_PAPERSIZE_JIS_B4,
      PCL_PAPERSIZE_HAGAKI_CARD,
      PCL_PAPERSIZE_OUFUKU_CARD,
      PCL_PAPERSIZE_A6_CARD,
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      PCL_PAPERSIZE_3x5,
      PCL_PAPERSIZE_HP_CARD,
      PCL_PAPERSIZE_MONARCH_ENV,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C5_ENV,
      PCL_PAPERSIZE_C6_ENV,
      PCL_PAPERSIZE_INVITATION_ENV,
      PCL_PAPERSIZE_JAPANESE_3_ENV,
      PCL_PAPERSIZE_JAPANESE_4_ENV,
      PCL_PAPERSIZE_KAKU_ENV,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    { -1,			/* No selectable paper sources */
    },
  },
  /* Deskjet 1100C, 1120C */
  { 1100,
    13 * 72, 19 * 72,
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600_MONO,
    3, 33, 18, 18,
    PCL_COLOR_CMYK | PCL_COLOR_CMYK4,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_TABLOID,
      PCL_PAPERSIZE_STATEMENT,
      PCL_PAPERSIZE_SUPER_B,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_A3,
      PCL_PAPERSIZE_JIS_B5,
      PCL_PAPERSIZE_JIS_B4,
      PCL_PAPERSIZE_HAGAKI_CARD,
      PCL_PAPERSIZE_A6_CARD,
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C6_ENV,
      PCL_PAPERSIZE_INVITATION_ENV,
      PCL_PAPERSIZE_JAPANESE_3_ENV,
      PCL_PAPERSIZE_JAPANESE_4_ENV,
      PCL_PAPERSIZE_KAKU_ENV,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_DJ_TRAY,
      -1,
    },
  },
  /* Deskjet 1200C */
  { 1200,
    17 * 72 / 2, 14 * 72,
    PCL_RES_150_150 | PCL_RES_300_300,
    12, 12, 18, 18,
    PCL_COLOR_CMY,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_BLANKLINE,
    {
/* This printer is not mentioned in the Comparison tables,
   so I'll just pick some likely sizes... */
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_DJ_TRAY,
      -1,
    },
  },
  /* Deskjet 1600C */
  { 1600,
    17 * 72 / 2, 14 * 72,
    PCL_RES_150_150 | PCL_RES_300_300,
    12, 12, 18, 18,
    PCL_COLOR_CMYK,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_BLANKLINE,
    {
/* This printer is not mentioned in the Comparison tables,
   so I'll just pick some likely sizes... */
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_DJ_TRAY,
      -1,
    },
  },
  /* Deskjet 2000 */
  { 2000,
    17 * 72 / 2, 14 * 72,
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600,
    12, 12, 18, 18,
    PCL_COLOR_CMYK,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_HAGAKI_CARD,
      PCL_PAPERSIZE_A6_CARD,
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      PCL_PAPERSIZE_3x5,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C6_ENV,
      PCL_PAPERSIZE_INVITATION_ENV,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      PCL_PAPERTYPE_QPHOTO,
      PCL_PAPERTYPE_QTRANS,
      -1,
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_DJ_TRAY,
      -1,
    },
  },
  /* Deskjet 2500 */
  { 2500,
    13 * 72, 19 * 72,
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600,
    12, 12, 18, 18,
    PCL_COLOR_CMYK,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_TABLOID,
      PCL_PAPERSIZE_STATEMENT,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_A3,
      PCL_PAPERSIZE_JIS_B5,
      PCL_PAPERSIZE_JIS_B4,
      PCL_PAPERSIZE_HAGAKI_CARD,
      PCL_PAPERSIZE_A6_CARD,
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      PCL_PAPERTYPE_QPHOTO,
      PCL_PAPERTYPE_QTRANS,
      -1,
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_DJ_AUTO,
      PCL_PAPERSOURCE_DJ_TRAY,
      PCL_PAPERSOURCE_DJ_TRAY2,
      PCL_PAPERSOURCE_DJ_OPTIONAL,
      -1,
    },
  },
  /* LaserJet II series */
  { 2,
    17 * 72 / 2, 14 * 72,
    PCL_RES_150_150 | PCL_RES_300_300,
    12, 12, 18, 18,
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_STATEMENT,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_MONARCH_ENV,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C5_ENV,
      PCL_PAPERSIZE_C6_ENV,
      -1,
    },
    { -1,			/* No selectable paper types */
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_LJ_TRAY1,
      PCL_PAPERSOURCE_LJ_TRAY2,
      PCL_PAPERSOURCE_LJ_TRAY3,
      PCL_PAPERSOURCE_LJ_TRAY4,
      -1,
    },
  },
  /* LaserJet III series */
  { 3,
    17 * 72 / 2, 14 * 72,
    PCL_RES_150_150 | PCL_RES_300_300,
    12, 12, 18, 18,
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_STATEMENT,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_MONARCH_ENV,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C5_ENV,
      PCL_PAPERSIZE_C6_ENV,
      -1,
    },
    { -1,			/* No selectable paper types */
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_LJ_TRAY1,
      PCL_PAPERSOURCE_LJ_TRAY2,
      PCL_PAPERSOURCE_LJ_TRAY3,
      PCL_PAPERSOURCE_LJ_TRAY4,
      -1,
    },
  },
  /* LaserJet 4L */
  { 4,
    17 * 72 / 2, 14 * 72,
    PCL_RES_150_150 | PCL_RES_300_300,
    12, 12, 18, 18,
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_STATEMENT,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_MONARCH_ENV,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C5_ENV,
      PCL_PAPERSIZE_C6_ENV,
      -1,
    },
    { -1,			/* No selectable paper types */
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_LJ_TRAY1,
      PCL_PAPERSOURCE_LJ_TRAY2,
      PCL_PAPERSOURCE_LJ_TRAY3,
      PCL_PAPERSOURCE_LJ_TRAY4,
      -1,
    },
  },
  /* LaserJet 4V, 4Si, 5Si */
  { 5,
    13 * 72, 19 * 72,
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600,
    12, 12, 18, 18,
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_STATEMENT,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_TABLOID,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_A3,
      PCL_PAPERSIZE_JIS_B5,
      PCL_PAPERSIZE_JIS_B4,		/* Guess */
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      PCL_PAPERSIZE_MONARCH_ENV,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C5_ENV,
      PCL_PAPERSIZE_C6_ENV,
      -1,
    },
    { -1,			/* No selectable paper types */
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_LJ_TRAY1,
      PCL_PAPERSOURCE_LJ_TRAY2,
      PCL_PAPERSOURCE_LJ_TRAY3,
      PCL_PAPERSOURCE_LJ_TRAY4,
      -1,
    },
  },
  /* LaserJet 4 series (except as above), 5 series, 6 series */
  { 6,
    17 * 72 / 2, 14 * 72,
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600,
    12, 12, 18, 18,
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_STATEMENT,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_MONARCH_ENV,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C5_ENV,
      PCL_PAPERSIZE_C6_ENV,
      -1,
    },
    { -1,			/* No selectable paper types */
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_LJ_TRAY1,
      PCL_PAPERSOURCE_LJ_TRAY2,
      PCL_PAPERSOURCE_LJ_TRAY3,
      PCL_PAPERSOURCE_LJ_TRAY4,
      -1,
    },
  },
  /* LaserJet 5Si */
  { 7,
    13 * 72, 19 * 72,
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600,
    12, 12, 18, 18,
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_STATEMENT,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_TABLOID,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_A3,
      PCL_PAPERSIZE_JIS_B5,
      PCL_PAPERSIZE_JIS_B4,		/* Guess */
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      PCL_PAPERSIZE_MONARCH_ENV,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C5_ENV,
      PCL_PAPERSIZE_C6_ENV,
      -1,
    },
    { -1,			/* No selectable paper types */
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_LJ_TRAY1,
      PCL_PAPERSOURCE_LJ_TRAY2,
      PCL_PAPERSOURCE_LJ_TRAY3,
      PCL_PAPERSOURCE_LJ_TRAY4,
      -1,
    },
  },
};


static const double sat_adjustment[49] =
{
  1.0,				/* C */
  1.1,
  1.2,
  1.3,
  1.4,
  1.5,
  1.6,
  1.7,
  1.8,				/* B */
  1.9,
  1.9,
  1.9,
  1.7,
  1.5,
  1.3,
  1.1,
  1.0,				/* M */
  1.0,
  1.0,
  1.0,
  1.0,
  1.0,
  1.0,
  1.0,
  1.0,				/* R */
  1.0,
  1.0,
  1.0,
  1.0,
  1.0,
  1.0,
  1.0,
  1.0,				/* Y */
  1.0,
  1.0,
  1.1,
  1.2,
  1.3,
  1.4,
  1.5,
  1.5,				/* G */
  1.4,
  1.3,
  1.2,
  1.1,
  1.0,
  1.0,
  1.0,
  1.0				/* C */
};

static const double lum_adjustment[49] =
{
  0.50,				/* C */
  0.6,
  0.7,
  0.8,
  0.9,
  0.86,
  0.82,
  0.79,
  0.78,				/* B */
  0.8,
  0.83,
  0.87,
  0.9,
  0.95,
  1.05,
  1.15,
  1.3,				/* M */
  1.25,
  1.2,
  1.15,
  1.12,
  1.09,
  1.06,
  1.03,
  1.0,				/* R */
  1.0,
  1.0,
  1.0,
  1.0,
  1.0,
  1.0,
  1.0,
  1.0,				/* Y */
  0.9,
  0.8,
  0.7,
  0.65,
  0.6,
  0.55,
  0.52,
  0.48,				/* G */
  0.47,
  0.47,
  0.49,
  0.49,
  0.49,
  0.52,
  0.51,
  0.50				/* C */
};

static const double hue_adjustment[49] =
{
  0.00,				/* C */
  0.05,
  0.04,
  0.01,
  -0.03,
  -0.10,
  -0.18,
  -0.26,
  -0.35,			/* B */
  -0.43,
  -0.40,
  -0.32,
  -0.25,
  -0.18,
  -0.10,
  -0.07,
  0.00,				/* M */
  -0.04,
  -0.09,
  -0.13,
  -0.18,
  -0.23,
  -0.27,
  -0.31,
  -0.35,			/* R */
  -0.38,
  -0.30,
  -0.23,
  -0.15,
  -0.08,
  0.00,
  -0.02,
  0.00,				/* Y */
  0.08,
  0.10,
  0.08,
  0.05,
  0.03,
  -0.03,
  -0.12,
  -0.20,			/* G */
  -0.17,
  -0.20,
  -0.17,
  -0.15,
  -0.12,
  -0.10,
  -0.08,
  0.00,				/* C */
};

/*
 * Convert a name into it's option value
 */

static int pcl_string_to_val(const char *string,		/* I: String */
                           const pcl_t *options,	/* I: Options */
			   int num_options)		/* I: Num options */
{

  int i;
  int code = -1;

 /*
  * Look up the string in the table and convert to the code.
  */

  for (i=0; i<num_options; i++) {
    if (!strcmp(string, _(options[i].pcl_name))) {
       code=options[i].pcl_code;
       break;
       }
  }

#ifdef DEBUG
  stp_erprintf( "String: %s, Code: %d\n", string, code);
#endif

  return(code);
}

/*
 * Convert a value into it's option name
 */

static const char * pcl_val_to_string(int code,			/* I: Code */
                           const pcl_t *options,	/* I: Options */
			   int num_options)		/* I: Num options */
{

  int i;
  const char *string = NULL;

 /*
  * Look up the code in the table and convert to the string.
  */

  for (i=0; i<num_options; i++) {
    if (code == options[i].pcl_code) {
       string=options[i].pcl_name;
       break;
       }
  }

#ifdef DEBUG
  stp_erprintf( "Code: %d, String: %s\n", code, string);
#endif

  return(string);
}

static const double dot_sizes[] = { 0.5, 0.832, 1.0 };
static const stp_simple_dither_range_t variable_dither_ranges[] =
{
  { 0.152, 0x1, 0 },
  { 0.255, 0x2, 0 },
  { 0.38,  0x3, 0 },
  { 0.5,   0x1, 1 },
  { 0.67,  0x2, 1 },
  { 1.0,   0x3, 1 }
};

/*
 * pcl_get_model_capabilities() - Return struct of model capabilities
 */

static const pcl_cap_t *				/* O: Capabilities */
pcl_get_model_capabilities(int model)	/* I: Model */
{
  int i;
  int models= sizeof(pcl_model_capabilities) / sizeof(pcl_cap_t);
  for (i=0; i<models; i++) {
    if (pcl_model_capabilities[i].model == model) {
      return &(pcl_model_capabilities[i]);
    }
  }
  stp_erprintf("pcl: model %d not found in capabilities list.\n",model);
  return &(pcl_model_capabilities[0]);
}

static char *
c_strdup(const char *s)
{
  char *ret = stp_malloc(strlen(s) + 1);
  strcpy(ret, s);
  return ret;
}

/*
 * Convert Media size name into PCL media code for printer
 */

static int pcl_convert_media_size(const char *media_size,	/* I: Media size string */
				  int  model)		/* I: model number */
{

  int i;
  int media_code = 0;
  const pcl_cap_t *caps;

 /*
  * First look up the media size in the table and convert to the code.
  */

  media_code = pcl_string_to_val(media_size, pcl_media_sizes,
                                 sizeof(pcl_media_sizes) / sizeof(pcl_t));

#ifdef DEBUG
  stp_erprintf( "Media Size: %s, Code: %d\n", media_size, media_code);
#endif

 /*
  * Now see if the printer supports the code found.
  */

  if (media_code != -1) {
    caps = pcl_get_model_capabilities(model);

    for (i=0; (i<NUM_PRINTER_PAPER_SIZES) && (caps->paper_sizes[i] != -1); i++) {
      if (media_code == caps->paper_sizes[i])
        return(media_code);		/* Is supported */
    }

#ifdef DEBUG
    stp_erprintf( "Media Code %d not supported by printer model %d.\n",
      media_code, model);
#endif
    return(-1);				/* Not supported */
  }
  else
    return(-1);				/* Not supported */
}

/*
 * 'pcl_parameters()' - Return the parameter values for the given parameter.
 */

static char **				/* O - Parameter values */
pcl_parameters(const stp_printer_t printer,/* I - Printer model */
               const char *ppd_file,		/* I - PPD file (not used) */
               const char *name,		/* I - Name of parameter */
               int  *count)		/* O - Number of values */
{
  int		model = stp_printer_get_model(printer);
  int		i;
  char		**valptrs;
  const pcl_cap_t *caps;

  static const char *ink_types[] =
  {
    N_ ("Color + Black Cartridges"),
    N_ ("Color + Photo Cartridges")
  };

  if (count == NULL)
    return (NULL);

  *count = 0;

  if (name == NULL)
    return (NULL);

  caps = pcl_get_model_capabilities(model);

#ifdef DEBUG
  stp_erprintf( "Printer model = %d\n", model);
  stp_erprintf( "PageWidth = %d, PageHeight = %d\n", caps->max_width, caps->max_height);
  stp_erprintf( "Margins: top = %d, bottom = %d, left = %d, right = %d\n",
    caps->top_margin, caps->bottom_margin, caps->left_margin, caps->right_margin);
  stp_erprintf( "Resolutions: %d\n", caps->resolutions);
  stp_erprintf( "ColorType = %d, PrinterType = %d\n", caps->color_type, caps->stp_printer_type);
#endif

  if (strcmp(name, "PageSize") == 0)
    {
      unsigned height_limit = caps->max_height;
      unsigned width_limit = caps->max_width;
      int papersizes = stp_known_papersizes();
#ifdef PCL_NO_CUSTOM_PAPERSIZES
      int use_custom = 0;
#else
      int use_custom = ((caps->stp_printer_type & PCL_PRINTER_CUSTOM_SIZE)
                         == PCL_PRINTER_CUSTOM_SIZE);
#endif
      valptrs = stp_malloc(sizeof(char *) * papersizes);
      *count = 0;
      for (i = 0; i < papersizes; i++)
	{
	  const stp_papersize_t pt = stp_get_papersize_by_index(i);
	  if (strlen(stp_papersize_get_name(pt)) > 0 &&
	      stp_papersize_get_width(pt) <= width_limit &&
	      stp_papersize_get_height(pt) <= height_limit &&
              ((use_custom == 1) ||
	       ((use_custom == 0) &&
		(pcl_convert_media_size(stp_papersize_get_name(pt), model)
		 != -1))))
	    {
	      valptrs[*count] = stp_malloc(strlen(stp_papersize_get_name(pt)) +1);
	      strcpy(valptrs[*count], stp_papersize_get_name(pt));
	      (*count)++;
	    }
	}
      return (valptrs);
    }
  else if (strcmp(name, "MediaType") == 0)
  {
    if (caps->paper_types[0] == -1)
    {
      *count = 0;
      return (NULL);
    }
    else
    {
      valptrs = stp_malloc(sizeof(char *) * NUM_PRINTER_PAPER_TYPES);
      *count = 0;
      for (i=0; (i < NUM_PRINTER_PAPER_TYPES) && (caps->paper_types[i] != -1); i++) {
        valptrs[i] = c_strdup(pcl_val_to_string(caps->paper_types[i], pcl_media_types,
                                        NUM_PRINTER_PAPER_TYPES));
        (*count)++;
      }
      return(valptrs);
    }
  }
  else if (strcmp(name, "InputSlot") == 0)
  {
    if (caps->paper_sources[0] == -1)
    {
      *count = 0;
      return (NULL);
    }
    else
    {
      valptrs = stp_malloc(sizeof(char *) * NUM_PRINTER_PAPER_SOURCES);
      *count = 0;
      for (i=0; (i < NUM_PRINTER_PAPER_SOURCES) && (caps->paper_sources[i] != -1); i++) {
        valptrs[i] = c_strdup(pcl_val_to_string(caps->paper_sources[i], pcl_media_sources,
                                        NUM_PRINTER_PAPER_SOURCES));
        (*count)++;
      }
      return(valptrs);
    }
  }
  else if (strcmp(name, "Resolution") == 0)
  {
    *count = 0;
    valptrs = stp_malloc(sizeof(char *) * NUM_RESOLUTIONS);
    for (i = 0; i < NUM_RESOLUTIONS; i++)
    {
      if (caps->resolutions & pcl_resolutions[i].pcl_code)
      {
         valptrs[*count] = c_strdup(pcl_val_to_string(pcl_resolutions[i].pcl_code,
					pcl_resolutions, NUM_RESOLUTIONS));
         (*count)++;
      }
    }
    return(valptrs);
  }
  else if (strcmp(name, "InkType") == 0)
  {
    if (caps->color_type & PCL_COLOR_CMYKcm)
    {
      valptrs = stp_malloc(sizeof(char *) * 2);
      valptrs[0] = c_strdup(ink_types[0]);
      valptrs[1] = c_strdup(ink_types[1]);
      *count = 2;
      return(valptrs);
    }
    else
      return(NULL);
  }
  else
    return (NULL);
}

static const char *
pcl_default_parameters(const stp_printer_t printer,
		       const char *ppd_file,
		       const char *name)
{
  int		model = stp_printer_get_model(printer);
  int		i;
  const pcl_cap_t *caps;

  static const char *ink_types[] =
  {
    N_ ("Color + Black Cartridges"),
    N_ ("Color + Photo Cartridges")
  };

  if (name == NULL)
    return (NULL);

  caps = pcl_get_model_capabilities(model);

#ifdef DEBUG
  stp_erprintf( "Printer model = %d\n", model);
  stp_erprintf( "PageWidth = %d, PageHeight = %d\n", caps->max_width, caps->max_height);
  stp_erprintf( "Margins: top = %d, bottom = %d, left = %d, right = %d\n",
	  caps->top_margin, caps->bottom_margin, caps->left_margin, caps->right_margin);
  stp_erprintf( "Resolutions: %d\n", caps->resolutions);
  stp_erprintf( "ColorType = %d, PrinterType = %d\n", caps->color_type, caps->stp_printer_type);
#endif

  if (strcmp(name, "PageSize") == 0)
    {
      unsigned height_limit = caps->max_height;
      unsigned width_limit = caps->max_width;
      int papersizes = stp_known_papersizes();
#ifdef PCL_NO_CUSTOM_PAPERSIZES
      int use_custom = 0;
#else
      int use_custom = ((caps->stp_printer_type & PCL_PRINTER_CUSTOM_SIZE)
			== PCL_PRINTER_CUSTOM_SIZE);
#endif
      for (i = 0; i < papersizes; i++)
	{
	  const stp_papersize_t pt = stp_get_papersize_by_index(i);
	  if (strlen(stp_papersize_get_name(pt)) > 0 &&
	      stp_papersize_get_width(pt) <= width_limit &&
	      stp_papersize_get_height(pt) <= height_limit &&
              ((use_custom == 1) ||
	       ((use_custom == 0) &&
		(pcl_convert_media_size(stp_papersize_get_name(pt), model)
		 != -1))))
	    {
	      return _(stp_papersize_get_name(pt));
	    }
	}
      return NULL;
    }
  else if (strcmp(name, "MediaType") == 0)
    {
      if (caps->paper_types[0] == -1)
	{
	  return (NULL);
	}
      else
	{
	  return _(pcl_val_to_string(caps->paper_types[0], pcl_media_types,
				     NUM_PRINTER_PAPER_TYPES));
	}
    }
  else if (strcmp(name, "InputSlot") == 0)
    {
      if (caps->paper_sources[0] == -1)
	{
	  return (NULL);
	}
      else
	{
	  return _(pcl_val_to_string(caps->paper_sources[0], pcl_media_sources,
				     NUM_PRINTER_PAPER_SOURCES));
	}
    }
  else if (strcmp(name, "Resolution") == 0)
    {
      for (i = 0; i < NUM_RESOLUTIONS; i++)
	{
	  if ((caps->resolutions & pcl_resolutions[i].pcl_code) &&
	      (pcl_resolutions[i].pcl_code >= PCL_RES_300_300))
	    {
	      return _(pcl_val_to_string(pcl_resolutions[i].pcl_code,
					 pcl_resolutions, NUM_RESOLUTIONS));
	    }
	}
      /* If printer can't handle at least 300x300, look for anything that */
      /* works */
      for (i = 0; i < NUM_RESOLUTIONS; i++)
	{
	  if ((caps->resolutions & pcl_resolutions[i].pcl_code))
	    {
	      return _(pcl_val_to_string(pcl_resolutions[i].pcl_code,
					 pcl_resolutions, NUM_RESOLUTIONS));
	    }
	}
      return NULL;
    }
  else if (strcmp(name, "InkType") == 0)
    {
      if (caps->color_type & PCL_COLOR_CMYKcm)
	{
	  return _(ink_types[0]);
	}
      else
	return(NULL);
    }
  else
    return (NULL);
}


/*
 * 'pcl_imageable_area()' - Return the imageable area of the page.
 */

static void
pcl_imageable_area(const stp_printer_t printer,	/* I - Printer model */
		   const stp_vars_t v,     /* I */
                   int  *left,		/* O - Left position in points */
                   int  *right,		/* O - Right position in points */
                   int  *bottom,	/* O - Bottom position in points */
                   int  *top)		/* O - Top position in points */
{
  int	width, height;			/* Size of page */
  const pcl_cap_t *caps;			/* Printer caps */

  caps = pcl_get_model_capabilities(stp_printer_get_model(printer));

  stp_default_media_size(printer, v, &width, &height);

/*
 * Note: The margins actually vary with paper size, but since you can
 * move the image around on the page anyway, it hardly matters.
 */

  *left   = caps->left_margin;
  *right  = width - caps->right_margin;
  *top    = height - caps->top_margin;
  *bottom = caps->bottom_margin;
}

static void
pcl_limit(const stp_printer_t printer,	/* I - Printer model */
	  const stp_vars_t v,  		/* I */
	  int  *width,			/* O - Left position in points */
	  int  *height)			/* O - Top position in points */
{
  const pcl_cap_t *caps= pcl_get_model_capabilities(stp_printer_get_model(printer));
  *width =	caps->max_width;
  *height =	caps->max_height;
}

/*
 * 'pcl_print()' - Print an image to an HP printer.
 */

static void
pcl_print(const stp_printer_t printer,		/* I - Model */
          stp_image_t *image,		/* I - Image to print */
	  const stp_vars_t v)
{
  int i;
  unsigned char *cmap = stp_get_cmap(v);
  int		model = stp_printer_get_model(printer);
  const char	*resolution = stp_get_resolution(v);
  const char	*media_size;
  const char	*media_type = stp_get_media_type(v);
  const char	*media_source = stp_get_media_source(v);
  const char	*ink_type = stp_get_ink_type(v);
  int 		output_type = stp_get_output_type(v);
  int		orientation = stp_get_orientation(v);
  double 	scaling = stp_get_scaling(v);
  int		top = stp_get_top(v);
  int		left = stp_get_left(v);
  int		y;		/* Looping vars */
  int		xdpi, ydpi;	/* Resolution */
  unsigned short *out;
  unsigned char	*in,		/* Input pixels */
		*black,		/* Black bitmap data */
		*cyan,		/* Cyan bitmap data */
		*magenta,	/* Magenta bitmap data */
		*yellow,	/* Yellow bitmap data */
		*lcyan,		/* Light Cyan bitmap data */
		*lmagenta;	/* Light Magenta bitmap data */
  int		page_left,	/* Left margin of page */
		page_right,	/* Right margin of page */
		page_top,	/* Top of page */
		page_bottom,	/* Bottom of page */
		page_width,	/* Width of page */
		page_height,	/* Height of page */
		out_width,	/* Width of image on page */
		out_height,	/* Height of image on page */
		out_bpp,	/* Output bytes per pixel */
		height,		/* Height of raster data */
		errdiv,		/* Error dividend */
		errmod,		/* Error modulus */
		errval,		/* Current error value */
		errline,	/* Current raster line */
		errlast;	/* Last raster line loaded */
  stp_convert_t	colorfunc;	/* Color conversion function... */
  int		zero_mask;
  void		(*writefunc)(const stp_vars_t, unsigned char *, int, int);
				/* PCL output function */
  int           image_height,
                image_width,
                image_bpp;
  void *	dither;
  const pcl_cap_t *caps;		/* Printer capabilities */
  int		do_cret,	/* 300 DPI CRet printing */
  		do_cretb,	/* 600 DPI CRet printing HP 840C*/
		do_6color,	/* CMY + cmK printing */
		planes;		/* # of output planes */
  int		pcl_media_size, /* PCL media size code */
		pcl_media_type, /* PCL media type code */
		pcl_media_source;	/* PCL media source code */
  const double *dot_sizes_use,dot_sizes_cret[]={1.0,1.0,1.0};         /* The dot size used */
  stp_vars_t	nv = stp_allocate_copy(v);
  stp_papersize_t pp;
  int		len_c,		/* Active length of Cyan buffers */
		len_lc,		/* Ditto Light Cyan */
		len_m,		/* Ditto Magenta */
		len_lm,		/* Ditto Light Magenta */
		len_y, 		/* Ditto Cyan */
		len_k;		/* Ditto Black */
  int		blank_lines,	/* Accumulated blank lines */
		is_blank,	/* Current line is blank */
		do_blank;	/* Blank line removal required */

  if (!stp_get_verified(nv))
    {
      stp_eprintf(nv, "Print options not verified; cannot print.\n");
      return;
    }

  caps = pcl_get_model_capabilities(model);

 /*
  * Setup a read-only pixel region for the entire image...
  */

  image->init(image);
  image_height = image->height(image);
  image_width = image->width(image);
  image_bpp = image->bpp(image);

 /*
  * Figure out the output resolution...
  */

  sscanf(resolution,"%dx%d",&xdpi,&ydpi);

#ifdef DEBUG
  stp_erprintf("pcl: resolution=%dx%d\n",xdpi,ydpi);
#endif

 /*
  * Choose the correct color conversion function...
  */
  if (((caps->resolutions & PCL_RES_600_600_MONO) == PCL_RES_600_600_MONO) &&
      output_type != OUTPUT_GRAY && output_type != OUTPUT_MONOCHROME &&
      xdpi == 600 && ydpi == 600)
    {
      stp_eprintf(v, "600x600 resolution only available in MONO\n");
      output_type = OUTPUT_GRAY;
      stp_set_output_type(nv, OUTPUT_GRAY);
    }

  if (caps->color_type == PCL_COLOR_NONE && output_type != OUTPUT_MONOCHROME)
    {
      output_type = OUTPUT_GRAY;
      stp_set_output_type(nv, OUTPUT_GRAY);
    }
  stp_set_output_color_model(nv, COLOR_MODEL_CMY);

  colorfunc = stp_choose_colorfunc(output_type, image_bpp, cmap, &out_bpp, nv);

  do_cret = (xdpi >= 300 &&
	     ((caps->color_type & PCL_COLOR_CMYK4) == PCL_COLOR_CMYK4) &&
	     output_type != OUTPUT_MONOCHROME);
  do_cretb = (xdpi >= 600 && ydpi >= 600 &&
	      ((caps->color_type & PCL_COLOR_CMYK4b) == PCL_COLOR_CMYK4b) &&
	      output_type != OUTPUT_MONOCHROME &&
	      output_type != OUTPUT_GRAY);
  if (do_cretb){
    do_cret = 1;
    dot_sizes_use=dot_sizes_cret;
  }else{
    dot_sizes_use=dot_sizes;
  }

#ifdef DEBUG
  stp_erprintf( "do_cret = %d\n", do_cret);
  stp_erprintf( "do_cretb = %d\n", do_cretb);
#endif

  do_6color = (strcmp(ink_type, _("Color + Photo Cartridges")) == 0);
#ifdef DEBUG
  stp_erprintf( "do_6color = %d\n", do_6color);
#endif

 /*
  * Compute the output size...
  */

  pcl_imageable_area(printer, nv, &page_left, &page_right,
                     &page_bottom, &page_top);
#ifdef DEBUG
  stp_erprintf("Before stp_compute_page_parameters()\n");
  stp_erprintf("page_left = %d, page_right = %d, page_top = %d, page_bottom = %d\n",
    page_left, page_right, page_top, page_bottom);
  stp_erprintf("top = %d, left = %d\n", top, left);
  stp_erprintf("scaling = %f, image_width = %d, image_height = %d\n", scaling,
    image_width, image_height);
#endif

  stp_compute_page_parameters(page_right, page_left, page_top, page_bottom,
			  scaling, image_width, image_height, image,
			  &orientation, &page_width, &page_height,
			  &out_width, &out_height, &left, &top);

  /*
   * Recompute the image height and width.  If the image has been
   * rotated, these will change from previously.
   */
  image_height = image->height(image);
  image_width = image->width(image);

#ifdef DEBUG
  stp_erprintf("After stp_compute_page_parameters()\n");
  stp_erprintf("page_width = %d, page_height = %d\n", page_width, page_height);
  stp_erprintf("out_width = %d, out_height = %d\n", out_width, out_height);
  stp_erprintf("top = %d, left = %d\n", top, left);
#endif /* DEBUG */

 /*
  * Let the user know what we're doing...
  */

  image->progress_init(image);

 /*
  * Send PCL initialization commands...
  */

  if (do_cretb)
    {
      stp_puts("\033*rbC", v);	/* End raster graphics */
    }
  stp_puts("\033E", v); 				/* PCL reset */
  if (do_cretb)
    {
      stp_zprintf(v, "\033%%-12345X@PJL ENTER LANGUAGE=PCL3GUI\n");
    }

 /*
  * Set media size
  */

  if (strlen(stp_get_media_size(v)) > 0)
    media_size = stp_get_media_size(v);
  else if ((pp = stp_get_papersize_by_size(stp_get_page_height(v),
					   stp_get_page_width(v))) != NULL)
    media_size = stp_papersize_get_name(pp);
  else
    media_size = "";

  pcl_media_size = pcl_convert_media_size(media_size, model);

#ifdef DEBUG
  stp_erprintf("pcl_media_size = %d, media_size = %s\n", pcl_media_size, media_size);
#endif

 /*
  * If the media size requested is unknown, try it as a custom size.
  *
  * Warning: The margins may need to be fixed for this!
  */

  if (pcl_media_size == -1) {
    stp_erprintf( "Paper size %s is not directly supported by printer.\n",
      media_size);
    stp_erprintf( "Trying as custom pagesize (watch the margins!)\n");
    pcl_media_size = PCL_PAPERSIZE_CUSTOM;			/* Custom */
  }

  stp_zprintf(v, "\033&l%dA", pcl_media_size);

  stp_puts("\033&l0L", v);			/* Turn off perforation skip */
  stp_puts("\033&l0E", v);			/* Reset top margin to 0 */

 /*
  * Convert media source string to the code, if specified.
  */

  if (strlen(media_source) != 0) {
    pcl_media_source = pcl_string_to_val(media_source, pcl_media_sources,
                         sizeof(pcl_media_sources) / sizeof(pcl_t));

#ifdef DEBUG
    stp_erprintf("pcl_media_source = %d, media_source = %s\n", pcl_media_source,
           media_source);
#endif

    if (pcl_media_source == -1)
      stp_erprintf( "Unknown media source %s, ignored.\n", media_source);
    else if (pcl_media_source != PCL_PAPERSOURCE_STANDARD) {

/* Correct the value by taking the modulus */

      pcl_media_source = pcl_media_source % PAPERSOURCE_MOD;
      stp_zprintf(v, "\033&l%dH", pcl_media_source);
    }
  }

 /*
  * Convert media type string to the code, if specified.
  */

  if (strlen(media_type) != 0) {
    pcl_media_type = pcl_string_to_val(media_type, pcl_media_types,
                       sizeof(pcl_media_types) / sizeof(pcl_t));

#ifdef DEBUG
    stp_erprintf("pcl_media_type = %d, media_type = %s\n", pcl_media_type,
           media_type);
#endif

    if (pcl_media_type == -1) {
      stp_erprintf( "Unknown media type %s, set to PLAIN.\n", media_type);
      pcl_media_type = PCL_PAPERTYPE_PLAIN;
    }
  }
  else
    pcl_media_type = PCL_PAPERTYPE_PLAIN;

 /*
  * Set DJ print quality to "best" if resolution >= 300
  */

  if ((xdpi >= 300) && ((caps->stp_printer_type & PCL_PRINTER_DJ) == PCL_PRINTER_DJ))
  {
    if ((caps->stp_printer_type & PCL_PRINTER_MEDIATYPE) == PCL_PRINTER_MEDIATYPE)
    {
      stp_puts("\033*o1M", v);			/* Quality = presentation */
      stp_zprintf(v, "\033&l%dM", pcl_media_type);
    }
    else
    {
      stp_puts("\033*r2Q", v);			/* Quality (high) */
      stp_puts("\033*o2Q", v);			/* Shingling (4 passes) */

 /* Depletion depends on media type and cart type. */

      if ((pcl_media_type == PCL_PAPERTYPE_PLAIN)
	|| (pcl_media_type == PCL_PAPERTYPE_BOND)) {
      if ((caps->color_type & PCL_COLOR_CMY) == PCL_COLOR_CMY)
          stp_puts("\033*o2D", v);			/* Depletion 25% */
        else
          stp_puts("\033*o5D", v);			/* Depletion 50% with gamma correction */
      }

      else if ((pcl_media_type == PCL_PAPERTYPE_PREMIUM)
             || (pcl_media_type == PCL_PAPERTYPE_GLOSSY)
             || (pcl_media_type == PCL_PAPERTYPE_TRANS))
        stp_puts("\033*o1D", v);			/* Depletion none */
    }
  }

  if ((xdpi != ydpi) || (do_cret) || (do_6color))
						/* Set resolution using CRD */
  {

   /*
    * Send configure image data command with horizontal and
    * vertical resolutions as well as a color count...
    */

    if (output_type != OUTPUT_GRAY && output_type != OUTPUT_MONOCHROME)
      if ((caps->color_type & PCL_COLOR_CMY) == PCL_COLOR_CMY)
        planes = 3;
      else
        if (do_6color)
          planes = 6;
        else
          planes = 4;
    else
      planes = 1;

    stp_zprintf(v, "\033*g%dW", 2 + (planes * 6));
    stp_putc(2, v);				/* Format 2 (Complex Direct Planar) */
    stp_putc(planes, v);				/* # output planes */

    if (planes != 3) {
      stp_putc(xdpi >> 8, v);			/* Black resolution */
      stp_putc(xdpi, v);
      stp_putc(ydpi >> 8, v);
      stp_putc(ydpi, v);
      stp_putc(0, v);
      if (do_cretb){
	stp_putc(2, v);
      }else{
	stp_putc(do_cret ? 4 : 2, v);
      }
    }

    if (planes != 1) {
      stp_putc(xdpi >> 8, v);			/* Cyan resolution */
      stp_putc(xdpi, v);
      stp_putc(ydpi >> 8, v);
      stp_putc(ydpi, v);
      stp_putc(0, v);
      stp_putc(do_cret ? 4 : 2, v);

      stp_putc(xdpi >> 8, v);			/* Magenta resolution */
      stp_putc(xdpi, v);
      stp_putc(ydpi >> 8, v);
      stp_putc(ydpi, v);
      stp_putc(0, v);
      stp_putc(do_cret ? 4 : 2, v);

      stp_putc(xdpi >> 8, v);			/* Yellow resolution */
      stp_putc(xdpi, v);
      stp_putc(ydpi >> 8, v);
      stp_putc(ydpi, v);
      stp_putc(0, v);
      stp_putc(do_cret ? 4 : 2, v);
    }
    if (planes == 6)
    {
      stp_putc(xdpi >> 8, v);			/* Light Cyan resolution */
      stp_putc(xdpi, v);
      stp_putc(ydpi >> 8, v);
      stp_putc(ydpi, v);
      stp_putc(0, v);
      stp_putc(do_cret ? 4 : 2, v);

      stp_putc(xdpi >> 8, v);			/* Light Magenta resolution */
      stp_putc(xdpi, v);
      stp_putc(ydpi >> 8, v);
      stp_putc(ydpi, v);
      stp_putc(0, v);
      stp_putc(do_cret ? 4 : 2, v);
    }
  }
  else
  {
    stp_zprintf(v, "\033*t%dR", xdpi);		/* Simple resolution */
    if (output_type != OUTPUT_GRAY && output_type != OUTPUT_MONOCHROME)
    {
      if ((caps->color_type & PCL_COLOR_CMY) == PCL_COLOR_CMY)
        stp_puts("\033*r-3U", v);		/* Simple CMY color */
      else
        stp_puts("\033*r-4U", v);		/* Simple KCMY color */
    }
  }

#ifndef PCL_DEBUG_DISABLE_COMPRESSION
  if ((caps->stp_printer_type & PCL_PRINTER_TIFF) == PCL_PRINTER_TIFF)
  {
    stp_puts("\033*b2M", v);			/* Mode 2 (TIFF) */
    writefunc = pcl_mode2;
  }
  else
#endif
  {
    stp_puts("\033*b0M", v);			/* Mode 0 (no compression) */
    writefunc = pcl_mode0;
  }

 /*
  * Convert image size to printer resolution and setup the page for printing...
  */

  out_width  = xdpi * out_width / 72;
  out_height = ydpi * out_height / 72;

#ifdef DEBUG
  stp_erprintf( "left %d margin %d top %d margin %d width %d height %d\n",
	  left, caps->left_margin, top, caps->top_margin, out_width, out_height);
#endif

  if (!do_cretb) {
    stp_zprintf(v, "\033&a%dH", 10 * left);		/* Set left raster position */
    stp_zprintf(v, "\033&a%dV", 10 * (top + caps->top_margin));
				/* Set top raster position */
  }
  stp_zprintf(v, "\033*r%dS", out_width);		/* Set raster width */
  stp_zprintf(v, "\033*r%dT", out_height);	/* Set raster height */

  if (do_cretb)
    {
      /* Move to top left of printed area */
      stp_zprintf(v, "\033*p%dY", (top + caps->top_margin)*4); /* Mesuret in dots. */
      stp_zprintf(v, "\033*p%dX", left*4);
    }
  stp_puts("\033*r1A", v); 			/* Start GFX */

 /*
  * Allocate memory for the raster data...
  */

  height = (out_width + 7) / 8;
  if (do_cret)
    height *= 2;

  if (output_type == OUTPUT_GRAY || output_type == OUTPUT_MONOCHROME)
  {
    black   = stp_malloc(height);
    cyan    = NULL;
    magenta = NULL;
    yellow  = NULL;
    lcyan    = NULL;
    lmagenta = NULL;
  }
  else
  {
    cyan    = stp_malloc(height);
    magenta = stp_malloc(height);
    yellow  = stp_malloc(height);

    if ((caps->color_type & PCL_COLOR_CMY) == PCL_COLOR_CMY)
      black = NULL;
    else
      black = stp_malloc(height);
    if (do_6color)
    {
      lcyan    = stp_malloc(height);
      lmagenta = stp_malloc(height);
    }
    else
    {
      lcyan    = NULL;
      lmagenta = NULL;
    }
  }

 /*
  * Output the page, rotating as necessary...
  */

  stp_compute_lut(nv, 256);

  if (xdpi > ydpi)
    dither = stp_init_dither(image_width, out_width, 1, xdpi / ydpi, nv);
  else
    dither = stp_init_dither(image_width, out_width, ydpi / xdpi, 1, nv);

/* Set up dithering for special printers. */

#if 1		/* Leave alone for now */
  for (i = 0; i <= NCOLORS; i++)
    stp_dither_set_black_level(dither, i, 1.2);
  stp_dither_set_black_lower(dither, .3);
  stp_dither_set_black_upper(dither, .999);
#endif

  if (do_cret)				/* 4-level printing for 800/1120 */
    {
      stp_dither_set_ranges_simple(dither, ECOLOR_Y, 3, dot_sizes_use, stp_get_density(nv));
      if (!do_cretb)
        stp_dither_set_ranges_simple(dither, ECOLOR_K, 3, dot_sizes_use, stp_get_density(nv));

/* Note: no printer I know of does both CRet (4-level) and 6 colour, but
   what the heck. variable_dither_ranges copied from print-escp2.c */

      if (do_6color)			/* Photo for 69x */
	{
	  stp_dither_set_ranges(dither, ECOLOR_C, 6, variable_dither_ranges,
			    stp_get_density(nv));
	  stp_dither_set_ranges(dither, ECOLOR_M, 6, variable_dither_ranges,
			    stp_get_density(nv));
	}
      else
	{
	  stp_dither_set_ranges_simple(dither, ECOLOR_C, 3, dot_sizes_use, stp_get_density(nv));
	  stp_dither_set_ranges_simple(dither, ECOLOR_M, 3, dot_sizes_use, stp_get_density(nv));
	}
    }
  else if (do_6color)
    {
/* Set light inks for 6 colour printers. Numbers copied from print-escp2.c */
      stp_dither_set_light_ink(dither, ECOLOR_C, .25, stp_get_density(nv));
      stp_dither_set_light_ink(dither, ECOLOR_M, .25, stp_get_density(nv));
    }

  switch (stp_get_image_type(nv))
    {
    case IMAGE_LINE_ART:
      stp_dither_set_ink_spread(dither, 19);
      break;
    case IMAGE_SOLID_TONE:
      stp_dither_set_ink_spread(dither, 15);
      break;
    case IMAGE_CONTINUOUS:
      stp_dither_set_ink_spread(dither, 14);
      break;
    }
  stp_dither_set_density(dither, stp_get_density(nv));

  in  = stp_malloc(image_width * image_bpp);
  out = stp_malloc(image_width * out_bpp * 2);

  errdiv  = image_height / out_height;
  errmod  = image_height % out_height;
  errval  = 0;
  errlast = -1;
  errline  = 0;
  blank_lines = 0;
  is_blank = 0;
#ifndef PCL_DEBUG_DISABLE_BLANKLINE_REMOVAL
  do_blank = ((caps->stp_printer_type & PCL_PRINTER_BLANKLINE) ==
		PCL_PRINTER_BLANKLINE);
#else
  do_blank = 0;
#endif

  for (y = 0; y < out_height; y ++)
  {
    int duplicate_line = 1;
#ifdef DEBUG
    stp_erprintf("pcl_print: y = %d, line = %d, val = %d, mod = %d, height = %d\n",
           y, errline, errval, errmod, out_height);
#endif /* DEBUG */
    if ((y & 63) == 0)
      image->note_progress(image, y, out_height);

    if (errline != errlast)
    {
      errlast = errline;
      duplicate_line = 0;
      if (image->get_row(image, in, errline) != STP_IMAGE_OK)
	break;
      (*colorfunc)(nv, in, out, &zero_mask, image_width, image_bpp, cmap,
		   hue_adjustment, lum_adjustment, NULL);
    }

    stp_dither(out, y, dither, cyan, lcyan, magenta, lmagenta,
		yellow, NULL, black, duplicate_line, zero_mask);

    len_c = stp_dither_get_last_position(dither, ECOLOR_C, 1);
    len_lc = stp_dither_get_last_position(dither, ECOLOR_C, 0);
    len_m = stp_dither_get_last_position(dither, ECOLOR_M, 1);
    len_lm = stp_dither_get_last_position(dither, ECOLOR_M, 0);
    len_y = stp_dither_get_last_position(dither, ECOLOR_Y, 1);
    len_k = stp_dither_get_last_position(dither, ECOLOR_K, 1);

/*
 * Blank line removal. If multiple lines are blank then they can be replaced 
 * by "Relative Vertical Pixel Movement" command. However, since there are
 * apparently some faulty implementations, we always output the first line,
 * then suppress the rest. This ensures that the printers's buffers are really
 * empty. As suggested by Mike Sweet.
 */

    is_blank = (do_blank && (len_c == -1) && (len_lc == -1) && (len_m == -1)
	&& (len_lm == -1) && (len_y == -1) && (len_k == -1));

    if (is_blank && (blank_lines != 0))	/* repeated blank line */
    {
      blank_lines++;
    }
    else				/* Not blank, or is first one */
    {
      if (! is_blank)
      {
        if (blank_lines > 1)		/* Output accumulated lines */
        {
	  blank_lines--;		/* correct for one already output */
#ifdef DEBUG
	  stp_erprintf( "Blank Lines = %d\n", blank_lines);
#endif
	  stp_zprintf(v, "\033*b%dY", blank_lines);
	  blank_lines=0;
        }
	else;
      }
      else
      {
	blank_lines++;
      }

      if (do_cret)
      {
       /*
        * 4-level (CRet) dithers...
        */
        if (output_type == OUTPUT_GRAY || output_type == OUTPUT_MONOCHROME)
        {
          (*writefunc)(v, black + height / 2, height / 2, 0);
          (*writefunc)(v, black, height / 2, 1);
        }
        else
        {
	  if(do_cretb)
	  {
/*	    (*writefunc)(v, black + height / 2, 0, 0); */
	    (*writefunc)(v, black, height/2, 0);
	  }
	  else
	  {
	    (*writefunc)(v, black + height / 2, height / 2, 0);
	    (*writefunc)(v, black, height / 2, 0);
	  }
          (*writefunc)(v, cyan + height / 2, height / 2, 0);
          (*writefunc)(v, cyan, height / 2, 0);
          (*writefunc)(v, magenta + height / 2, height / 2, 0);
          (*writefunc)(v, magenta, height / 2, 0);
          (*writefunc)(v, yellow + height / 2, height / 2, 0);
          if (do_6color)
          {
            (*writefunc)(v, yellow, height / 2, 0);
            (*writefunc)(v, lcyan + height / 2, height / 2, 0);
            (*writefunc)(v, lcyan, height / 2, 0);
            (*writefunc)(v, lmagenta + height / 2, height / 2, 0);
            (*writefunc)(v, lmagenta, height / 2, 1);		/* Last plane set on light magenta */
          }
          else
            (*writefunc)(v, yellow, height / 2, 1);		/* Last plane set on yellow */
        }
      }
      else
      {
       /*
        * Standard 2-level dithers...
        */

        if (output_type == OUTPUT_GRAY || output_type == OUTPUT_MONOCHROME)
        {
          (*writefunc)(v, black, height, 1);
        }
        else
        {
          if (black != NULL)
            (*writefunc)(v, black, height, 0);
          (*writefunc)(v, cyan, height, 0);
          (*writefunc)(v, magenta, height, 0);
          if (do_6color)
          {
            (*writefunc)(v, yellow, height, 0);
            (*writefunc)(v, lcyan, height, 0);
            (*writefunc)(v, lmagenta, height, 1);		/* Last plane set on light magenta */
          }
          else
            (*writefunc)(v, yellow, height, 1);		/* Last plane set on yellow */
        }
      }
    }

    errval += errmod;
    errline += errdiv;
    if (errval >= out_height)
    {
      errval -= out_height;
      errline ++;
    }
  }

/* Output trailing blank lines (may not be required?) */

  if (blank_lines > 1)
  {
    blank_lines--;		/* correct for one already output */
#ifdef DEBUG
    stp_erprintf( "Blank Lines = %d\n", blank_lines);
#endif
    stp_zprintf(v, "\033*b%dY", blank_lines);
    blank_lines=0;
  }

  image->progress_conclude(image);

  stp_free_dither(dither);


 /*
  * Cleanup...
  */

  stp_free_lut(nv);
  stp_free(in);
  stp_free(out);

  if (black != NULL)
    stp_free(black);
  if (cyan != NULL)
  {
    stp_free(cyan);
    stp_free(magenta);
    stp_free(yellow);
  }
  if (lcyan != NULL)
  {
    stp_free(lcyan);
    stp_free(lmagenta);
  }

  if ((caps->stp_printer_type & PCL_PRINTER_NEW_ERG) == PCL_PRINTER_NEW_ERG)
    stp_puts("\033*rC", v);
  else
    stp_puts("\033*rB", v);

  stp_puts("\033&l0H", v);		/* Eject page */
  if (do_cretb)
    {
      stp_zprintf(v, "\033%%-12345X\n");
    }
  stp_puts("\033E", v); 				/* PCL reset */
  stp_free_vars(nv);
}

const stp_printfuncs_t stp_pcl_printfuncs =
{
  pcl_parameters,
  stp_default_media_size,
  pcl_imageable_area,
  pcl_limit,
  pcl_print,
  pcl_default_parameters,
  pcl_describe_resolution,
  stp_verify_printer_params
};


/*
 * 'pcl_mode0()' - Send PCL graphics using mode 0 (no) compression.
 */

static void
pcl_mode0(const stp_vars_t v,		/* I - Print file or command */
          unsigned char *line,		/* I - Output bitmap data */
          int           height,		/* I - Height of bitmap data */
          int           last_plane)	/* I - True if this is the last plane */
{
  stp_zprintf(v, "\033*b%d%c", height, last_plane ? 'W' : 'V');
  stp_zfwrite((const char *) line, height, 1, v);
}


/*
 * 'pcl_mode2()' - Send PCL graphics using mode 2 (TIFF) compression.
 */

static void
pcl_mode2(const stp_vars_t v,		/* I - Print file or command */
          unsigned char *line,		/* I - Output bitmap data */
          int           height,		/* I - Height of bitmap data */
          int           last_plane)	/* I - True if this is the last plane */
{
  unsigned char	comp_buf[1536],		/* Compression buffer */
		*comp_ptr;		/* Current slot in buffer */

  stp_pack_tiff(line, height, comp_buf, &comp_ptr);

 /*
  * Send a line of raster graphics...
  */

  stp_zprintf(v, "\033*b%d%c", (int)(comp_ptr - comp_buf), last_plane ? 'W' : 'V');
  stp_zfwrite((const char *)comp_buf, comp_ptr - comp_buf, 1, v);
}
