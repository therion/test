/**
 * @file thparse.cxx
 * Parsing module.
 */
  
/* Copyright (C) 2000 Stacho Mudrak
 * 
 * $Date: $
 * $RCSfile: $
 * $Revision: $
 *
 * -------------------------------------------------------------------- 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * --------------------------------------------------------------------
 */

#include "thparse.h"
#include "therion.h"
#include <string.h>
#include "thinfnan.h"
#include "thdatabase.h"
#include "thtflength.h"
#include "thexception.h"
#include <errno.h>
#include <stdlib.h>
#include <math.h>

#ifdef HAVE_ROUND
extern double round(double); /* prototype is often missing... */
# define thround round
#else
static double
thround(double x) {
   if (x >= 0.0) return floor(x + 0.5);
   return ceil(x - 0.5);
}
#endif


thmbuffer thparse_mbuff;

int thmatch_stok(char *buffer, const thstok *tab, int tab_size)
{
  int a = 0, b = tab_size - 1, c, r;
  while (a <= b) {
    c = unsigned((a + b) / 2);
    r = strcmp(tab[c].s, buffer);
    if (r == 0) return tab[c].tok;
    if (r < 0)
      a = c + 1;
    else
      b = c - 1;
   }
   return tab[tab_size].tok; /* no match */
}


char * thmatch_tstring(int token, const thstok *tab, int tab_size)
{
  int i = 0;
  while ((i < tab_size) && (tab[i].tok != token))
    i++;
  if (tab[i].tok == token)
    return tab[i].s;
  else
    return "unknown";  /* no match */
}


void thsplit_word(thbuffer * dword, thbuffer * drest, char * src)
{
  long srcl = strlen(src),
    idx = 0,
    idx0 = 0;
  short state = -1; // -1 before, 0 in, 1 out, 2 on the rest, 3 out
  unsigned char * s1 = NULL, * s2 = (unsigned char *) src;
  while ((state < 2) && (idx < srcl)) {
    switch (state) {
      case -1: 
        if (*s2 > 32) {
          state = 0;
          s1 = s2;
          idx0 = idx;
        }
        break;
      case 0:
        if (*s2 < 33) {
          state = 1;
          dword->strncpy((char *)s1, idx - idx0);
        }
        break;
      case 1:
        if (*s2 > 32) {
          state = 2;
          s1 = s2;
          idx0 = idx;
        }
    }
    idx++;
    s2++;
  }
  
  switch (state) {
    case -1:
      dword->strcpy("");
      drest->strcpy("");
      break;
    case 0:
      dword->strcpy((char *)s1);
      drest->strcpy("");
      break;
    case 1:
      drest->strcpy("");
      break;
    default:
      idx = srcl;
      s2 = (unsigned char *) src + idx;
      while ((idx >= idx0) && (*s2 < 33)) {
        s2--;
        idx--;
      }
      drest->strncpy((char *) s1, idx - idx0 + 1);
  }

}


void thsplit_words(thmbuffer * dest, char * src)
{
  long srcl = strlen(src),
    idx = 0,
    idx0 = 0;
  dest->clear();
  short state = 0; // 0 before, 1 in
  unsigned char * s1 = NULL, * s2 = (unsigned char *) src;
  while (idx < srcl) {
    switch (state) {
      case 0:
        if (*s2 > 32) {
          state = 1;
          s1 = s2;
          idx0 = idx;
        }
        break;
      case 1:
        if (*s2 < 33) {
          state = 0;
          dest->appendn((char *) s1, idx - idx0);
        }
    }
    idx++;
    s2++;
  }
  
  if (state == 1) {
    dest->append((char *) s1);
  }
}


