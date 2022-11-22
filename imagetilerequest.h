#ifndef IMAGETILEREQUEST_H
#define IMAGETILEREQUEST_H

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
    };

    void setKey(const QString& key);
    void setCache(QHash<QString, Qt3DCore::QEntity*>* toCache);
    ImageTileRequest::imageTile getImageTile(int zoom, int x, int y);


public slots:
    void replyFinished(QNetworkReply* reply);


private:
    QNetworkAccessManager* manager;

    const QString baseUrl = "https://api.tomtom.com/map/1/tile/sat/main/";
    const QString baseUrlSuffix = ".jpg?key=";
    QString baseUrlSuffixPrepared = "";

    bool isReady();
    QQueue<QString> queue;

    QHash<QString, Qt3DCore::QEntity*>* cacheQuad = nullptr;
};

#endif // IMAGETILEREQUEST_H
