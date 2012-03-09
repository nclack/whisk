/**
  \author Nathan Clack <clackn@janelia.hhmi.org>
  \copyright 2012, HHMI
  */
#include<QMainWindow>
#include<QLabel>
#include<QMap>
#include<QString>

class QAction;
class Editor;

//////////////////////////////////////////////////////////////////////////////
// MAINWINDOW                                                      QMAINWINDOW
//////////////////////////////////////////////////////////////////////////////
class MainWindow : public QMainWindow
{ Q_OBJECT;
  public:
    MainWindow(QWidget *parent = 0, Qt::WindowFlags flags = 0);

    static const char defaultConfigPathKey[];

  public slots:
    void openFileDialog();
    void saveToLastLocation();
    void saveFileDialog();

  signals:
    void loadRequest(const QString& filename);
    void saveRequest(const QString& filename);

  protected:
    void createActions();
    void createMenus();

    Editor* view_;
    QMap<QString,QAction*> actions_;
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
