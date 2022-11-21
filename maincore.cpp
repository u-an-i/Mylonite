#include "maincore.h"
#include "mainwindow.h"
#include "include/mm.hpp"
#include "include/qt3dwidget.h"



mainCore::mainCore()
{
    mainWindow = new MainWindow();

    mainWindow->show();

    view = (new Derived<Qt3DWidget>())->get();
    viewHeight = view->height();
    connect(view, &Qt3DWidget::resized, this, &mainCore::resized);

    view->renderSettings()->pickingSettings()->setPickMethod(Qt3DRender::QPickingSettings::TrianglePicking);

    mainWindow->getCentralWidget()->setParent(view);

    mainWindow->setCentralWidget(view);

    // Camera
    camera = view->camera();
    camera->lens()->setPerspectiveProjection(45.0f, 16.0f/9.0f, 0.1f, 1000.0f);
    camera->setPosition(cameraFarRestPosition);
    camera->setUpVector(QVector3D(.0f, .0f, -1.0f));
    camera->setViewCenter(QVector3D(0.0f, 0.0f, 0.0f));
    camera->setNearPlane(.01f);         // .0f lets QScreenRayCast hit something at center of screen and not one of the created mesh
    camera->setFarPlane(cameraFarRestPosition.y() + 1.0f);

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
    cacheQuad.insert("0_0_0", qe);

    qe->setParent(rootEntity);
    qe->addComponent(layer[zoomCurrentLevel]);

    Qt3DRender::QMaterial* m = (new Derived<Qt3DExtras::QTextureMaterial>())->get();
    Qt3DRender::QTextureImage* ti = (new Derived<Qt3DRender::QTextureImage>())->get();
    ti->setSource(QUrl("file:cache/0_0_0.jpg"));
    Qt3DRender::QTexture2D* t = (new Derived<Qt3DRender::QTexture2D>())->get();
    t->addTextureImage(ti);
    ((Qt3DExtras::QTextureMaterial*)m)->setTexture(t);

    Qt3DExtras::QPlaneMesh* qm = (new Derived<Qt3DExtras::QPlaneMesh>())->get();
    qm->setMeshResolution(QSize(2, 2));
    qm->setWidth(pow(2, zoomCurrentLevel) * smallestQuadSize);
    qm->setHeight(pow(2, zoomCurrentLevel) * smallestQuadSize);

    Qt3DCore::QTransform* qt = (new Derived<Qt3DCore::QTransform>())->get();
    qt->setScale3D(QVector3D(1.0f, 1.0f, 1.0f));
    qt->setRotation(QQuaternion::fromAxisAndAngle(QVector3D(0.0f, 1.0f, 0.0f), 0.0f));
    qt->setTranslation(QVector3D(.0f, .0f, .0f));

    qe->addComponent(qm);
    qe->addComponent(qt);
    qe->addComponent(m);

    return rootEntity;
}


void mainCore::setAPIKey()
{
    qDebug() << "API key: " << mainWindow->getLineEditForAPIKey()->text();
}


void mainCore::resized()
{
    viewHeight = view->height();
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
        camera->setPosition(posNew);
        int correspondingZoomLevel = log2f(posNew.y() / (2.0f * smallestQuadSize));
        posNew.setY(.0f);
        camera->setViewCenter(posNew);
        if(correspondingZoomLevel != zoomCurrentLevel)
        {
            qDebug() << "co: " << correspondingZoomLevel << ", cu: " << zoomCurrentLevel;
            //src->removeLayer(layer[zoomCurrentLevel]);
            //src->addLayer(layer[correspondingZoomLevel]);
            lf->removeLayer(layer[zoomCurrentLevel]);
            lf->addLayer(layer[correspondingZoomLevel]);
            int zcl = zoomCurrentLevel;
            zoomCurrentLevel = correspondingZoomLevel;
            if(correspondingZoomLevel < zcl)
            {
                int sizeMax = pow(2, zoomLevelMax - zoomCurrentLevel);
                float quadSize = maxQuadSize / sizeMax;
                int indexRow = (maxQuadSize/2 - (int)posHit.z()) / quadSize;
                int indexCol = (maxQuadSize/2 + (int)posHit.x()) / quadSize;
                int indexStartRow = indexRow > 0 ? indexRow - 1 : 0;
                int indexEndRow = indexRow < sizeMax - 1 ? indexRow + 2 : sizeMax;
                int indexStartCol = indexCol > 0 ? indexCol - 1 : 0;
                int indexEndCol = indexCol < sizeMax - 1 ? indexCol + 2 : sizeMax;
                for(int i = indexStartRow; i < indexEndRow; ++i)
                {
                    for(int j = indexStartCol; j < indexEndCol; ++j)
                    {
                        QString key = QString::number(zoomCurrentLevel) + "_" + QString::number(j) + "_" + QString::number(i);
                        Qt3DCore::QEntity* quadEntity = cacheQuad[key];
                        if(quadEntity == nullptr) {
                            quadEntity = (new Derived<Qt3DCore::QEntity>())->get();
                            cacheQuad[key] = quadEntity;

                            quadEntity->setParent(scene);
                            quadEntity->addComponent(layer[zoomCurrentLevel]);

                            Qt3DRender::QMaterial* m = (new Derived<Qt3DExtras::QTextureMaterial>())->get();
                            Qt3DRender::QTextureImage* ti = (new Derived<Qt3DRender::QTextureImage>())->get();
                            ti->setSource(QUrl("file:cache/0_0_0.jpg"));
                            Qt3DRender::QTexture2D* t = (new Derived<Qt3DRender::QTexture2D>())->get();
                            t->addTextureImage(ti);
                            ((Qt3DExtras::QTextureMaterial*)m)->setTexture(t);

                            Qt3DExtras::QPlaneMesh* qm = (new Derived<Qt3DExtras::QPlaneMesh>())->get();
                            qm->setMeshResolution(QSize(2, 2));
                            qm->setWidth(quadSize);
                            qm->setHeight(quadSize);

                            Qt3DCore::QTransform* qt = (new Derived<Qt3DCore::QTransform>())->get();
                            qt->setScale3D(QVector3D(1.0f, 1.0f, 1.0f));
                            qt->setRotation(QQuaternion::fromAxisAndAngle(QVector3D(0.0f, 1.0f, 0.0f), 0.0f));
                            qt->setTranslation(QVector3D(-maxQuadSize/2 + quadSize * (.5f + j), .0f, maxQuadSize/2 - quadSize * (.5f + i)));

                            quadEntity->addComponent(qm);
                            quadEntity->addComponent(qt);
                            quadEntity->addComponent(m);
                        }
                    }
                }
            }
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
        QVector3D posNew = cameraStartPosition + (cameraFarRestPosition - cameraStartPosition).normalized() * (cameraStartPosition.y() < zoomBackDistance ? zoomBackDistanceSmall : zoomBackDistance);
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
