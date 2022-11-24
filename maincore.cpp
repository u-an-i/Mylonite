#include "maincore.h"
#include "mainwindow.h"
#include "include/mm.hpp"
#include "include/qt3dwidget.h"



mainCore::mainCore()
{
    mainWindow = new MainWindow();

    mainWindow->show();

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
    lf->addLayer(layer[zoomCurrentLevel]);

    view->setActiveFrameGraph(rss);

    scene = createScene();

    src = (new Derived<Qt3DRender::QScreenRayCaster>())->get();
    //src->addLayer(layer[zoomCurrentLevel]);
    connect(src, &Qt3DRender::QScreenRayCaster::hitsChanged, this, &mainCore::rayHit);

    Qt3DLogic::QFrameAction* fa = (new Derived<Qt3DLogic::QFrameAction>())->get();
    connect(fa, &Qt3DLogic::QFrameAction::triggered, this, &mainCore::frameUpdate);

    Qt3DInput::QMouseHandler* mh = (new Derived<Qt3DInput::QMouseHandler>())->get();
    mh->setSourceDevice((new Derived<Qt3DInput::QMouseDevice>)->get());
    connect(mh, &Qt3DInput::QMouseHandler::wheel, this, &mainCore::wheeled);

    scene->addComponent(src);
    scene->addComponent(fa);
    scene->addComponent(mh);

    view->setRootEntity(scene);

    connect(mainWindow->getPushButtonForAPIKey(), &QPushButton::clicked, this, &mainCore::setAPIKey);
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

    Qt3DCore::QEntity* qe = (new Derived<Qt3DCore::QEntity>())->get();
    cacheQuad.insert("0-0-0", qe);

    qe->setParent(rootEntity);
    qe->addComponent(layer[zoomCurrentLevel]);

    Qt3DExtras::QPlaneMesh* qm = (new Derived<Qt3DExtras::QPlaneMesh>())->get();
    qm->setMeshResolution(QSize(2, 2));
    qm->setWidth(pow(2, zoomCurrentLevel) * smallestQuadSize);
    qm->setHeight(pow(2, zoomCurrentLevel) * smallestQuadSize);

    Qt3DCore::QTransform* qt = (new Derived<Qt3DCore::QTransform>())->get();
    qt->setScale3D(QVector3D(1.0f, 1.0f, 1.0f));
    qt->setRotation(QQuaternion::fromAxisAndAngle(QVector3D(0.0f, 1.0f, 0.0f), 0.0f));
    qt->setTranslation(QVector3D(.0f, .0f, .0f));

    ImageTileRequest::imageTile it = imageTileRequest.getImageTile(0, 0, 0);

    if(it.m != nullptr)
    {
        qe->addComponent(it.m);
    }
    qe->addComponent(qm);
    qe->addComponent(qt);

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


void mainCore::frameUpdate(float dt)
{
    if(zoom)
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
            zoom = false;
        }
        float base = (t/appliedZoomDuration - 1.0f);
        QVector3D posNew = cameraStartPosition + easingCoefficient * appliedZoomDuration * (base * base * base + 1.0f) / 3.0f * cameraDirHit;
        if(posNew.y() <= fmax(cameraFarRestPositionFactor * smallestQuadSize, .01f + (zoomLevelMax + 1) * layerYStackingGap)) {     // keep correspondingZoomLevel >= 0; .01f is camera->nearPlane()
            posNew.setY(fmax(cameraFarRestPositionFactor * smallestQuadSize, .01f + (zoomLevelMax + 1) * layerYStackingGap));
            if(zoom) {
                zoom = false;
                delete this->t;
            }
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
            if(correspondingZoomLevel < zoomCurrentLevel)
            {
                int extraZoom = 2;                                                                                          // generalise for start position
                int appliedZoomLevel = zoomLevelMax - correspondingZoomLevel + extraZoom;
                lf->addLayer(layer[correspondingZoomLevel]);
                int sizeMax = pow(2, appliedZoomLevel);
                float quadSize = maxQuadSize / sizeMax;
                int indexRow = (maxQuadSize/2 - (int)posHit.z()) / quadSize;
                int indexCol = (maxQuadSize/2 + (int)posHit.x()) / quadSize;
                int appliedExtensionY = extensionY * pow(2, extraZoom);
                int appliedExtensionX = extensionX * pow(2, extraZoom);
                int indexStartRow = indexRow > appliedExtensionY - 1 ? indexRow - appliedExtensionY : 0;                    // optimise based on mouse pointer position relative to viewport boundary
                int indexEndRow = indexRow < sizeMax - appliedExtensionY ? indexRow + appliedExtensionY + 1 : sizeMax;
                int indexStartCol = indexCol > appliedExtensionX - 1 ? indexCol - appliedExtensionX : 0;
                int indexEndCol = indexCol < sizeMax - appliedExtensionX ? indexCol + appliedExtensionX + 1 : sizeMax;
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
                            quadEntity->addComponent(layer[correspondingZoomLevel]);

                            Qt3DExtras::QPlaneMesh* qm = (new Derived<Qt3DExtras::QPlaneMesh>())->get();
                            qm->setMeshResolution(QSize(2, 2));
                            qm->setWidth(quadSize);
                            qm->setHeight(quadSize);

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
                if(zoomLastLevel != -1) {
                    lf->removeLayer(layer[zoomLastLevel]);
                }
                zoomLastLevel = zoomCurrentLevel;
            }
            else
            {
                lf->addLayer(layer[correspondingZoomLevel]);
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
        qDebug() << "hit at y = " << hits[0].worldIntersection();
        posHit = hits[0].worldIntersection();
        cameraStartPosition = camera->position();
        cameraDirHit = posHit - cameraStartPosition;
        float distance = cameraDirHit.length();
        cameraDirHit /= distance;
        if(zoom) {
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
            zoom = true;
        }
    }
}


void mainCore::wheeled(Qt3DInput::QWheelEvent* wheel)
{
    if(wheel->angleDelta().y() > 0)
    {
        src->trigger(QPoint(wheel->x(), viewHeight - wheel->y()));
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
            if(zoom) {
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
                zoom = true;
            }
        }
    }
    qDebug() << "mouse: " << wheel->x() << " " << wheel->y() << " ";
}
