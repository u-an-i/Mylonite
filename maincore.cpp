#include "maincore.h"
#include "mainwindow.h"
#include "include/mm.hpp"
#include "include/qt3dwidget.h"



mainCore::mainCore()
{
    mainWindow = new MainWindow();

    mainWindow->show();

    connect(mainWindow->getPushButtonForURL(), &QPushButton::clicked, this, &mainCore::setURL);
    connect(mainWindow->getType(), &QComboBox::activated, this, &mainCore::changeType);

    QComboBox* cbURL = mainWindow->getURL();
    QFile file("urls.txt");
    if (file.open(QIODeviceBase::ReadOnly | QIODeviceBase::Text))
    {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine();
            if(line.length() > 0)
            {
                MapIdentifyingText* url = new MapIdentifyingText();
                int result = url->addText(line);
                if(result == 0)
                {
                    cbURL->addItem(line, QVariant(urls.count()));
                    urls.append(url);
                    createHolders(url);
                }
            }
        }
        cbURL->model()->sort(0);
        file.close();
    }

    if(!cbURL->currentData().isValid() || !urls[cbURL->currentData().toInt()]->hasTypes())
    {
        mainWindow->getType()->hide();
    }

    imageTileRequest.setCache(&cacheQuad);

    view = (new Derived<Qt3DWidget>())->get();
    resized();
    connect(view, &Qt3DWidget::resized, this, &mainCore::resized);

    view->renderSettings()->pickingSettings()->setPickMethod(Qt3DRender::QPickingSettings::TrianglePicking);

    mainWindow->getCentralWidget()->setParent(view);

    mainWindow->setCentralWidget(view);

    camera = view->camera();
    camera->lens()->setPerspectiveProjection(45.0f, 16.0f/9.0f, .01f, cameraFarRestDistance + 2.0f);        // nearPlane .0f lets QScreenRayCast (when used) hit something at center of screen and not one of the created mesh
    camera->setPosition(posCameraFarRest);
    camera->setUpVector(QVector3D(.0f, .0f, -1.0f));
    camera->setViewCenter(QVector3D(0.0f, mapPlaneY, 0.0f));

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

    scene = new Qt3DCore::QEntity;

    mh = (new Derived<Qt3DInput::QMouseHandler>())->get();
    mh->setSourceDevice((new Derived<Qt3DInput::QMouseDevice>)->get());

    fa = (new Derived<Qt3DLogic::QFrameAction>())->get();

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


void mainCore::changeType(int index)
{
    if(currentLayer != nullptr)
    {
        for(int i=0; i<currentLayer->count(); ++i)
        {
            lf->removeLayer(currentLayer->at(i));
        }
    }
    QString type = mainWindow->getType()->currentText();
    currentCacheQuad = currentTypeCacheQuad->value(type);
    currentLayer = currentTypeLayer->value(type);
    doTiles(zoomCurrentLevel, false);
    lf->addLayer(currentLayer->value(zoomCurrentLevel));
}


void mainCore::createHolders(MapIdentifyingText* url)
{
    QString urlHashKey = url->getHashKey();
    layer[urlHashKey] = (new Derived<QHash<QString, QList<Qt3DRender::QLayer*>*>>())->get();
    currentTypeLayer = layer[urlHashKey];
    cacheQuad[urlHashKey] = (new Derived<QHash<QString, QHash<QString, Qt3DCore::QEntity*>*>>())->get();
    currentTypeCacheQuad = cacheQuad[urlHashKey];
    zoomLevelMax = abs(url->getZoomFarest() - url->getZoomClosest());
    QComboBox* type = mainWindow->getType();
    url->setSourceType(type);
    if(url->hasTypes())
    {
        type->clear();
        QStringList types = url->getTypes();
        type->addItems(types);
        for(int i=0; i<types.length(); ++i)
        {
            currentTypeLayer->insert(types[i], (new Derived<QList<Qt3DRender::QLayer*>>())->get());
            currentLayer = currentTypeLayer->value(types[i]);
            for(int j=0; j<=zoomLevelMax; ++j)
            {
                currentLayer->insert(j, (new Derived<Qt3DRender::QLayer>())->get());
            }
            currentTypeCacheQuad->insert(types[i], (new Derived<QHash<QString, Qt3DCore::QEntity*>>())->get());
            currentCacheQuad = currentTypeCacheQuad->value(types[i]);
        }
        lastTypes[urlHashKey] = 0;
        type->setCurrentIndex(0);
        type->show();
    }
    else
    {
        currentTypeLayer->insert("d", (new Derived<QList<Qt3DRender::QLayer*>>())->get());
        currentLayer = currentTypeLayer->value("d");
        for(int j=0; j<=zoomLevelMax; ++j)
        {
            currentLayer->insert(j, (new Derived<Qt3DRender::QLayer>())->get());
        }
        currentTypeCacheQuad->insert("d", (new Derived<QHash<QString, Qt3DCore::QEntity*>>())->get());
        currentCacheQuad = currentTypeCacheQuad->value("d");
        type->hide();
    }
}


