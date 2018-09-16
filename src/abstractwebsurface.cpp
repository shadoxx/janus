#include "abstractwebsurface.h"

AbstractWebSurface::AbstractWebSurface()
{
    SetS("_type", "assetwebsurface");
    SetS("_tagname", "AssetWebSurface");
}

AbstractWebSurface::~AbstractWebSurface()
{
//    qDebug() << "AbstractWebSurface::~AbstractWebSurface()" << this;
}

