#ifndef CEFWEBVIEW_H
#define CEFWEBVIEW_H

#include <QtCore>
#include <QImage>
#include <QMutex>

#include "abstractwebview.h"
#include "cookiejar.h"

#include "include/cef_app.h"
#include "include/cef_client.h"
#include "include/cef_dom.h"
#include "include/cef_focus_handler.h"
#include "include/cef_render_handler.h"

class CEFV8Handler : public CefV8Handler {
public:
  CEFV8Handler() {}

  virtual bool Execute(const CefString& name,
                       CefRefPtr<CefV8Value> object,
                       const CefV8ValueList& arguments,
                       CefRefPtr<CefV8Value>& retval,
                       CefString& exception) OVERRIDE;

  // Provide the reference counting implementation for this class.
  IMPLEMENT_REFCOUNTING(CEFV8Handler);
};

class CEFRenderProcessHandler : public CefRenderProcessHandler
{
public:
    CEFRenderProcessHandler();

    virtual void OnContextCreated(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefFrame> frame,
                                  CefRefPtr<CefV8Context> context);

public:
      IMPLEMENT_REFCOUNTING(CEFRenderProcessHandler);

private:

      CefRefPtr<CEFV8Handler> m_v8handler;
};

class CEFApp : public CefApp {
public:
  CEFApp();

  CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler();
  void OnBeforeCommandLineProcessing(const CefString& process_type, CefRefPtr<CefCommandLine> command_line);

public:
  // Provide the reference counting implementation for this class.
  IMPLEMENT_REFCOUNTING(CEFApp);

private:

  CefRefPtr<CefRenderProcessHandler> renderProcessHandler;
};

class CEFLifeSpanHandler : public CefLifeSpanHandler
{
public:
    CEFLifeSpanHandler();

    QString GetPopupURL() const;
    void ClearPopupURL();

    void OnAfterCreated(CefRefPtr<CefBrowser> browser);
    bool OnBeforePopup(CefRefPtr<CefBrowser> browser,
                               CefRefPtr<CefFrame> frame,
                               const CefString& target_url,
                               const CefString& target_frame_name,
                               WindowOpenDisposition target_disposition,
                               bool user_gesture,
                               const CefPopupFeatures& popupFeatures,
                               CefWindowInfo& windowInfo,
                               CefRefPtr<CefClient>& client,
                               CefBrowserSettings& settings,
                               bool* no_javascript_access);

public:
    IMPLEMENT_REFCOUNTING(CEFLifeSpanHandler);

private:
    QString popup_url;
    CefRefPtr <CefClient> m_client;

};

class CEFDOMVisitor : public CefDOMVisitor
{
public:
    CEFDOMVisitor();
    void Visit(CefRefPtr<CefDOMDocument> document);

public:
    IMPLEMENT_REFCOUNTING(CEFDOMVisitor);
};

class CEFFocusHandler : public CefFocusHandler
{
public:
    CEFFocusHandler();

    void OnTakeFocus(CefRefPtr<CefBrowser> browser, bool next);
    void OnGotFocus(CefRefPtr< CefBrowser > browser );
    bool OnSetFocus(CefRefPtr<CefBrowser> browser, FocusSource source);
//    void OnFocusedNodeChanged( CefRefPtr< CefBrowser > browser, CefRefPtr< CefFrame > frame, CefRefPtr< CefDOMNode > node );

public:
    IMPLEMENT_REFCOUNTING(CEFFocusHandler);

};

class CEFRenderHandler : public CefRenderHandler        
{
public:

    CEFRenderHandler();

    TextureHandle * GetTextureHandle();

    void SetViewportSize(const int w, const int h);
    bool GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect);
    bool GetRootScreenRect(CefRefPtr<CefBrowser> browser, CefRect &rect);
    void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList &dirtyRects, const void *buffer, int width, int height);
    void OnCursorChange( CefRefPtr< CefBrowser > browser, CefCursorHandle cursor );

public:
    IMPLEMENT_REFCOUNTING(CEFRenderHandler);

