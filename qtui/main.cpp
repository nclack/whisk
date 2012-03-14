/**
  \author Nathan Clack <clackn@janelia.hhmi.org>
  \copyright 2012, HHMI
  */
#include <QtGui>
#include <QtOpenGL>
#include <QDebug>
#include "MainWindow.h"
#include "LockedCalls.h"

#define log(...)   qDebug(__VA_ARGS__)
#define error(...) qFatal(__VA_ARGS__)

void Init()
{
  { const char* paramfile = "default.parameters";
    if(locked::Load_Params_File((char*)paramfile))
    { log("Writing %s\n",paramfile);
      locked::Print_Params_File((char*)paramfile);
      if(locked::Load_Params_File((char*)paramfile))
        qFatal("\tStill could not load parameters.\n");
    }
  }
}

int main(int argc, char *argv[])
{ QGL::setPreferredPaintEngine(QPaintEngine::OpenGL);
  QCoreApplication::setOrganizationName("Howard Hughes Medical Institute");
  QCoreApplication::setOrganizationDomain("janelia.hhmi.org");
  QCoreApplication::setApplicationName("Whisk");

  QApplication app(argc,argv);
  app.setWindowIcon(QIcon(":/icons/faceindicator"));
  Init();
  MainWindow mainwindow;
  mainwindow.show();

  unsigned int eflag = app.exec();
  return eflag;
}
