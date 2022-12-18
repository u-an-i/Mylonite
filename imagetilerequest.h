#ifndef IMAGETILEREQUEST_H
#define IMAGETILEREQUEST_H

#include "mapidentifyingtext.h"
#include <QObject>
#include <QNetworkReply>
#include <Qt3DRender>


class ImageTileRequest : public QObject
{
    Q_OBJECT

public:
    ImageTileRequest();
    ~ImageTileRequest();

    struct imageTile
    {
        Qt3DRender::QMaterial* m;
    };

    class requestObject : public QObject
    {
    public:
        requestObject()
        {}
        ~requestObject()
        {}

        QString key;
        MapIdentifyingText* url;
        QString type;
    };

    void setURL(MapIdentifyingText* url);
    void setCache(QHash<QString, QHash<QString, QHash<QString, Qt3DCore::QEntity*>*>*>* toCache);
    ImageTileRequest::imageTile getImageTile(int zoom, int x, int y);


public slots:
    void replyFinished(QNetworkReply* reply);


private:
    QNetworkAccessManager* manager;

    MapIdentifyingText* url;

    bool isReady();
    QQueue<QString> queue;
    int currentZoomQueue = -1;

    QHash<QString, QHash<QString, QHash<QString, Qt3DCore::QEntity*>*>*>* cacheQuad = nullptr;
};

#endif // IMAGETILEREQUEST_H
