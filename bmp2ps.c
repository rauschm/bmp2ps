/****************************************************************************/
/*  B M P 2 P S . C                                                         */
/*                                                                          */
/*  Das Programm erzeugt aus einer BMP-Datei (alle Formate) ein monochromes */
/*  Postscript-Image, wobei der Hintergrund weiß ist.                       */
/*  Die Farbe, an der ein gesetztes Pixel erkannt werden kann, und die      */
/*  Ausgabefarbe müssen angegeben werden.                                   */
/*                                                                          */
/*  Hinweis: Wenn Weiß ein geseztes Pixel angibt, kann irgendeine der       */
/*  anderen Farben angegeben werden.                                        */
/*                                                                          */
/*  Aufruf: bmp2ps [-[{S,W,R,G,B}]{S,R,G,B,Y,M,C}] Datei                    */
/*                                                                          */
/*          R = Rot   Y = Yellow   S = Schwarz                              */
/*          G = Grün  M = Magenta  W = Weiß                                 */
/*          B = Blau  C = Cyan                                              */
/****************************************************************************/

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct
{
  FILE* f;
  unsigned char* buffer;
} tidy_info = { NULL, NULL };

#define TIDY_INFO(x) do { tidy_info.x = x; } while (0)

typedef struct { unsigned char B, G, R; } BGR;
typedef struct { unsigned char B, G, R, A; } BGRA;

#pragma pack(push,1)
typedef struct BmpFile {
  struct {
    char          BM[2];
    unsigned int  file_size;               /* unit: Bytes */
    char          application_value_1[2];  /* = 0 */
    char          application_value_2[2];  /* = 0 */
    unsigned int  bitmap_offset;           /* unit: Bytes = 54/62/70/118/1078 */
  } header;
  
  struct {
    unsigned int   dib_header_size;        /* unit: Bytes = 40 */
    signed int     image_width;            /* unit = Pixel */
    signed int     image_height;           /* unit = Pixel, >0: flip image */
    unsigned short color_planes;           /* := 1 */
    unsigned short bits_per_pixel;         /* = 1, 2, 4, 8, 24 für RGB */
    unsigned int   compression_method;     /* = 0, d.h. keine Kompression */
    unsigned int   bitmap_raw_size;        /* unit: bytes = (width * bpp +31)/32*4 * |height| */
    signed int     horizontal_resolution;  /* = width im Druck, z.B. 0x2E23 für 300 DPI */
    signed int     vertical_resolution;    /* = height im Druck, z.B. 11811 für 300 DPI */
    unsigned int   number_of_colors;       /* = 0, d. h. default = 2^bpp */
    unsigned int   number_of_important_colors;  /* = 0, d. h. jede Farbe ist wichtig */
  } dib_header;

  union {
    struct bpp_1 { 
      BGRA colormap[2];
      struct {
        unsigned int ix00:1, ix01:1, ix02:1, ix03:1, ix04:1, ix05:1, ix06:1, ix07:1,
                     ix08:1, ix09:1, ix10:1, ix11:1, ix12:1, ix13:1, ix14:1, ix15:1,
                     ix16:1, ix17:1, ix18:1, ix19:1, ix20:1, ix21:1, ix22:1, ix23:1,
                     ix24:1, ix25:1, ix26:1, ix27:1, ix28:1, ix29:1, ix30:1, ix31:1;
      } bitmap[1];
    } b1;
    struct bpp_2 {
      BGRA colormap[4];
      struct {
        unsigned int ix00:2, ix01:2, ix02:2, ix03:2, ix04:2, ix05:2, ix06:2, ix07:2,
                     ix08:2, ix09:2, ix10:2, ix11:2, ix12:2, ix13:2, ix14:2, ix15:2;
      } bitmap[1];
    } b2;
    struct bpp_4 {
      BGRA colormap[16];
      struct {
        unsigned int ix00:4, ix01:4, ix02:4, ix03:4, ix04:4, ix05:4, ix06:4, ix07:4;
      } bitmap[1];
    } b4;
    struct bpp_8 { 
      BGRA colormap[256];
      struct {
        unsigned int ix00:8, ix01:8, ix02:8, ix03:8;
      } bitmap[1];
    } b8;
    struct bpp_24 {
      BGR bitmap[4];
    } b24;
  }; 
} BmpFile;
#pragma pack(pop)

