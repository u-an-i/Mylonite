#include "maincore.h"
#include "mainwindow.h"
#include "include/mm.hpp"
#include "include/qt3dwidget.h"



mainCore::mainCore()
{
    mainWindow = new MainWindow();

    mainWindow->show();

    connect(mainWindow->getPushButtonForAPIKey(), &QPushButton::clicked, this, &mainCore::setAPIKey);

    imageTileRequest.setCache(&cacheQuad);

    view = (new Derived<Qt3DWidget>())->get();
    viewHeight = view->height();
    extensionX = ceil(extensionY * view->width() / viewHeight);
    connect(view, &Qt3DWidget::resized, this, &mainCore::resized);

    view->renderSettings()->pickingSettings()->setPickMethod(Qt3DRender::QPickingSettings::TrianglePicking);

    mainWindow->getCentralWidget()->setParent(view);

    mainWindow->setCentralWidget(view);

    // Camera
    camera = view->camera();
    camera->lens()->setPerspectiveProjection(45.0f, 16.0f/9.0f, .01f, 2.0f * cameraFarRestPosition.y());        // nearPlane .0f lets QScreenRayCast hit something at center of screen and not one of the created mesh
    camera->setPosition(cameraFarRestPosition);
    camera->setUpVector(QVector3D(.0f, .0f, -1.0f));
    camera->setViewCenter(QVector3D(0.0f, 0.0f, 0.0f));

    for(int i=0; i<20; ++i)
    {
        layer[i] = (new Derived<Qt3DRender::QLayer>())->get();
    }

    rss = (new Derived<Qt3DRender::QRenderSurfaceSelector>())->get();
    rss->setSurface(view);
    cb = (new Derived<Qt3DRender::QClearBuffers>())->get();
    cb->setParent(rss);
    cb->setBuffers(Qt3DRender::QClearBuffers::AllBuffers);
    cb->setClearColor(Qt::black);
    vp = (new Derived<Qt3DRender::QViewport>())->get();
    vp->setParent(cb);
    cs = (new Derived<Qt3DRender::QCameraSelector>())->get();
    cs->setParent(vp);
    cs->setCamera(camera);

    lf = (new Derived<Qt3DRender::QLayerFilter>())->get();
    lf->setParent(cs);

    view->setActiveFrameGraph(rss);

    scene = createScene();

    src = (new Derived<Qt3DRender::QScreenRayCaster>())->get();
    //src->addLayer(layer[zoomCurrentLevel]);
    connect(src, &Qt3DRender::QScreenRayCaster::hitsChanged, this, &mainCore::rayHit);

    mh = (new Derived<Qt3DInput::QMouseHandler>())->get();
    mh->setSourceDevice((new Derived<Qt3DInput::QMouseDevice>)->get());
    connect(mh, &Qt3DInput::QMouseHandler::wheel, this, &mainCore::wheeled);
    connect(mh, &Qt3DInput::QMouseHandler::pressed, this, &mainCore::pressed);
    connect(mh, &Qt3DInput::QMouseHandler::released, this, &mainCore::released);

    fa = (new Derived<Qt3DLogic::QFrameAction>())->get();
    connect(fa, &Qt3DLogic::QFrameAction::triggered, this, &mainCore::initUpdate);

    scene->addComponent(src);
    scene->addComponent(mh);
    scene->addComponent(fa);

    view->setRootEntity(scene);
}


mainCore::~mainCore()
{
    scene !=  nullptr ? delete scene : NOP_FUNCTION;
    delete (Derived<Qt3DRender::QLayerFilter>*)lf;
    delete (Derived<Qt3DRender::QCameraSelector>*)cs;
    delete (Derived<Qt3DRender::QViewport>*)vp;
    delete (Derived<Qt3DRender::QClearBuffers>*)cb;
    delete (Derived<Qt3DRender::QRenderSurfaceSelector>*)rss;
    MemRegistry::obliviate();
    mainWindow !=  nullptr ? delete mainWindow : NOP_FUNCTION;
}


