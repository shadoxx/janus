#include "androidwebview.h"

int AndroidWebView::sm_tag = 0;

//TODO: keyboard events, Janus window object (for translators and menu/pocketspace dashboard)

AndroidWebView::AndroidWebView()
    : m_androidID(generateNewTag())
{
    repaint_requested = false;
    m_tex_handle = nullptr;

    JNIUtil::CreateNewWebView(androidID());
    JNIUtil::AttachWebViewToMainLayout(androidID());

#ifdef OCULUS_SUBMISSION_BUILD
    move(0, 0);
#else
    move(5000,5000);
#endif
}

AndroidWebView::~AndroidWebView()
{
    JNIUtil::RemoveWebView(androidID());
}

void AndroidWebView::move(int x, int y)
{
    //QWidget::move(x, y);
    JNIUtil::MoveWebView(androidID(), x, y);
}

void AndroidWebView::move(QPoint const& p)
{
    AndroidWebView::move(p.x(), p.y());
}

int AndroidWebView::generateNewTag()
{
    sm_tag++;
    return sm_tag;
}

void AndroidWebView::initialize()
{

}

void AndroidWebView::initializeMenu()
{

}

void AndroidWebView::initializeNonMenu()
{

}

void AndroidWebView::terminate()
{

}

void AndroidWebView::update()
{
    GetScrollRequested();
    GetURLChanged();
    GetRepaintRequested();
}

void AndroidWebView::mousePressEvent(QMouseEvent * e)
{
    JNIUtil::MousePressWebView(androidID(), e->x(), e->y());
}

void AndroidWebView::mouseMoveEvent(QMouseEvent * e)
{
    JNIUtil::MouseMoveWebView(androidID(), e->x(), e->y());
}

void AndroidWebView::mouseReleaseEvent(QMouseEvent * e)
{
    JNIUtil::MouseReleaseWebView(androidID(), e->x(), e->y());
}

unsigned int AndroidWebView::GetCode(QKeyEvent *e) const
{
    unsigned int key_code;

    unsigned int key = e->key();

    if (key >= Qt::Key_A && key <= Qt::Key_Z) {
        //Qt::Key_A = 0x41 = 65; AKEYCODE_A = 29
        key_code = key - (abs(Qt::Key_A - AKEYCODE_A));
    }
    else if (key >= Qt::Key_0 && key <= Qt::Key_9) {
        //Qt::Key_0 = 0x30 = 48; AKEYCODE_A = 29
        key_code = key - (abs(Qt::Key_0 - AKEYCODE_0));
    }
    else if (key == Qt::Key_Backspace) {
        key_code = AKEYCODE_DEL;
    }
    else if (key == Qt::Key_Alt) {
        key_code = AKEYCODE_ALT_LEFT;
    }
    else if (key == Qt::Key_CapsLock) {
        key_code = AKEYCODE_CAPS_LOCK;
    }
    else if (key == Qt::Key_Semicolon) {
        key_code = AKEYCODE_SEMICOLON;
    }
    else if (key == Qt::Key_Apostrophe) {
        key_code = AKEYCODE_APOSTROPHE;
    }
    else if (key == Qt::Key_Comma) {
        key_code = AKEYCODE_COMMA;
    }
    else if (key == Qt::Key_Control) {
        key_code = AKEYCODE_CTRL_LEFT;
    }
    else if (key == Qt::Key_Minus) {
        key_code = AKEYCODE_MINUS;
    }
    else if (key == Qt::Key_Return) {
        key_code = AKEYCODE_ENTER;
    }
    else if (key == Qt::Key_Equal) {
        key_code = AKEYCODE_EQUALS;
    }
    else if (key == Qt::Key_Period) {
        key_code = AKEYCODE_PERIOD;
    }
    else if (key == Qt::Key_Shift) {
        key_code = AKEYCODE_SHIFT_LEFT;
    }
    else if (key == Qt::Key_Slash) {
        key_code = AKEYCODE_SLASH;
    }
    else if (key == Qt::Key_Space) {
        key_code = AKEYCODE_SPACE;
    }
    else if (key == Qt::Key_QuoteLeft) {
        key_code = AKEYCODE_GRAVE;
    }

    return key_code;
}