void mainCore::setURL()
{
    disconnect(mh, &Qt3DInput::QMouseHandler::wheel, this, &mainCore::wheeled);
    disconnect(mh, &Qt3DInput::QMouseHandler::pressed, this, &mainCore::pressed);
    disconnect(mh, &Qt3DInput::QMouseHandler::released, this, &mainCore::released);
    disconnect(fa, &Qt3DLogic::QFrameAction::triggered, this, &mainCore::zoomUpdate);
    zooming = false;
    if(currentLayer != nullptr)
    {
        for(int i=0; i<currentLayer->count(); ++i)
        {
            lf->removeLayer(currentLayer->at(i));
        }
    }
    QComboBox* cbURL = mainWindow->getURL();
    QString text = cbURL->currentText();
    QVariant data = cbURL->currentData();
    if(data.isValid())
    {
        MapIdentifyingText* url = urls[data.toInt()];
        QString urlHashKey = url->getHashKey();
        currentTypeLayer = layer[urlHashKey];
        currentTypeCacheQuad = cacheQuad[urlHashKey];
        zoomLevelMax = abs(url->getZoomFarest() - url->getZoomClosest());
        QComboBox* type = mainWindow->getType();
        if(url->hasTypes())
        {
            type->clear();
            QStringList types = url->getTypes();
            type->addItems(types);
            currentCacheQuad = currentTypeCacheQuad->value(types[lastTypes[urlHashKey]]);
            currentLayer = currentTypeLayer->value(types[lastTypes[urlHashKey]]);
            type->setCurrentIndex(lastTypes[urlHashKey]);
            type->show();
        }
        else
        {
            currentCacheQuad = currentTypeCacheQuad->value("d");
            currentLayer = currentTypeLayer->value("d");
            type->hide();
        }
        imageTileRequest.setURL(url);
        connect(fa, &Qt3DLogic::QFrameAction::triggered, this, &mainCore::initUpdate);
    }
    else
    {
        MapIdentifyingText* url = new MapIdentifyingText();
        int result = url->addText(text);
        if(result == 0)
        {
            QFile file("urls.txt");
            if (file.open(QIODeviceBase::Append | QIODeviceBase::Text))
            {
                QTextStream out(&file);
                out << text + "\n";
                out.flush();
                file.close();
            }
            cbURL->setItemData(cbURL->currentIndex(), QVariant(urls.count()));
            urls.append(url);
            createHolders(url);
            imageTileRequest.setURL(url);
            connect(fa, &Qt3DLogic::QFrameAction::triggered, this, &mainCore::initUpdate);
        }
        else
        {
            delete url;
            cbURL->removeItem(cbURL->currentIndex());
            QComboBox* type = mainWindow->getType();
            type->hide();
            QString bailedAts[] = {
                "no zoom range given ('+' missing)",
                "not 1 farest and 1 closest zoom-level given or '-' not present",
                "farest zoom-level no integer",
                "closest zoom-level no integer",
                "placeholders not present or too many of them",
                "url could not be converted to a operating system folder for a cache,\nurl should contain at least 1 alphabetical character.",
                "cache folder could not be created.\nPerhaps operating system permissions insufficient or out of space."
                "every map type name should contain at least 1 alphabetical character (different from every other)\nand operating system permissions and space must be sufficient to create a cache folder for each."
            };
            if(result > 7)
            {
                result = 7;
            }
            else
            {
                --result;
            }
            QMessageBox::warning(mainWindow, "Info", "map identifying text not accepted, reason:\n\n" +  bailedAts[result]);
        }
    }
}