Qt3DCore::QEntity* mainCore::createScene()
{
    Qt3DCore::QEntity* rootEntity = new Qt3DCore::QEntity;

    Qt3DCore::QEntity* rayCastTarget = (new Derived<Qt3DCore::QEntity>())->get();
    rayCastTarget->setParent(rootEntity);

    Qt3DExtras::QPlaneMesh* qm = (new Derived<Qt3DExtras::QPlaneMesh>())->get();
    qm->setMeshResolution(QSize(2, 2));
    qm->setWidth(pow(2, zoomCurrentLevel) * smallestQuadSize);
    qm->setHeight(pow(2, zoomCurrentLevel) * smallestQuadSize);

    Qt3DCore::QTransform* qt = (new Derived<Qt3DCore::QTransform>())->get();
    qt->setScale3D(QVector3D(1.0f, 1.0f, 1.0f));
    qt->setRotation(QQuaternion::fromAxisAndAngle(QVector3D(0.0f, 1.0f, 0.0f), 0.0f));
    qt->setTranslation(QVector3D(.0f, .0f, .0f));

    rayCastTarget->addComponent(qm);
    rayCastTarget->addComponent(qt);

    return rootEntity;
}


void mainCore::setAPIKey()
{
    imageTileRequest.setKey(mainWindow->getLineEditForAPIKey()->text());
}


void mainCore::resized()
{
    viewHeight = view->height();
    extensionX = ceil(extensionY * view->width() / viewHeight);
}


bool mainCore::doTiles(int forZoomLevel)
{                                                                                      // generalise for start position
    int appliedZoomLevel = zoomLevelMax - forZoomLevel + extraZoom;
    if(appliedZoomLevel <= zoomLevelMax) {
        int sizeMax = pow(2, appliedZoomLevel);
        float quadSize = maxQuadSize / sizeMax;
        int indexCol = (maxQuadSize/2 + posHit.x()) / quadSize;
        int indexRow = (maxQuadSize/2 - posHit.z()) / quadSize;
        int appliedExtensionX = extensionX * pow(2, extraZoom);
        int appliedExtensionY = extensionY * pow(2, extraZoom);
        int tileSizePerViewX = ceil(view->width() / appliedExtensionX);
        int tileSizePerViewY = ceil(view->height() / appliedExtensionY);
        int viewTileIndexX = mouseX / tileSizePerViewX;
        int viewTileIndexY = mouseY / tileSizePerViewY;
        int indexStartCol = indexCol - viewTileIndexX;
        indexStartCol = indexStartCol > 0 ? indexStartCol : 0;
        int indexStartRow = indexRow - viewTileIndexY;
        indexStartRow = indexStartRow > 0 ? indexStartRow : 0;
        int indexEndCol = indexCol + appliedExtensionX - viewTileIndexX + 1;
        indexEndCol = indexEndCol < sizeMax ? indexEndCol : sizeMax;
        int indexEndRow = indexRow + appliedExtensionY - viewTileIndexY + 1;
        indexEndRow = indexEndRow < sizeMax ? indexEndRow : sizeMax;qDebug() << indexEndCol << " " << indexEndRow;
        for(int i = indexStartRow; i < indexEndRow; ++i)
        {
            for(int j = indexStartCol; j < indexEndCol; ++j)
            {
                QString key = QString::number(appliedZoomLevel) + "-" + QString::number(j) + "-" + QString::number(i);
                Qt3DCore::QEntity* quadEntity = cacheQuad[key];
                if(quadEntity == nullptr) {
                    quadEntity = (new Derived<Qt3DCore::QEntity>())->get();
                    cacheQuad[key] = quadEntity;

                    quadEntity->setParent(scene);
                    quadEntity->addComponent(layer[forZoomLevel]);

                    Qt3DExtras::QPlaneMesh* qm = (new Derived<Qt3DExtras::QPlaneMesh>())->get();
                    qm->setMeshResolution(QSize(2, 2));
                    qm->setWidth(quadSize * 1.005f);        // * 1.005f = prevent gaps between quads on close camera
                    qm->setHeight(quadSize * 1.005f);       // * 1.005f = see above

                    Qt3DCore::QTransform* qt = (new Derived<Qt3DCore::QTransform>())->get();
                    qt->setScale3D(QVector3D(1.0f, 1.0f, 1.0f));
                    qt->setRotation(QQuaternion::fromAxisAndAngle(QVector3D(0.0f, 1.0f, 0.0f), 0.0f));
                    qt->setTranslation(QVector3D(-maxQuadSize/2 + quadSize * (.5f + j), layerYStackingGap * appliedZoomLevel, maxQuadSize/2 - quadSize * (.5f + i)));

                    ImageTileRequest::imageTile it = imageTileRequest.getImageTile(appliedZoomLevel, j, i);

                    if(it.m != nullptr)
                    {
                        quadEntity->addComponent(it.m);
                    }
                    quadEntity->addComponent(qm);
                    quadEntity->addComponent(qt);
                }
            }
        }
        return true;
    }
    return false;
}