unsigned int AndroidWebView::GetModifiers(QKeyEvent *e) const
{
    unsigned int mod = 0;
    if (e->modifiers() & Qt::ShiftModifier) {
        mod |= AMETA_SHIFT_ON;
    }
    if (e->modifiers() & Qt::AltModifier) {
        mod |= AMETA_ALT_ON;
    }
    if (e->modifiers() & Qt::ControlModifier) {
        mod |= AMETA_CTRL_ON;
    }
    return mod;
}

void AndroidWebView::keyPressEvent(QKeyEvent * e)
{
    unsigned int code = GetCode(e);
    unsigned int modifiers = GetModifiers(e);
    //qDebug() << "keycode" << code << (modifiers & AMETA_SHIFT_ON) << (modifiers & AMETA_ALT_ON) << (modifiers & AMETA_CTRL_ON);
    JNIUtil::KeyPressWebView(androidID(), code, modifiers);
}

void AndroidWebView::keyReleaseEvent(QKeyEvent * e)
{
    unsigned int code = GetCode(e);
    unsigned int modifiers = GetModifiers(e);
    //qDebug() << "keycode" << code << (modifiers & AMETA_SHIFT_ON) << (modifiers & AMETA_ALT_ON) << (modifiers & AMETA_CTRL_ON);
    JNIUtil::KeyReleaseWebView(androidID(), code, modifiers);
}

void AndroidWebView::wheelEvent(QWheelEvent * e)
{

}

void AndroidWebView::dragEnterEvent(QDragEnterEvent *)
{

}

void AndroidWebView::dragMoveEvent(QDragMoveEvent *)
{

}

void AndroidWebView::dragLeaveEvent(QDragLeaveEvent *)
{

}

void AndroidWebView::dropEvent(QDropEvent *)
{

}

void AndroidWebView::setCookieJar(QPointer <CookieJar> j)
{
    //AndroidWebView->page()->networkAccessManager()->setCookieJar(j);
}

WebHitTestResult AndroidWebView::getHitTestContent(QPoint p)
{
    return JNIUtil::GetHitTestContentWebView(androidID());
}

bool AndroidWebView::getTextEditing()
{
    browser_focus = getHitTestContent(QPoint()).editable;
    return browser_focus;
}

QPoint AndroidWebView::getTextCursorPosition()
{
    return QPoint();
}

void AndroidWebView::addToJavaScriptWindowObject(QString n, QObject* o)
{

}

void AndroidWebView::addToChildFramesJavaScriptWindowObject(QString n, QObject* o)
{

}

void AndroidWebView::evaluateJavaScript(QString s)
{
    JNIUtil::EvaluateJavaScriptAtWebView(androidID(), s);
}

void AndroidWebView::render(QPainter* p, QRect r)
{
    /*QImage image;
    if (repaint_requested)
    {
        image = JNIUtil::GetBitmap(androidID());
        repaint_requested = false;
    }

    if (!image.isNull())
    {
        //qDebug() << "RENDERINGIMAGE";
        QRect s(0,0,image.width(),image.height());
        //qDebug() << s;
        p->drawImage(r, image, s);

        old_image = image;
    }
    else if (!old_image.isNull()){
        //qDebug() << "RENDERINGOLDIMAGE";
        QRect s(0,0,old_image.width(),old_image.height());
        p->drawImage(r, old_image, s);
    }*/
}
QPalette AndroidWebView::palette()
{
    return QWidget::palette();
}

void AndroidWebView::setPalette(QPalette p)
{
    //QWidget::setPalette(p);
}

void AndroidWebView::setUpdatesEnabled(bool b)
{
    //QWidget::setUpdatesEnabled(b);
    //JNIUtil::SetUpdatesEnabledWebView(androidID(),b);
}