void thsplit_strings(thmbuffer * dest, const char * src, const char separator)
{
  long srcl = strlen(src),
    idx = 0,
    idx0 = 0;
  dest->clear();
  short state = 0; // 0 before, 1 in
  unsigned char * s1 = NULL, * s2 = (unsigned char *) src;
  while (idx < srcl) {
    switch (state) {
      case 0:
        if (*s2 != separator) {
          state = 1;
          s1 = s2;
          idx0 = idx;
        }
        break;
      case 1:
        if (*s2 == separator) {
          state = 0;
          dest->appendn((char *)s1, idx - idx0);
        }
    }
    idx++;
    s2++;
  }
  
  if (state == 1)
    dest->append((char *)s1);
}


void thsplit_paths(thmbuffer * dest, char * src, char separator)
{
  long srcl = strlen(src),
    idx = 0,
    idx0 = 0;
  dest->clear();
  short state = 0; // 0 before, 1 in
  unsigned char * s1 = NULL, * s2 = (unsigned char *) src;
  while (idx < srcl) {
    switch (state) {
      case 0:
        if (*s2 != separator) {
          state = 1;
          s1 = s2;
          idx0 = idx;
        }
        break;
      case 1:
        if (*s2 == separator) {
#ifdef THWIN32  
          if ((idx - idx0 == 1) &&
            (((src[idx0] >= 'A') && (src[idx0] <= 'Z')) || 
            ((src[idx0] >= 'a') && (src[idx0] <= 'z'))))
            break;
#endif
          state = 0;
          dest->appendn((char *)s1, idx - idx0);
        }
    }
    idx++;
    s2++;
  }
  
  if (state == 1)
    dest->append((char *)s1);
}


void thsplit_args_postp_quotes(char * buf)
{
  long bl = strlen(buf),
    idx = 0;
  while (idx < bl) {
    if (*buf == '"') {
      strcpy(buf, buf+1);
      bl--;
    }
    buf++;
    idx++;
  }
}


void thsplit_args(thmbuffer * dest, char * src)
{
  long srcl = strlen(src),
    idx = 0,
    idx0 = 0;
  dest->clear();
  short state = 0, qlevel = 0; // 0 before, 1 in word, 2 in "", 3 in []
  unsigned char * s1 = NULL, * s2 = (unsigned char *) src;
  bool postp_quotes = false;
  char * postp_ptr = NULL;
  while (idx < srcl) {
    switch (state) {
      case 0:
        if (*s2 > 32) {
          switch (*s2) {
            case '#':
              idx = srcl;
              break;  
            case '"':
              state = 2;
              postp_quotes = false;
              break;
            case '[':
              state = 3;
              qlevel = 1;
              break;
            default:
              state = 1;
          }
          s1 = s2;
          idx0 = idx;
        }
        break;
      case 1:
        if (*s2 < 33) {
          state = 0;
          dest->appendn((char *) s1, idx - idx0);
        }
        break;
      case 3:
        switch (*s2) {
          case ']':
            qlevel--;
            if (qlevel <= 0) {
              state = 0;
              if (idx > (idx0 + 1))
                dest->appendn((char *) s1 + 1, idx - idx0 - 1);
              else
                dest->appendn("",0);
            }
            break;
          case '[':
            qlevel++;
            break;
        }
        break;
      case 2:
        if (*s2 == '"') {
          if ((idx + 1 < srcl) && (s2[1] == '"')) {
            postp_quotes = true;
            idx++;
            s2++;
            break;
          }
          else {
            state = 0;
            if (idx > (idx0 + 1)) {
              postp_ptr = dest->appendn((char *) s1 + 1, idx - idx0 - 1);
              if (postp_quotes)
                thsplit_args_postp_quotes(postp_ptr);
              }
            else
              dest->appendn("",0);
          }
        }
    }
    idx++;
    s2++;
  }
  
  switch (state) {
    case 1:
      dest->append((char *) s1);
      break;
    case 2:
      if (idx > (idx0 + 1))
        dest->appendn((char *) s1 + 1, idx - idx0 - 1);
      else
        dest->appendn("",0);
      break;
    case 3:
      if (idx > (idx0 + 1))
        postp_ptr = dest->appendn((char *) s1 + 1, idx - idx0 - 1);
        if (postp_quotes)
          thsplit_args_postp_quotes(postp_ptr);
      else
        dest->appendn("",0);
      break;
  }
}


