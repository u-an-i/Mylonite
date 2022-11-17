#ifndef NETWORKREQUEST_H
#define NETWORKREQUEST_H

#include <QObject>
#include <QNetworkReply>


class NetworkRequest : public QObject
{
    Q_OBJECT

public:
    NetworkRequest();
    ~NetworkRequest();

    void setKey(QString& key);
    void request(int zoom, int x, int y);


public slots:
    void replyFinished(QNetworkReply* reply);


private:
    QNetworkAccessManager* manager;

    const QString baseUrl = "https://api.tomtom.com/map/1/tile/sat/main/";
    const QString baseUrlSuffix = ".jpg?key=";
    QString baseUrlSuffixPrepared = "";

    bool isReady();
};

#endif // NETWORKREQUEST_H
