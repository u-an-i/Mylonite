#ifndef MAPIDENTIFYINGTEXT_H
#define MAPIDENTIFYINGTEXT_H

#include "include/mm.hpp"

#include <QString>
#include <QComboBox>
#include <QStringList>
#include <QDir>


class MapIdentifyingText : MemObject
{
public:
    MapIdentifyingText();

    int addText(const QString& text);
    QString getURL(int x, int y, int zoom);
    bool isValid();
    bool hasTypes();
    QStringList getTypes();
    void setSourceType(QComboBox* type);
    QString getCacheFolder();
    QString getHashKey();
    QString getCurrentType();
    int getZoomFarest();
    int getZoomClosest();

private:
    int zoomFarest;
    int zoomClosest;
    QStringList mapTypes;
    QComboBox* type;
    QString url;
    QString cacheFolder;
    bool valid = false;
};

#endif // MAPIDENTIFYINGTEXT_H
