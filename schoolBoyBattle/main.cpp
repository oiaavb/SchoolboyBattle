
#include "mainwidget.h"

#include <QApplication>
#include <QMainWindow>

int main(int argc, char *argv[])
{
    srand(time(NULL));
    QApplication a(argc, argv);

    QMainWindow w;
    w.setFocusPolicy(Qt::FocusPolicy::StrongFocus);
    w.setWindowTitle("SchoolBoyBattle");
    w.setWindowIcon(QIcon(":/Resources/brand/schoolboybattle-icon.ico"));
    w.setCentralWidget(new MainWidget);


    //w.showFullScreen();
    w.show();
    return a.exec();
}
