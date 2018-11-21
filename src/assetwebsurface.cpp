#include "assetwebsurface.h"

int AssetWebSurface::m_max_width = 4096;
int AssetWebSurface::m_max_height = 2160;

AssetWebSurface::AssetWebSurface() :
    update_viewport(true),
    tex_alpha(false),
    load_started(false),
    loaded(true),
    progress(0.0f),
    cursor_active(false)
{
//    qDebug() << "AssetWebSurface::AssetWebSurface" << this;
    props->SetWidth(1000);
    props->SetHeight(800);

    cursor_pos = QPoint(500, 400);

//#ifdef __ANDROID__
//    webview = (AbstractWebView*)new AndroidWebView();
//#else
//    webview = (AbstractWebView*)new CEFWebView();
//#endif
//    webview->initialize();
//    webview->initializeMenu();

//    webview->setCookieJar(CookieJar::cookie_jar);
//    if (CookieJar::cookie_jar) {
//        CookieJar::cookie_jar->setParent(NULL); //prevem_max_width, m_max_heightnt NAM from taking ownership of cookie jar
//    }

//    connect(webview, SIGNAL(repaintRequested(QRect)), this, SLOT(UpdateTexture(QRect)));
//    connect(webview, SIGNAL(scrollRequested(int,int,QRect)), this, SLOT(UpdateTexture())); //49.24 bugfix clicking page anchors
//    connect(webview, SIGNAL(loadStarted()), this, SLOT(LoadStarted()));
//    connect(webview, SIGNAL(loadProgress(int)), this, SLOT(LoadProgress(int)));
//    connect(webview, SIGNAL(loadFinished(bool)), this, SLOT(LoadFinished()));
//    connect(webview, SIGNAL(urlChanged(QUrl)), this, SLOT(URLChanged(QUrl)));

}

AssetWebSurface::~AssetWebSurface()
{
//    qDebug() << "AssetWebSurface::~AssetWebSurface()" << this;
    if (webview) {
        webview->terminate();
        delete webview;
        webview = NULL;
    }
}

void AssetWebSurface::SetTextureAlpha(const bool b)
{
    tex_alpha = b;
}

void AssetWebSurface::SetProperties(const QVariantMap & d)
{
    Asset::SetProperties(d);

    if (d.contains("width")) {
        SetWidth(d["width"].toInt());
    }
    if (d.contains("height")) {
        SetHeight(d["height"].toInt());
    }
}

void AssetWebSurface::SetSrc(const QString & base, const QString & u)
{
//    qDebug() << "AssetWebSurface::SetURL()" << base << u;
    Asset::SetSrc(base, u);
    if (original_url.isEmpty()) {
        original_url = props->GetSrcURL();
    }
    URLChanged(QUrl(props->GetSrcURL()));
}

void AssetWebSurface::Load()
{
//    qDebug() << "AssetWebSurface::Load()" << url;
    if (webview) {        
        webview->load(props->GetSrcURL());
    }
}

void AssetWebSurface::Reload()
{
    if (webview) {
        webview->reload();
    }
}

void AssetWebSurface::Unload()
{
    if (webview) {
        delete webview;
        webview = NULL;
    }
}

bool AssetWebSurface::GetLoadStarted()
{
    return load_started;
}

bool AssetWebSurface::GetLoaded() const
{
    return loaded;
}

float AssetWebSurface::GetProgress() const
{
    return progress;
}

bool AssetWebSurface::GetFinished() const
{
    return loaded;
}

void AssetWebSurface::SetURL(const QString & u)
{
//    qDebug() << "AssetWebSurface::SetURL()" << u;
    QUrl url(u);
    if (webview && webview->url() != url) {
//        webview->setHtml(QString(), QUrl()); //32.9: fixes a bug relating to being at HTTPS link
        webview->setUrl(url);
        if (original_url.isEmpty()) {
            original_url = u;
        }
        URLChanged(u);
    }
}

QString AssetWebSurface::GetURL() const
{
//    qDebug() << "AssetWebSurface::GetURL()" << u;
    if (webview) {
        return webview->url().toString();
    }
    else {
        return QString();
    }
}

