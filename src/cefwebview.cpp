#include "cefwebview.h"

QList <CefRefPtr<CefBrowser> > CEFWebView::browser_list;

CEFRenderProcessHandler::CEFRenderProcessHandler()
{
//    qDebug() << "CEFRenderProcessHandler::CEFRenderProcessHandler()";
    m_v8handler = new CEFV8Handler();
}

void CEFRenderProcessHandler::OnContextCreated(CefRefPtr<CefBrowser> browser,
                              CefRefPtr<CefFrame> frame,
                              CefRefPtr<CefV8Context> context)
{
    //setup Janus object in RENDER thread
//    qDebug() << "CEFRenderProcessHandler::OnContextCreated";
    CefRefPtr<CefV8Value> j = CefV8Value::CreateObject(NULL, NULL);
    CefRefPtr<CefV8Value> func = CefV8Value::CreateFunction("launchurl", m_v8handler);
    CefRefPtr<CefV8Value> object = context->GetGlobal();

    j->SetValue("launchurl", func, V8_PROPERTY_ATTRIBUTE_NONE);
    object->SetValue("janus", j, V8_PROPERTY_ATTRIBUTE_NONE);
}

CEFApp::CEFApp()
{
//    qDebug() << "CEFApp::CEFApp()";
    renderProcessHandler = new CEFRenderProcessHandler();
}

CefRefPtr<CefRenderProcessHandler> CEFApp::GetRenderProcessHandler()
{
//    qDebug() << "CEFApp::GetRenderProcessHandler()";
    return renderProcessHandler;
}

void CEFApp::OnBeforeCommandLineProcessing(
        const CefString& process_type,
        CefRefPtr<CefCommandLine> command_line)
{
    //Use AppendSwitch if the switch doesn't need a value to be set
    command_line->AppendSwitchWithValue("--limit-fps", "60");
    command_line->AppendSwitchWithValue("--renderer-process-limit", "3");
    command_line->AppendSwitchWithValue("--memory-pressure-thresholds-mb", "1024");
    command_line->AppendSwitchWithValue("--force-gpu-mem-available-mb", "512");
    command_line->AppendSwitchWithValue("--process-per-site", "1");
    CefApp::OnBeforeCommandLineProcessing(process_type, command_line);
}

bool CEFV8Handler::Execute(const CefString& name,
                           CefRefPtr<CefV8Value> object,
                           const CefV8ValueList& arguments,
                           CefRefPtr<CefV8Value>& retval,
                           CefString& exception) {
//    qDebug() << "CEFV8Handler::Execute" << QString(name.ToString().c_str());
    if (name == "launchurl") {
        //two arguments - (string) url and (int) use_portal
        if (arguments.size() >= 1) {
//            qDebug() << "CEFV8Handler::Execute launchurl" << QString(arguments[0].get()->GetStringValue().ToString().c_str());
            CefRefPtr<CefBrowser> browser = CefV8Context::GetCurrentContext()->GetBrowser();
            CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create(name);
            message->GetArgumentList()->SetString(0, arguments[0].get()->GetStringValue());
            browser->SendProcessMessage(PID_BROWSER, message);
        }
        return true;
    }

    // Function does not exist.
    return false;
}

CEFCookieVisitor::CEFCookieVisitor()
{

}

bool CEFCookieVisitor::Visit(const CefCookie& cookie, int count, int total, bool& deleteCookie)
{
    QNetworkCookie c;
    c.setSecure((cookie.secure == 0)?false:true);
    c.setHttpOnly((cookie.httponly == 0)?false:true);

    if (cookie.has_expires)
    {
        cef_time_t cef_expiry = cookie.expires;
        QDate expiry_date(cef_expiry.year, cef_expiry.month, cef_expiry.day_of_month);
        QTime expiry_time(cef_expiry.hour, cef_expiry.minute, cef_expiry.second, cef_expiry.millisecond);
        QDateTime expiry(expiry_date, expiry_time);
        c.setExpirationDate(expiry);
    }

    c.setDomain(StringFromCefString(cookie.domain));
    c.setPath(StringFromCefString(cookie.path));
    c.setName(StringFromCefString(cookie.name).toLatin1());
    c.setValue(StringFromCefString(cookie.value).toLatin1());

    QString domainStr = c.domain();
    if (!domainStr.isEmpty() && domainStr.at(0) == '.') {
        domainStr.remove(0,1);
    }
    domainStr = QString("http://") + domainStr;
    CookieJar::cookie_jar->setCookiesFromUrl(QList<QNetworkCookie>({c}), domainStr);
    CookieJar::cookie_jar->SaveToDisk();

    deleteCookie = false;
    return true;
}