void thsplit_fpath(thbuffer * dest, char * src)
{
  long len = strlen(src);
  char * s = src + len++;
  while ((len > 0) && (*s != '/')) {
    s--;
    len--;
  }
  dest->strncpy(src, len);
}


bool th_is_keyword(char * str)
{
  size_t sl = strlen(str), i;
  unsigned char * s = (unsigned char *) str;
  if (sl == 0)
    return false;
  else
    for(i = 0; i < sl; i++, s++) {
      if ((*s > 96) && (*s < 123)) continue;
      if ((*s > 64) && (*s < 91)) continue;
      if ((*s > 47) && (*s < 58)) continue;
      if (*s == 95) continue;
      if ((*s == 45) && (i > 0)) continue;
      return false;
    }
  return true;
}


bool th_is_index(char * str)
{
  size_t sl = strlen(str), i;
  unsigned char * s = (unsigned char *) str;
  if (sl == 0)
    return false;
  else
    for(i = 0; i < sl; i++, s++) {
      if ((*s > 47) && (*s < 58)) continue;
      return false;
    }
  return true;
}


bool th_is_keyword_list(char * str, char sep)
{
  size_t sl = strlen(str), i;
  unsigned char * s = (unsigned char *) str;
  if (sl == 0)
    return false;
  else
    for(i = 0; i < sl; i++, s++) {
      if ((*s > 96) && (*s < 123)) continue;
      if ((*s > 64) && (*s < 91)) continue;
      if ((*s > 47) && (*s < 58)) continue;
      if ((*s == 95) || (*s == 45)) continue;
      if (*s == sep) continue;
      return false;
    }
  return true;
}


bool th_is_extkeyword(char * str)
{
  size_t sl = strlen(str), i;
  unsigned char * s = (unsigned char *) str;
  if (sl == 0)
    return false;
  else
    for(i = 0; i < sl; i++, s++) {
      if ((*s > 96) && (*s < 123)) continue;
      if ((*s > 64) && (*s < 91)) continue;
      if ((*s > 47) && (*s < 58)) continue;
      if ((*s == 95) || (*s == 45)) continue;
      if (i > 0) {
        if (*s == 39) continue;
        if ((*s > 41) && (*s < 48)) continue;
      }
      return false;
    }
  return true;
}


void thparse_double(int & sv, double & dv, char * src)
{
  char * endptr;
  sv = thmatch_token(src, thtt_special_val);
  switch (sv) {
    case TT_SV_NAN:
      dv = thnan;
      break;
    case TT_SV_UP:
    case TT_SV_PINF:
      dv = thinf;
      break;
    case TT_SV_DOWN:
    case TT_SV_NINF:
      dv = - thinf;
      break;
    default:
      errno = 0;
      dv = strtod(src,&endptr);
      if ((*endptr == 0) && (errno == 0))
        sv = TT_SV_NUMBER;
      else
        dv = 0.0;
  }
}


void thparse_double_dms(int & sv, double & dv, char * src)
{
  int ssv;
  double dms;
  thsplit_strings(&thparse_mbuff,src,':');
  if (thparse_mbuff.get_size() > 0) {
    thparse_double(sv, dv, thparse_mbuff.get_buffer()[0]);
//    thprintf("DEG:%lf\n", dv);
    if (sv == TT_SV_NUMBER) {
      // ok, mame stupne
      if (thparse_mbuff.get_size() > 1) {
        thparse_double(ssv, dms, thparse_mbuff.get_buffer()[1]);
//        thprintf("MIN:%lf\n", dms);
        if ((ssv == TT_SV_NUMBER) && (dms == double(int(dms))) && (dms >= 0.0) && (dv == double(int(dv)))) {
          // ok, mame minuty
          if (dv > 0.0) {
            dv += dms / 60.0;
          } else {
            dv -= dms / 60.0;
          }
          if (thparse_mbuff.get_size() == 3) {
            thparse_double(ssv, dms, thparse_mbuff.get_buffer()[2]);
//            thprintf("SEC:%lf\n", dms);
            if ((ssv == TT_SV_NUMBER) && (dms >= 0.0)) {
              if (dv > 0.0) {
                dv += dms / 3600.0;
              } else {
                dv -= dms / 3600.0;
              }
            } else {
              // error
              sv = TT_SV_UNKNOWN;
            }
          } else if (thparse_mbuff.get_size() > 3) {
            // error
            sv = TT_SV_UNKNOWN;
          }
        } else {
          // nemame nic
          sv = TT_SV_UNKNOWN;
        }
      }
    }
  } else {
    sv = TT_SV_UNKNOWN;
    dv = 0.0;
  }
//  thprintf("\n%s -> %lf\n", src, dv);
}