typedef struct FileInfo
{
  unsigned long  fileSize;
  unsigned char* fileData;
} FileInfo;

struct Options
{
  char inputColor;
  char outputColor;
} opt = { 'S', 'S' };

#include "bmp2ps_proto.h"


void main(int argc, char* argv[])
{
  parseOptions(&argc, &argv, &opt);
  if (argc != 2)
    usage();

  BmpFile* bmpData = readBmpFile(argv[1]);
  writePsFile(bmpData, opt.inputColor, opt.outputColor);
  freeBuffer(bmpData);
}


int isnt_SWRGBYMC(char c)  { return (c &= ~32) != 'S' && c != 'W' && c != 'R' && c != 'G' && c != 'B' && c != 'Y' && c != 'M' && c != 'C'; }
int is_W(char c)           { return (c &= ~32) == 'W'; }
int isnt_SRGBYMC(char c)   { return (c &= ~32) != 'S' && c != 'R' && c != 'G' && c != 'B' && c != 'Y' && c != 'M' && c != 'C'; }
int is_YMC(char c)         { return (c &= ~32) == 'Y' || c == 'M' || c == 'C'; }

void parseOptions(int* argcRef, char** argvRef[], struct Options* opt)
{
  int    argc = *argcRef;
  char** argv = *argvRef;

  if (argc > 1 && argv[1][0] == '-')
  {
    if (   argv[1][1] == '\0'
        || isnt_SWRGBYMC(argv[1][1])
        || argv[1][2] != '\0' && (   is_YMC(argv[1][1])
                                  || isnt_SRGBYMC(argv[1][2])
                                  || argv[1][3] != '\0')
        || argc > 2 && argv[2][0] == '-')
      usage();

    if (argv[1][2] != '\0' || is_W(argv[1][1]))
      opt->inputColor = argv[1][1] & ~32;
    else
      opt->outputColor = argv[1][1] & ~32;
    
    if (argv[1][2] != '\0')
      opt->outputColor = argv[1][2] & ~32;

    --argc;
    ++argv;
  }

  *argcRef = argc;
  *argvRef = argv;
}


void writePsFile(BmpFile* b, char pixelColor, char imageColor)
{
  printf("%%!PS\n");
  printf("%d %d\n", b->dib_header.image_width, b->dib_header.image_height);
  printf("1 [1 0 0 1 0 0]\n");
  printf("{<\n");

  unsigned int pixelMask = (1 << b->dib_header.bits_per_pixel) - 1; 
  int bytesPerLine = ((b->dib_header.image_width * b->dib_header.bits_per_pixel + 31) / 32) * 4;
  unsigned char* y_ptr = ((unsigned char *) b)
                       + b->header.bitmap_offset
                       + (b->dib_header.image_height > 0 ? 0
                          : (abs(b->dib_header.image_height) - 1) * bytesPerLine);
  int delta_y = b->dib_header.image_height > 0 ? bytesPerLine : -bytesPerLine; 

  for (int y = abs(b->dib_header.image_height); y > 0; --y, y_ptr += delta_y)
  {
    unsigned char* x_ptr = y_ptr;
    unsigned char bytes[32];
    unsigned char bits = 0;
    int bytePos = 0;
    int bit = 128;
    int shift = 8;

    for (int x = b->dib_header.image_width; x > 0; --x)
    {
      BGR bgr;
      if (b->dib_header.bits_per_pixel == 24)
      {
        bgr = *(BGR*) x_ptr;
        x_ptr += 3;
      }
      else
      {
        shift -= b->dib_header.bits_per_pixel;
        unsigned int colorIdx = (*x_ptr >> shift) & pixelMask;
        bgr = *(BGR*) &(b->b8.colormap[colorIdx]);
        if (shift == 0)
        {
          shift = 8;
          x_ptr += 1;
        }
      }
      int pixel_is_on = pixelColor == 'S' ? (bgr.B <  85 &&  bgr.G <  85 && bgr.R <  85)
                      : pixelColor == 'W' ? (bgr.B > 170 &&  bgr.G > 170 && bgr.R > 170)
                      : pixelColor == 'R' ? (bgr.R > 170 && (bgr.B <  85 || bgr.G <  85))
                      : pixelColor == 'G' ? (bgr.G > 170 && (bgr.R <  85 || bgr.B <  85))
                      :/*pixelColor=='B'*/  (bgr.B > 170 && (bgr.R <  85 || bgr.G <  85));
      if (!pixel_is_on)
        bits |= bit;
      bit >>= 1;
      if (bit == 0 || x == 1) {
        bit = 128;
        bytes[bytePos++] = bits;
        bits = 0;
        if (bytePos == sizeof(bytes) || x == 1) {
          writePsBytes(bytes, bytePos);
          bytePos = 0;
        }
      }
    }
  }

  printf(">}\n");
  if (imageColor == 'S')
  {
    printf("image\n");
  }
  else
  {
    char* finish = imageColor == 'R' ? "{<ff>} exch dup"
                 : imageColor == 'G' ? "dup {<ff>} exch"
                 : imageColor == 'B' ? "dup {<ff>}"
                 : imageColor == 'Y' ? "{<ff>} exch {<ff>} exch"
                 : imageColor == 'M' ? "{<ff>} exch {<ff>}"
                 : /*imageColor=='C'*/ "{<ff>} dup";
    printf("%s true 3 colorimage\n", finish);
  }
}


