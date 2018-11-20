#ifndef TEXTUREIMPORTERCMFT_H
#define TEXTUREIMPORTERCMFT_H
#ifndef __ANDROID__
#include "asset.h"
#include "assetshader.h"
#include "webasset.h"

#include "contentimporter.h"

#include "renderergl33.h"
#include "assetimagedata.h"

#include "cmft/image.h"
#include "cmft/cubemapfilter.h"
#include "cmft/clcontext.h"
#include "cmft/print.h"

class TextureImporterCMFT : public ContentImporter
{
public:
    TextureImporterCMFT(void);
    ~TextureImporterCMFT();

    bool CanImport(const QByteArray& buffer, QString extension, QPointer <DOMNode> props);
    QPointer<BaseAssetData> Import(const QByteArray& buffer, QPointer <DOMNode> props);
};
#endif
#endif // TEXTUREIMPORTERCMFT_H
