#include "maincore.h"
#include "mainwindow.h"
#include "include/mm.hpp"
#include "include/qt3dwidget.h"

#include <Qt3DRender>


mainCore::mainCore()
{
    mainWindow = new MainWindow();

    mainWindow->show();

    Qt3DWidget* view = (new Derived<Qt3DWidget>())->get();

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


void mainCore::rayHit(const Qt3DRender::QAbstractRayCaster::Hits &hits)
{
    qDebug() << "triggered";
    if(hits.size() > 0)
    {
        qDebug() << "hit";
        QVector3D posCam = camera->position();
        QVector3D posNew = posCam + (hits[0].worldIntersection() - posCam).normalized() * wheelDir / 120.0f;
        posNew = posNew.y() < camera->nearPlane() ? QVector3D(posNew.x(), camera->nearPlane(), posNew.z()) : posNew;
        camera->setPosition(posNew);
        posNew.setY(.0f);
        camera->setViewCenter(posNew);
    }
}


void mainCore::wheeled(Qt3DInput::QWheelEvent* wheel)
{
    float wheelDir = wheel->angleDelta().y();
    if(wheelDir > 0)
    {
        this->wheelDir = wheelDir;
        src->trigger(QPoint(wheel->x(), wheel->y()));
    }
    else
    {
        QVector3D posCam = camera->position();
        QVector3D posNew = posCam + (posCam - cameraFarRestPosition).normalized() * wheelDir / 120.0f;
        posNew = posNew.y() > cameraFarRestPosition.y() ? cameraFarRestPosition : posNew;
        camera->setPosition(posNew);
        posNew.setY(.0f);
        camera->setViewCenter(posNew);
    }
    qDebug() << "mouse: " << wheelDir << " " << wheel->x()<< " " << wheel->y();
}