void thdecode_c(thbuffer * dest, const char * src)
{

  size_t srcln = strlen(src), srcx = 0;
  unsigned char * srcp, * dstp;
  unsigned num;
  dest->guarantee(srcln * 8 + 1);  // check buffer size
  srcp = (unsigned char*) src;
  dstp = (unsigned char*) dest->get_buffer();
  while (srcx < srcln) {
    if ((*srcp < 32) || (*srcp > 127)) {
        *dstp = '"';
        dstp++;
        *dstp = '"';
        dstp++;
        *dstp = '\\';
        dstp++;
        *dstp = 'x';
        dstp++;
        num = *srcp / 16;
        if (num < 10)
          *dstp = '0' + num;
        else
          *dstp = 'A' + (num - 10);
        dstp++;
        num = *srcp % 16;
        if (num < 10)
          *dstp = '0' + num;
        else
          *dstp = 'A' + (num - 10);
        dstp++;
        *dstp = '"';
        dstp++;
        *dstp = '"';
    } else {
      if ((*srcp == '\\') || (*srcp == '"')) {
        *dstp = '\\';
        dstp++;
      }
      *dstp = *srcp;  
    }
    srcx++;
    srcp++;
    dstp++;
  }
  // end destination string with 0
  *dstp = 0;
  
}

void thdecode_tex(thbuffer * dest, const char * src)
{

  size_t srcln = strlen(src), srcx = 0;
  unsigned char * srcp, * dstp;
//  unsigned num;
  dest->guarantee(srcln * 8 + 1);  // check buffer size
  srcp = (unsigned char*) src;
  dstp = (unsigned char*) dest->get_buffer();
  while (srcx < srcln) {
    switch (*srcp) {
      case '%':
      case '_':
      case '&':
      case '$':
      case '#':
      case '@':
      case '~':
      case '`':
      case '^':
      case '*':
      case '\\':
      case '{':
      case '}':
      case '|':
      case '"':
        *dstp = '\\';
        dstp++;
        *dstp = *srcp;  
        break;      
      default:
        *dstp = *srcp;  
        break;
    }
    srcx++;
    srcp++;
    dstp++;
  }
  // end destination string with 0
  *dstp = 0;
  
}

void thdecode_sql(thbuffer * dest, const char * src)
{

  size_t srcln = strlen(src), srcx = 0;
  unsigned char * srcp, * dstp;
//  unsigned num;
  if ((src == NULL) || (srcln == 0)) {
    *dest = "NULL";
    return;
  }
  dest->guarantee(srcln * 8 + 3);  // check buffer size
  srcp = (unsigned char*) src;
  dstp = (unsigned char*) dest->get_buffer();
  *dstp = '\'';
  dstp++;
  while (srcx < srcln) {
    switch (*srcp) {
      case '\'':
        *dstp = '\'';
        dstp++;
        *dstp = *srcp;  
        break;      
      default:
        *dstp = *srcp;  
        break;
    }
    srcx++;
    srcp++;
    dstp++;
  }
  // end destination string with 0
  *dstp = '\'';
  dstp++;
  *dstp = 0;
  
}