CEFDOMVisitor::CEFDOMVisitor()
{

}

void CEFDOMVisitor::Visit(CefRefPtr<CefDOMDocument> document)
{
//    qDebug() << "CEFDOMVisitor::Visit" << this
//             << document->GetFocusedNode()->GetElementTagName().c_str()
//             << document->GetFocusedNode()->GetName().c_str()
//             << document->GetFocusedNode()->GetFormControlElementType().c_str();
}

CEFLifeSpanHandler::CEFLifeSpanHandler()
{

}

QString CEFLifeSpanHandler::GetPopupURL() const
{
    return popup_url;
}

void CEFLifeSpanHandler::ClearPopupURL()
{
    popup_url.clear();
}

void CEFLifeSpanHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
//    qDebug() << "CEFLifeSpanHandler::OnAfterCreated" << browser;
}

bool CEFLifeSpanHandler::OnBeforePopup(CefRefPtr<CefBrowser> browser,
                           CefRefPtr<CefFrame> frame,
                           const CefString& target_url,
                           const CefString& target_frame_name,
                           WindowOpenDisposition target_disposition,
                           bool user_gesture,
                           const CefPopupFeatures& popupFeatures,
                           CefWindowInfo& windowInfo,
                           CefRefPtr<CefClient>& client,
                           CefBrowserSettings& settings,
                           bool* no_javascript_access)
{
    //browser->GetMainFrame()->LoadURL(target_url); //doesn't seem to work?
//    qDebug() << "CEFLifeSpanHandler::OnBeforePopup" << browser << QString(target_url.ToString().c_str());
    // in linux set a gtk widget, in windows a hwnd. If not available set nullptr - may cause some render errors, in context-menu and plugins.
//    settings.windowless_frame_rate = 60; // 30 is default
//#ifdef WIN32
//    windowInfo.SetAsWindowless(NULL); // false means no transparency (site background colour)
//#else
//    std::size_t windowHandle = 0;
//    windowInfo.SetAsWindowless(windowHandle); // false means no transparency (site background colour)
//#endif
    popup_url = QString(target_url.ToString().c_str());
    return true;
}

CEFFocusHandler::CEFFocusHandler()
{

}

void CEFFocusHandler::OnTakeFocus(CefRefPtr<CefBrowser> browser, bool next)
{
//    qDebug() << "CEFFocusHandler::OnTakeFocus" << this << next;
}

void CEFFocusHandler::OnGotFocus(CefRefPtr< CefBrowser > browser )
{
//    qDebug() << "CEFFocusHandler::OnGotFocus" << this;
}

bool CEFFocusHandler::OnSetFocus(CefRefPtr<CefBrowser> browser, FocusSource source)
{
//    qDebug() << "CEFFocusHandler::OnSetFocus" << this << (int)source;
    return false;
}

CEFRenderHandler::CEFRenderHandler()
{
}

TextureHandle * CEFRenderHandler::GetTextureHandle()
{
    return m_tex_handle.get();
}

void CEFRenderHandler::SetViewportSize(const int w, const int h)
{
//    qDebug() << "CEFRenderHandler::SetViewportSize" << w << h;
    QImage img(w, h, QImage::Format_ARGB32_Premultiplied);
    img.fill(0x00000000);
    m_tex_handle = RendererInterface::m_pimpl->CreateTextureQImage(img, true, true, true, TextureHandle::ALPHA_TYPE::BLENDED, TextureHandle::COLOR_SPACE::SRGB);
}

bool CEFRenderHandler::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect)
{
//    qDebug() << "CEFRenderHandler::GetViewRect()" << this << browser << RendererInterface::m_pimpl->GetTextureWidth(m_tex_handle.get()) << RendererInterface::m_pimpl->GetTextureHeight(m_tex_handle.get());
    rect = CefRect(0, 0,
                   RendererInterface::m_pimpl->GetTextureWidth(m_tex_handle.get()),
                   RendererInterface::m_pimpl->GetTextureHeight(m_tex_handle.get()));
    return true;
}