void mainCore::initUpdate(float dt)
{
    posHit = QVector3D(.0f, .0f, .0f);
    mouseX = view->width() / 2;
    mouseY = view->height() / 2;
    doTiles(zoomCurrentLevel);
    lf->addLayer(layer[zoomCurrentLevel]);
    disconnect(fa, &Qt3DLogic::QFrameAction::triggered, this, &mainCore::initUpdate);
}


void mainCore::zoomUpdate(float dt)
{
    /*
     *
     * v = A * (t/T - 1)^2;     // ease-out for speed as quadratic function
     * S[v]dt = distance;
     * S[v]dt = S[A * (t/T - 1)^2]dt = [A * (T/3)*(t/T - 1)^3]0..T = 0 - A * (T/3) * (-1) = A * T/3 = distance;
     * A = (distance * 3) / T;
     * [A * (T/3)*(t/T - 1)^3]0..t = A * (T/3)*(t/T - 1)^3 + A * T/3
     *
     */
    float t = this->t->elapsed() / 1000.0f;
    if(t > appliedZoomDuration)
    {
        t = appliedZoomDuration;
        delete this->t;
        zooming = false;
        disconnect(fa, &Qt3DLogic::QFrameAction::triggered, this, &mainCore::zoomUpdate);
    }
    float base = (t/appliedZoomDuration - 1.0f);
    QVector3D posNew = cameraStartPosition + easingCoefficient * appliedZoomDuration * (base * base * base + 1.0f) / 3.0f * cameraDirHit;
    if(posNew.y() <= fmax(cameraFarRestPositionFactor * smallestQuadSize, .01f + (zoomLevelMax + 1) * layerYStackingGap)) {     // keep correspondingZoomLevel >= 0; .01f is camera->nearPlane()
        posNew.setY(fmax(cameraFarRestPositionFactor * smallestQuadSize, .01f + (zoomLevelMax + 1) * layerYStackingGap));
        if(zooming) {
            zooming = false;
            disconnect(fa, &Qt3DLogic::QFrameAction::triggered, this, &mainCore::zoomUpdate);
            delete this->t;
        }qDebug() << "here";
    }
    camera->setPosition(posNew);
    int correspondingZoomLevel = log2f(posNew.y() / (cameraFarRestPositionFactor * smallestQuadSize));
    posNew.setY(.0f);
    camera->setViewCenter(posNew);
    if(correspondingZoomLevel != zoomCurrentLevel)
    {qDebug() << "c p: " << camera->position().y();
        qDebug() << "co: " << correspondingZoomLevel << ", cu: " << zoomCurrentLevel;
        //src->removeLayer(layer[zoomCurrentLevel]);
        //src->addLayer(layer[correspondingZoomLevel]);
        if(doTiles(correspondingZoomLevel))
        {
            lf->addLayer(layer[correspondingZoomLevel]);
            if(correspondingZoomLevel < zoomCurrentLevel)
            {
                if(zoomLastLevel != -1) {
                    lf->removeLayer(layer[zoomLastLevel]);
                }
                zoomLastLevel = zoomCurrentLevel;
            }
            else
            {
                lf->removeLayer(layer[zoomCurrentLevel]);
                if(zoomLastLevel != -1 && zoomLastLevel != correspondingZoomLevel)
                {
                    lf->removeLayer(layer[zoomLastLevel]);
                    zoomLastLevel = -1;
                }
            }
            zoomCurrentLevel = correspondingZoomLevel;
        }
    }
}


