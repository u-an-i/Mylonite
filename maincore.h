#ifndef MAINCORE_H
#define MAINCORE_H

#include "mainwindow.h"
#include "include/qt3dwidget.h"
#include "imagetilerequest.h"

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
    Qt3DRender::QRenderSurfaceSelector* rss;
    Qt3DRender::QClearBuffers* cb;
    Qt3DRender::QViewport* vp;
    Qt3DRender::QCameraSelector* cs;
    Qt3DRender::QLayerFilter* lf;
    Qt3DCore::QEntity* scene = nullptr;
    const float cameraFarRestPositionFactor = 1.5f;
    const float smallestQuadSize = .054f * 3.0f / cameraFarRestPositionFactor;                // "anything" larger lets raycast not hit correctly from the cameraFarRestPosition
    Qt3DRender::QScreenRayCaster* src;
    Qt3DRender::QCamera* camera;
    const int zoomLevelMax = 19;
    const float maxQuadSize = pow(2, zoomLevelMax) * smallestQuadSize;
    const QVector3D cameraFarRestPosition = QVector3D(.0f, cameraFarRestPositionFactor * maxQuadSize, .0f);
    QVector3D cameraStartPosition;
    QVector3D cameraDirHit;
    int zoomCurrentLevel = 19;
    int zoomLastLevel = -1;
    bool zoom = false;
    const float zoomDistanceFactor = .5f;               // factor to distance from camera to map giving that distance's reduction per zoom action
    const float zoomDuration = 1.25f;                   // duration in seconds of zooming a reduction of above mentioned distance(1 for all)
    float easingCoefficient;
    float appliedZoomDuration;
    QElapsedTimer* t;
    QVector3D posHit;
    int extensionX = 0;                                                 // = viewport width / (viewport height / extensionY) = extensionY*w/h, ceiled
    const int extensionY = ceil(cameraFarRestPositionFactor);           // cameraFarRestPositionFactor tiles fit into a given height thus depending on where the targeted is, ceiled cameraFarRestPositionFactor additional tiles must be visible to the top or bottom to cover the whole height
    const float layerYStackingGap = .075f;               // below is z-fighting at a certain zoom level in fullscreen for cameraFarRestPositionFactor = 1.5f

    Qt3DCore::QEntity* createScene();

    Qt3DRender::QLayer* layer[19 + 1];                  // 19 is zoomLevelMax
    QHash<QString, Qt3DCore::QEntity*> cacheQuad;

    ImageTileRequest imageTileRequest;
};

#endif // MAINCORE_H