void thdecode_arg(thbuffer * dest, const char * src)
{

  size_t srcln = strlen(src), srcx;
  unsigned char * srcp, * dstp;
  
//  unsigned num;
  srcx = 0;
  srcp = (unsigned char*) src;

  // rychlo prescanuje, an nenajde whitespace, tak len skopiruje
  bool quickcpy = true;
  while (srcx < srcln) {
    if (*srcp <= 32) {
      quickcpy = false;
      break;
    }
    srcp++;
    srcx++;
  }  
  
  if (quickcpy) {
    dest->strcpy(src);
    return;
  }
  
  // zaciatocna uvodzovka
  dest->guarantee(srcln * 8 + 3);  // check buffer size
  srcx = 0;
  srcp = (unsigned char*) src;
  dstp = (unsigned char*) dest->get_buffer();
  *dstp = '"';
  dstp++;
  while (srcx < srcln) {
    if (*srcp == '"') {
      *dstp = '"';
      dstp++;
    }
    *dstp = *srcp;  
    srcx++;
    srcp++;
    dstp++;
  }
  // end destination string with 0
  *dstp = '"';
  dstp++;
  *dstp = 0;
  
}




void thparse_altitude(char * src, double & altv, double & fixv)
{

  thsplit_args(& thdb.db2d.mbf, src);
  int npar = thdb.db2d.mbf.get_size();
  char ** pars = thdb.db2d.mbf.get_buffer();
  int sv, ux = 0;
  bool parsev = true;
  thtflength lentf;

  altv = 0.0;
  fixv = 1.0;
  
  if ((npar > 1) && (strcmp(pars[0],"fix") == 0)) {
    fixv = 0.0;
    npar--;
    pars++;
  }

  switch (npar) {
    case 0:
      parsev = false;
      break;
    case 1:
      break;
    case 2:
      ux = 1;
      break;
    default:
      ththrow(("invalid altitude specification -- %s",src))
  }

  if (parsev) {
    thparse_double(sv,altv,pars[0]);
    if ((sv != TT_SV_NUMBER) && (sv != TT_SV_NAN))
      ththrow(("invalid altitude value -- %s", pars[0]));
    if (sv == TT_SV_NAN)
      altv = 0.0;
  }

  if ((ux > 0) && (sv != TT_SV_NAN)) {
    lentf.parse_units(pars[ux]);
    altv = lentf.transform(altv);
  }
}

