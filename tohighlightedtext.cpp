/****************************************************************************
 *
 * TOra - An Oracle Toolkit for DBA's and developers
 * Copyright (C) 2000 GlobeCom AB
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *      As a special exception, you have permission to link this program
 *      with the Qt and Oracle Client libraries and distribute executables,
 *      as long as you follow the requirements of the GNU GPL in regard to
 *      all of the software in the executable aside from Qt and Oracle client
 *      libraries.
 *
 ****************************************************************************/


#include <ctype.h>

#include <qpainter.h>
#include <qpalette.h>
#include <qsimplerichtext.h>
#include <qstylesheet.h>
#include <qapplication.h>

#include "tohighlightedtext.h"
#include "tomain.h"
#include "totool.h"
#include "toconf.h"

#define MARGIN 2

#include "todefaultkeywords.h"

toSyntaxAnalyzer::toSyntaxAnalyzer(const char **keywords)
{
  for (int i=0;keywords[i];i++) {
    list<const char *> &curKey=Keywords[(unsigned char)char(toupper(*keywords[i]))];
    curKey.insert(curKey.end(),keywords[i]);
  }
}

toSyntaxAnalyzer::posibleHit::posibleHit(const char *text)
{
  Pos=1;
  Text=text;
}

list<toSyntaxAnalyzer::highlightInfo> toSyntaxAnalyzer::analyzeLine(const QString &str)
{
  list<highlightInfo> highs;
  list<posibleHit> search;

  bool inWord;
  bool wasWord=false;
  int inString=-1;
  QChar endString;

  for (int i=0;i<int(str.length());i++) {
    list<posibleHit>::iterator j=search.begin();

    bool nextSymbol=((int(str.length())!=i+1)&&isSymbol(str[i+1]));
    if (inString>=0) {
      if (str[i]==endString) {
	highs.insert(highs.end(),highlightInfo(inString,false,true,false,false));
	highs.insert(highs.end(),highlightInfo(i+1));
	inString=-1;
      }
    } else if (str[i]=='\''||str[i]=='\"') {
      inString=i;
      endString=str[i];
      search.clear();
      wasWord=false;
    } else if (str[i]=='-'&&str[i+1]=='-') {
      highs.insert(highs.end(),highlightInfo(i,false,false,false,true));
      highs.insert(highs.end(),highlightInfo(str.length()+1));
      return highs;
    } else {
      while (j!=search.end()) {
	posibleHit &cur=(*j);
	if (cur.Text[cur.Pos]==str[i].upper()) {
	  cur.Pos++;
	  if (!cur.Text[cur.Pos]&&!nextSymbol) {
	    search.clear();
	    highs.insert(highs.end(),highlightInfo(i-cur.Pos,true,false,false,false));
	    highs.insert(highs.end(),highlightInfo(i+1));
	    break;
	  }
	  j++;
	} else {
	  list<posibleHit>::iterator k=j;
	  j++;
	  search.erase(k);
	}
      }
      if (isSymbol(str[i]))
	inWord=true;
      else
	inWord=false;

      if (!wasWord&&inWord) {
	list<const char *> &curKey=Keywords[(unsigned char)char(str[i].upper())];
	for (list<const char *>::iterator j=curKey.begin();
	     j!=curKey.end();j++) {
	  if (strlen(*j)==1) {
	    if (!nextSymbol) {
	      highs.insert(highs.end(),highlightInfo(i,true,false,false,false));
	      highs.insert(highs.end(),highlightInfo(i));
	    }
	  } else
	    search.insert(search.end(),posibleHit(*j));
	}
      }
      wasWord=inWord;
    }
  }
  if (inString>=0) {
    highs.insert(highs.end(),highlightInfo(inString,false,false,true,false));
    highs.insert(highs.end(),highlightInfo(str.length()+1));
  }

  return highs;
}

static toSyntaxAnalyzer DefaultAnalyzer((const char **)DefaultKeywords);

toHighlightedText::toHighlightedText(QWidget *parent,const char *name=NULL)
  : toMarkedText(parent,name)
{
  LastCol=LastRow=-1;
  Highlight=!toTool::globalConfig(CONF_HIGHLIGHT,"Yes").isEmpty();
  KeywordUpper=!toTool::globalConfig(CONF_KEYWORD_UPPER,"").isEmpty();
}

