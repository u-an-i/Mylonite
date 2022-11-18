#include "maincore.h"
#include "mainwindow.h"
#include "include/mm.hpp"

#include <Qt3DRender>


mainCore::mainCore()
{
    mainWindow = new MainWindow();

    mainWindow->show();

    view = new Qt3DWidget(mainWindow);

    mainWindow->getCentralWidget()->setParent(view);

    mainWindow->setCentralWidget(view);


    scene = createScene();

    // Camera
    Qt3DRender::QCamera* camera = view->camera();
    camera->lens()->setPerspectiveProjection(45.0f, 16.0f/9.0f, 0.1f, 1000.0f);
    camera->setPosition(QVector3D(0, 10.0f, 0.0f));
    camera->setViewCenter(QVector3D(0.0f, 0.0f, 0.0f));

    // For camera controls
    camController = new Qt3DExtras::QOrbitCameraController(scene);
    camController->setLinearSpeed( 50.0f );
    camController->setLookSpeed( 180.0f );
    camController->setCamera(camera);

    view->setRootEntity(scene);

    connect(mainWindow->getPushButtonForAPIKey(), &QPushButton::clicked, this, &mainCore::setAPIKey);
}


mainCore::~mainCore()
{
    delete camController;
    delete scene;
    MemRegistry::obliviate();
    delete view;
    delete mainWindow;
}


Qt3DCore::QEntity* mainCore::createScene()
{
    Qt3DCore::QEntity* rootEntity = new Qt3DCore::QEntity;

    Qt3DCore::QEntity* quadEntity = (new Derived<Qt3DCore::QEntity>())->get();
    quadEntity->setParent(rootEntity);

    Qt3DRender::QMaterial* material = (new Derived<Qt3DExtras::QTextureMaterial>())->get();
    material->setParent(rootEntity);
    Qt3DRender::QTextureImage* ti = (new Derived<Qt3DRender::QTextureImage>())->get();
    ti->setSource(QUrl("file:cache/0_0_0.jpg"));
    Qt3DRender::QTexture2D* t = (new Derived<Qt3DRender::QTexture2D>())->get();
    t->addTextureImage(ti);
    ((Qt3DExtras::QTextureMaterial*)material)->setTexture(t);

    Qt3DExtras::QPlaneMesh* quadMesh = (new Derived<Qt3DExtras::QPlaneMesh>())->get();
    quadMesh->setMeshResolution(QSize(2, 2));
    quadMesh->setWidth(1.0f);
    quadMesh->setHeight(1.0f);

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
