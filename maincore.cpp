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

    mainWindow->getCentralWidget()->setParent(view);

    mainWindow->setCentralWidget(view);


    scene = createScene();

    // Camera
    camera = view->camera();
    camera->lens()->setPerspectiveProjection(45.0f, 16.0f/9.0f, 0.1f, 1000.0f);
    camera->setPosition(cameraFarRestPosition);
    camera->setUpVector(QVector3D(.0f, .0f, -1.0f));
    camera->setViewCenter(QVector3D(0.0f, 0.0f, 0.0f));

    Qt3DInput::QMouseHandler* mh = (new Derived<Qt3DInput::QMouseHandler>())->get();
    mh->setParent(scene);
    mh->setSourceDevice((new Derived<Qt3DInput::QMouseDevice>)->get());
    connect(mh, &Qt3DInput::QMouseHandler::wheel, this, &mainCore::wheeled);

    view->setRootEntity(scene);

    connect(mainWindow->getPushButtonForAPIKey(), &QPushButton::clicked, this, &mainCore::setAPIKey);
}


mainCore::~mainCore()
{
    scene !=  nullptr ? delete scene : NOP_FUNCTION;
    MemRegistry::obliviate();
    mainWindow !=  nullptr ? delete mainWindow : NOP_FUNCTION;
}


Qt3DCore::QEntity* mainCore::createScene()
{
    Qt3DCore::QEntity* rootEntity = new Qt3DCore::QEntity;

    Qt3DLogic::QFrameAction* fa = (new Derived<Qt3DLogic::QFrameAction>())->get();
    rootEntity->addComponent(fa);
    connect(fa, &Qt3DLogic::QFrameAction::triggered, this, &mainCore::frameUpdate);

    /*Qt3DRender::QPickingSettings* ps = (new Derived<Qt3DRender::QPickingSettings>())->get();
    ps->setPickMethod(Qt3DRender::QPickingSettings::TrianglePicking);
    ps->setParent(rootEntity);*/
    Qt3DRender::QLayer* l = (new Derived<Qt3DRender::QLayer>())->get();
    l->setRecursive(true);
    rootEntity->addComponent(l);
    src = (new Derived<Qt3DRender::QScreenRayCaster>())->get();
    src->addLayer(l);
    rootEntity->addComponent(src);

    connect(src, &Qt3DRender::QScreenRayCaster::hitsChanged, this, &mainCore::rayHit);

    Qt3DCore::QEntity* quadEntity = (new Derived<Qt3DCore::QEntity>())->get();
    cacheQuad.insert("0_0_0", quadEntity);
    quadEntity->setParent(rootEntity);

    Qt3DRender::QMaterial* material = (new Derived<Qt3DExtras::QTextureMaterial>())->get();
    Qt3DRender::QTextureImage* ti = (new Derived<Qt3DRender::QTextureImage>())->get();
    ti->setSource(QUrl("file:cache/0_0_0.jpg"));
    Qt3DRender::QTexture2D* t = (new Derived<Qt3DRender::QTexture2D>())->get();
    t->addTextureImage(ti);
    ((Qt3DExtras::QTextureMaterial*)material)->setTexture(t);

    Qt3DExtras::QPlaneMesh* quadMesh = (new Derived<Qt3DExtras::QPlaneMesh>())->get();
    quadMesh->setMeshResolution(QSize(2, 2));
    quadMesh->setWidth(100.0f);
    quadMesh->setHeight(100.0f);

    Qt3DCore::QTransform* quadTransform = (new Derived<Qt3DCore::QTransform>())->get();
    quadTransform->setScale3D(QVector3D(1.0f, 1.0f, 1.0f));
    quadTransform->setRotation(QQuaternion::fromAxisAndAngle(QVector3D(0.0f, 1.0f, 0.0f), 0.0f));

    quadEntity->addComponent(quadMesh);
    quadEntity->addComponent(quadTransform);
    quadEntity->addComponent(material);

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
         * v = A * (t/T - 1)^2;
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
        posNew.setY(.0f);
        camera->setViewCenter(posNew);
    }
}


void mainCore::rayHit(const Qt3DRender::QAbstractRayCaster::Hits &hits)
{
    qDebug() << "triggered";
    if(hits.size() > 0)
    {
        qDebug() << "hit at y = " << hits[0].worldIntersection().y() << " (should be 0)";
        QVector3D posHit = hits[0].worldIntersection();
        posHit.setY(.0f);
        cameraStartPosition = camera->position();
        cameraDirHit = (posHit - cameraStartPosition);
        float distance = cameraDirHit.length();
        cameraDirHit /= distance;
        if(zoom) {
            appliedZoomDuration *= .5f;
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
    float wheelDir = wheel->angleDelta().y();
    if(wheelDir > 0)
    {
        this->wheelDir = wheelDir;
        src->trigger(QPoint(wheel->x(), viewHeight-wheel->y()));
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
                appliedZoomDuration *= .5f;
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
    qDebug() << "mouse: " << wheelDir << " " << wheel->x() << " " << wheel->y() << " " << viewHeight-wheel->y();
}
