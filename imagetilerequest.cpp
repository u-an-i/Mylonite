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


void ImageTileRequest::setKey(const QString& key) {
    if(!key.trimmed().isEmpty())
    {
        baseUrlSuffixPrepared = baseUrlSuffix + key;
        while(!queue.isEmpty())
        {
            QString key = queue.dequeue();
            requestObject* ro = (new Derived<ImageTileRequest::requestObject>())->get();
            ro->key = key;
            QStringList parts = key.split('-');
            QNetworkRequest nr = QNetworkRequest(baseUrl + parts[0] + "/" + parts[1] + "/" + parts[2] + baseUrlSuffixPrepared);
            nr.setOriginatingObject(ro);
            manager->get(nr);
        }
    }
    else
    {
        baseUrlSuffixPrepared = "";
    }
}


void ImageTileRequest::setCache(QHash<QString, Qt3DCore::QEntity*>* toCache)
{
    cacheQuad = toCache;
}


bool ImageTileRequest::isReady()
{
    return this->baseUrlSuffixPrepared.length() > 0;
}


void ImageTileRequest::replyFinished(QNetworkReply* reply)
{
    QString key = ((requestObject*)(reply->request().originatingObject()))->key;
    if(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) == 200)
    {
        QByteArray data = reply->readAll();
        QSaveFile sf = QSaveFile("cache/" + key + ".jpg");
        if(sf.open(QIODevice::WriteOnly))
        {
            sf.write(data);
            sf.commit();qDebug() << "a";
            if(cacheQuad != nullptr)
            {
                Qt3DRender::QMaterial* m = (new Derived<Qt3DExtras::QTextureMaterial>())->get();qDebug() << "b";
                Qt3DRender::QTextureImage* ti = (new Derived<Qt3DRender::QTextureImage>())->get();qDebug() << "c";
                ti->setSource(QUrl("file:cache/" + key + ".jpg"));qDebug() << "d";
                Qt3DRender::QTexture2D* t = (new Derived<Qt3DRender::QTexture2D>())->get();qDebug() << "e";
                t->addTextureImage(ti);qDebug() << "f";
                ((Qt3DExtras::QTextureMaterial*)m)->setTexture(t);qDebug() << "g";
                cacheQuad->value(key)->addComponent(m);qDebug() << "h";
            }
        }
    }
    else
    {
        queue.enqueue(key);     // consider later auto dequeuing
    }qDebug() << "j";
    //delete (requestObject*)(reply->request().originatingObject());
    reply->deleteLater();
}

ImageTileRequest::imageTile ImageTileRequest::getImageTile(int zoom, int x, int y)
{
    ImageTileRequest::imageTile returnValue;
    QString key = QString::number(zoom) + "-" + QString::number(x) + "-" + QString::number(y);
    if(QFileInfo::exists("cache/" + key + ".jpg"))
    {
        Qt3DRender::QMaterial* m = (new Derived<Qt3DExtras::QTextureMaterial>())->get();
        Qt3DRender::QTextureImage* ti = (new Derived<Qt3DRender::QTextureImage>())->get();
        ti->setSource(QUrl("file:cache/" + key + ".jpg"));
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
            QNetworkRequest nr = QNetworkRequest(baseUrl + QString::number(zoom) + "/" + QString::number(x) + "/" + QString::number(y) + baseUrlSuffixPrepared);
            nr.setOriginatingObject(ro);
            manager->get(nr);
        }
        else
        {
            queue.enqueue(key);
        }
        returnValue.m = nullptr;
    }
    return returnValue;
}
