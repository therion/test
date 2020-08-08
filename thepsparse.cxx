/**
 * @file thconvert.cxx
 */
  
/* Copyright (C) 2005 Martin Budaj
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
 
// #include <iomanip>
#include <iostream>
#include <fstream>
#include <list>
#include <deque>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <sstream>

#include <cstring>
#include <cstdio>
#include <cmath>
#include <cassert>

#include "thepsparse.h"
#include "thpdfdbg.h"
#include "thpdfdata.h"
#include "thtexfonts.h"
#include "thconvert.h"

using namespace std;

#define IOerr(F) ((string)"Can't open file "+F+"!\n").c_str()

extern map<string,string> RGB, ALL_FONTS, ALL_PATTERNS;
typedef set<unsigned char> FONTCHARS;
extern map<string,FONTCHARS> USED_CHARS;
list<pattern> PATTERNLIST;
list<converted_data> GRIDLIST;
converted_data NArrow, ScBar;

extern unsigned font_id, patt_id;

////////////////////////////////////////////////////////////////////////////////

CGS::CGS () {
  color[0] = -1;
  color[1] = -1;
  color[2] = -1;
  
//  clippathID = random();
//  clippath = false;
}

int CGS::clippathID = 0;

string CGS::svg_color() {
  if (color[0] == -1) return "inherit";
  
//  char ch[8];
//  sprintf(ch,"#%02x%02x%02x",int(255*color[0]) % 256,
//                             int(255*color[1]) % 256,
//                             int(255*color[2]) % 256);
//  return (string) ch;
  return rgb2svg(color[0],color[1],color[2]);
}

MP_data::MP_data () {
  idx = 0;
}

void MP_text::clear() {
  x = y = 0;
  xx = yy = 1;
  xy = yx = 0;
  size = 0;
  r = g = b = -1;
  transformed = false;
}

void MP_text::print_svg(ofstream & F, CGS & gstate) {
  F << "<text font-family=\"" << font << "\" font-size=\"" << size << "\" ";
  if (LAYOUT.colored_text && r>=0 && g>=0 && b>=0) F << 
    "fill=\"" << rgb2svg(r,g,b) << "\" " <<
    "stroke=\"black\" stroke-width=\"0.1\" ";
  else F << "fill=\"black\" stroke=\"none\" ";
  F << "transform=\"matrix(" << xx << " " << xy << " " << -yx << " " << -yy <<
       " " << x << " " << y << ")\">";
  for (unsigned int i = 0; i < text.size(); i++)
    F << "&#x" << hex << tex2uni(font, int(text[i])) << dec << ";";
  F << "</text>" << endl;
}


MP_text::MP_text() {
  clear();
}

void MP_transform::clear () {
  command = MP_notransf;
  transf[0] = 1;
  transf[1] = 0;
  transf[2] = 0;
  transf[3] = 1;
  transf[4] = 0;
  transf[5] = 0;
}

MP_transform::MP_transform () {
  clear();
}

void MP_transform::set(int T, string s1, string s2,double dx, double dy) {
  command = T;
  transf[0] = atof(s1.c_str())-dx;
  transf[1] = atof(s2.c_str())-dy;
}

void MP_transform::set(int T, string s1, string s2, string s3,
                              string s4, string s5, string s6,
                              double dx, double dy) {
  command = T;
  transf[0] = atof(s1.c_str());
  transf[1] = atof(s2.c_str());
  transf[2] = atof(s3.c_str());
  transf[3] = atof(s4.c_str());
  transf[4] = atof(s5.c_str())-dx;
  transf[5] = atof(s6.c_str())-dy;
}

void MP_path::clear() {
  segments.clear();
  closed=false;
  fillstroke = -1;
  transformation.clear();
}

MP_path::MP_path() {
  clear();
}

void MP_path::add(int command, string s1, string s2, double dx, double dy) {
  MP_path_segment seg;
  seg.command = command;
  seg.coord[0] = atof(s1.c_str())-dx;
  seg.coord[1] = atof(s2.c_str())-dy;
  segments.push_back(seg);
}

void MP_path::add(int command, string s1, string s2, string s3, 
                               string s4, string s5, string s6,
                               double dx, double dy) {
  MP_path_segment seg;
  seg.command = command;
  seg.coord[0] = atof(s1.c_str())-dx;
  seg.coord[1] = atof(s2.c_str())-dy;
  seg.coord[2] = atof(s3.c_str())-dx;
  seg.coord[3] = atof(s4.c_str())-dy;
  seg.coord[4] = atof(s5.c_str())-dx;
  seg.coord[5] = atof(s6.c_str())-dy;
  segments.push_back(seg);
}

void MP_path::print_svg(ofstream & F, CGS & gstate, string unique_prefix) {
  if (fillstroke == MP_clip) {
    CGS::clippathID++;
    gstate.clippathdepth.insert(make_pair(CGS::clippathID,0));
    F << "<clipPath id=\"clip_" << CGS::clippathID << "_" << unique_prefix << "\">" << endl << "  ";
  }
  F << "<path ";
  if (fillstroke != MP_clip) {
    F << "fill=\"" << (fillstroke==MP_fill || fillstroke==MP_fillstroke ? 
           (gstate.pattern == ""? gstate.svg_color() : 
              "url(#patt_" + gstate.pattern + "_" + unique_prefix + ")") : "none") <<  
         "\" stroke=\"" << (fillstroke==MP_stroke || fillstroke==MP_fillstroke ? 
           gstate.svg_color() : "none") <<  "\" ";
    if (fillstroke==MP_stroke || fillstroke==MP_fillstroke) {
      F << "stroke-width=\"" << gstate.linewidth << "\" ";
      if (gstate.linecap != MP_rounded) {
        F << "stroke-linecap=";
        if (gstate.linecap == MP_butt) F << "\"butt\" ";
        else F << "\"square\" ";
      }
      if (gstate.linejoin != MP_rounded) {
        F << "stroke-linejoin=";
        if (gstate.linecap == MP_mitered) F << "\"miter\" ";
        else F << "\"bevel\" ";
      }
      if (gstate.miterlimit != 10) F << "stroke-miterlimit=\"" << 
          gstate.miterlimit << "\" ";
      if (!gstate.dasharray.empty()) {
        F << "stroke-dasharray=\"";
        unsigned int i = 1;
        for(list<float>::iterator I = gstate.dasharray.begin();
                                  I != gstate.dasharray.end(); I++) {
          F << *I;
          if (i++ < gstate.dasharray.size()) F << ",";
        }
        F << "\" stroke-dashoffset=\"" << gstate.dashoffset << "\" ";
      }
    }
  }
  F << "d=\"";
  for (unsigned i=0; i < segments.size(); i++) {
    switch (segments[i].command) {
      case MP_moveto:
        F << "M" << segments[i].coord[0] << " " << segments[i].coord[1];
        break;
      case MP_lineto:
        F << "L" << segments[i].coord[0] << " " << segments[i].coord[1];
        break;
      case MP_rlineto:
        if (segments[i].coord[0] == 0 && segments[i].coord[1] == 0)
          F << "h0.001";  // rlineto 0 0 used for dot rendering doesn't work in SVG
	else
          F << "l" << segments[i].coord[0] << " " << segments[i].coord[1];
        break;
      case MP_curveto:
        F << "C" << segments[i].coord[0] << " " << segments[i].coord[1] << " "
                 << segments[i].coord[2] << " " << segments[i].coord[3] << " " 
                 << segments[i].coord[4] << " " << segments[i].coord[5];
        break;
    }
  }
  if (closed) F << "Z";
  F << "\" />" << endl;
  if (fillstroke == MP_clip) F << "</clipPath>" << endl << 
     "<g clip-path=\"url(#clip_" << CGS::clippathID << "_" << unique_prefix << ")\">" << endl;
}

void MP_data::add(int i) {
  MP_index ind;
  ind.vector = I_gsave;
  ind.idx = i;
  index.push_back(ind);
}

void MP_data::add(int i, string s) {
  MP_index ind;
  MP_setting set;
  ind.vector = I_setting;
  ind.idx = settings.size();
  float fl;
  
  if (i == MP_dash) {
    s.replace(s.find("["),1,"");
    s.replace(s.find("]"),1,"");
    set.dasharray.clear();
    istringstream ss(s);
    while (ss >> fl) set.dasharray.push_back(fl);
    set.dasharray.pop_back();
    set.dashoffset = fl;
    //set.str = s;
  }
  else if (i == MP_rgb) {
    istringstream ss(s); // prerobit!!!
    ss >> set.data[0];
    ss >> set.data[1];
    ss >> set.data[2];
  }
  else if (i == MP_cmyk) {
    istringstream ss(s);
    ss >> set.data[0];
    ss >> set.data[1];
    ss >> set.data[2];
    ss >> set.data[3];
  }
  else if (i == MP_pattern) {
    set.pattern = s;
  }
  else {
    set.data[0] = atof(s.c_str());
  }
  set.command = i;
  settings.push_back(set);
  index.push_back(ind);
}

void MP_data::add(MP_path P) {
  MP_index ind;
  ind.vector = I_path;
  ind.idx = paths.size();
  paths.push_back(P);
  index.push_back(ind);
}

void MP_data::add(MP_transform T) {
  MP_index ind;
  ind.vector = I_transform;
  ind.idx = transforms.size();
  transforms.push_back(T);
  index.push_back(ind);
}

void MP_data::add(MP_text T) {
  MP_index ind;
  ind.vector = I_text;
  ind.idx = texts.size();
  texts.push_back(T);
  index.push_back(ind);
}

void MP_data::clear() {
  index.clear();
  paths.clear();
  texts.clear();
  settings.clear();
  transforms.clear();
  idx = 0;
}

void MP_setting::print_svg (ofstream & F, CGS & gstate) {
  switch (command) {
    case MP_rgb:
      for (int i=0; i<3; i++) gstate.color[i] = data[i];
      gstate.pattern = "";
      break;
    case MP_gray:
      for (int i=0; i<3; i++) gstate.color[i] = data[0];
      gstate.pattern = "";
      break;
    case MP_cmyk:  // cmyk to rgb conversion necessary in SVG
      for (int i=0; i<3; i++) gstate.color[i] = 1.0 - min(1.0, data[i] + data[3]);
      gstate.pattern = "";
      break;
    case MP_pattern:
      gstate.pattern = pattern;
      break;
    case MP_linejoin:
      gstate.linejoin = int(data[0]);
      break;
    case MP_linecap:
      gstate.linecap = int(data[0]);
      break;
    case MP_miterlimit:
      gstate.miterlimit = data[0];
      break;
    case MP_linewidth:
      gstate.linewidth = data[0];
      break;
    case MP_dash:
      gstate.dasharray = dasharray;
      gstate.dashoffset = dashoffset;
      break;
  }
}

map<int,int> tmpclip;

void MP_data::print_svg (ofstream & F, string unique_prefix) {
//  F << "<g id=\"" << ID <<  // plain MP settings follow
//       "\" stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-miterlimit=\"10\">" << endl;
  for (unsigned int i=0; i<index.size(); i++) {
    switch (index[i].vector) {
      case I_path:
        paths[index[i].idx].print_svg(F,gstate,unique_prefix);
        break;
      case I_setting:
        settings[index[i].idx].print_svg(F,gstate);
        break;
      case I_text:
        texts[index[i].idx].print_svg(F,gstate);
        break;
      case I_gsave:
        switch (index[i].idx) {
          case MP_gsave:
            for (map<int,int>::iterator I = gstate.clippathdepth.begin();
                                        I!= gstate.clippathdepth.end(); I++) 
              I->second++;
            F << "<g>" << endl;
            GSTATE_stack.push_back(gstate);
            break;
          case MP_grestore:
            for (map<int,int>::iterator I = gstate.clippathdepth.begin();
                                        I!= gstate.clippathdepth.end(); I++) {
              I->second--;
              if (I->second < 0) F << "</g>" << endl;
            }
            // nemoze ist do predch. cyklu, lebo zmazanie smernika
            // urobi chaos
            {auto I = gstate.clippathdepth.begin();
              while (I!= gstate.clippathdepth.end()) {
                if (I->second < 0) I = gstate.clippathdepth.erase(I);
                else I++;
              }
            }
            tmpclip = gstate.clippathdepth;
            gstate = GSTATE_stack.back();
            gstate.clippathdepth = tmpclip;
            GSTATE_stack.pop_back();
            F << "</g>" << endl;
            break;
          case MP_transp_on:
            
            break;
          case MP_transp_off:
            
            break;
        }
        break;
    }
  }
//  F << "</g>" << endl;
  
  assert(gstate.clippathdepth.empty());
}

void converted_data::print_svg (ofstream & F, string unique_prefix) { 
  ostringstream s;
  static long i_patt_def(10000);
  s << ++i_patt_def;   // i_patt maju byt rozne
  unique_prefix += "_";
  unique_prefix += s.str();

  F << "<svg width=\"" << 2.54/72*(urx - llx) << 
      "cm\" height=\"" << 2.54/72*(ury - lly) << 
      "cm\" viewBox=\"" << llx << " " << -ury << 
      " " << urx-llx << " " << ury-lly << 
      "\" xmlns=\"http://www.w3.org/2000/svg\" " << 
      "xmlns:xlink=\"http://www.w3.org/1999/xlink\">" << endl;
  F << "<defs>" << endl;
  //patterns
  if (!patterns.empty()) {
    for (list<pattern>::iterator J = PATTERNLIST.begin();
                                J != PATTERNLIST.end(); J++) {
    if (patterns.count(J->name) > 0) {
        F << "<pattern id=\"patt_" << J->name <<  "_" << unique_prefix <<
            "\" patternUnits=\"userSpaceOnUse\"" << 
            " width=\"" << J->xstep <<   
            "\" height=\"" << J->ystep << 
            "\" patternTransform=\"matrix(" << J->xx << " " << J->xy << " " 
                                            << J->yx << " " << J->yy << " " 
                                            << J->x <<  " " << J->y  << 
            ")\">" << endl;
        F << "<g transform=\"translate(" 
                      << J->llx1-J->llx << " " << J->lly1-J->lly << ")\">" << endl;
        J->data.MP.print_svg(F,unique_prefix);
        F << "</g>" << endl;
        F << "</pattern>" << endl;
      }
    }
  }
  // clip to initial viewBox
  F << "<clipPath id=\"clip_viewBox_" << unique_prefix << "\">" << endl;
  F << "<path d=\"M" << llx << " " << lly << 
      "L" << urx << " " << lly << 
      "L" << urx << " " << ury << 
      "L" << llx << " " << ury << "z\" />" << endl;
  F << "</clipPath>" << endl;
  
  F << "</defs>" << endl;
  // --- end of definitions ---
  F << "<g transform=\"scale(1,-1)\" fill=\"#000000\" stroke=\"#000000\" stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-miterlimit=\"10\" fill-rule=\"evenodd\" clip-rule=\"evenodd\" clip-path=\"url(#clip_viewBox_" << unique_prefix << ")\">" << endl;
  MP.print_svg(F,unique_prefix);
  F << "</g>" << endl;
  F << "</svg>" << endl;
}

void converted_data::clear() {
  MP.clear();
  fonts.clear();
}

converted_data::converted_data() {
  clear();
}


string process_pdf_string2(string s, string font) {
  string r,t;
  unsigned char c;
  char *err;
  unsigned j;
  map<string,FONTCHARS>::iterator I; 

  I = USED_CHARS.find(font);
  assert (I != USED_CHARS.end());
  s = s.substr(1,s.length()-3);  // delete surrounding parentheses and final space
  for (unsigned i=0; i<s.size(); i++) {
    c = s[i];
    if (c == 92) {
      i++;
      c = s[i];
      if (c == 92 || c == 40 || c == 41) {     // escape sequences \\, \(, \)
        r += c;
      }
      else if (c>=48 && c<=57) {
        j = i+1;
        t = c;
        while((c=s[j])>=48 && c<=57 && j<i+3) {   // octal numbers
          t += s[j];
          j++;
        }
        i = j-1;
        c = strtol(t.c_str(),&err,8);
        r += c;
      }
      else i--;                  // otherwise backslash is ignored
    }
    else {
      r += c;
    }
  }
//  char ch[10];
  t = "";
  for (unsigned i=0; i<r.size(); i++) {
    c = r[i];
    if (((*I).second).find(c) == ((*I).second).end()) {
      ((*I).second).insert(c);
    }
//    sprintf(ch,"%02x",c);
//    t += ch;
  }
//  return "<" + t + ">";
  return r;
}



void parse_eps(string fname, string cname, double dx, double dy, 
               double & c1, double & c2, double & c3, double & c4, 
               converted_data & data, double R, double G, double B) {
  string tok, buffer;
  string font, patt;
  string pattcolor = "0 0 0";
  bool comment = true, concat = false, 
       already_transp = false, transp_used = false, before_group_transp = false;
  double llx = 0, lly = 0, urx = 0, ury = 0, HS = 0.0, VS = 0.0;
  deque<string> thbuffer;
  set<string> FORM_FONTS, FORM_PATTERNS;
  bool inpath = false, gsaveinpath = false;
  
  MP_path mp_path;
  MP_transform mp_trans, fntmatr;
  MP_text text;

  data.clear();

  ifstream F(fname.c_str());
  if(!F) therror((IOerr(fname)));
  while(F >> tok) {
    if (comment) {                      // File header
      if (tok == "%%BoundingBox:") {
        F >> llx >> lly >> urx >> ury;

        c1 = llx+dx;  // bbox pre absolutnu polohu 
        c2 = lly+dy;
        c3 = urx+dx;
        c4 = ury+dy;

	HS = urx - llx;
	VS = ury - lly;

        data.llx = 0;  // skutocny bbox 
        data.lly = 0;
        data.urx = HS;
        data.ury = VS;

	if (cname != "") { // beginning of boundary cl.path definition
                           // for F and G scraps
          data.MP.add(MP_gsave);
          ifstream G(cname.c_str());
          if(!G) therror((IOerr(cname)));
          mp_path.clear();
          while(G >> buffer) {
            if (buffer == "m") {
              mp_path.add(MP_moveto, thbuffer[0],thbuffer[1],llx,lly);
              thbuffer.clear();
            }
            else if (buffer == "c") {
              mp_path.add(MP_curveto, thbuffer[0], thbuffer[1], thbuffer[2], 
                          thbuffer[3], thbuffer[4], thbuffer[5],llx,lly);
              thbuffer.clear();
            }
            else if (buffer == "l") {
              mp_path.add(MP_lineto, thbuffer[0],thbuffer[1],llx,lly);
              thbuffer.clear();
            }
            else {
              thbuffer.push_back(buffer);
            }
          }
          mp_path.fillstroke = MP_clip;
          data.MP.add(mp_path);
          thbuffer.clear();
          G.close();
	}
      }
      else if (tok == "%%Page:") {
        F >> tok; F >> tok;
        comment = false;
      }
    }
    else {                              // PostScript commands
      if (tok == "showpage") {
        break;
      }
      else if (tok == "moveto") {
        if (inpath)
          mp_path.add(MP_moveto, thbuffer[0],thbuffer[1],llx,lly);
        else if (!concat) {
          fntmatr.transf[0] = 1;
          fntmatr.transf[1] = 0;        
          fntmatr.transf[2] = 0;          
          fntmatr.transf[3] = 1;          
          fntmatr.transf[4] = atof(thbuffer[0].c_str())-llx;
          fntmatr.transf[5] = atof(thbuffer[1].c_str())-lly;
        }
        thbuffer.clear();
      }
      else if (tok == "curveto") {
        mp_path.add(MP_curveto, thbuffer[0], thbuffer[1], thbuffer[2], 
                    thbuffer[3], thbuffer[4], thbuffer[5],llx,lly);
        thbuffer.clear();
      }
      else if (tok == "lineto") {
        mp_path.add(MP_lineto, thbuffer[0],thbuffer[1],llx,lly);
        thbuffer.clear();
      }
      else if (tok == "rlineto") {
        mp_path.add(MP_rlineto, thbuffer[0],thbuffer[1],0,0);
        thbuffer.clear();
      }
      else if (tok == "newpath") {
        inpath = true;
        mp_path.clear();
        thbuffer.clear();
      }
      else if (tok == "closepath") {
        mp_path.closed = true;
        thbuffer.clear();
      }
      else if (tok == "fill") {
        if (!gsaveinpath) {
          mp_path.fillstroke = MP_fill;
          data.MP.add(mp_path);
          inpath=false;
        }
        thbuffer.clear();
      }
      else if (tok == "stroke") {
        if (!gsaveinpath) mp_path.fillstroke = MP_stroke;
        else {
          mp_path.fillstroke = MP_fillstroke;
          gsaveinpath = false;
        }
        data.MP.add(mp_path);
        inpath=false;
        thbuffer.clear();
      }
      else if (tok == "clip") {
        mp_path.fillstroke = MP_clip;
        data.MP.add(mp_path);
        inpath=false;
        thbuffer.clear();
      }
      else if (tok == "setlinejoin") {
        data.MP.add(MP_linejoin, thbuffer[0]);
        thbuffer.clear();
      }
      else if (tok == "setlinecap") {
        data.MP.add(MP_linecap, thbuffer[0]);
        thbuffer.clear();
      }
      else if (tok == "setmiterlimit") {
        data.MP.add(MP_miterlimit, thbuffer[0]);
        thbuffer.clear();
      }
      else if (tok == "setgray") {
        if (already_transp) {  // transp off
          data.MP.add(MP_transp_off);
          already_transp = false;
        }
        data.MP.add(MP_gray, thbuffer[0]);
        thbuffer.clear();
      }
      else if (tok == "setrgbcolor") {
        if ((!((thbuffer[0] == "0.00002") && (thbuffer[1] == "0.00018"))) 
              && already_transp) {           // transp off
          data.MP.add(MP_transp_off);
          already_transp = false;
        };
        if (thbuffer[0] == "0.00002") {        // special commands
          if (thbuffer[1] == "0.00015") {          // patterns
            patt = thbuffer[2];
            if (FORM_PATTERNS.find(patt) == FORM_PATTERNS.end()) {
              FORM_PATTERNS.insert(patt);
            }
            if (ALL_PATTERNS.find(patt) == ALL_PATTERNS.end()) {
              ALL_PATTERNS.insert(make_pair(patt,u2str(patt_id)));
              patt_id++;
            }
            data.MP.add(MP_pattern, patt);
          }
          else if (thbuffer[1] == "0.00018") {     // transparency
            transp_used = true;
            if (!already_transp) {
              data.MP.add(MP_transp_on);
              already_transp = true;
            }
            map<string,string>::iterator I = RGB.find(thbuffer[2]);
            if (I != RGB.end()) {
              data.MP.add(MP_rgb, I->second);
            } else cerr << "Unknown color!" << endl;
          }
          else cerr << "Unknown special!" << endl;
	}
	else {                               // regular RGB color
          data.MP.add(MP_rgb, thbuffer[0]+" "+thbuffer[1]+" "+thbuffer[2]);
	}
        thbuffer.clear();
      }
      else if (tok == "setcmykcolor") {
        if (already_transp) {  // transp off
          data.MP.add(MP_transp_off);
          already_transp = false;
        }
        data.MP.add(MP_cmyk, thbuffer[0]+" "+thbuffer[1]+" "+thbuffer[2]+" "+thbuffer[3]);
        thbuffer.clear();
      }
      else if (tok == "setdash") {
        buffer = "";
        for(unsigned i=0; i<thbuffer.size(); i++) {
	  buffer = buffer + thbuffer[i] + " ";
	}
        data.MP.add(MP_dash, buffer);
        thbuffer.clear();
      }
      else if (tok == "setlinewidth") {
        if(thbuffer[0] != "0") {
	  buffer = thbuffer[0];
	}
	else {
	  buffer = thbuffer[1];
	  F >> tok; // redundant pop
	}
        data.MP.add(MP_linewidth, buffer);
        thbuffer.clear();
      }
      else if (tok == "gsave") {
        if (!inpath) data.MP.add(MP_gsave);
        else gsaveinpath = true;
        thbuffer.clear();
        if (already_transp) before_group_transp = true;
        else before_group_transp = false;
      }
      else if (tok == "grestore") {
        if (!inpath) data.MP.add(MP_grestore);
        thbuffer.clear();
        if (before_group_transp) already_transp = true;
        else already_transp = false;
      }
      else if (tok == "translate") {
        mp_trans.set(MP_translate,thbuffer[0],thbuffer[1],llx,lly);
        data.MP.add(mp_trans);
        thbuffer.clear();
      }
      else if (tok == "scale") {
        mp_trans.set(MP_scale,thbuffer[0],thbuffer[1],0,0);
        if (!inpath) data.MP.add(mp_trans);
        else mp_path.transformation = mp_trans;
        thbuffer.clear();
      }
      else if (tok == "concat") {     
        if (thbuffer[0] != "[") {  // opening bracket
          thbuffer[0].erase(0,1);
        }
        else {
          thbuffer.pop_front();
        }
        mp_trans.set(MP_concat,thbuffer[0],thbuffer[1],thbuffer[2],
                     thbuffer[3],thbuffer[4],thbuffer[5],llx,lly);
        if (!inpath) {
          fntmatr = mp_trans;
          concat = true;
        }
        else mp_path.transformation = mp_trans;
        thbuffer.clear();
      }
      
      // text conversion should be
      // A B moveto (C) D E fshow
      // -> 
      // BT /Fiii E Tf 1 0 0 1 A B Tm (C) Tj ET
      // or
      // gsave [A1 A2 A3 A4 A5 A6 ] concat 0 0 moveto (C) D E fshow grestore
      // ->
      // BT /Fiii E Tf A1 A2 A3 A4 A5 A6 Tm (C) Tj ET
      // 
      // currently we leave moveto, gsave, grestore unchanged;
      // path started with moveto is terminated with the `n' operator
      
      else if (tok == "fshow") {            // font changes should be optimized
        text.clear();
        unsigned i = thbuffer.size();
        font = thbuffer[i-2];
        text.font = font;
        text.size = atof(thbuffer[i-1].c_str());
        if (FORM_FONTS.count(font) == 0) {
          FORM_FONTS.insert(font);
        }
        if (ALL_FONTS.count(font) == 0) {
          ALL_FONTS.insert(make_pair(font,u2str(font_id)));
          font_id++;
        }
//        font = tex_Fname(ALL_FONTS[font]);
        if (USED_CHARS.count(font) == 0) {
          FONTCHARS FCH;
          USED_CHARS.insert(make_pair(font,FCH));
        }
        buffer = "";
        for (unsigned j=0; j<i-2; j++) {
          buffer = buffer + thbuffer[j] + " ";
        }
        text.text = process_pdf_string2(buffer,font);
        text.xx = fntmatr.transf[0];
        text.xy = fntmatr.transf[1];
        text.yx = fntmatr.transf[2];
        text.yy = fntmatr.transf[3];
        text.x = fntmatr.transf[4];
        text.y = fntmatr.transf[5];
	text.r = R;
	text.g = G;
	text.b = B;
        concat = false;
        data.MP.add(text);
        thbuffer.clear();
      }
      else if (tok == "THsetpatterncolor") {  // currently unused as it is not completely trivial to implement uncolored patterns in SVG
        pattcolor = thbuffer[0] + " " + thbuffer[1] + " " + thbuffer[2];
        thbuffer.clear();
      }
      else {
        thbuffer.push_back(tok);
      }
    }
  }  // end of while loop
  F.close();
  if (cname != "") { // end of boundary cl.path
    data.MP.add(MP_grestore);
  }
  data.fonts = FORM_FONTS;
  data.patterns = FORM_PATTERNS;
  if (transp_used) data.transparency = true;
  else data.transparency = false;
}

void convert_scraps_new() {
  
  for(list<scraprecord>::iterator I = SCRAPLIST.begin(); 
                                  I != SCRAPLIST.end(); I++) {
    if (I->F != "") parse_eps(I->F, I->C, I->S1, I->S2, I->F1, I->F2, I->F3, I->F4, I->Fc, I->r, I->g, I->b);
    if (I->G != "") parse_eps(I->G, I->C, I->S1, I->S2, I->G1, I->G2, I->G3, I->G4, I->Gc);
    if (I->B != "") parse_eps(I->B, "", I->S1, I->S2, I->B1, I->B2, I->B3, I->B4, I->Bc);
    if (I->I != "") parse_eps(I->I, "", I->S1, I->S2, I->I1, I->I2, I->I3, I->I4, I->Ic);
    if (I->E != "") parse_eps(I->E, "", I->S1, I->S2, I->E1, I->E2, I->E3, I->E4, I->Ec, I->r, I->g, I->b);
    if (I->X != "") parse_eps(I->X, "", I->S1, I->S2, I->X1, I->X2, I->X3, I->X4, I->Xc, I->r, I->g, I->b);
  }

  for(list<legendrecord>::iterator I = LEGENDLIST.begin(); 
                                   I != LEGENDLIST.end(); I++) {
    double a,b,c,d;
    if (I->fname != "") parse_eps(I->fname, "",0,0,a,b,c,d,I->ldata);
  }
  
  if (LAYOUT.northarrow != "") {
    double a, b, c, d;
    parse_eps(LAYOUT.northarrow, "",0,0,a,b,c,d,NArrow);
  }
  if (LAYOUT.scalebar != "") {
    double a, b, c, d;
    parse_eps(LAYOUT.scalebar, "",0,0,a,b,c,d,ScBar);
  }

  GRIDLIST.clear();
  if (LAYOUT.grid > 0) {
    converted_data scr;
    double a,b,c,d;
    parse_eps(LAYOUT.gridAA, "",0,0,a,b,c,d,scr); GRIDLIST.push_back(scr);scr.clear();
    LAYOUT.gridcell[0].x = a;
    LAYOUT.gridcell[0].y = b;
    parse_eps(LAYOUT.gridAB, "",0,0,a,b,c,d,scr); GRIDLIST.push_back(scr);scr.clear();
    LAYOUT.gridcell[1].x = a;
    LAYOUT.gridcell[1].y = b;
    parse_eps(LAYOUT.gridAC, "",0,0,a,b,c,d,scr); GRIDLIST.push_back(scr);scr.clear();
    LAYOUT.gridcell[2].x = a;
    LAYOUT.gridcell[2].y = b;
    parse_eps(LAYOUT.gridBA, "",0,0,a,b,c,d,scr); GRIDLIST.push_back(scr);scr.clear();
    LAYOUT.gridcell[3].x = a;
    LAYOUT.gridcell[3].y = b;
    parse_eps(LAYOUT.gridBB, "",0,0,a,b,c,d,scr); GRIDLIST.push_back(scr);scr.clear();
    LAYOUT.gridcell[4].x = a;
    LAYOUT.gridcell[4].y = b;
    parse_eps(LAYOUT.gridBC, "",0,0,a,b,c,d,scr); GRIDLIST.push_back(scr);scr.clear();
    LAYOUT.gridcell[5].x = a;
    LAYOUT.gridcell[5].y = b;
    parse_eps(LAYOUT.gridCA, "",0,0,a,b,c,d,scr); GRIDLIST.push_back(scr);scr.clear();
    LAYOUT.gridcell[6].x = a;
    LAYOUT.gridcell[6].y = b;
    parse_eps(LAYOUT.gridCB, "",0,0,a,b,c,d,scr); GRIDLIST.push_back(scr);scr.clear();
    LAYOUT.gridcell[7].x = a;
    LAYOUT.gridcell[7].y = b;
    parse_eps(LAYOUT.gridCC, "",0,0,a,b,c,d,scr); GRIDLIST.push_back(scr);scr.clear();
    LAYOUT.gridcell[8].x = a;
    LAYOUT.gridcell[8].y = b;
  }

  PATTERNLIST.clear();

  ifstream P("patterns.dat");
  if(!P) therror(("Can't open patterns definition file!"));
  char buf[500];
  char delim[] = ":";
  string line,num,pfile,bbox,xstep,ystep,matr;
  while(P.getline(buf,500,'\n')) {
    num = strtok(buf,delim);
    pfile = strtok(NULL,delim);
    bbox = strtok(NULL,delim);
    xstep = strtok(NULL,delim);
    ystep = strtok(NULL,delim);
    matr = strtok(NULL,delim);
//    if (ALL_PATTERNS.count(num) > 0) {  // changed to patt.used flag
                                          // because thsymbolset.cxx 
                                          // calls eps_parse after
                                          // this function is called
                                          // and patterns referenced
                                          // there would be missing in this list
      pattern patt;
      patt.used = (ALL_PATTERNS.count(num) > 0);
      patt.name = num;

      matr.replace(matr.find("["),1,"");
      matr.replace(matr.find("]"),1,"");
      istringstream s1(matr);
      s1 >> patt.xx >> patt.xy >> patt.yx >> patt.yy >> patt.x >> patt.y;
      bbox.replace(bbox.find("["),1,"");
      bbox.replace(bbox.find("]"),1,"");
      istringstream s2(bbox);
      s2 >> patt.llx >> patt.lly >> patt.urx >> patt.ury;
//      F << "/Matrix " << matr << endl;
//      F << "/BBox " << bbox << endl;
      patt.xstep = atof(xstep.c_str());
      patt.ystep = atof(ystep.c_str());

      parse_eps(pfile , "", 0,0, patt.llx1,patt.lly1,patt.urx1,patt.ury1,patt.data);
      PATTERNLIST.push_back(patt);
//    }  // patt.used
  }
  P.close();
 
  
}



int thconvert_new() { 

  thprintf("converting scraps* ... ");
  
  RGB.clear();
  ALL_FONTS.clear();
  ALL_PATTERNS.clear();
  USED_CHARS.clear();
  PATTERNLIST.clear();
  GRIDLIST.clear();
  font_id = 1;
  patt_id = 1;

  read_rgb();
  convert_scraps_new();

//  thpdfdbg();  // in the debugging mode only
  
  thprintf("done\n");
  return(0);
}