void thparse_image(char * fname, double & width, double & height, double & dpi)
{

  FILE * pictf = fopen(fname, "rb");
#define picths 2048
  size_t phsize;
  double xdpi, ydpi;
  unsigned char picth [picths], * scan;
  size_t sx;
  width = 0.0;
  height = 0.0;
  dpi = 300.0;
  if (pictf != NULL) {
    phsize = fread(&(picth[0]), 1, picths, pictf);
    fclose(pictf);
    // najde si format a vytiahne informacie
    if ((picth[0] == 0xFF) && (picth[1] == 0xD8) &&
        (picth[2] == 0xFF) && (picth[3] == 0xE0)) {
      // JPEG
      xdpi = 300.0;
      ydpi = 300.0;
      switch (picth[13]) {
        case 1:
          xdpi = thround(double(picth[14] * 256.0 + picth[15]));
          ydpi = thround(double(picth[16] * 256.0 + picth[17]));
          break;
        case 2:
          xdpi = thround(double(picth[14] * 256 + picth[15]) * 2.54);
          ydpi = thround(double(picth[16] * 256 + picth[17]) * 2.54);
          break;
      }
      if (xdpi != ydpi) {
        ththrow(("X and Y image resolution not equal -- %s", fname));
      }
      dpi = xdpi;
      if (dpi < 1.0) {
        dpi = 300.0;
      }      
      for(sx = 0, scan = &(picth[0]); sx < (phsize - 10); sx++, scan++) {
        if ((scan[0] == 0xFF) && ((scan[1] == 0xC0) || (scan[1] == 0xC1))) {
          height = thround(double(scan[5] * 256.0 + scan[6]));
          width = thround(double(scan[7] * 256.0 + scan[8]));
          break;
        }
      }
    } else if (
      (picth[0] == 0x89) && (picth[1] == 0x50) &&
      (picth[2] == 0x4E) && (picth[3] == 0x47) &&
      (picth[4] == 0x0D) && (picth[5] == 0x0A) &&
      (picth[6] == 0x1A) && (picth[7] == 0x0A)) {
      // PNG
      // najde pHYs za nim 4(x), 4(y), 1(units) = 01-meters, 00-unspec
      for(sx = 0, scan = &(picth[0]); sx < (phsize - 12); sx++, scan++) {
        if (strncmp((char *) scan,"pHYs",4) == 0) {
          xdpi = double(scan[4] * 0x1000000 + scan[5] * 0x10000 + scan[6] * 0x100 + scan[7]);
          ydpi = double(scan[8] * 0x1000000 + scan[9] * 0x10000 + scan[10] * 0x100 + scan[11]);
          if (xdpi != ydpi) {
            ththrow(("X and Y image resolution not equal -- %s", fname));
          }
          switch (scan[12]) {
            case 1:
              xdpi = thround(xdpi * 0.0254);
              break;
            default:
              xdpi = 300.0;
              break;
          }
          dpi = xdpi;
          break;
        }
      }      
      width = thround(double(picth[16] * 0x1000000 + picth[17] * 0x10000 + picth[18] * 0x100 + picth[19]));
      height = thround(double(picth[20] * 0x1000000 + picth[21] * 0x10000 + picth[22] * 0x100 + picth[23]));
    } else {
      ththrow(("file format not supported -- %s", fname))
    }
  } else {
    ththrow(("file not found -- %s", fname))
  }

#ifdef THDEBUG
  thprintf("\nIMAGE FILE:%s\n", fname);
  thprintf(  "       DPI:%.0f\n", dpi);
  thprintf(  "      SIZE:%.0f x %.0f\n\n", width, height);
#endif
}


void thHSV2RGB(double H, double S, double V, double & R, double & G, double & B) {
  if (S == 0.0) {
    R = V;
    G = V;
    B = V;
  } else {
    double var_h = H * 6;
    int var_i = (int) floor( var_h );
    double var_1 = V * ( 1 - S );
    double var_2 = V * ( 1 - S * ( var_h - var_i ) );
    double var_3 = V * ( 1 - S * ( 1 - ( var_h - var_i ) ) );
    switch (var_i) {
       case 0: R = V     ; G = var_3 ; B = var_1 ; break;
       case 1: R = var_2 ; G = V     ; B = var_1 ; break; 
       case 2: R = var_1 ; G = V     ; B = var_3 ; break;
       case 3: R = var_1 ; G = var_2 ; B = V     ; break;
       case 4: R = var_3 ; G = var_1 ; B = V     ; break;
      default: R = V     ; G = var_1 ; B = var_2 ; break;
    }
  }
}


void thset_color(int color_map, double index, double total, double & R, double & G, double & B) {
  switch (color_map) {
    default:
      if (total > 0)
        thHSV2RGB(index / total * 0.833333, 1.0, 1.0, R, G, B);
      else {
        R = 1.0;
        G = 1.0;
        B = 1.0;
      }
  }
  R = double(int(100 * R)) / 100.0;
  G = double(int(100 * G)) / 100.0;
  B = double(int(100 * B)) / 100.0;
}


void thset_grid(
  double gorigin, double gsize, 
  double min, double max, 
  double & qstart, long & nquads)
{
  // treba nam najst origin start a quads aby
  if (min < gorigin)
    qstart = gorigin - gsize * ceil((gorigin - min) / gsize);
  else
    qstart = gorigin + gsize * floor((min - gorigin) / gsize);    
  nquads = long(ceil(max - qstart) / gsize);
}




