/**
  \author Nathan Clack <clackn@janelia.hhmi.org>
  \copyright 2012, HHMI
  */
#include <QtGui>
#include <QKeySequence>
#include "MainWindow.h"
#include "Editor.h"


#define ENDL "\n"
#define HERE         qDebug("%s(%d): HERE"ENDL,__FILE__,__LINE__); 
#define REPORT(expr) qDebug("%s(%d):"ENDL "\t%s"ENDL "\tExpression evaluated as false."ENDL,__FILE__,__LINE__,#expr) 
#define TRY(expr)    if(!(expr)) {REPORT(expr);}
#define DIE          qFatal("%s(%d): Aborting."ENDL,__FILE__,__LINE__); 

const char MainWindow::defaultConfigPathKey[] = "Whisk/Config/DefaultFilename";

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

  Editor *d;
  setCentralWidget(d=new Editor(this));
  view_ = d;
  TRY(connect(this,SIGNAL(loadRequest(const QString&)),d        ,SLOT(  open(const QString&))));
  TRY(connect(this,SIGNAL(saveRequest(const QString&)),d->data(),SLOT(saveAs(const QString&))));

  statusBar()->setSizeGripEnabled(true);
  /*
  { FileNameDisplay *w = new FileNameDisplay("Nothing loaded");
    statusBar()->addPermanentWidget(w);
    TRY(connect(d,SIGNAL(opened(const QString&)),
                w,  SLOT(update(const QString&))),Error);
  }
  */

  createActions();
  createMenus();
  TRY(connect(actions_["save"],SIGNAL(triggered()),d->data(),SLOT(save())));
  return;
Error:
  DIE;
}

void MainWindow::openFileDialog()
{ QSettings settings;
  QString filename = QFileDialog::getOpenFileName(this,
    tr("Open video, whiskers or measurements files."),
    settings.value(defaultConfigPathKey,QDir::currentPath()).toString(),
    tr("Video (*.tif *.seq *.avi *.mp4 *.mpg);;"
       "Whiskers Files (*.whiskers);;"
       "Measurements Files (*.measurements);;"
       "Any (*.*)"));
  if(filename.isEmpty())
    return;

  emit loadRequest(filename);
}

void MainWindow::saveToLastLocation()
{ QSettings s;
  QVariant v = s.value(defaultConfigPathKey);
  if(v.isValid())
    emit saveRequest(v.toString());
  else
  { qDebug("%s(%d): Attempted save to last location, but there was no last location."ENDL
           "\tThe dude who wrote this should disable the menu item until a valid location is stored."ENDL
           "\tOr he should store the last loaded location."ENDL
           ,__FILE__,__LINE__);
    QMessageBox mb;
    mb.setText("Did not save.");
    mb.setInformativeText("No known last location for save. Use \"Save As\" first.");
    mb.setIcon(QMessageBox::Warning);
    mb.exec();
  }
}


void MainWindow::saveFileDialog()
{
  QSettings settings;
  QString filename = QFileDialog::getSaveFileName(this,
    tr("Save data."),
    settings.value(defaultConfigPathKey,QDir::currentPath()).toString(),
    tr("Whiskers (*.whiskers);;Measurements (*.measurements);;Any (*.*)"));
  if(filename.isEmpty())
    return;

  emit saveRequest(filename);
}

void MainWindow::createActions()
{ QAction *a;
  {
    a = new QAction(QIcon(":/icons/fullscreen"),"&Full Screen",this);
    a->setShortcut(Qt::CTRL + Qt::Key_F);
    a->setStatusTip("Show full screen");

    QStateMachine *sm = new QStateMachine(this);
    QState *fs = new QState(),
          *nfs = new QState();
    TRY(connect(fs, SIGNAL(entered()),this,SLOT(showFullScreen())));
    TRY(connect(fs, SIGNAL(entered()),statusBar(),SLOT(hide())));
    TRY(connect(nfs,SIGNAL(entered()),this,SLOT(showNormal())));  
    TRY(connect(fs, SIGNAL(entered()),statusBar(),SLOT(show())));
    fs->addTransition(a,SIGNAL(triggered()),nfs);
    nfs->addTransition(a,SIGNAL(triggered()),fs);
    sm->addState(fs);
    sm->addState(nfs);
    sm->setInitialState(nfs);
    sm->start();
  }
  actions_["fullscreen"] = a;
  
  {
    a = new QAction(QIcon(":/icons/quit"),"&Quit",this);
    QList<QKeySequence> quitShortcuts;
    quitShortcuts 
      << QKeySequence::Close
      << QKeySequence::Quit
      << (Qt::CTRL+Qt::Key_Q);
    a->setShortcuts(quitShortcuts);
    a->setStatusTip("Exit the application.");
    connect(a,SIGNAL(triggered()),this,SLOT(close()));
  }
  actions_["quit"] = a;

  {
    a = new QAction(QIcon(":/icons/open"),"&Open",this);
    a->setShortcut(QKeySequence::Open);
    a->setStatusTip("Open a video, whiskers or measurements file.");
    connect(a,SIGNAL(triggered()),this,SLOT(openFileDialog()));
  }
  actions_["open"] = a;

  {
    a = new QAction(QIcon(":/icons/save"),"&Save",this);
    a->setShortcut(QKeySequence::Save);
    a->setStatusTip("Save whiskers and measurements."); 
    //connect(a,SIGNAL(triggered()),this,SLOT(saveToLastLocation()));
  }
  actions_["save"] = a;

  {
    a = new QAction(QIcon(":/icons/saveas"),"Save &As",this);
    a->setShortcut(QKeySequence::SaveAs);
    a->setStatusTip("Save whiskers and measurements to a new location."); 
    connect(a,SIGNAL(triggered()),this,SLOT(saveFileDialog()));
  }
  actions_["saveas"] = a;
  return;
Error:
  DIE;
}

void MainWindow::createMenus()
{
  QMenu *s,*m = menuBar()->addMenu("&File");
  { static const char *names[] = {"open","save","saveas","quit",NULL};
           const char **c      = names;
    while(*c)
      m->addAction(actions_[*c++]);
  }

  m = menuBar()->addMenu("&View");
  m->addAction(actions_["fullscreen"]);
  s = m->addMenu("&Video");
  foreach(QAction* a,view_->videoPlayerActions())
    s->addAction(a);  

  m = menuBar()->addMenu("&Edit");
  foreach(QAction* a,view_->tracingActions())
    m->addAction(a);
  s = m->addMenu("&Identity");  
  foreach(QAction* a,view_->identityActions())
    s->addAction(a);

}
