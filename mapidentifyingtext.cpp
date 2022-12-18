#include "mapidentifyingtext.h"

MapIdentifyingText::MapIdentifyingText()
{ }


int MapIdentifyingText::addText(const QString& text)
{
    int progress = 1;
    int plus = text.lastIndexOf("+");
    if(plus > -1)
    {
        ++progress;
        QStringList zoomRange = text.last(text.length() - plus - 1).split("-");
        if(zoomRange.count() == 2)
        {
            ++progress;
            bool ok;
            zoomFarest = zoomRange[0].toInt(&ok);
            if(ok)
            {
                ++progress;
                zoomClosest = zoomRange[1].toInt(&ok);
                if(ok)
                {
                    ++progress;
                    int urlEnd = plus;
                    if(text[plus - 1] == ']')
                    {
                        int typeBegin = text.lastIndexOf(":[");
                        if(typeBegin > -1)
                        {
                            mapTypes = text.sliced(typeBegin + 2, plus - typeBegin - 2).split(",");
                            urlEnd = typeBegin;
                        }
                    }
                    QString url = text.first(urlEnd);
                    if(url.count("{T}") <= 1 && url.count("{X}") == 1 && url.count("{Y}") == 1 && url.count("{Z}") == 1)
                    {
                        ++progress;
                        this->url = url;
                        cacheFolder = QString(url.mid(url.indexOf("//") + 2, url.indexOf('?') - url.indexOf("//") - 2)).replace('/', "").replace("{X}", "").replace("{Y}", "").replace("{Z}", "").replace("{T}", "");
                        cacheFolder.replace(QRegularExpression("[^A-Za-z]"), "");
                        if(cacheFolder.length() > 0)
                        {
                            ++progress;
                            QDir dir = QDir("cache/" + cacheFolder);
                            if(dir.mkpath("."))
                            {
                                ++progress;
                                valid = true;
                                for(int i=0;i<mapTypes.count();i++)
                                {
                                    QString subfolder = mapTypes[i].replace(QRegularExpression("[^A-Za-z]"), "");
                                    if(subfolder.length() > 0)
                                    {
                                        ++progress;
                                        QDir dir2 = QDir("cache/" + cacheFolder + "/" + subfolder);
                                        if(dir2.mkpath("."))
                                        {
                                            ++progress;
                                            mapTypes[i] = subfolder;
                                            continue;
                                        }
                                    }
                                    valid = false;
                                    break;
                                }
                                return valid ? 0 : progress;
                            }
                        }
                    }
                }
            }
        }
    }
    return progress;
}


QString MapIdentifyingText::getURL(int x, int y, int zoom)
{
    if(zoomFarest > zoomClosest)
    {
        zoom = zoomFarest - zoom;
    }
    else
    {
        zoom += zoomFarest;
    }
    return QString(url).replace("{X}", QString::number(x)).replace("{Y}", QString::number(y)).replace("{Z}", QString::number(zoom)).replace("{T}", type->currentText());
}


bool MapIdentifyingText::isValid()
{
    return valid;
}


bool MapIdentifyingText::hasTypes()
{
    return mapTypes.length() > 0;
}


QStringList MapIdentifyingText::getTypes()
{
    return mapTypes;
}


void MapIdentifyingText::setSourceType(QComboBox* type)
{
    this->type = type;
}


QString MapIdentifyingText::getCacheFolder()
{
    return cacheFolder + (mapTypes.length() > 0 ? "/" + type->currentText() : "");
}


QString MapIdentifyingText::getHashKey()
{
    return cacheFolder;
}


QString MapIdentifyingText::getCurrentType()
{
    return mapTypes.length() > 0 ? type->currentText() : "d";
}


int MapIdentifyingText::getZoomFarest()
{
    return zoomFarest;
}


int MapIdentifyingText::getZoomClosest()
{
    return zoomClosest;
}
