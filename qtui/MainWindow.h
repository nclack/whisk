/**
  \author Nathan Clack <clackn@janelia.hhmi.org>
  \copyright 2012, HHMI
  */
#include<QMainWindow>
#include<QLabel>

//////////////////////////////////////////////////////////////////////////////
// MAINWINDOW                                                      QMAINWINDOW
//////////////////////////////////////////////////////////////////////////////
class MainWindow : public QMainWindow
{
  public:
    MainWindow(QWidget *parent = 0, Qt::WindowFlags flags = 0);
};

//////////////////////////////////////////////////////////////////////////////
// FILENAMEDISPLAY                                                      QLABEL
//////////////////////////////////////////////////////////////////////////////
class FileNameDisplay : public QLabel
{ Q_OBJECT

  public:
    FileNameDisplay(const QString& filename, QWidget *parent=0);

  public slots:
    void update(const QString& filename);
};
