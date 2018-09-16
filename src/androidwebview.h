#ifndef ANDROIDWEBVIEW_H
#define ANDROIDWEBVIEW_H

#include "jniutil.h"

#include <QtGui>
#include <QtWidgets>
#include <QtCore>

#include "abstractwebview.h"
#include "cookiejar.h"

#include <android/input.h>
#include <android/keycodes.h>

class AndroidWebView : public AbstractWebView
{
    Q_OBJECT
public:
    AndroidWebView();
    ~AndroidWebView();

    void move(int x, int y);
    void move(QPoint const& p);

    int androidID() const { return m_androidID; }

    virtual void initialize();
    virtual void initializeMenu();
    virtual void initializeNonMenu();
    virtual void terminate();
    virtual void update();

    virtual void mousePressEvent(QMouseEvent * e);
    virtual void mouseMoveEvent(QMouseEvent * e);
    virtual void mouseReleaseEvent(QMouseEvent * e);

    unsigned int GetCode(QKeyEvent * e) const;
    unsigned int GetModifiers(QKeyEvent * e) const;

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
    virtual void SetFocus(const bool b);
    virtual bool GetFocus() const;

    bool GetRepaintRequested();
    bool GetScrollRequested();
    bool GetURLChanged();

protected:
    static int sm_tag;
    int m_androidID;

    int generateNewTag();

    bool repaint_requested;

public slots:

private:
    std::shared_ptr<TextureHandle> m_tex_handle;

    bool browser_focus;
};

#endif // ANDROIDWEBVIEW_H
