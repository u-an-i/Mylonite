#include "maincore.h"

#include <QApplication>


int main(int argc, char* argv[])
{
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setSamples(4);
    format.setVersion(3, 0);
    QSurfaceFormat::setDefaultFormat(format);
    QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    QApplication a(argc, argv);

    mainCore code;

    return a.exec();
}
