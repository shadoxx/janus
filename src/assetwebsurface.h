#ifndef ASSETWEBSURFACE_H
#define ASSETWEBSURFACE_H

#ifdef __ANDROID__
//#define GLdouble float
#include <GLES3/gl31.h>
#include <GLES2/gl2ext.h>
#endif

#include <QtGui>
#include <QtConcurrent/QtConcurrent>
#include <QGLWidget>
#include <QSharedMemory>
#include <QProcess>

#include "asset.h"
#include "mathutil.h"
#include "cookiejar.h"
#ifdef __ANDROID__
    #include "androidwebview.h"
#else
    #include "cefwebview.h"
#endif
#include "renderer.h"
#include "rendererinterface.h"
#include "abstractwebsurface.h"
#include "texturemanager.h"
#include "performancelogger.h"
#include "assetimage.h"

class AssetWebSurface : public AbstractWebSurface
{
    Q_OBJECT

public:

    AssetWebSurface();
    virtual ~AssetWebSurface();

    virtual void SetProperties(const QVariantMap & d);

    virtual void SetTextureAlpha(const bool b);

    virtual void SetSrc(const QString & base, const QString & url_str);

    virtual void SetURL(const QString & u);
    virtual QString GetURL() const;

    virtual void SetOriginalURL(const QString & u);
    virtual QString GetOriginalURL() const;

    virtual void Load();
    virtual void Reload();
    virtual void Unload();

    virtual bool GetLoadStarted();
    virtual bool GetLoaded() const;
    virtual float GetProgress() const;
    bool GetFinished() const;

    virtual void mousePressEvent(QMouseEvent * e, const int cursor_index);
    virtual void mouseMoveEvent(QMouseEvent * e, const int cursor_index);
    virtual void mouseReleaseEvent(QMouseEvent * e, const int cursor_index);
    virtual void wheelEvent(QWheelEvent * e);

    virtual void keyPressEvent(QKeyEvent * e);
    virtual void keyReleaseEvent(QKeyEvent * e);

    virtual void GoForward();
    virtual void GoBack();

    virtual void SetCursorPosition(const QPoint & p);
    virtual QPoint GetCursorPosition() const;

    virtual QPoint GetTextCursorPosition() const;

    virtual void SetWidth(const int w);
    virtual void SetHeight(const int h);

    virtual TextureHandle* GetTextureHandle() const;
    virtual void UpdateGL();

    virtual QUrl GetLinkClicked(const int cursor_index);
    virtual WebHitTestResult GetHitTestResult(const int cursor_index) const;

    virtual QString GetFireBoxCodePacket() const;
    virtual void ReadFireBoxCode(const QString & userid, const QString & s);

    virtual QPointer <AbstractWebView> GetWebView();

    virtual QRect GetHitTestResultRect(const int cursor_index) const;
    virtual bool GetTextEditing();

    virtual void SetFocus(const bool b);
    virtual bool GetFocus() const;

public slots:

    void UpdateTexture();
    void UpdateTextureURLBar();
    void UpdateTexture(QRect r);

    void LoadStarted();
    void LoadProgress(int p);
    void LoadFinished();
    void URLChanged(QUrl url);

private:

    QPoint GetURLBarPos() const;
    int GetURLBarWidth() const;
    int GetURLBarHeight() const;
    void UpdateViewport();

    QUrl original_url;

    bool update_viewport;

    QPoint cursor_pos;
    QPoint cursor_click_pos;

    QPointer <AbstractWebView> webview; //36.1 note: using qpointer stops the destructor from deleting this, which causes a crash

    bool tex_alpha;
	std::shared_ptr<TextureHandle> m_texture_handle;

    QList <QRect > dirty_rects;
    QRect texture_to_send;

    WebHitTestResult hit_test_result[2];

    bool load_started;
    bool loaded;
    float progress;    

    static int m_max_width;
    static int m_max_height;

    bool cursor_active;
};

#endif // ASSETWEBSURFACE_H
