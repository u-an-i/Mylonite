#include "maincore.h"
#include "mainwindow.h"

#include <Qt3DRender>


mainCore::mainCore()
{
    mainWindow = new MainWindow();

    mainWindow->show();

    view = new Qt3DWidget(mainWindow);

    mainWindow->getCentralWidget()->setParent(view);

    mainWindow->setCentralWidget(view);


    Qt3DCore::QEntity* scene = createScene();

    // Camera
    Qt3DRender::QCamera* camera = view->camera();
    camera->lens()->setPerspectiveProjection(45.0f, 16.0f/9.0f, 0.1f, 1000.0f);
    camera->setPosition(QVector3D(0, 40.0f, 0.0f));
    camera->setViewCenter(QVector3D(0.0f, 0.0f, 0.0f));

    // For camera controls
    Qt3DExtras::QOrbitCameraController* camController = new Qt3DExtras::QOrbitCameraController(scene);
    camController->setLinearSpeed( 50.0f );
    camController->setLookSpeed( 180.0f );
    camController->setCamera(camera);

    view->setRootEntity(scene);


    QObject::connect(mainWindow->getPushButtonForAPIKey(), &QPushButton::clicked, this, &mainCore::setAPIKey);
}


mainCore::~mainCore()
{
    delete view;
    delete mainWindow;
}


Qt3DCore::QEntity* mainCore::createScene()
{
    Qt3DCore::QEntity* rootEntity = new Qt3DCore::QEntity;


    Qt3DCore::QEntity* quadEntity = new Qt3DCore::QEntity(rootEntity);

    Qt3DRender::QMaterial* material = new Qt3DExtras::QTextureMaterial(rootEntity);
    //((Qt3DExtras::QTextureMaterial)material).setTexture()

    Qt3DExtras::QPlaneMesh* quadMesh = new Qt3DExtras::QPlaneMesh;
    quadMesh->setMeshResolution(QSize(2, 2));
    quadMesh->setWidth(1.0f);
    quadMesh->setHeight(1.0f);

    Qt3DCore::QTransform* quadTransform = new Qt3DCore::QTransform;
    quadTransform->setScale3D(QVector3D(1.0f, 1.0f, 1.0f));
    quadTransform->setRotation(QQuaternion::fromAxisAndAngle(QVector3D(0.0f, 1.0f, 0.0f), 0.0f));

    quadEntity->addComponent(quadMesh);
    quadEntity->addComponent(quadTransform);
    quadEntity->addComponent(material);

    return rootEntity;
}


void mainCore::setAPIKey()
{

}
