/**
  \author Nathan Clack <clackn@janelia.hhmi.org>
  \copyright 2012, HHMI
  */
#include <QtGui>
#include <QKeySequence>
#include "MainWindow.h"
#include "Display.h"


#define ENDL "\n"
#define HERE         qDebug("%s(%d): HERE"ENDL,__FILE__,__LINE__); 
#define REPORT(expr) qDebug("%s(%d):"ENDL "\t%s"ENDL "\tExpression evaluated as false."ENDL,__FILE__,__LINE__,#expr) 
#define TRY(expr,lbl) if(!(expr)) {REPORT(expr); goto lbl;}

//////////////////////////////////////////////////////////////////////////////
// FILENAMEDISPLAY                                                      QLABEL
//////////////////////////////////////////////////////////////////////////////

FileNameDisplay::FileNameDisplay(const QString& filename, QWidget *parent)
  : QLabel(parent)
{             
  setOpenExternalLinks(true);
  setTextFormat(Qt::RichText);
  setTextInteractionFlags(Qt::NoTextInteraction
      |Qt::TextBrowserInteraction
      |Qt::LinksAccessibleByKeyboard
      |Qt::LinksAccessibleByMouse);
  update(filename);
}

void FileNameDisplay::update(const QString& filename)
{ //QString link = QString("<a href=\"file:///%1\">%1</a>").arg(filename);
  //setText(link);
  // Link to each folder in the path individually  
  QString sep = QDir::separator();
  QStringList names = filename.split(sep,QString::SkipEmptyParts);
  QString text;
  QString path;
  foreach(QString n,names)
  { n += sep;
    path += n;
    //qDebug() << n << " " << path;
    QString link = QString("<a href=\"file:///%1\">%2</a> ").arg(path,n);
    text += link;    
  }     
  setText(text);
}

//////////////////////////////////////////////////////////////////////////////
// MAINWINDOW                                                      QMAINWINDOW
//////////////////////////////////////////////////////////////////////////////

MainWindow::MainWindow(QWidget *parent, Qt::WindowFlags flags)
  :QMainWindow(parent,flags)
{ 
  setUnifiedTitleAndToolBarOnMac(true);

  Display *d;
  setCentralWidget(d=new Display);

  statusBar()->setSizeGripEnabled(true);
  { FileNameDisplay *w = new FileNameDisplay("Nothing loaded");
    statusBar()->addPermanentWidget(w);
    TRY(connect(d,SIGNAL(opened(const QString&)),
                w,  SLOT(update(const QString&))),Error);
  }

  return;
Error:
  exit(-1);
}