private:

    std::shared_ptr<TextureHandle> m_tex_handle;
};

// for manual render handler
class CEFBrowserClient : public CefClient
{
public:
    CEFBrowserClient(CEFRenderHandler *renderHandler, CEFFocusHandler *focusHandler, CEFLifeSpanHandler *lifespanHandler)
        : m_renderHandler(renderHandler),
          m_focusHandler(focusHandler),
          m_lifespanHandler(lifespanHandler)
    {
//        qDebug() << "CEFBrowserClient::CEFBrowserClient()" << this;
    }

    virtual CefRefPtr<CefRenderHandler> GetRenderHandler() {
        return m_renderHandler;
    }

    virtual CefRefPtr<CefFocusHandler> GetFocusHandler()
    {
        return m_focusHandler;
    }

    virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler()
    {
        return m_lifespanHandler;
    }

    virtual bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                          CefProcessId source_process,
                                          CefRefPtr<CefProcessMessage> message);
    QString GetLaunchURL();
    void ClearLaunchURL();

    CefRefPtr<CEFFocusHandler> m_focusHandler;
    CefRefPtr<CEFRenderHandler> m_renderHandler;
    CefRefPtr<CEFLifeSpanHandler> m_lifespanHandler;

    QString launchurl;

public:
    IMPLEMENT_REFCOUNTING(CEFBrowserClient);
};

class CEFWebView : public AbstractWebView
{
    Q_OBJECT

public:

    CEFWebView();
    virtual ~CEFWebView();

    virtual void initialize();
    virtual void initializeMenu();
    virtual void initializeNonMenu();
    virtual void terminate();
    virtual void update();

    virtual void mousePressEvent(QMouseEvent * e);
    virtual void mouseMoveEvent(QMouseEvent * e);
    virtual void mouseReleaseEvent(QMouseEvent * e);

    virtual void keyPressEvent(QKeyEvent * e);
    virtual void keyReleaseEvent(QKeyEvent * e);

    virtual void wheelEvent(QWheelEvent * e);

    virtual void dragEnterEvent(QDragEnterEvent *);
    virtual void dragMoveEvent(QDragMoveEvent *);
    virtual void dragLeaveEvent(QDragLeaveEvent *);
    virtual void dropEvent(QDropEvent *);

    virtual WebHitTestResult getHitTestContent(QPoint p = QPoint());
    virtual bool getTextEditing();
    virtual QPoint getTextCursorPosition();
    virtual void addToJavaScriptWindowObject(QString n, QObject* o);
    virtual void addToChildFramesJavaScriptWindowObject(QString n, QObject* o);
    virtual void evaluateJavaScript(QString s);

    virtual void setCookieJar(QPointer <CookieJar> j);

    virtual void render(QPainter* p, QRect r);
    virtual QPalette palette();
    virtual void setPalette(QPalette p);
    virtual void setUpdatesEnabled(bool b);

    virtual void setViewportSize(QSize s);
    virtual QSize viewportSize();
    virtual void scroll(int dx, int dy);
    virtual void setScrollPosition(QPoint p);
    virtual QPoint scrollPosition();

    virtual QUrl url();
    virtual void setUrl(QUrl url);
    virtual void setHtml(QString s, QUrl url);
    virtual void load(QUrl url);
    virtual void reload();
    virtual void stop();
    virtual void back();
    virtual void forward();

    TextureHandle * GetTextureHandle();
    void SetFocus(const bool b);
    bool GetFocus() const;

    QString GetLaunchURL();

    static void Shutdown();

private:

    unsigned int GetKeyCode(QKeyEvent * e) const;
    unsigned int GetModifiers(QKeyEvent * e) const;

    CEFFocusHandler * focusHandler;
    CEFRenderHandler * renderHandler;
    CEFLifeSpanHandler * lifespanHandler;
    CefRefPtr<CefBrowser> browser;
    CefRefPtr<CEFBrowserClient> browserClient;

    bool browser_focus;

    static QList <CefRefPtr<CefBrowser> > browser_list;
};
#endif // CEFWEBVIEW_H
