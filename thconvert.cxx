/**
 * @file thconvert.cxx
 */
  
/* Copyright (C) 2003 Martin Budaj
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

#include "thpdfdbg.h"
#include "thpdfdata.h"
#include "thtexfonts.h"

using namespace std;

// extern list<scraprecord> SCRAPLIST;

#define IOerr(F) ((string)"Can't open file "+F+"!\n").c_str()

map<string,string> RGB, ALL_FONTS, ALL_PATTERNS;
typedef set<unsigned char> FONTCHARS;
map<string,FONTCHARS> USED_CHARS;

unsigned font_id, patt_id;
int convert_mode;

////////////////////////////////////////////////////////////////////////////////

void read_hash() {
  ifstream F("data.pl");
  if(!F) therror((IOerr("data.pl")));
  string line, tmp = "";
  char buf[100];
  scraprecord S;
  list<scraprecord>::iterator I;
  while(F.getline(buf,100,'\n')) {
    line = buf;
    if (line.find(';') != string::npos) break;
    if (line.find(" => {") != string::npos) {
      tmp = line.substr(line.find_first_not_of('\t'),7);
      S.name = tmp;
      SCRAPLIST.push_front(S);
      I = SCRAPLIST.begin();
    }
    else if (line.find("F => ") != string::npos) {
      tmp = line.substr(line.find_first_of('"')+1,
                          line.find_last_of('"')-line.find_first_of('"')-1);
      I->F = tmp;
    }
    else if (line.find("B => ") != string::npos) {
      tmp = line.substr(line.find_first_of('"')+1,
                          line.find_last_of('"')-line.find_first_of('"')-1);
      I->B = tmp;
    }
    else if (line.find("I => ") != string::npos) {
      tmp = line.substr(line.find_first_of('"')+1,
                          line.find_last_of('"')-line.find_first_of('"')-1);
      I->I = tmp;
    }
    else if (line.find("E => ") != string::npos) {
      tmp = line.substr(line.find_first_of('"')+1,
                          line.find_last_of('"')-line.find_first_of('"')-1);
      I->E = tmp;
    }
    else if (line.find("X => ") != string::npos) {
      tmp = line.substr(line.find_first_of('"')+1,
                          line.find_last_of('"')-line.find_first_of('"')-1);
      I->X = tmp;
    }
    else if (line.find("G => ") != string::npos) {
      tmp = line.substr(line.find_first_of('"')+1,
                          line.find_last_of('"')-line.find_first_of('"')-1);
      I->G = tmp;
    }
    else if (line.find("C => ") != string::npos) {
      tmp = line.substr(line.find_first_of('"')+1,
                          line.find_last_of('"')-line.find_first_of('"')-1);
      I->C = tmp;
    }
    else if (line.find("P => ") != string::npos) {
      tmp = line.substr(line.find_first_of('"')+1,
                          line.find_last_of('"')-line.find_first_of('"')-1);
      I->P = tmp;
    }
    else if (line.find("S => ") != string::npos) {
      tmp = line.substr(line.find_first_of('"')+1,
                          line.find_last_of('"')-line.find_first_of('"')-1);
      tmp = tmp.substr(0,tmp.find_first_of(' '));
      I->S1 = atof(tmp.c_str());
      tmp = line.substr(line.find_first_of('"')+1,
                          line.find_last_of('"')-line.find_first_of('"')-1);
      tmp = tmp.substr(tmp.find_first_of(' ')+1,
                              tmp.size()-tmp.find_first_of(' ')-1);
      I->S2 = atof(tmp.c_str());
    }
    else if (line.find("Y => ") != string::npos) {
      tmp = line.substr(line.find_first_of('>')+2,
                          line.find_last_of(',')-line.find_first_of('>')-2);
      I->layer = atol(tmp.c_str());
    }
    else if (line.find("V => ") != string::npos) {
      tmp = line.substr(line.find_first_of('>')+2,
                          line.find_last_of(',')-line.find_first_of('>')-2);
      I->level = atol(tmp.c_str());
    }
    else if (line.find("Z => ") != string::npos) {
      I->sect = 1;
    }
  }
  F.close();
  SCRAPLIST.reverse();
}


string tex_Fname(string s) {return("THF"+s);}
string tex_Pname(string s) {return("THP"+s);}
string tex_Lname(string s) {return("THL"+s);}

list<scraprecord>::iterator find_scrap(string name) {
    list<scraprecord>::iterator I;
    for (I = SCRAPLIST.begin(); I != SCRAPLIST.end(); I++) {
      if (I->name == name) break;
    }
    if (I == SCRAPLIST.end()) {
      cerr << "This can't happen!" << endl;
    }
    return (I);
}

void print_queue(deque<string>& thstack, double llx, double lly, 
                string command, ofstream& TEX) {
  if (convert_mode>0) {TEX << "\\PL{";}
  for(unsigned i=0; i<thstack.size(); i=i+2) {
    TEX << atof(thstack[i].c_str())-llx << " " << 
           atof(thstack[i+1].c_str())-lly << " ";
  }
  TEX << command;
  if (convert_mode>0) {TEX << "}%";}
  TEX << endl;
}

void print_str(string str, ofstream& TEX) {
  if (convert_mode>0) {TEX << "\\PL{";}
  TEX << str;
  if (convert_mode>0) {TEX << "}%";}
  TEX << endl;
}


string process_pdf_string(string s, string font) {
  string r,t;
  unsigned char c;
  char *err;
  unsigned j;
  map<string,FONTCHARS>::iterator I; 

  I = USED_CHARS.find(font);
  if (I == USED_CHARS.end()) cerr << "This can't happen!";
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
  char ch[10];
  t = "";
  for (unsigned i=0; i<r.size(); i++) {
    c = r[i];
    if (((*I).second).find(c) == ((*I).second).end()) {
      ((*I).second).insert(c);
    }
    sprintf(ch,"%02x",c);
    t += ch;
  }
  return "<" + t + ">";
}


// convert modes:  0 -- patterns
//                10 -- scrap content which shoul be clipped
//                11 -- background
//                12 -- outline
//                13 -- filled outline
//                20 -- nonclipped scrap data
//                30 -- legend
//                31 -- northarrow, scalebar

void distill_eps(string name, string fname, string cname, int mode, ofstream& TEX, double r = -1, double g = -1, double b = -1) {
  string form_id;
  string tok, lastmovex, lastmovey, buffer;
  string font, patt, fntmatr;
  string pattcolor = "0 0 0";
  bool comment = true, concat = false, 
       already_transp = false, transp_used = false, before_group_transp = false;
  double llx = 0, lly = 0, urx = 0, ury = 0, HS = 0.0, VS = 0.0;
  double dx, dy;
  char x[20],y[20];
  deque<string> thstack;
  set<string> FORM_FONTS, FORM_PATTERNS;
  list<scraprecord>::iterator J;
  
  convert_mode = mode;
  
  ostringstream text_attr;
  if (LAYOUT.colored_text && r >= 0 && g >= 0 && b >= 0) {
    text_attr << "0.1 w " << r << " " << g << " " << b << " rg 2 Tr ";
  };

  ifstream F(fname.c_str());
  if(!F) therror((IOerr(fname)));
  while(F >> tok) {
    if (comment) {                      // File header
      if ((tok == "%%BoundingBox:") && (mode > 0)) {
        F >> llx >> lly >> urx >> ury;
	
        if ((mode>0) && (mode<30)) {
          J = find_scrap(name);
          dx = J->S1;
          dy = J->S2;
          if (J->name == "") J->name = name;
          if (mode == 10) {
            J->F1 = llx+dx;
            J->F2 = lly+dy;
            J->F3 = urx+dx;
            J->F4 = ury+dy;
            form_id = tex_Xname(name);
          }
          else if (mode == 11) {
            J->G1 = llx+dx;
            J->G2 = lly+dy;
            J->G3 = urx+dx;
            J->G4 = ury+dy;
            form_id = tex_Xname("G"+name);
          }
          else if (mode == 12) {
            J->B1 = llx+dx;
            J->B2 = lly+dy;
            J->B3 = urx+dx;
            J->B4 = ury+dy;
            form_id = tex_Xname("B"+name);
          }
          else if (mode == 13) {
            J->I1 = llx+dx;
            J->I2 = lly+dy;
            J->I3 = urx+dx;
            J->I4 = ury+dy;
            form_id = tex_Xname("I"+name);
          }
          else if (mode == 14) {
            J->E1 = llx+dx;
            J->E2 = lly+dy;
            J->E3 = urx+dx;
            J->E4 = ury+dy;
            form_id = tex_Xname("E"+name);
          }
          else if (mode == 20) {
            J->X1 = llx+dx;
            J->X2 = lly+dy;
            J->X3 = urx+dx;
            J->X4 = ury+dy;
            form_id = tex_Xname("X"+name);
          }
          else cerr << "Unknown mode!" << endl; 
        }
        else if (mode == 30) {
          form_id = tex_Lname(name);
        }
        else if (mode == 31) {
          form_id = tex_Wname(name);
        }
        else if (mode > 100 && mode < 110) {
          form_id = tex_Wname(name);
	  LAYOUT.gridcell[mode - 101].x = llx;
	  LAYOUT.gridcell[mode - 101].y = lly;
        }
        
	HS = urx - llx;
	VS = ury - lly;
        TEX << "%\n\\setbox\\xxx=\\hbox{\\vbox to" << VS << "bp{\\vfill" << endl;
	if ((mode <= 11) && (cname != "")) { // beginning of boundary cl.path definition
          TEX << "\\PL{q}";          // for F and G scraps
          ifstream G(cname.c_str());
          if(!G) therror((IOerr(cname)));
          while(G >> buffer) {
            if ((buffer == "m") || (buffer == "l") || (buffer == "c")) {
              print_queue(thstack,llx,lly,buffer,TEX);
              thstack.clear();
            }
            else {
              thstack.push_back(buffer);
            }
          }
          G.close();
          thstack.clear();
          TEX << "\\PL{W* n}";  // end of boundary clipping path definition
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
        lastmovex = thstack[0];
        lastmovey = thstack[1];
        print_queue(thstack,llx,lly,"m",TEX);
        thstack.clear();
      }
      else if (tok == "curveto") {
        print_queue(thstack,llx,lly,"c",TEX);
        thstack.clear();
      }
      else if (tok == "lineto") {
        print_queue(thstack,llx,lly,"l",TEX);
        thstack.clear();
      }
      else if (tok == "rlineto") {
        thstack.clear();
        thstack.push_back(lastmovex);
        thstack.push_back(lastmovey);
        print_queue(thstack,llx,lly,"l",TEX);
        thstack.clear();
      }
      else if (tok == "newpath") {
        thstack.clear();
      }
      else if (tok == "closepath") {
        print_str("h",TEX);
        thstack.clear();
      }
      else if (tok == "fill") {
        print_str("f*",TEX);
        thstack.clear();
      }
      else if (tok == "stroke") {
        print_str("S",TEX);
        thstack.clear();
      }
      else if (tok == "clip") {
        print_str("W* n",TEX);
        thstack.clear();
      }
      else if (tok == "setlinejoin") {
        print_str(thstack[0]+" j",TEX);
        thstack.clear();
      }
      else if (tok == "setlinecap") {
        print_str(thstack[0]+" J",TEX);
        thstack.clear();
      }
      else if (tok == "setmiterlimit") {
        print_str(thstack[0]+" M",TEX);
        thstack.clear();
      }
      else if (tok == "setgray") {
        if (mode == 0) {
          thstack.clear();
          continue;            // ignore color for uncolored patterns
        }
        if (already_transp) {  // transp off
          print_str("/GS0 gs",TEX);
          already_transp = false;
        }
        print_str(thstack[0]+" g "+thstack[0]+" G",TEX);
        thstack.clear();
      }
      else if (tok == "setrgbcolor") {
        if (mode == 0) {
          thstack.clear();
          continue;            // ignore color for uncolored patterns
        }
        if ((!((thstack[0] == "0.00002") && (thstack[1] == "0.00018"))) 
              && already_transp) {           // transp off
          print_str("/GS0 gs",TEX);
          already_transp = false;
        };
        if (thstack[0] == "0.00002") {        // special commands
          if (thstack[1] == "0.00015") {          // patterns
            patt = thstack[2];
            if (FORM_PATTERNS.find(patt) == FORM_PATTERNS.end()) {
              FORM_PATTERNS.insert(patt);
            }
            if (ALL_PATTERNS.find(patt) == ALL_PATTERNS.end()) {
              ALL_PATTERNS.insert(make_pair(patt,u2str(patt_id)));
              patt_id++;
            }
            print_str("/CS1 cs "+pattcolor+" /"+patt+" scn",TEX);
          }
          else if (thstack[1] == "0.00018") {     // transparency
            transp_used = true;
            if (!already_transp) {
              print_str("/GS1 gs",TEX);
              already_transp = true;
            }
            map<string,string>::iterator I = RGB.find(thstack[2]);
            if (I != RGB.end()) {
              print_str((*I).second+" rg "+(*I).second+" RG",TEX);
            } else cerr << "Unknown color!" << endl;
          }
          else cerr << "Unknown special!" << endl;
	}
	else {                               // regular RGB color
          print_str(thstack[0]+" "+thstack[1]+" "+thstack[2]+" rg "
                   +thstack[0]+" "+thstack[1]+" "+thstack[2]+" RG",TEX);
	}
        thstack.clear();
      }
      else if (tok == "setcmykcolor") {
        if (mode == 0) {
          thstack.clear();
          continue;            // ignore color for uncolored patterns
        }
        if (already_transp) {           // transp off
          print_str("/GS0 gs",TEX);
          already_transp = false;
        }
        print_str(thstack[0]+" "+thstack[1]+" "+thstack[2]+" "+thstack[3]+" k "
                 +thstack[0]+" "+thstack[1]+" "+thstack[2]+" "+thstack[3]+" K",TEX);
        thstack.clear();
      }
      else if (tok == "setdash") {
        buffer = "";
        for(unsigned i=0; i<thstack.size(); i++) {
	  buffer = buffer + thstack[i] + " ";
	}
	print_str(buffer+"d",TEX);
        thstack.clear();
      }
      else if (tok == "setlinewidth") {
        if(thstack[0] != "0") {
	  buffer = thstack[0];
	}
	else {
	  buffer = thstack[1];
	  F >> tok; // redundant pop
	}
	print_str(buffer+" w",TEX);
        thstack.clear();
      }
      else if (tok == "gsave") {
        print_str("q",TEX);
        thstack.clear();
        if (already_transp) before_group_transp = true;
        else before_group_transp = false;
      }
      else if (tok == "grestore") {
        print_str("Q",TEX);
        thstack.clear();
        if (before_group_transp) already_transp = true;
        else already_transp = false;
      }
      else if (tok == "translate") {
        print_str("1 0 0 1 "+thstack[0]+" "+thstack[1]+" cm",TEX);
        thstack.clear();
      }
      else if (tok == "scale") {
        print_str(thstack[0]+" 0 0 "+thstack[1]+" 0 0 cm",TEX);
        thstack.clear();
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
        unsigned i = thstack.size();
        font = thstack[i-2];
        if (FORM_FONTS.find(font) == FORM_FONTS.end()) {
          FORM_FONTS.insert(font);
        }
        if (ALL_FONTS.find(font) == ALL_FONTS.end()) {
          ALL_FONTS.insert(make_pair(font,u2str(font_id)));
          font_id++;
        }
        font = tex_Fname(ALL_FONTS[font]);
        if (USED_CHARS.find(font) == USED_CHARS.end()) {
          FONTCHARS FCH;
          USED_CHARS.insert(make_pair(font,FCH));
        }
        buffer = "";
        for (unsigned j=0; j<i-2; j++) {
          buffer = buffer + thstack[j] + " ";
        }
        buffer = process_pdf_string(buffer,font);
        print_str("n BT",TEX); // we end the path started with `x y moveto'
                               // should be done more cleanly
        print_str("/F\\pdffontname\\"+font+"\\space "+thstack[i-1]+" Tf",TEX);
        if (concat) {
          print_str(fntmatr+" Tm",TEX);
        }
        else {
          sprintf(x, "%.1f", atof(lastmovex.c_str())-llx);  // modify this part
          sprintf(y, "%.1f", atof(lastmovey.c_str())-lly);
          print_str("1 0 0 1 "+string(x)+" "+string(y)+" Tm",TEX);
        }
	print_str(text_attr.str(),TEX);
        TEX << "\\PL{" << buffer << " Tj}%" << endl;
        print_str("ET",TEX);
        concat = false;
        thstack.clear();
      }
      else if (tok == "concat") {      // only when applied to texts
        if (thstack[0] != "[") {  // opening bracket
          thstack[0].erase(0,1);
        }
        else {
          thstack.pop_front();
        }
        sprintf(x, "%.1f", atof(thstack[4].c_str())-llx);  // modify this part
        sprintf(y, "%.1f", atof(thstack[5].c_str())-lly);
        fntmatr = thstack[0] + " " + thstack[1] + " " + thstack[2] + 
                  " " + thstack[3] + " " + x + " " + y;
        concat = true;
        thstack.clear();
      }
      else if (tok == "THsetpatterncolor") {
        pattcolor = thstack[0] + " " + thstack[1] + " " + thstack[2];
        thstack.clear();
      }
      else {
        thstack.push_back(tok);
      }
    }
  }  // end of while loop
  F.close();
  if (mode>0) {
    if ((mode <= 11) && (cname != "")) { // end of boundary cl.path
      TEX << "\\PL{Q}%" << endl;
    }
    TEX << "}}\\wd\\xxx=" << HS << "bp" << endl;
    TEX << "\\immediate\\pdfxform";

//    if (mode == 12 || mode == 13) {
//      TEX << " attr {";
//      if (mode == 12) {
//        TEX << "/OC \\the\\ocU\\space 0 R ";
//      }
//      else if (mode == 13) {
//        TEX << "/OC \\the\\ocD\\space 0 R ";
//      }
//      TEX << "} ";
//    }

    if (transp_used || !FORM_FONTS.empty() || !FORM_PATTERNS.empty()) {
      TEX << " resources { /ProcSet [/PDF /Text] ";
      if (transp_used) {
        TEX << "/ExtGState \\the\\resid\\space 0 R ";
      }
      if (!FORM_FONTS.empty()) {
        TEX << "/Font << ";
        for(set<string>::iterator I = FORM_FONTS.begin(); 
                                  I != FORM_FONTS.end(); I++) {
          font = tex_Fname(ALL_FONTS[*I]);
          TEX << "/F\\pdffontname\\" << font << 
                 "\\space\\pdffontobjnum\\" << font << "\\space 0 R ";
        }
        TEX << ">> ";
      }
      if (!FORM_PATTERNS.empty()) {
        TEX << "/Pattern << ";
        for(set<string>::iterator I = FORM_PATTERNS.begin(); 
                                    I != FORM_PATTERNS.end(); I++) {
          TEX << "/" << *I << " \\the\\" << tex_Pname(ALL_PATTERNS[*I]) << 
                 "\\space 0 R ";
        }
        TEX << ">> ";
        TEX << "/ColorSpace << /CS1 [/Pattern /DeviceRGB] >> ";
      }
      TEX << "} ";
    }
    
    TEX << "\\xxx\n\\newcount\\" << form_id <<
           "\\" << form_id << "=\\pdflastxform" << endl;
  }
}


void convert_scraps() {
  unsigned char c;
 
  ofstream TEX("th_formdef.tex");
  if(!TEX) therror((IOerr("th_formdef.tex")));
  TEX.setf(ios::fixed, ios::floatfield);
  TEX.precision(2);
  
  for(list<scraprecord>::iterator I = SCRAPLIST.begin(); 
                                  I != SCRAPLIST.end(); I++) {
//    cout << "*" << flush;
    if (I->F != "") distill_eps(I->name, I->F, I->C, 10, TEX, I->r, I->g, I->b);
    if (I->G != "") distill_eps(I->name, I->G, I->C, 11, TEX);
    if (I->B != "") distill_eps(I->name, I->B, "", 12, TEX);
    if (I->I != "") distill_eps(I->name, I->I, "", 13, TEX);
    if (I->E != "") distill_eps(I->name, I->E, "", 14, TEX, I->r, I->g, I->b);
    if (I->X != "") distill_eps(I->name, I->X, "", 20, TEX, I->r, I->g, I->b);
  }

  // similarly with legend (distill_eps( , , , 30, TEX))
  for(list<legendrecord>::iterator I = LEGENDLIST.begin(); 
                                  I != LEGENDLIST.end(); I++) {
    if (I->fname != "") distill_eps(I->name, I->fname, "", 30, TEX);
  }

  // north arrow &c.
  if (LAYOUT.northarrow != "") 
             distill_eps("northarrow", LAYOUT.northarrow, "", 31, TEX);
  if (LAYOUT.scalebar != "") 
             distill_eps("scalebar", LAYOUT.scalebar, "", 31, TEX);

  if (LAYOUT.grid > 0) {
    distill_eps("grida", LAYOUT.gridAA, "", 101, TEX);
    distill_eps("gridb", LAYOUT.gridAB, "", 102, TEX);
    distill_eps("gridc", LAYOUT.gridAC, "", 103, TEX);
    distill_eps("gridd", LAYOUT.gridBA, "", 104, TEX);
    distill_eps("gride", LAYOUT.gridBB, "", 105, TEX);
    distill_eps("gridf", LAYOUT.gridBC, "", 106, TEX);
    distill_eps("gridg", LAYOUT.gridCA, "", 107, TEX);
    distill_eps("gridh", LAYOUT.gridCB, "", 108, TEX);
    distill_eps("gridi", LAYOUT.gridCC, "", 109, TEX);
  }

  TEX.close();

  ofstream F("th_fontdef.tex");
  if(!F) therror((IOerr("th_fontdef.tex")));
  F.setf(ios::fixed, ios::floatfield);
  F.precision(2);
  for (map<string,string>::iterator I = ALL_FONTS.begin(); 
                                    I != ALL_FONTS.end(); I++) {
    F << "\\font\\" << tex_Fname((*I).second) << "=" << (*I).first << endl;

  }
  F << "\\begingroup" << endl           // make special characters normal
    << "\\catcode`\\^^@=12\\catcode`\\^^?=12\\catcode`\\{=12" << endl
    << "\\catcode`\\}=12\\catcode`\\$=12\\catcode`\\&=12" << endl
    << "\\catcode`\\#=12\\catcode`\\_=12\\catcode`\\~=12" << endl
    << "\\catcode`\\%=12" << endl
    << "\\catcode`\\^^L=12\\catcode`\\^^A=12\\catcode`\\^^K=12\\catcode`\\^^I=12" << endl
    << "\\catcode`\\^^M=12" << endl;   // na tomto riadku ma tex este stare catcode konca riadku,
                                       // vsetko nasledovne musi byt v jednom riadku
  for (map<string,FONTCHARS>::iterator I = USED_CHARS.begin(); 
                                       I != USED_CHARS.end(); I++) {
    F << "\\includechars\\" << (*I).first << ":";
    for (FONTCHARS::iterator J = ((*I).second).begin();
                             J != ((*I).second).end(); J++) {
      c = *J;
      if (c > 31) {
//        if (c==37) F << "\\";    // % remains a comment
        F << c;
        if (c==92) F << " ";     // \ has to be followed by space
      }
      else {
        F << "^^" << char(c+64);
      }
    }
    F << "\\endinclude";
  }
  F << "\\endgroup" << endl;
  ifstream P("patterns.dat");
  if(!P) therror((IOerr("patterns.dat")));
  char buf[200];
  char delim[] = ":";
  string line,num,pfile,bbox,xstep,ystep,matr;
  while(P.getline(buf,200,'\n')) {
    num = strtok(buf,delim);
    pfile = strtok(NULL,delim);
    bbox = strtok(NULL,delim);
    xstep = strtok(NULL,delim);
    ystep = strtok(NULL,delim);
    matr = strtok(NULL,delim);
    map<string,string>::iterator I = ALL_PATTERNS.find(num);
    if (I != ALL_PATTERNS.end()) {
      F << "\\immediate\\pdfobj stream attr {/Type /Pattern" << endl;
      F << "/PaintType 2 /PatternType 1 /TilingType 1" << endl;
      F << "/Matrix " << matr << endl;
      F << "/BBox " << bbox << endl;
      F << "/XStep " << xstep << endl;
      F << "/YStep " << ystep << endl;
      F << "/Resources << /ProcSet [/PDF ] >> } {" << endl;
      distill_eps("", pfile , "", 0, F);
      F << "} \\newcount \\" << tex_Pname((*I).second) << 
           "\\" << tex_Pname((*I).second) << "=\\pdflastobj" << endl;
    }
  }
  P.close();
  F.close();

  vector<string> legend_arr_n, legend_arr_d;
  for(list<legendrecord>::iterator I = LEGENDLIST.begin(); 
                                   I != LEGENDLIST.end(); I++) {
    legend_arr_n.push_back(I->name);
    legend_arr_d.push_back(I->descr);
  }
  ofstream LEG("th_legend.tex");
  if(!LEG) therror(("Can't write a file!"));
/*  for(list<legendrecord>::iterator I = LEGENDLIST.begin(); 
                                   I != LEGENDLIST.end(); I++) {
    LEG << "\\legendsymbolbox{\\" << tex_Lname(I->name) << "}{" <<
                               utf2tex(I->descr) << "}" << endl;
  } */
  int legendbox_num = LEGENDLIST.size();
  int columns = LAYOUT.legend_columns; 
  int rows = (int) ceil(double(legendbox_num) / columns);
  int pos = 0;
  
  LEG << "\\legendcolumns" << columns << endl;

  for (int i = 0; i < rows; i++) {
    LEG << "\\line{%" << endl;
    for (int j = 0; j < columns; j++) {
      pos = i + j * rows;
      if (pos < legendbox_num) 
        LEG << "  \\legendsymbolbox{\\" << tex_Lname(legend_arr_n[pos]) << 
               "}{" << utf2tex(legend_arr_d[pos]) << "}\\hskip10pt" << endl;
    }
    LEG << "\\hss}" << endl;
  }

  LEG.close();

  vector<colorlegendrecord> legend_color;
  colorlegendrecord lcr;
  for(list<colorlegendrecord>::iterator I = COLORLEGENDLIST.begin(); 
                                   I != COLORLEGENDLIST.end(); I++) {
    lcr.R = I->R;
    lcr.G = I->G;
    lcr.B = I->B;
    lcr.texname = I->texname;
    legend_color.push_back(lcr);
  }
  ofstream LEGCOLOR("th_legendcolor.tex");
  if(!LEGCOLOR) therror(("Can't write a file!"));

  legendbox_num = COLORLEGENDLIST.size();
  columns = 1;
  rows = (int) ceil(double(legendbox_num) / columns);
  pos = 0;
  
  LEGCOLOR << "\\legendcolumns" << columns << endl;

  for (int i = 0; i < rows; i++) {
    LEGCOLOR << "\\line{%" << endl;
    for (int j = 0; j < columns; j++) {
      pos = i + j * rows;
      if (pos < legendbox_num) {
        LEGCOLOR << "  \\colorlegendbox{" << 
	legend_color[pos].R << "}{" <<
	legend_color[pos].G << "}{" << 
	legend_color[pos].B << "}%" << endl;
        LEGCOLOR << "  \\legendsymbolbox{\\pdflastxform}{" <<
	legend_color[pos].texname << "}\\hskip10pt" << endl;
      }
    }
    LEGCOLOR << "\\hss}" << endl;
  }

  LEGCOLOR.close();

}

void read_rgb() {
  ifstream F("rgbcolors.dat");
  if(!F) therror((IOerr("rgbcolors.dat")));
  char buf[100];
  string line, color, value;
  while(F.getline(buf,100,'\n')) {
    line = buf;
    color = line.substr(0,line.find(':'));
    value = line.substr(line.find(':')+1,line.size()-line.find(':')-1);
    RGB.insert(make_pair(color,value));
  }
  F.close();
}

int thconvert() { 

#ifdef NOTHERION
  cout << "converting scraps ... " << flush;
#else
  thprintf("converting scraps ... ");
#endif
  
  RGB.clear();
  ALL_FONTS.clear();
  ALL_PATTERNS.clear();
  USED_CHARS.clear();
  font_id = 1;
  patt_id = 1;

#ifdef NOTHERION
  read_hash();
#endif
  read_rgb();
  convert_scraps();

  thpdfdbg();  // in the debugging mode only
  
#ifdef NOTHERION
  cout << "done" << endl;
#else
  thprintf("done\n");
#endif
  return(0);
}

#ifdef NOTHERION
int main() {
  thconvert();
  return(0);
}
#endif


