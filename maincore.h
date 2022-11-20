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
    void resized();
    void frameUpdate(float dt);
    void rayHit(const Qt3DRender::QAbstractRayCaster::Hits &hits);
    void wheeled(Qt3DInput::QWheelEvent* wheel);


private:
    MainWindow* mainWindow = nullptr;
    Qt3DWidget* view;
    int viewHeight;
    Qt3DCore::QEntity* scene = nullptr;
    Qt3DRender::QScreenRayCaster* src;
    Qt3DRender::QCamera* camera;
    const QVector3D cameraFarRestPosition = QVector3D(.0f, 200.0f, .0f);
    QVector3D cameraStartPosition;
    QVector3D cameraDirHit;
    bool zoom = false;
    const float zoomDistanceFactor = .5f;               // factor to distance from camera to map giving that distance's reduction per zoom action
    const float zoomDuration = 1.25f;                   // duration in seconds of zooming a reduction of above mentioned distance(1 for all)
    const float zoomBackDistance = 23.5f;               // fixed distance to zoom back
    const float zoomBackDistanceSmall = 8.0f;           // fixed distance to zoom back when close map
    float easingCoefficient;
    float appliedZoomDuration;
    QElapsedTimer* t;

    Qt3DCore::QEntity* createScene();

    QHash<QString, Qt3DCore::QEntity*> cacheQuad;       // later: image (disk) cache
    int zoomLevelCurrent = 0;
};

#endif // MAINCORE_H
