#include "networkrequest.h"

#include <QNetworkAccessManager>


NetworkRequest::NetworkRequest()
{
    manager = new QNetworkAccessManager();
    QObject::connect(manager, &QNetworkAccessManager::finished, this, &NetworkRequest::replyFinished);
}


NetworkRequest::~NetworkRequest()
{
    delete manager;
}


void NetworkRequest::setKey(QString& key) {
    if(!key.trimmed().isEmpty())
    {
        this->baseUrlSuffixPrepared = baseUrlSuffix + key;
    }
    else
    {
        this->baseUrlSuffixPrepared = "";
    }
}


bool NetworkRequest::isReady()
{
    return this->baseUrlSuffixPrepared.length() > 0;
}


void NetworkRequest::replyFinished(QNetworkReply* reply)
{


    reply->deleteLater();
}