void mainCore::rayHit(const Qt3DRender::QAbstractRayCaster::Hits &hits)
{
    qDebug() << "triggered";
    if(hits.size() > 0)
    {
        qDebug() << hits.size() << " hit(s) at " << hits[0].worldIntersection() << " (...)";
        switch(rct)
        {
            case raycasttype::dozoom:
            {
                posHit = hits[0].worldIntersection();
                cameraStartPosition = camera->position();
                cameraDirHit = posHit - cameraStartPosition;
                float distance = cameraDirHit.length();
                cameraDirHit /= distance;
                if(zooming) {
                    appliedZoomDuration = t->elapsed() / 1000.0f;
                    easingCoefficient = zoomDistanceFactor * distance * 3.0f / appliedZoomDuration;
                    t->restart();
                }
                else
                {
                    appliedZoomDuration = zoomDuration;
                    easingCoefficient = zoomDistanceFactor * distance * 3.0f / appliedZoomDuration;
                    t = new QElapsedTimer();
                    t->start();
                    zooming = true;
                    connect(fa, &Qt3DLogic::QFrameAction::triggered, this, &mainCore::zoomUpdate);
                }
                break;
            }
            case raycasttype::dopan:
            {
                if(panning)
                {
                    posHit = hits[0].worldIntersection();
                    QVector3D panDelta = posHit - panHit;
                    panDelta.setY(.0f);
                    camera->setPosition(camera->position() - panDelta);
                    QVector3D cvc = camera->position();
                    cvc.setY(.0f);
                    camera->setViewCenter(cvc);
                    doTiles(zoomCurrentLevel);
                }
                else
                {
                    panning = newPanning;
                    panHit = hits[0].worldIntersection();
                }
                break;
            }
        }
    }
}


void mainCore::wheeled(Qt3DInput::QWheelEvent* wheel)
{
    if(wheel->angleDelta().y() > 0)
    {
        mouseX = wheel->x();
        mouseY = viewHeight - wheel->y();
        rct = raycasttype::dozoom;
        src->trigger(QPoint(mouseX, mouseY));
    }
    else
    {
        cameraStartPosition = camera->position();
        QVector3D posNew = cameraStartPosition + (cameraFarRestPosition - cameraStartPosition).normalized() * cameraStartPosition.y() * 2.0f;
        posNew = posNew.y() >= cameraFarRestPosition.y() ? cameraFarRestPosition : posNew;
        cameraDirHit = (posNew - cameraStartPosition);
        float distance = cameraDirHit.length();
        if(distance > .0f) {
            cameraDirHit /= distance;
            if(zooming) {
                appliedZoomDuration = t->elapsed() / 1000.0f;
                easingCoefficient = distance * 3.0f / appliedZoomDuration;
                t->restart();
            }
            else
            {
                appliedZoomDuration = zoomDuration;
                easingCoefficient = distance * 3.0f / appliedZoomDuration;
                t = new QElapsedTimer();
                t->start();
                zooming = true;
                connect(fa, &Qt3DLogic::QFrameAction::triggered, this, &mainCore::zoomUpdate);
            }
        }
    }
    qDebug() << "mouse: " << wheel->x() << " " << wheel->y() << " ";
}

void mainCore::pressed(Qt3DInput::QMouseEvent* mouse)
{
    rct = raycasttype::dopan;
    newPanning = true;
    src->trigger(QPoint(mouse->x(), viewHeight - mouse->y()));
    connect(mh, &Qt3DInput::QMouseHandler::positionChanged, this, &mainCore::moved);
    qDebug() << "mouse pressed: " << mouse->x() << " " << mouse->y() << " ";
}

void mainCore::moved(Qt3DInput::QMouseEvent* mouse)
{
    rct = raycasttype::dopan;
    mouseX = mouse->x();
    mouseY = viewHeight - mouse->y();
    src->trigger(QPoint(mouseX, mouseY));
    qDebug() << "mouse moved: " << mouse->x() << " " << mouse->y() << " ";
}

void mainCore::released(Qt3DInput::QMouseEvent* mouse)
{
    panning = false;
    newPanning = false;
    disconnect(mh, &Qt3DInput::QMouseHandler::positionChanged, this, &mainCore::moved);
}