void writePsBytes(unsigned char* bytes, int count)
{
  char buf[65];
  unsigned char c;

  for (int i = 0; i < count; ++i)
  {
    c = bytes[i];
    int h1 = c / 16;
    int h2 = c % 16;
    buf[2 * i]     = h1 + (h1 < 10 ? '0' : 'a' - 10);
    buf[2 * i + 1] = h2 + (h2 < 10 ? '0' : 'a' - 10);
  }
  buf[2 * count] = '\0';
  printf("%s\n", buf);
}


BmpFile* readBmpFile(char* fileName)
{
  FILE* f = openFile(fileName);
  FileInfo bmpFileInfo = readFile(f);
  closeFile(f);
  return (BmpFile*) bmpFileInfo.fileData;
}


FILE* openFile(char* fileName)
{
  FILE* f;

  if ((f = fopen(fileName, "rb")) == NULL)
    exitWithErrorMessage("open error %s: %s\n", fileName, strerror(errno));
  TIDY_INFO(f);    
  return f;
}


void closeFile(FILE* f)
{
  fclose(f);
  f = NULL;
  TIDY_INFO(f);
}


FileInfo readFile(FILE* f)
{
  FileInfo fi = { getFileSize(f), allocBuffer(fi.fileSize) };
  if (fread(fi.fileData, 1, fi.fileSize, f) != fi.fileSize)
    exitWithErrorMessage("read error: %s\n", strerror(errno));
  return fi;
}


unsigned int getFileSize(FILE* f)
{
  fseek(f, 0L, SEEK_END);
  unsigned int fileSize = ftell(f);
  fseek(f, 0L, SEEK_SET);
  return fileSize;
}


unsigned char* allocBuffer(unsigned int bufferSize)
{
  unsigned char* buffer = malloc(bufferSize);
  if (buffer == NULL)
    exitWithErrorMessage("malloc error: %s", strerror(errno));
  TIDY_INFO(buffer);
  return buffer;
}


void freeBuffer(void* buffer)
{
  free(buffer);
  buffer = NULL;
  TIDY_INFO(buffer);
}


void tidy_up(void)
{
  if (tidy_info.f != NULL)
    fclose(tidy_info.f);
  if (tidy_info.buffer != NULL)
    free(tidy_info.buffer);
}


void usage(void)
{
  exitWithErrorMessage("Gibt eine BMP-Datei im Postscript-Format aus\n"
                       "Aufruf: bmp2ps [-[{S,R,G,B}]{S,R,G,B,Y,M,C}] Datei\n\n"
                       "        R = Rot   Y = Yellow   S = Schwarz\n"
                       "        G = Grün  M = Magenta\n"
                       "        B = Blau  C = Cyan\n");
}


void exitWithErrorMessage(char* format, ...)
{
  va_list args;

  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  tidy_up();
  exit(1);
}
