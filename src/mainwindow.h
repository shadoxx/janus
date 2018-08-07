#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#ifdef __ANDROID__
#include <GLES3/gl31.h>
#include <GLES2/gl2ext.h>
#include "jniutil.h"
#else
#include <qopengl.h>
#include <qopenglext.h>
#endif

#include <QKeyEvent>
#include <memory>

#include "renderer.h"

#include "soundmanager.h"
#include "abstracthmdmanager.h"
#include "performancelogger.h"

#include "controllermanager.h"

#include "socialwindow.h"
#include "settingswindow.h"
#ifndef __ANDROID__
#include "hierarchywindow.h"
#include "propertieswindow.h"
#include "assetwindow.h"
#include "codeeditorwindow.h"
#include "navigationwindow.h"
#endif
#include "glwidget.h"

#ifdef __ANDROID__
#include <unistd.h>
#include "slidingtabwidget.h"
#endif

#ifdef WIN32
#include "riftmanager.h"

#ifndef OCULUS_SUBMISSION_BUILD
#include "vivemanager.h"
#endif

#elif defined __linux__
    #ifndef __ANDROID__
        #include "vivemanager.h"
    #else
        #ifndef OCULUS_SUBMISSION_BUILD
            #include "gvrmanager.h"
        #endif
        #if  !defined(__i386__) && !defined(DAYDREAM_SUBMISSION_BUILD) //__arm__
            #include "gearmanager.h"
        #endif
    #endif
#endif

class CloseEventFilter : public QObject
{
     Q_OBJECT
public:
     CloseEventFilter(QObject *parent) : QObject(parent) {}

signals:
    void Closed();

protected:
     bool eventFilter(QObject *obj, QEvent *event)
     {
          if (event->type() == QEvent::Close)
          {
              // Do something interesting, emit a signal for instance.
              emit Closed();
          }

          return QObject::eventFilter(obj, event);
     }

};

//class MainWindow : public QWindow
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    MainWindow();
    ~MainWindow();

    void dragEnterEvent(QDragEnterEvent* event);
    void dragMoveEvent(QDragMoveEvent* event);
    void dragLeaveEvent(QDragLeaveEvent* event);
    void dropEvent(QDropEvent* event);

    void keyPressEvent(QKeyEvent*);
    void keyReleaseEvent(QKeyEvent*);

    bool focusNextPrevChild(bool next);

    void Initialize();

    static int use_screen;
    static bool window_mode;
    static int window_width;
    static int window_height;
    static bool display_help;        
    static bool display_version;
    static bool output_cubemap;
    static bool output_equi;
    static QString output_cubemap_filename_prefix;

#ifdef __ANDROID__
    void EnterVR();
    void ExitVR();

    void Pause();
    void Resume();
#endif

public slots:

    void Closed();
    void TimeOut();
    void CEFTimeOut();    

    void ActionNew();
    void ActionOpen();
    void ActionOpenURL();
    void ActionOpenURL(QAction * a);
    void ActionSave();
    void ActionSaveAs();
    void ActionImportLocal();
    void ActionImportRemote();
    void ActionSaveThumb();
    void ActionSaveScreenshot();
    void ActionSaveEqui();
    void ActionExit();

    void ActionBack();
    void ActionForward();
    void ActionReload();
    void ActionHome();
    void ActionBookmark();

    void ActionOpenEdit();
    void ActionStartRecording();
    void ActionStartRecordingEveryone();
    void ActionStopRecording();
    void ActionSyncTo();

    void ActionOpenWindow();
    void ActionToggleFullscreen();
    void ActionSocial();
#ifndef __ANDROID__
    void ActionCodeEditor();
    void ActionNavigation();
    void ActionAssets();
    void ActionHierarchy();
    void ActionProperties();
#endif

    void ActionOpenBookmarks();
    void ActionAddBookmark();
    void ActionRemoveBookmark();

    void ActionSettings();
    void ActionEllipsisMenu();

#ifdef __ANDROID__
    void ActionTogglePerf();
    void ActionEnterVR();

protected:

    bool eventFilter(QObject *obj, QEvent *event);

    void resizeEvent(QResizeEvent *event);
#endif

private:

    void DoOpenURL(const QString url);
    void SetupMenuWidgets();
    void SetupWidgets();

    void Update();    
    void UpdateHands();

    QString GetNewWorkspaceDirectory();

    QTimer timer;
    QTimer timer2;    
    QMutex cef_mutex;

    QPointer <Game> game;
    QPointer <AbstractHMDManager> hmd_manager;
    QPointer <QOpenGLDebugLogger> opengl_debug_logger;

    int cur_screen;
    bool fullscreened;

    QPointer <SocialWindow> social_window;
    QPointer <SettingsWindow> settings_window;
#ifndef __ANDROID__
    QPointer <CodeEditorWindow> codeeditor_window;
    QPointer <HierarchyWindow> hierarchy_window;
    QPointer <PropertiesWindow> properties_window;
    QPointer <AssetWindow> asset_window;
    QPointer <NavigationWindow> navigation_window;
#endif

    QMenu * ellipsisMenu;
    QPushButton * button_ellipsis;

    QPushButton * button_back;    
    QPushButton * button_forward;
    QPushButton * button_reload;    
    QPushButton * button_home;
    QLineEdit * urlbar;
    QProgressBar * progressbar;
    QPushButton * button_bookmark;
    int button_bookmark_state;
    QPointer <GLWidget> glwidget;
#ifndef __ANDROID__
    QMenu *webspaceMenu;
    QMenu *usersMenu;
    QMenu *panelsMenu;
#endif
    QMenu *assetsMenu;
    QMenu *bookmarkMenu;
    QMenu *viewMenu;
    QAction *newAct;
    QAction *openAct;
    QAction *saveAct;
    QAction *saveAsAct;
    QAction *importLocalAct;
    QAction *importRemoteAct;
    QAction *saveThumbAct;
    QAction *saveScreenshotAct;
    QAction *saveEquiAct;
    QAction *exitAct;
    QAction *socialAct;
    QAction *startRecordingAct;
    QAction *startRecordingEveryoneAct;
    QAction *stopRecordingAct;
    QAction *syncToAct;
    QAction *settingsAct;
#ifndef __ANDROID__
    QAction *codeEditorAct;
    QAction *navigationAct;
    QAction *hierarchyAct;
    QAction *propertiesAct;
    QAction *assetAct;
#endif
    QAction *addBookmarkAct;
    QAction *removeBookmarkAct;
    QAction *toggleFullscreenAct;
#ifdef __ANDROID__
    QAction *togglePerfAct;
    QAction *enterVRAct;
#endif

    QSplitter * splitter;

#ifdef __ANDROID__
    SlidingTabWidget * url_bar_tab;
    SlidingTabWidget * social_window_tab;

    QElapsedTimer url_bar_tab_timer;

    bool show_splash;

    bool paused;
    bool require_permissions;
    bool asked_permissions;
#endif

    bool repaint_queued;
    Qt::WindowFlags default_window_flags;
};

#endif // MAINWINDOW_H