void toHighlightedText::paintCell(QPainter *painter,int row,int col)
{
  if (!Highlight) {
    toMarkedText::paintCell(painter,row,col);
    return;
  }

  QString str=textLine(row);

  int line1,col1,line2,col2;
  if (!getMarkedRegion(&line1,&col1,&line2,&col2)) {
    line2=line1=-1;
    col2=col1=-1;
  }

  list<toSyntaxAnalyzer::highlightInfo> highs=DefaultAnalyzer.analyzeLine(str);
  list<toSyntaxAnalyzer::highlightInfo>::iterator highPos=highs.begin();

  int posx=MARGIN;
  QRect rect;

  int height=cellHeight();
  int width=cellWidth();
  QPalette cp=qApp->palette();

  painter->setBrush(cp.active().highlight());

  if (!str.isEmpty()) {
    bool marked;
    bool bold=false;
    bool italic=false;
    bool red=false;
    bool green=false;
    bool wasMarked;
    bool wasBold;
    bool wasItalic;
    bool wasRed;
    bool wasGreen;
    QString c;
    for (int i=0;i<=int(str.length())&&posx<width;i++) {
      if (i==int(str.length())) {
	marked=!wasMarked;
      } else {
	marked=false;
	if (row==line1&&i>=col1)
	  marked=true;
	else if (row>line1&&row<=line2)
	  marked=true;
	if (row==line2&&i>=col2)
	  marked=false;
      }

      while (highPos!=highs.end()&&(*highPos).Start<=i) {
	bold=(*highPos).Keyword;
	red=(*highPos).Error;
	if ((*highPos).Comment) {
	  green=true;
	  italic=true;
	} else {
	  green=false;
	  italic=(*highPos).String;
	}
	highPos++;
      }

      if (i<int(str.length())) {
	if (bold&&KeywordUpper)
	  c.append(str[i].upper());
	else
	  c.append(str[i]);
      }
      if (i==0) {
	wasMarked=marked;
	wasBold=bold;
	wasItalic=italic;
	wasRed=red;
	wasGreen=green;
      }

      if (wasMarked!=marked||wasBold!=bold||wasItalic!=italic||wasRed!=red||wasGreen!=green) {
	QString nc=QString(c[c.length()-1]);
	if (i<int(str.length()))
	  c.truncate(c.length()-1);

	QFont font=painter->font();
	font.setBold(wasBold);
	font.setItalic(wasItalic);
	painter->setFont(font);

	rect=painter->boundingRect(0,0,width,height,AlignLeft|AlignTop|ExpandTabs,c);
	int left=posx;
	int cw=rect.right()+1;
	if (i==int(str.length()))
	  cw+=MARGIN;
	if (i==int(c.length())) {
	  cw+=left;
	  left=0;
	}
	if (wasMarked) {
	  painter->fillRect(left,0,cw,height,painter->brush());
	  painter->setPen(cp.active().highlightedText());
	} else {
	  painter->setPen(cp.active().text());
	  painter->eraseRect(left,0,cw,height);
	}
	if (wasRed)
	  painter->setPen(Qt::red);
	else if (wasGreen)
	  painter->setPen(Qt::darkGreen);

	painter->drawText(posx,0,width-posx,height,AlignLeft|AlignTop|ExpandTabs,c,c.length(),&rect);
	posx=rect.right()+1;
	wasMarked=marked;
	wasBold=bold;
	wasRed=red;
	wasItalic=italic;
	c=nc;
      }
    }
    if (posx<width)
      painter->eraseRect(posx,0,width-posx,height);
  } else
      painter->eraseRect(0,0,width,height);
  painter->setPen(cp.active().text());

  if (hasFocus()) {
    int curline,curcol;
    getCursorPosition (&curline,&curcol);
    if (row==curline&&(LastRow!=curline||LastCol!=curcol)) {
      LastRow=curline;
      LastCol=curcol;
      QPoint pos=cursorPoint();
      painter->drawLine(pos.x(),0,pos.x(),
			painter->fontMetrics().ascent()+painter->fontMetrics().descent());
    } else
      LastRow=LastCol=-1;
  }
}