void AssetWebSurface::SetOriginalURL(const QString & s)
{
    original_url = QUrl(s);
}

QString AssetWebSurface::GetOriginalURL() const
{
    return original_url.toString();
}

QUrl AssetWebSurface::GetLinkClicked(const int cursor_index)
{
    if (webview == NULL) {
        return QUrl("");
    }

#ifdef __ANDROID__
    hit_test_result[cursor_index] = webview->getHitTestContent(QPoint());
#endif

    if (!hit_test_result[cursor_index].isNull()) {
        if (!hit_test_result[cursor_index].linkUrl().isEmpty()) {
            return hit_test_result[cursor_index].linkUrl();
        }
        else if (!hit_test_result[cursor_index].mediaUrl().isEmpty()) {
            return hit_test_result[cursor_index].mediaUrl();
        }
        else if (!hit_test_result[cursor_index].imageUrl().isEmpty()) {
            return hit_test_result[cursor_index].imageUrl();
        }
        else {
            return QUrl("");
        }
    }
    else {
        return QUrl("");
    }
}

void AssetWebSurface::mousePressEvent(QMouseEvent * e, const int )
{
//    qDebug() << "AssetWebSurface::mousePressEvent" << this << e->pos();
    const int height = props->GetHeight();

    cursor_pos = e->pos();

    if (e->button() == Qt::LeftButton) {
        cursor_click_pos = e->pos();
    }

    if (webview) {
        QMouseEvent e2(QEvent::MouseButtonPress, e->pos(), e->button(), e->buttons(), e->modifiers());
        webview->mousePressEvent(&e2);
    }
}

void AssetWebSurface::mouseMoveEvent(QMouseEvent * e, const int cursor_index)
{
//    qDebug() << "AssetWebSurface::mouseMoveEvent" << this << e->pos();
    const int height = props->GetHeight();

    cursor_pos = e->pos();

    if (webview) {        
        QMouseEvent e2(QEvent::MouseMove, e->pos(), e->button(), e->buttons(), e->modifiers());
        webview->mouseMoveEvent(&e2); //TODO/BUG: uncommenting this causes render loop to stall when clicking UI URL bar thing
        hit_test_result[cursor_index] = webview->getHitTestContent(e->pos());
    }
}

void AssetWebSurface::mouseReleaseEvent(QMouseEvent * e, const int cursor_index)
{
//    qDebug() << "AssetWebSurface::mouseReleaseEvent" << this << e->pos();
    const int height = props->GetHeight();

    if (webview) {
        QMouseEvent e2(QEvent::MouseButtonRelease, e->pos(), e->button(), e->buttons(), e->modifiers());
        webview->mouseReleaseEvent(&e2);
        hit_test_result[cursor_index] = WebHitTestResult();
    }
}

void AssetWebSurface::keyPressEvent(QKeyEvent * e)
{
    const int height = props->GetHeight();

    if (webview) {
        webview->keyPressEvent(e);
    }
}

void AssetWebSurface::keyReleaseEvent(QKeyEvent * e)
{
    const int height = props->GetHeight();

    if (webview) {        
        webview->keyReleaseEvent(e);
    }
}

void AssetWebSurface::wheelEvent(QWheelEvent * e)
{
    if (webview) {
        if (e->orientation() == Qt::Vertical){
            QWheelEvent e2(cursor_pos, e->delta(), e->buttons(), e->modifiers());
            webview->wheelEvent(&e2);
            webview->scroll(0,-e->delta());
        }
        if (e->orientation() == Qt::Horizontal){
            QWheelEvent e2(cursor_pos, e->delta(), e->buttons(), e->modifiers(), Qt::Horizontal);
            webview->wheelEvent(&e2);            
            webview->scroll(e->delta(),0);
        }
    }
}

void AssetWebSurface::GoForward()
{
    if (webview) {
        webview->forward();
    }
}

void AssetWebSurface::GoBack()
{
    if (webview) {
        webview->back();
    }
}

