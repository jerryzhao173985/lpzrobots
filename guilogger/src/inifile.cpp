/***************************************************************************
 *   Copyright (C) 2005-2011 LpzRobots development team                    *
 *    Dominic Schneider (original author)                                  *
 *    Georg Martius  <georg dot martius at web dot de>                     *
 *    Frank Guettler <guettler at informatik dot uni-leipzig dot de        *
 *    Frank Hesse    <frank at nld dot ds dot mpg dot de>                  *
 *    Ralf Der       <ralfder at mis dot mpg dot de>                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *                                                                         *
 ***************************************************************************/

#include "inifile.h"
#include <iostream>
using namespace std;
#include <QRegExp>
#include <QDebug>


IniFile::IniFile(){
  // Qt5: No need for setAutoDelete, using qDeleteAll
  bloaded=false;
}

IniFile::IniFile(QString _filename){
  // Qt5: No need for setAutoDelete, using qDeleteAll
  bloaded=false;
  setFilename(_filename);
}


IniFile::~IniFile(){
  // Delete all sections
  qDeleteAll(sections);
  sections.clear();
}

void IniFile::setFilename(QString _filename){
  filename=_filename;
}

QString IniFile::getFilename(){
  return filename;
}


bool IniFile::Load(){
  sections.clear();
  char buffer[1024];
  QString line;
  IniSection* momsection = nullptr;
  IniVar* momvar;
  bool beforeFirstSection=true;
  int LineType;
  QString str1,str2,str3;

  file.setFileName(filename);
  if (!file.exists()){
#ifdef DEBUG
    cerr << "\nFile doesn't exist: " << filename.toLatin1().data();
#else

#endif
    return false;
  }
  if (!file.open( QIODevice::ReadOnly)) {
#ifdef DEBUG
    cerr << "\nCannot read File: " << filename.toLatin1().data();
#else

#endif
    return false;
  }
  // Zeile fr Zeile auslesen und zuordnen
  while( (file.readLine(buffer,1024))>0 ){
    line=QString(buffer);
    str1="";
    str2="";
    str3="";
    LineType=getLineType(line,str1,str2,str3);
    switch (LineType){
      case EMPTY:
      break;
      case COMMENT:
        if (beforeFirstSection){
          comment.append(str1);
        }else{
          momsection->addComment(str1);
        }
      break;
      case SECTION:
        beforeFirstSection=false;
        momsection=new IniSection(str1);
        // qDebug() << "IniFile::Load - Parsed Section: [" << str1 << "]";
        sections.append(momsection);
      break;
      case VAR:
        if (beforeFirstSection){ // wird als Kommentar gewertet
          comment.append(str1);
        }else{                    // Normale Variable
          if (!momsection) { // ADD CHECK: Ensure momsection is valid
              qWarning() << "IniFile::Load - Error: Trying to add variable '" << str1 << "' without a current section.";
              continue; // Skip this invalid variable
          }
          momvar=new IniVar(str1,str2,str3);
          // qDebug() << "IniFile::Load - Parsed Var: Name='" << str1 << "' Value='" << str2 << "' Comment='" << str3.trimmed() << "' (in Section: " << momsection->getName() << ")";
          momsection->vars.append(momvar);
        }
      break;
    }
  }

  file.close();

  bloaded=true;
  return true;
}


int IniFile::getLineType( QString _line, QString &str1, QString &str2, QString &str3){
  // Trim whitespace and newline characters first
  _line = _line.trimmed();

  if (_line.isEmpty()) return EMPTY;

  // Simplified comment check
  if (_line.startsWith('#') || _line.startsWith(';')){
    str1=_line;
    return COMMENT;
  }

  // Simplified section check
  if (_line.startsWith('[') && _line.endsWith(']')) {
    str1 = _line.mid(1, _line.length() - 2);
    return SECTION;
  }

  // Simplified VAR check using indexOf
  int equalsPos = _line.indexOf('=');
  if (equalsPos != -1) {
    str1 = _line.left(equalsPos).trimmed(); // Variable name
    QString remaining = _line.mid(equalsPos + 1);

    // Find the first comment character ('#' or ';')
    int commentPos = -1;
    int hashPos = remaining.indexOf('#');
    int semiPos = remaining.indexOf(';');
    if (hashPos != -1 && semiPos != -1) {
        commentPos = qMin(hashPos, semiPos);
    } else if (hashPos != -1) {
        commentPos = hashPos;
    } else {
        commentPos = semiPos; // Might be -1 if neither exists
    }

    if (commentPos != -1) { // Comment found
      str2 = remaining.left(commentPos).trimmed(); // Value
      str3 = remaining.mid(commentPos); // Comment (includes # or ;)
    } else { // No comment
      str2 = remaining.trimmed(); // Value
      str3 = "";
    }
    return VAR;
  }

  // Ignore lines that don't match the expected formats (COMMENT, SECTION, VAR with '=')
  return EMPTY;
}