bool CEFRenderHandler::GetRootScreenRect(CefRefPtr<CefBrowser> browser, CefRect &rect)
{
//    qDebug() << "CEFRenderHandler::GetScreenRect()" << this << browser << RendererInterface::m_pimpl->GetTextureWidth(m_tex_handle.get()) << RendererInterface::m_pimpl->GetTextureHeight(m_tex_handle.get());
    rect = CefRect(0, 0,
                   RendererInterface::m_pimpl->GetTextureWidth(m_tex_handle.get()),
                   RendererInterface::m_pimpl->GetTextureHeight(m_tex_handle.get()));
    return true;
}

void CEFRenderHandler::OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList &dirtyRects, const void *buffer, int width, int height)
{
//    qDebug() << "CEFRenderHandler::OnPaint()" << browser << QTime::currentTime().msecsSinceStartOfDay() << width << height << dirtyRects.size() << QString(browser->GetMainFrame()->GetURL().ToString().c_str());
    RendererInterface::m_pimpl->UpdateTextureHandleData(m_tex_handle.get(), 0, 0, 0, width, height, (int)GL_BGRA, (int)GL_UNSIGNED_BYTE, (void *)buffer, width * height * 4);
    RendererInterface::m_pimpl->GenerateTextureHandleMipMap(m_tex_handle.get());
//    QImage img(width, height, QImage::Format_ARGB32_Premultiplied);
//    memcpy(img.bits(), buffer, width * height * 4);
//    img.save("test.png");
}

void CEFRenderHandler::OnCursorChange( CefRefPtr< CefBrowser > browser, CefCursorHandle cursor ) {
//    qDebug() << "CEFRenderHandler::OnCursorChange";
}

CEFJSDialogHandler::CEFJSDialogHandler()
{

}

bool CEFJSDialogHandler::OnJSDialog(CefRefPtr<CefBrowser> browser,
                const CefString& origin_url,
                JSDialogType dialog_type,
                const CefString& message_text,
                const CefString& default_prompt_text,
                CefRefPtr<CefJSDialogCallback> callback,
                bool& suppress_message)
{
   auto msg = message_text.ToString();
   QString s = QString(msg.c_str());
   if (s.left(16) == "janus_hittest://"){
       mDOMHitTestResult = s.remove(0,16);
       //qDebug() << "hittest" << mDOMHitTestResult;
   }
   else {
       mDOMHitTestResult = "null";
   }
   return true;
}

bool CEFJSDialogHandler::OnBeforeUnloadDialog(CefRefPtr<CefBrowser> browser,
                                  const CefString& message_text,
                                  bool is_reload,
                                  CefRefPtr<CefJSDialogCallback> callback)
{
  return false;
}

void CEFJSDialogHandler::OnResetDialogState(CefRefPtr<CefBrowser> browser)
{

}

void CEFJSDialogHandler::OnDialogClosed(CefRefPtr<CefBrowser> browser) {

}

QString CEFJSDialogHandler::getHitTest()
{
    return mDOMHitTestResult;
}

bool CEFBrowserClient::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                      CefProcessId source_process,
                                      CefRefPtr<CefProcessMessage> message)
{
    const QString name = message->GetName().ToString().c_str();
//    qDebug() << "CEFBrowserClient::OnProcessMessageReceived" << name;
    if (name == "launchurl" && message->GetArgumentList()->GetSize() >= 1) {
        launchurl = QString(message->GetArgumentList()->GetString(0).ToString().c_str());
//        qDebug() << "CEFBrowserClient::OnProcessMessageReceived" << launchurl;
        return true;
    }
    return false;
}

QString CEFBrowserClient::GetLaunchURL()
{
    return launchurl;
}

void CEFBrowserClient::ClearLaunchURL()
{
    launchurl.clear();
}