void mainCore::initUpdate(float dt)
{
    float zoomCurrent = log2f((camera->position().y() - mapPlaneY) / (cameraFarRestPositionFactor * smallestQuadSize));
    double relativeX = camera->position().x() / (maxQuadSize/2.0);
    double relativeZ = camera->position().z() / (maxQuadSize/2.0);
    maxQuadSize = pow(2, zoomLevelMax) * smallestQuadSize;
    cameraFarRestDistance = cameraFarRestPositionFactor * maxQuadSize;
    mapPlaneY = -cameraFarRestDistance / 2.0;
    posCameraFarRest = QVector3D(.0f, cameraFarRestDistance / 2.0, .0);
    if(camera->farPlane() < cameraFarRestDistance + 2.0f)
    {
        camera->setFarPlane(cameraFarRestDistance + 2.0f);
    }
    if(firstSet || zoomCurrent >= zoomLevelMax)
    {
        firstSet = false;
        zoomCurrent = zoomLevelMax;
        relativeX = 0.0;
        relativeZ = 0.0;
        posHit = QVector3D(.0f, mapPlaneY, .0f);
        mouseX = view->width() / 2;
        mouseY = view->height() / 2;
    }
    QVector3D cp = QVector3D(relativeX * maxQuadSize/2.0, mapPlaneY + pow(2, zoomCurrent) * cameraFarRestPositionFactor * smallestQuadSize, relativeZ * maxQuadSize/2.0);
    camera->setPosition(cp);
    cp.setY(mapPlaneY);
    camera->setViewCenter(cp);
    zoomCurrentLevel = zoomCurrent;
    doTiles(zoomCurrentLevel, false);
    lf->addLayer(currentLayer->value(zoomCurrentLevel));
    disconnect(fa, &Qt3DLogic::QFrameAction::triggered, this, &mainCore::initUpdate);
    connect(mh, &Qt3DInput::QMouseHandler::wheel, this, &mainCore::wheeled);
    connect(mh, &Qt3DInput::QMouseHandler::pressed, this, &mainCore::pressed);
    connect(mh, &Qt3DInput::QMouseHandler::released, this, &mainCore::released);
}


void mainCore::resized()
{
    viewHeight = view->height();
    viewWidth = view->width();
}