void AndroidWebView::setViewportSize(QSize s)
{
    //QWidget::resize(s.width(), s.height());
    JNIUtil::ResizeWebView(androidID(), s.width(), s.height());

    QImage img(s.width(), s.height(), QImage::Format_RGBA8888);
    img.fill(Qt::darkGray);
    m_tex_handle = RendererInterface::m_pimpl->CreateTextureQImage(img, true, true, true, TextureHandle::ALPHA_TYPE::BLENDED, TextureHandle::COLOR_SPACE::SRGB);
}

QSize AndroidWebView::viewportSize()
{
    return QSize(JNIUtil::GetWidthWebView(androidID()), JNIUtil::GetHeightWebView(androidID()));
}

void AndroidWebView::scroll(int dx, int dy)
{
    //qDebug() << "SCROLLINGWEBVIEW" << dx << dy;
    JNIUtil::ScrollByWebView(androidID(), dx, dy);
}

void AndroidWebView::setScrollPosition(QPoint p)
{
    int x_range = JNIUtil::GetHorizontalScrollRange(androidID());
    int y_range = JNIUtil::GetVerticalScrollRange(androidID());

    p.setX(qMin((qMax(0, p.x())), x_range));
    p.setY(qMin((qMax(0, p.y())), y_range));

    JNIUtil::ScrollToWebView(androidID(), p.x(), p.y());
}

QPoint AndroidWebView::scrollPosition()
{
    //TODO: implement in Java; get x/y
    return QPoint();
}

QUrl AndroidWebView::url()
{
    return JNIUtil::GetURLWebView(androidID());
}

void AndroidWebView::setUrl(QUrl url)
{
    JNIUtil::LoadUrlAtWebView(androidID(), url);
}

void AndroidWebView::setHtml(QString s, QUrl url)
{
    JNIUtil::LoadHtmlAtWebView(androidID(), s);
}

void AndroidWebView::load(QUrl url)
{
    JNIUtil::LoadUrlAtWebView(androidID(), url);
}

void AndroidWebView::reload()
{
    JNIUtil::ReloadWebView(androidID());
}

void AndroidWebView::stop()
{
    JNIUtil::StopWebView(androidID());
}

void AndroidWebView::back()
{
    JNIUtil::GoBackOrForwardAtWebView(androidID(), -1);
}

void AndroidWebView::forward()
{
    JNIUtil::GoBackOrForwardAtWebView(androidID(), 1);
}

bool AndroidWebView::GetRepaintRequested()
{
    //repaint_requested = JNIUtil::GetRepaintRequestedAtWebView(androidID());
    //if (repaint_requested){
        //RepaintRequested(QRect(0,0,JNIUtil::GetWidthWebView(androidID()), JNIUtil::GetHeightWebView(androidID())));

        if (m_tex_handle != nullptr) {
            //JNIUtil::SetUpdatesEnabledWebView(androidID(), false);
            JNIUtil::UpdateTextureWebView(androidID(), m_tex_handle.get());
            //JNIUtil::SetUpdatesEnabledWebView(androidID(), true);
        }
    //}
    return repaint_requested;
}

bool AndroidWebView::GetScrollRequested()
{
    if (JNIUtil::GetScrollRequestedAtWebView(androidID()))
    {
        //ScrollRequested(JNIUtil::GetScrollXWebView(androidID()),JNIUtil::GetScrollYWebView(androidID()),QRect());
        return true;
    }
    return false;
}

bool AndroidWebView::GetURLChanged()
{
    if (JNIUtil::GetURLChangedAtWebView(androidID()))
    {
        URLChanged(QUrl(JNIUtil::GetURLWebView(androidID())));
        return true;
    }
    return false;
}

TextureHandle * AndroidWebView::GetTextureHandle()
{
    return m_tex_handle.get();
}

void AndroidWebView::SetFocus(const bool b)
{

}

bool AndroidWebView::GetFocus() const
{
    return browser_focus;
}
