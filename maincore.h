#ifndef MAINCORE_H
#define MAINCORE_H

#include "mainwindow.h"
#include "include/qt3dwidget.h"
#include "imagetilerequest.h"

#include <QObject>
#include <Qt3DExtras>
#include <Qt3DCore>
#include <QHash>
#include <QMessageBox>


class mainCore : public QObject
{
    Q_OBJECT

public:
    mainCore();
    ~mainCore();


public slots:
    void setURL();
    void changeType(int index);
    void resized();
    void initUpdate(float dt);
    void zoomUpdate(float dt);
    void wheeled(Qt3DInput::QWheelEvent* wheel);
    void pressed(Qt3DInput::QMouseEvent* mouse);
    void moved(Qt3DInput::QMouseEvent* mouse);
    void released(Qt3DInput::QMouseEvent* mouse);


private:
    void createHolders(MapIdentifyingText* url);
    MainWindow* mainWindow = nullptr;
    Qt3DWidget* view;
    int viewHeight;
    int viewWidth;
    int mouseX;
    int mouseY;
    Qt3DRender::QRenderSurfaceSelector* rss;
    Qt3DRender::QClearBuffers* cb;
    Qt3DRender::QViewport* vp;
    Qt3DRender::QCameraSelector* cs;
    Qt3DRender::QLayerFilter* lf;
    Qt3DCore::QEntity* scene = nullptr;
    const double cameraFarRestPositionFactor = 1.5;
    const double smallestQuadSize = .0054 * 3.0 / cameraFarRestPositionFactor;
    Qt3DRender::QCamera* camera;
    int zoomLevelMax = 19;
    double maxQuadSize = pow(2, zoomLevelMax) * smallestQuadSize;
    double cameraFarRestDistance = cameraFarRestPositionFactor * maxQuadSize;
    double mapPlaneY = -cameraFarRestDistance / 2.0;
    QVector3D posCameraFarRest = QVector3D(.0f, cameraFarRestDistance / 2.0, .0);
    bool firstSet = true;
    QVector3D posCameraStart;
    QVector3D dirCameraHit;
    int zoomCurrentLevel = zoomLevelMax;
    int zoomLastLevel = -1;
    bool zooming = false;
    const double zoomDistanceFactor = .5;                       // factor to distance from camera to map giving that distance's reduction per zoom action
    const double zoomDuration = 1250000000.0;                   // duration in nanoseconds of zooming a reduction of above mentioned distance (1 for all)
    double appliedZoomDuration;
    double minZoomDuration = 250000000.0;
    double easingCoefficient;
    QElapsedTimer* t;
    QVector3D posHit;
    int extraZoom = 2;                                          // zoom level to add to originally intended zoom at camera distance for fetching textures
    //int extensionX;                                                           // number of tiles visible across width at zoom level entrance, determined dynamically
    const int extensionY = ceil(cameraFarRestPositionFactor) + 0;               // number of tiles visible across height at zoom level entrance, cameraFarRestPositionFactor tiles fit into a given height
    const double layerYStackingGap = .0015;                     // below is z-fighting at a certain zoom level in fullscreen for cameraFarRestPositionFactor = 1.5f
    Qt3DInput::QMouseHandler* mh;
    Qt3DLogic::QFrameAction* fa;
    bool panning = false;
    bool newPanning = false;

    void zoomInit(double distance, float wheelDelta);
    bool doTiles(int forZoomLevel, bool zoomingOut);

    QHash<QString, QHash<QString, QList<Qt3DRender::QLayer*>*>*> layer;
    QHash<QString, QList<Qt3DRender::QLayer*>*>* currentTypeLayer;
    QList<Qt3DRender::QLayer*>* currentLayer = nullptr;
    QHash<QString, int> lastTypes;
    QHash<QString, QHash<QString, QHash<QString, Qt3DCore::QEntity*>*>*> cacheQuad;
    QHash<QString, QHash<QString, Qt3DCore::QEntity*>*>* currentTypeCacheQuad;
    QHash<QString, Qt3DCore::QEntity*>* currentCacheQuad;

    ImageTileRequest imageTileRequest;

    struct mapHitCoordinates
    {
        double x;
        double y;
        double d;
    };

    mapHitCoordinates panHit;
    double posCamX, posCamY, posCamZ;

    mainCore::mapHitCoordinates getMapHit();

    QList<MapIdentifyingText*> urls;
};

#endif // MAINCORE_H