CEFWebView::CEFWebView() :
    browser_focus(false)
{
//    qDebug() << "CEFWebView::CEFWebView()" << this;
    focusHandler = new CEFFocusHandler();
    renderHandler = new CEFRenderHandler();
    lifespanHandler = new CEFLifeSpanHandler();
    jsDialogHandler = new CEFJSDialogHandler();

    // create browser-window
    CefWindowInfo window_info;
    CefBrowserSettings browserSettings;
    browserSettings.windowless_frame_rate = 60; // 30 is default

    // in linux set a gtk widget, in windows a hwnd. If not available set nullptr - may cause some render errors, in context-menu and plugins.
#ifdef WIN32
    window_info.SetAsWindowless(NULL); // false means no transparency (site background colour)
#else
    std::size_t windowHandle = 0;
    window_info.SetAsWindowless(windowHandle); // false means no transparency (site background colour)
#endif

    browserClient = new CEFBrowserClient(renderHandler, focusHandler, lifespanHandler, jsDialogHandler);
    browser = CefBrowserHost::CreateBrowserSync(window_info, browserClient.get(), "about:blank", browserSettings, nullptr);

    browser_list.push_back(browser);

    // Create the new V8 object.
//    CefRefPtr<CefV8Value> testObjPtr = CefV8Value::CreateObject(NULL);
//    // Add the new V8 object to the global window object with the name
//    // "cef_test".
//    object->SetValue("cef_test", testObjPtr, V8_PROPERTY_ATTRIBUTE_NONE);

//    // Create an instance of ClientV8FunctionHandler as the V8 handler.
//    CefRefPtr<CefV8Handler> handlerPtr = new ClientV8FunctionHandler();

//    // Add a new V8 function to the cef_test object with the name "Dump".
//    testObjPtr->SetValue("Dump",
//                         CefV8Value::CreateFunction("Dump", handlerPtr),
//                         V8_PROPERTY_ATTRIBUTE_NONE);
}

CEFWebView::~CEFWebView()
{
//    qDebug() << "CEFWebView::~CEFWebView()" << this;
//    delete browserClient;
//    delete renderHandler;
//    browser->Release();
//    CefBrowserHost::CloseBrowser()
//    delete browser;
    if (browser && browser->GetHost()) {
        qDebug() << "CEFWebView::~CEFWebView() closebrowser" << this;
        browser->GetHost()->CloseBrowser(true);
    }
}

void CEFWebView::initialize()
{

}

void CEFWebView::initializeMenu()
{

}

void CEFWebView::initializeNonMenu()
{

}

void CEFWebView::terminate()
{

}

void CEFWebView::update()
{
//    qDebug() << "CEFWebView::update()";
    if (!lifespanHandler->GetPopupURL().isEmpty()) {
        browser->GetMainFrame()->LoadURL(lifespanHandler->GetPopupURL().toStdString());
        lifespanHandler->ClearPopupURL();
    }

    CefRefPtr<CefCookieManager> cm = CefCookieManager::GetGlobalManager(NULL);
    cm->VisitUrlCookies(browser->GetMainFrame()->GetURL(), true, new CEFCookieVisitor());

}

void CEFWebView::mousePressEvent(QMouseEvent * e)
{
    CefMouseEvent mouse_event;
    mouse_event.x = e->x();
    mouse_event.y = e->y();

    const CefBrowserHost::MouseButtonType btnType = (((e->buttons() & Qt::LeftButton) > 0) ? MBT_LEFT : MBT_RIGHT);
    browser->GetHost()->SendMouseClickEvent(mouse_event, btnType, false, 1);
}

void CEFWebView::mouseMoveEvent(QMouseEvent * e)
{
    CefMouseEvent mouse_event;
    mouse_event.x = e->x();
    mouse_event.y = e->y();
    browser->GetHost()->SendMouseMoveEvent(mouse_event, false);

    if (!hit_test_timer.isValid() || hit_test_timer.elapsed() > 100) // Hit test every 100 ms
    {
        CefString script =
                "var n = document.elementFromPoint(" + std::to_string(e->x()) + "," + std::to_string(e->y()) + ");"
                "if (n != null) {"
                "if (n.tagName.toLowerCase() == 'img' && n.parentNode.tagName.toLowerCase() == 'a') {alert('janus_hittest://' + n.parentNode.href);}"
                "else if (n.tagName.toLowerCase() == 'a') {alert('janus_hittest://' + n.href);}"
                "else {alert('janus_hittest://' + n.src);}"
                "}";
        //qDebug() << script.ToString().c_str();
        browser->GetFocusedFrame()->ExecuteJavaScript(script,browser->GetMainFrame()->GetURL(), 0);
        hit_test_timer.restart();
    }
}

