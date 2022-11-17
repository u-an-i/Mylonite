#ifndef MAINCORE_H
#define MAINCORE_H

#include "mainwindow.h"
#include "include/qt3dwidget.h"

#include <QObject>
#include <Qt3DExtras>
#include <Qt3DCore>
#include <QHash>


class mainCore : public QObject
{
    Q_OBJECT

public:
    mainCore();
    ~mainCore();


public slots:
    void setAPIKey();


private:
    Qt3DWidget* view;
    MainWindow* mainWindow;

    Qt3DCore::QEntity* createScene();

    QHash<QString, bool> cache[20];     // there shall be 2 caches: map image and quad caches, bool is a placeholder for likely further hashes of id to actual Qt data structure representing image or quad
};

#endif // MAINCORE_H