bool mainCore::doTiles(int forZoomLevel, bool zoomingOut)
{
    int appliedZoomLevel = zoomLevelMax - forZoomLevel + extraZoom;
    if(appliedZoomLevel <= zoomLevelMax) {
        int sizeMax = pow(2, appliedZoomLevel);
        float quadSize = maxQuadSize / sizeMax;
        int indexCol = (maxQuadSize/2 + posHit.x()) / quadSize;
        int indexRow = (maxQuadSize/2 - posHit.z()) / quadSize;
        int appliedExtensionX = (ceil((extensionY + (zoomingOut ? 1 : 0)) * viewWidth / viewHeight)) * pow(2, extraZoom);
        int appliedExtensionY = (extensionY + (zoomingOut ? 1 : 0)) * pow(2, extraZoom);
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
        indexEndRow = indexEndRow < sizeMax ? indexEndRow : sizeMax;
        for(int i = indexStartRow; i < indexEndRow; ++i)
        {
            for(int j = indexStartCol; j < indexEndCol; ++j)
            {
                QString key = QString::number(appliedZoomLevel) + "-" + QString::number(j) + "-" + QString::number(i);
                Qt3DCore::QEntity* quadEntity = currentCacheQuad->value(key);
                if(quadEntity == nullptr) {
                    quadEntity = (new Derived<Qt3DCore::QEntity>())->get();
                    currentCacheQuad->insert(key, quadEntity);

                    quadEntity->setParent(scene);
                    quadEntity->addComponent(currentLayer->value(forZoomLevel));

                    Qt3DExtras::QPlaneMesh* qm = (new Derived<Qt3DExtras::QPlaneMesh>())->get();
                    qm->setMeshResolution(QSize(2, 2));
                    qm->setWidth(quadSize * (appliedZoomLevel > zoomLevelMax - 3 ? 1.02f : 1.0f));        // * 1.02f = prevent gaps between quads on close camera
                    qm->setHeight(quadSize * (appliedZoomLevel > zoomLevelMax - 3 ? 1.02f : 1.0f));       // * 1.02f = see above

                    Qt3DCore::QTransform* qt = (new Derived<Qt3DCore::QTransform>())->get();
                    qt->setScale3D(QVector3D(1.0f, 1.0f, 1.0f));
                    qt->setRotation(QQuaternion::fromAxisAndAngle(QVector3D(0.0f, 1.0f, 0.0f), 0.0f));
                    qt->setTranslation(QVector3D(-maxQuadSize/2 + quadSize * (.5f + j), mapPlaneY + layerYStackingGap * appliedZoomLevel, maxQuadSize/2 - quadSize * (.5f + i)));

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
    bool newZooming = true;
    double t = this->t->nsecsElapsed();
    if(t > appliedZoomDuration)
    {
        t = appliedZoomDuration;
        newZooming = false;
        disconnect(fa, &Qt3DLogic::QFrameAction::triggered, this, &mainCore::zoomUpdate);
        delete this->t;
    }
    double base = (t/appliedZoomDuration - 1.0);
    QVector3D posNew = posCameraStart + easingCoefficient * (base * base * base + 1.0) / 3.0 * dirCameraHit;                                // don't multiply by appliedZoomDuration here, don't divide by that earlier
    if(posNew.y() - mapPlaneY <= fmax(cameraFarRestPositionFactor * smallestQuadSize, .01 + (zoomLevelMax + 1) * layerYStackingGap)) {      // keep correspondingZoomLevel >= 0; .01f is camera->nearPlane()
        posNew.setY(mapPlaneY + fmax(cameraFarRestPositionFactor * smallestQuadSize, .01 + (zoomLevelMax + 1) * layerYStackingGap));
        if(newZooming) {
            newZooming = false;
            disconnect(fa, &Qt3DLogic::QFrameAction::triggered, this, &mainCore::zoomUpdate);
            delete this->t;
        }qDebug() << "here";
    }
    camera->setPosition(posNew);
    int correspondingZoomLevel = log2f((posNew.y() - mapPlaneY) / (cameraFarRestPositionFactor * smallestQuadSize));
    posNew.setY(mapPlaneY);
    camera->setViewCenter(posNew);
    if(correspondingZoomLevel != zoomCurrentLevel)
    {qDebug() << "cam pos: " << camera->position().y() - mapPlaneY;
        qDebug() << "co: " << correspondingZoomLevel << ", cu: " << zoomCurrentLevel;
        if(doTiles(correspondingZoomLevel, correspondingZoomLevel > zoomCurrentLevel))
        {
            lf->addLayer(currentLayer->value(correspondingZoomLevel));
            if(correspondingZoomLevel < zoomCurrentLevel)
            {
                if(zoomLastLevel != -1) {
                    lf->removeLayer(currentLayer->value(zoomLastLevel));
                }
                zoomLastLevel = zoomCurrentLevel;
            }
            else
            {
                lf->removeLayer(currentLayer->value(zoomCurrentLevel));
                if(zoomLastLevel != -1 && zoomLastLevel != correspondingZoomLevel)
                {
                    lf->removeLayer(currentLayer->value(zoomLastLevel));
                    zoomLastLevel = -1;
                }
            }
            zoomCurrentLevel = correspondingZoomLevel;
        }
    }
    zooming = newZooming;
}


void mainCore::zoomInit(double distance, float wheelDelta)
{
    if(zooming)
    {
        appliedZoomDuration = t->nsecsElapsed();
        if(appliedZoomDuration < minZoomDuration)
        {
            appliedZoomDuration = minZoomDuration;
        }
        easingCoefficient = distance * 3.0;                         // don't divide by appliedZoomDuration here, don't multiply by that later
        t->restart();
    }
    else
    {
        appliedZoomDuration = zoomDuration * 120.0 / wheelDelta;
        easingCoefficient = distance * 3.0;                         // don't divide by appliedZoomDuration here, don't multiply by that later
        t = new QElapsedTimer();
        t->start();
        zooming = true;
    }
    connect(fa, &Qt3DLogic::QFrameAction::triggered, this, &mainCore::zoomUpdate);
}


mainCore::mapHitCoordinates mainCore::getMapHit()
{
    mapHitCoordinates returnValue;
    returnValue.d = mapPlaneY + layerYStackingGap * (zoomLevelMax - zoomCurrentLevel + extraZoom);
    double distance = posCamY - returnValue.d;
    double factor1 = camera->fieldOfView() * camera->aspectRatio() * 3.1415926535897931 / (180.0 * (viewWidth - 1.0));          // viewWidth must be > 1
    double factor2 = camera->fieldOfView() * 3.1415926535897931 / (180.0 * (viewHeight - 1.0));                                 // viewHeight must be > 1
    returnValue.x = posCamX + tan((mouseX + .5 - viewWidth / 2.0) * factor1) * distance;
    returnValue.y = posCamZ + tan((mouseY + .5 - viewHeight / 2.0) * factor2) * distance;
    qDebug() << "hit: " << returnValue.x << " " << returnValue.y;
    return returnValue;
}


void mainCore::wheeled(Qt3DInput::QWheelEvent* wheel)
{
    qDebug() << "mouse: " << wheel->x() << " " << wheel->y() << " " << wheel->angleDelta().y();
    if(zooming)
    {
        disconnect(fa, &Qt3DLogic::QFrameAction::triggered, this, &mainCore::zoomUpdate);
    }
    mouseX = wheel->x();
    mouseY = viewHeight - wheel->y();
    posCameraStart = camera->position();
    double distanceFactor;
    if(wheel->angleDelta().y() > 0)
    {
        if(viewWidth == 1 || viewHeight == 1)
        {
            return;
        }
        posCamX = camera->position().x();
        posCamY = camera->position().y();
        posCamZ = camera->position().z();
        mapHitCoordinates hit = getMapHit();
        posHit = QVector3D(hit.x, hit.d, hit.y);
        distanceFactor = zoomDistanceFactor;
    }
    else
    {
        posHit = posCameraStart + (posCameraFarRest - posCameraStart).normalized() * (posCameraStart.y() - (mapPlaneY + layerYStackingGap * (zoomLevelMax - zoomCurrentLevel + extraZoom))) * 2.0f;
        posHit = posHit.y() > posCameraFarRest.y() ? posCameraFarRest : posHit;
        distanceFactor = 1.0;
    }
    dirCameraHit = posHit - posCameraStart;
    double x = dirCameraHit.x(), y = dirCameraHit.y(), z = dirCameraHit.z();
    double distance = sqrt(x*x + y*y + z*z);
    if(distance > .0)
    {
        dirCameraHit /= distance;
        zoomInit(distanceFactor * distance, abs(wheel->angleDelta().y()));
    }
}


void mainCore::pressed(Qt3DInput::QMouseEvent* mouse)
{
    if(viewWidth != 1 && viewHeight != 1)
    {
        qDebug() << "mouse pressed: " << mouse->x() << " " << mouse->y() << " ";
        mouseX = mouse->x();
        mouseY = viewHeight - mouse->y();
        posCamX = camera->position().x();
        posCamY = camera->position().y();
        posCamZ = camera->position().z();
        panHit = getMapHit();
        connect(mh, &Qt3DInput::QMouseHandler::positionChanged, this, &mainCore::moved);
    }
}


void mainCore::moved(Qt3DInput::QMouseEvent* mouse)
{
    qDebug() << "mouse moved: " << mouse->x() << " " << mouse->y() << " ";
    mouseX = mouse->x();
    mouseY = viewHeight - mouse->y();
    mapHitCoordinates hit = getMapHit();
    posHit = QVector3D(hit.x, hit.d, hit.y);
    qDebug() << "hit: y: " << hit.y;
    double panDeltaX = hit.x - panHit.x, panDeltaY = hit.y - panHit.y;
    posCamX -= panDeltaX;
    qDebug() << "hit: cam y before: " << posCamZ << " " << camera->position().z();
    posCamZ -= panDeltaY;
    qDebug() << "hit: delta y: " << panDeltaY;
    camera->setPosition(QVector3D(posCamX, camera->position().y(), posCamZ));
    qDebug() << "hit: cam y after:  " << posCamZ << " " << camera->position().z();
    /*camera->setPosition(QVector3D(posCamX, camera->position().y(), 871491.0));
    qDebug() << camera->position().z();
    camera->setPosition(QVector3D(posCamX, camera->position().y(), 871491));
    qDebug() << camera->position().z();
    camera->setPosition(QVector3D(posCamX, camera->position().y(), 871491.0f));
    qDebug() << camera->position().z();*/
    QVector3D cvc = camera->position();
    cvc.setY(mapPlaneY);
    camera->setViewCenter(cvc);
    doTiles(zoomCurrentLevel, true);
}


void mainCore::released(Qt3DInput::QMouseEvent* mouse)
{
    disconnect(mh, &Qt3DInput::QMouseHandler::positionChanged, this, &mainCore::moved);
}