void CEFWebView::mouseReleaseEvent(QMouseEvent * e)
{
    CefMouseEvent mouse_event;
    mouse_event.x = e->x();
    mouse_event.y = e->y();

    const CefBrowserHost::MouseButtonType btnType = (((e->buttons() & Qt::LeftButton) > 0) ? MBT_LEFT : MBT_RIGHT);
    browser->GetHost()->SendMouseClickEvent(mouse_event, btnType, true, 1);
}

void CEFWebView::wheelEvent(QWheelEvent * e)
{
    CefMouseEvent mouse_event;
    mouse_event.x = e->x();
    mouse_event.y = e->y();

    browser->GetHost()->SendMouseWheelEvent(mouse_event, 0, e->delta());
}

unsigned int CEFWebView::GetKeyCode(QKeyEvent * e) const
{
    unsigned int key_code = ((unsigned int)(e->nativeVirtualKey()) & 0x000000ff);
    switch (e->key()) {
    case Qt::Key_Insert:
        key_code = 45;
        break;
    case Qt::Key_Delete:
        key_code = 46;
        break;
    case Qt::Key_Home:
        key_code = 36;
        break;
    case Qt::Key_End:
        key_code = 35;
        break;
    case Qt::Key_PageUp:
        key_code = 33;
        break;
    case Qt::Key_PageDown:
        key_code = 34;
        break;
    case Qt::Key_Left:
        key_code = 37;
        break;
    case Qt::Key_Up:
        key_code = 38;
        break;
    case Qt::Key_Right:
        key_code = 39;
        break;
    case Qt::Key_Down:
        key_code = 40;
        break;
    case Qt::Key_Return:
        key_code = 13;
        break;
     //TODO - fix these
//    case Qt::Key_Period:
//        qDebug() << "PERIOD";
//        //key_code = 190;
//        break;
//    case Qt::Key_Apostrophe:
//        key_code = 222;
//        break;
    default:
        break;
    }
    return key_code;
}

unsigned int CEFWebView::GetModifiers(QKeyEvent * e) const
{
    unsigned int mod = 0;
    if (e->modifiers() & Qt::ShiftModifier) {
        mod |= EVENTFLAG_SHIFT_DOWN;
    }
    if (e->modifiers() & Qt::AltModifier) {
        mod |= EVENTFLAG_ALT_DOWN;
    }
    if (e->modifiers() & Qt::ControlModifier) {
        mod |= EVENTFLAG_CONTROL_DOWN;
    }
    return mod;
}

void CEFWebView::keyPressEvent(QKeyEvent * e)
{
//    qDebug() << "CEFWebView::keyPressEvent" << e->key() << e->nativeVirtualKey();
    unsigned int key_code = GetKeyCode(e);
    const unsigned int mod = GetModifiers(e);

    CefKeyEvent keyevent;
    keyevent.type = KEYEVENT_KEYDOWN;
    keyevent.windows_key_code = key_code;
    keyevent.modifiers = mod;
    browser->GetHost()->SendKeyEvent(keyevent);

//    qDebug() << "CEFWebView::keyPressEvent" << e->key();
    if (e->text().length() > 0) {
        const bool is_lowercase = (e->text().toLower() == e->text() && e->key() >= Qt::Key_A && e->key() <= Qt::Key_Z);
        CefKeyEvent keyevent2;
        keyevent2.type = KEYEVENT_CHAR;
        keyevent2.character = key_code; //60.0 - needed for Linux
        keyevent2.windows_key_code = (e->nativeVirtualKey() == 13 ? 13 : e->key() + (is_lowercase ? 32 : 0)); //60.0 - needed for Windows
        keyevent2.modifiers = mod;
        browser->GetHost()->SendKeyEvent(keyevent2);
    }
}

void CEFWebView::keyReleaseEvent(QKeyEvent * e)
{
    const unsigned int key_code = GetKeyCode(e);
    const unsigned int mod = GetModifiers(e);

    CefKeyEvent keyevent;
    keyevent.type = KEYEVENT_KEYUP;
    keyevent.windows_key_code = key_code;
    keyevent.modifiers = mod;

//    qDebug() << "CEFWebView::keyReleaseEvent" << key_code;
    browser->GetHost()->SendKeyEvent(keyevent);
}

void CEFWebView::dragEnterEvent(QDragEnterEvent *)
{

}

void CEFWebView::dragMoveEvent(QDragMoveEvent *)
{

}

void CEFWebView::dragLeaveEvent(QDragLeaveEvent *)
{

}

