#include "imagetilerequest.h"
#include "include/mm.hpp"

#include <QNetworkAccessManager>
#include <Qt3DExtras>


ImageTileRequest::ImageTileRequest()
{
    manager = new QNetworkAccessManager();
    QObject::connect(manager, &QNetworkAccessManager::finished, this, &ImageTileRequest::replyFinished);
}


ImageTileRequest::~ImageTileRequest()
{
    delete manager;
}


void ImageTileRequest::setURL(MapIdentifyingText* url) {
    this->url = url;
    while(!queue.isEmpty())
    {
        QString key = queue.dequeue();
        requestObject* ro = (new Derived<ImageTileRequest::requestObject>())->get();
        ro->key = key;
        ro->url = url;
        QStringList parts = key.split('-');
        QNetworkRequest nr = QNetworkRequest(url->getURL(parts[1].toInt(), parts[2].toInt(), parts[0].toInt()));
        nr.setOriginatingObject(ro);
        manager->get(nr);
    }
}


void ImageTileRequest::setCache(QHash<QString, QHash<QString, QHash<QString, Qt3DCore::QEntity*>*>*>* toCache)
{
    cacheQuad = toCache;
}


bool ImageTileRequest::isReady()
{
    return url->isValid();
}


void ImageTileRequest::replyFinished(QNetworkReply* reply)
{
    requestObject* ro = (requestObject*)(reply->request().originatingObject());
    QString key = ro->key;
    MapIdentifyingText* url = ro->url;
    if(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) == 200)
    {
        QString path = "cache/" + url->getCacheFolder() + "/" + key;
        QByteArray data = reply->readAll();
        QSaveFile sf = QSaveFile(path);
        if(sf.open(QIODevice::WriteOnly))
        {
            sf.write(data);
            sf.commit();
            if(cacheQuad != nullptr)
            {
                Qt3DRender::QMaterial* m = (new Derived<Qt3DExtras::QTextureMaterial>())->get();
                Qt3DRender::QTextureImage* ti = (new Derived<Qt3DRender::QTextureImage>())->get();
                ti->setSource(QUrl("file:" + path));
                Qt3DRender::QTexture2D* t = (new Derived<Qt3DRender::QTexture2D>())->get();
                t->setMagnificationFilter(Qt3DRender::QAbstractTexture::Linear);
                t->setMinificationFilter(Qt3DRender::QAbstractTexture::Linear);
                t->addTextureImage(ti);
                ((Qt3DExtras::QTextureMaterial*)m)->setTexture(t);
                cacheQuad->value(url->getHashKey())->value(ro->type)->value(key)->addComponent(m);
            }
        }
    }
    else
    {
        queue.enqueue(key);     // consider later auto dequeuing
    }
    reply->deleteLater();
}

ImageTileRequest::imageTile ImageTileRequest::getImageTile(int zoom, int x, int y)
{
    ImageTileRequest::imageTile returnValue;
    QString key = QString::number(zoom) + "-" + QString::number(x) + "-" + QString::number(y);
    QString path = "cache/" + url->getCacheFolder() + "/" + key;
    if(QFileInfo::exists(path))
    {
        Qt3DRender::QMaterial* m = (new Derived<Qt3DExtras::QTextureMaterial>())->get();
        Qt3DRender::QTextureImage* ti = (new Derived<Qt3DRender::QTextureImage>())->get();
        ti->setSource(QUrl("file:" + path));
        Qt3DRender::QTexture2D* t = (new Derived<Qt3DRender::QTexture2D>())->get();
        t->addTextureImage(ti);
        ((Qt3DExtras::QTextureMaterial*)m)->setTexture(t);
        returnValue.m = m;
    }
    else
    {
        if(isReady()) {
            requestObject* ro = (new Derived<ImageTileRequest::requestObject>())->get();
            ro->key = key;
            ro->url = url;
            ro->type = url->getCurrentType();
            QNetworkRequest nr = QNetworkRequest(url->getURL(x, y, zoom));
            nr.setOriginatingObject(ro);
            manager->get(nr);
        }
        else
        {
            if(zoom != currentZoomQueue)
            {
                currentZoomQueue = zoom;
                queue.empty();
            }
            queue.enqueue(key);
        }
        returnValue.m = nullptr;
    }
    return returnValue;
}