void AssetWebSurface::SetCursorPosition(const QPoint & p)
{
    const int width = props->GetWidth();
    const int height = props->GetHeight();

    cursor_pos = p;
    if (cursor_pos.x() < 0.0) {
        cursor_pos.setX(0.0f);
    }
    if (cursor_pos.y() < 0.0) {
        cursor_pos.setY(0.0f);
    }
    if (cursor_pos.x() > width) {
        cursor_pos.setX(width);
    }
    if (cursor_pos.y() > height) {
        cursor_pos.setY(height);
    }
}

QPoint AssetWebSurface::GetCursorPosition() const
{
    return cursor_pos;
}

QPoint AssetWebSurface::GetTextCursorPosition() const
{
    if (webview) {
        return webview->getTextCursorPosition();
    }
    return QPoint(0,0);
}

void AssetWebSurface::SetWidth(const int w)
{
    const int width = props->GetWidth();
    const int height = props->GetHeight();

    if (w != width && w <= m_max_width) {
        cursor_pos = QPoint(w/2, height/2);
        update_viewport = true;

        dirty_rects.clear();
        props->SetWidth(w);
    }
}

void AssetWebSurface::SetHeight(const int h)
{
    const int width = props->GetWidth();
    const int height = props->GetHeight();

    if (h != height && h <= m_max_height) {
        cursor_pos = QPoint(width/2, h/2);
        update_viewport = true;

        dirty_rects.clear();
        props->SetHeight(h);
    }
}

TextureHandle* AssetWebSurface::GetTextureHandle() const
{
    return webview ? webview->GetTextureHandle() : AssetImage::null_image_tex_handle.get();
}

void AssetWebSurface::UpdateTexture()
{
    UpdateTexture(QRect(0,0,props->GetWidth(), props->GetHeight()));
}

void AssetWebSurface::UpdateTextureURLBar()
{
    UpdateTexture(QRect(0,this->GetURLBarPos().y(),props->GetWidth(),props->GetHeight()-this->GetURLBarPos().y()));
}

void AssetWebSurface::UpdateTexture(QRect r)
{
//    qDebug() << "  queueing" << r;
    //dirty_rects.push_back(r);
}

void AssetWebSurface::LoadStarted()
{
    load_started = true;
    loaded = false;
}

void AssetWebSurface::LoadProgress(int p)
{
    progress = float(p) / 100.0f;
}

void AssetWebSurface::LoadFinished()
{
    loaded = true;
    progress = 1.0f;
}

void AssetWebSurface::URLChanged(QUrl url)
{
//    webview->setUrl(url);
}

QPoint AssetWebSurface::GetURLBarPos() const
{
    return QPoint(168, props->GetHeight()-GetURLBarHeight());
}

int AssetWebSurface::GetURLBarWidth() const
{
    return props->GetWidth() - 176;
}

int AssetWebSurface::GetURLBarHeight() const
{
    return 28;
}

void AssetWebSurface::UpdateViewport()
{
    /*const int width = GetI("width");
    const int height = GetI("height");

    if (webview) {
		m_texture_handle = nullptr;

        webview->setViewportSize(QSize(width, height)); //100ms for 1920x1080
    }*/
}

void AssetWebSurface::UpdateGL()
{
    const int width = props->GetWidth();
    const int height = props->GetHeight();

    if (webview && QSize(width, height) != webview->viewportSize()) {
        webview->setViewportSize(QSize(width, height));
    }
//    qDebug() << "AssetWebSurface::UpdateGL() completed" << GetS("_src_url");
}

QString AssetWebSurface::GetFireBoxCodePacket() const
{
    return QString();
}

void AssetWebSurface::ReadFireBoxCode(const QString & , const QString & )
{
}

WebHitTestResult AssetWebSurface::GetHitTestResult(const int cursor_index) const
{
    return hit_test_result[cursor_index];
}

QRect AssetWebSurface::GetHitTestResultRect(const int cursor_index) const
{
    return hit_test_result[cursor_index].boundingRect();
}

QPointer <AbstractWebView> AssetWebSurface::GetWebView()
{
    return webview;
}

bool AssetWebSurface::GetTextEditing()
{
    return webview ? webview->getTextEditing() : false;
}

void AssetWebSurface::SetFocus(const bool b)
{    
    if (webview) {
        webview->SetFocus(b);
    }
}

bool AssetWebSurface::GetFocus() const
{
    return webview ? webview->GetFocus() : false;
}