void CEFWebView::dropEvent(QDropEvent *)
{

}

void CEFWebView::setCookieJar(QPointer <CookieJar> j)
{
    cookie_jar = j;
}

WebHitTestResult CEFWebView::getHitTestContent(QPoint p)
{
    WebHitTestResult h;

    h.link_url = QUrl(jsDialogHandler->getHitTest());
    h.bounding_rect = QRect();
    h.is_null = (jsDialogHandler->getHitTest() == "null" || jsDialogHandler->getHitTest() == "undefined");

    return h;
}

bool CEFWebView::getTextEditing()
{
//    qDebug() << "CEFWebView::getTextEditing()" << browser_focus;
    return browser_focus;
}

QPoint CEFWebView::getTextCursorPosition()
{
    return QPoint(0,0);
}

void CEFWebView::addToJavaScriptWindowObject(QString n, QObject* o)
{
}

void CEFWebView::addToChildFramesJavaScriptWindowObject(QString n, QObject* o)
{
}

void CEFWebView::evaluateJavaScript(QString s)
{
//    qDebug() << "CEFWebView::evaluateJavaScript()";
    browser->GetMainFrame()->ExecuteJavaScript(s.toStdString(), browser->GetMainFrame()->GetURL(), 0);
}

void CEFWebView::render(QPainter* p, QRect r)
{
//    qDebug() << "CEFWebView::render" << r;
}

QPalette CEFWebView::palette()
{
    return QPalette();
}

void CEFWebView::setPalette(QPalette p)
{

}

void CEFWebView::setUpdatesEnabled(bool b)
{

}

void CEFWebView::setViewportSize(QSize s)
{
    renderHandler->SetViewportSize(s.width(), s.height());
    browser->GetHost()->WasResized();
}

QSize CEFWebView::viewportSize()
{
    return QSize(RendererInterface::m_pimpl->GetTextureWidth(renderHandler->GetTextureHandle()),
          RendererInterface::m_pimpl->GetTextureHeight(renderHandler->GetTextureHandle()));
}

void CEFWebView::scroll(int dx, int dy)
{

}

void CEFWebView::setScrollPosition(QPoint p)
{

}

QPoint CEFWebView::scrollPosition()
{
    return QPoint(0,0);
}

QUrl CEFWebView::url()
{
    return QUrl(browser->GetMainFrame()->GetURL().ToString().c_str());
}

void CEFWebView::setUrl(QUrl url)
{
//    qDebug() << "CEFWebView::setUrl" << url;
    browser->GetMainFrame()->LoadURL(url.toString().toStdString());
}

void CEFWebView::setHtml(QString s, QUrl url)
{

}

void CEFWebView::load(QUrl url)
{
//    qDebug() << "CEFWebView::load(QUrl url)" << url;
    browser->GetMainFrame()->LoadURL(url.toString().toStdString());
}

void CEFWebView::reload()
{
    browser->Reload();
}

void CEFWebView::stop()
{
    browser->StopLoad();
}

void CEFWebView::back()
{
    if (browser->CanGoBack()) {
        browser->GoBack();
    }
}

void CEFWebView::forward()
{
    if (browser->CanGoForward()) {
        browser->GoForward();
    }
}

TextureHandle * CEFWebView::GetTextureHandle()
{
    return renderHandler->GetTextureHandle();
}

void CEFWebView::SetFocus(const bool b)
{
//    qDebug() << "CEFWebView::SetFocus" << b;
    if (browser_focus != b) {
        browser_focus = b;
        browser->GetHost()->SetFocus(browser_focus);
    }
}

bool CEFWebView::GetFocus() const
{
    return browser_focus;
}

QString CEFWebView::GetLaunchURL()
{
    QString l = browserClient->GetLaunchURL();
    browserClient->ClearLaunchURL();
    return l;
}

void CEFWebView::Shutdown()
{
    qDebug() << "CEFWebView::Shutdown()";
    for (int i=0; i<browser_list.size(); ++i) {
        if (browser_list[i] && browser_list[i]->GetHost()) {
            qDebug() << " Closing browser" << i << browser_list[i] << browser_list[i]->GetMainFrame()->GetURL().ToString().c_str();
            browser_list[i]->GetHost()->CloseBrowser(true);
//            delete browser_list[i];
        }
    }
//    browser_list.clear();
}
