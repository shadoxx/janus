#include "texturemanager.h"

std::map<unsigned int, TextureElement> TextureManager::textures;

uint TextureManager::uuid = 0;

TextureManager::TextureManager()
{

}

void TextureManager::CreateTexture(QPointer<AssetImageData> data, QPointer <DOMNode> props)
{
    if (data == nullptr || data->IsUploadSubmitted()) {
		return;
	}

    const bool tex_linear = props->GetTexLinear();
    const bool tex_mipmap = props->GetTexMipmap();
    const bool tex_clamp = props->GetTexClamp();

    TextureHandle::ALPHA_TYPE tex_alpha = TextureHandle::ALPHA_TYPE::UNDEFINED;

    // If we have multiple frames we are a GIF which can only have CUTOUT or NONE alpha,
    // So set to CUTOUT to avoid having to parse every frame of a GIF
    auto numTex = data->GetTotalTextures();
    if (numTex > 1) {
        tex_alpha = TextureHandle::ALPHA_TYPE::CUTOUT;
    }
    else {
        QString const tex_alpha_string = props->GetTexAlpha();

        if (tex_alpha_string.contains("none")) {
            tex_alpha = TextureHandle::ALPHA_TYPE::NONE;
        }
        else if (tex_alpha_string.contains("cutout")) {
            tex_alpha = TextureHandle::ALPHA_TYPE::CUTOUT;
        }
        else if (tex_alpha_string.contains("blended")) {
            tex_alpha = TextureHandle::ALPHA_TYPE::BLENDED;
        }
        else {
            tex_alpha = TextureHandle::ALPHA_TYPE::UNDEFINED;
        }
    }

    TextureHandle::COLOR_SPACE tex_colorspace = TextureHandle::COLOR_SPACE::SRGB;
    QString const tex_colorspace_string = props->GetTexColorspace();

    if (tex_colorspace_string.contains("sRGB")) {
        tex_colorspace = TextureHandle::COLOR_SPACE::SRGB;
    }
    else {
        tex_colorspace = TextureHandle::COLOR_SPACE::LINEAR;
    }

	int current = data->GetUploadedTextures();

    //uint ltex = 0;
    //uint rtex = 0;
    std::shared_ptr<TextureHandle> ltex_handle(nullptr);
    std::shared_ptr<TextureHandle> rtex_handle(nullptr);

	AssetImageDataQ* dataq = dynamic_cast<AssetImageDataQ*>(data.data());
	if (dataq != nullptr)
	{
		// TODO: abstract data comes from QImage, we'll delete this part soon enough
		// (this is already deprecated and unitilized, but is here for in case something breaks)
        QVector<QImage> left = dataq->GetLeftImages();
        QVector<QImage> right = dataq->GetRightImages();

		if (left.size() > current && !left[current].isNull())
		{
            //ltex = CreateTexture(left[current], tex_mipmap, tex_linear, tex_clamp, tex_alpha, tex_colorspace);
            ltex_handle = RendererInterface::m_pimpl->CreateTextureQImage(left[current], tex_mipmap, tex_linear, tex_clamp, tex_alpha, tex_colorspace);
		}

		if (right.size() > current && !right[current].isNull())
		{
            //rtex = CreateTexture(right[current], tex_mipmap, tex_linear, tex_clamp, tex_alpha, tex_colorspace);
            ltex_handle = RendererInterface::m_pimpl->CreateTextureQImage(right[current], tex_mipmap, tex_linear, tex_clamp, tex_alpha, tex_colorspace);
		}
	}
	else
	{

        ltex_handle = RendererInterface::m_pimpl->CreateTextureFromAssetImageData(data, true, tex_mipmap, tex_linear, tex_clamp, tex_alpha, tex_colorspace);

        rtex_handle = RendererInterface::m_pimpl->CreateTextureFromAssetImageData(data, false, tex_mipmap, tex_linear, tex_clamp, tex_alpha, tex_colorspace);
    }

    //auto ltex_handle = TextureManager::GetTextureHandle(ltex);
    //auto rtex_handle = TextureManager::GetTextureHandle(rtex);
    if (ltex_handle)
    {
        data->SetUploadedTexture(ltex_handle, (rtex_handle == nullptr) ? ltex_handle : rtex_handle);
    }
    else
    {
//        qDebug() << "TextureManager::CreateTexture() - ltex_handle is NULL";
    }

    // If we have uploaded all the textures, clear out the CPU copy of the pixel data
    if (data->GetUploadedTextures() == data->GetTotalTextures())
    {
        data->ClearPixelData();
    }
}

void TextureManager::Clear()
{
	textures.clear();
}