bool IniFile::Save(){
  if (filename.isEmpty()) return false;
  file.setFileName(filename);
  if (! file.open(QIODevice::WriteOnly)){
#ifdef DEBUG
    cerr << "\nCannot write File: " << filename.toLatin1().data();
#else

#endif
    return false;
  }

  QString line;

  // Durchgehen und alles reinschreiben
  file.write(comment.toLatin1(),comment.length());
  foreach(IniSection* section, sections) {
    line="\n[";
    line.append(section->getName());
    line.append("]\n");
    file.write(line.toLatin1(),line.length());
    line=section->getComment();
    if (!line.isEmpty()) {
      line += "\n";
      file.write(line.toLatin1(),line.length());
    }
    foreach(IniVar* var, section->vars) {
      line=var->getName();
      if (! var->getValue().isEmpty()){
        line.append("=");
        line.append(var->getValue());
        line.append(var->getComment());
//      }else{
        line.append("\n");
      }
      file.write(line.toLatin1(),line.length());
    }
  }

  file.close();

  return true;
}

void IniFile::Clear(){
  sections.clear();
  filename="";
  bloaded=false;
  comment="";
}


bool IniFile::getSection(IniSection& _section,QString _name,bool _next){
  static QString lastname;
  static int currentIndex = 0;

  if (_next==false || (_next==true && _name!=lastname)) {
    lastname ="";
    currentIndex = 0;
  } else {
    currentIndex++;
  }

  lastname=_name;

  for(int i = currentIndex; i < sections.size(); i++){
    IniSection* sec = sections.at(i);
    if (sec->getName()==_name){  // gefunden
      sec->copy(_section);
      currentIndex = i;
      return true;
    }
  }
  return false;
}


IniSection *IniFile::addSection(QString name)
{   IniSection* sec = new IniSection(name);
    sections.append(sec);
    return sec;
}


void IniFile::delSection(IniSection* _section)
{
  sections.removeOne(_section);
  delete _section;
  _section = NULL;
}


QString IniFile::getValueDef(QString _section, QString _var, QString _default){
  IniSection sec;
  if(getSection(sec, QString(_section), false)){
    IniVar var;
    if(sec.getVar(var, QString(_var))){
      QString value = QString(var.getValue());
      return value; // Return a copy of the value, not a reference
    }
  }
  return QString(_default);
}

void IniFile::setComment(QString _comment){
  comment=_comment;
}

void IniFile::addComment(QString _addcomment){
  comment.append(_addcomment);
}

QString IniFile::getComment(){
  return comment;
}


//////////////////////////////////////////////////////////////////////////////////////////(
//SECTION

IniSection::IniSection(){
  // Qt5: No need for setAutoDelete, using qDeleteAll
}

IniSection::IniSection(QString _name){
  // Qt5: No need for setAutoDelete, using qDeleteAll
  setName(_name);
}

IniSection::~IniSection(){
  // Delete all vars
  qDeleteAll(vars);
  vars.clear();
}

void IniSection::setName(QString _name){
  name=_name;
}

QString IniSection::getName() const {
  return name;
}

void IniSection::setComment(QString _comment){
  comment=_comment;
}

void IniSection::addComment(QString _addcomment){
  comment.append(_addcomment);
}

QString IniSection::getComment() const {
  return comment;
}

bool IniSection::operator== (IniSection& _section){
  return name==_section.getName();
}


void IniSection::copy(IniSection& _section){
  _section.setName(name);
  _section.setComment(comment);
  
  // Clear existing vars in the destination to prevent memory leaks
  qDeleteAll(_section.vars);
  _section.vars.clear();
  
  // Create a deep copy of each IniVar object
  foreach(IniVar* var, vars) {
    if (!var) continue; // Skip null pointers
    IniVar* newVar = new IniVar(var->getName(), var->getValue(), var->getComment());
    _section.vars.append(newVar);
  }
}


bool IniSection::getVar(IniVar& _var, QString _name){
  // Initialize _var with empty values to ensure it's valid even if we don't find a match
  _var.setName(QString());
  _var.setValue(QString());
  _var.setComment(QString());
  
  // Use a safe copy of the name to match
  QString name_to_match = QString(_name);
  
  for(int i = 0; i < vars.size(); i++) {
    IniVar* var = vars.at(i);
    if (!var) continue; // Skip null pointers
    if (var->getName() == name_to_match){
      var->copy(_var);
      return true;
    }
  }
  return false;
}


void IniSection::delVar(IniVar* _var)
{
  vars.removeOne(_var);
  delete _var;  // manual deletion in Qt5
  _var = NULL;
}


void IniSection::addValue(QString name, QString value,QString comment)
{   IniVar* tmpvar = new IniVar(name, value, comment);
    vars.append(tmpvar);
}


/////////////////////////////////////////////////////////////////////////////////////////
// VAR
IniVar::IniVar(){
  // Initialize with empty values to avoid undefined behavior
  name = QString();
  value = QString();
  comment = QString();
}

IniVar::IniVar(QString _name, QString _value, QString _comment){
  // Ensure we get copies of the strings, not references
  name = QString(_name);
  value = QString(_value);
  comment = QString(_comment);
}

IniVar::~IniVar(){
}

void IniVar::setName(QString _name){
  name = QString(_name);
}

QString IniVar::getName() const {
  return QString(name);
}

void IniVar::setValue(QString _value){
  value = QString(_value);
}

QString IniVar::getValue() const {
  return QString(value);
}

void IniVar::setComment(QString _comment){
  comment = QString(_comment);
}

QString IniVar::getComment() const {
  return QString(comment);
}

bool IniVar::operator== (IniVar& _var){
  return name==_var.getName();
}

void IniVar::copy(IniVar& _var){
  _var.setName(QString(name));
  _var.setComment(QString(comment));
  _var.setValue(QString(value));
}
