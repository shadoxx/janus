#include <QApplication>

#include "mainwindow.h"
#include "mathutil.h"
#include "multiplayermanager.h"

#ifdef __ANDROID__
#include "jniutil.h"
#include "settingsmanager.h"
#endif

#ifdef WIN32
#include <conio.h>
#include <Windows.h>
#endif

#ifndef __ANDROID__
#include "include/cef_app.h"
#endif

//Uncomment to enable Leak debugging in Visual Studio when using Visual Leak Detector
//#include <vld.h>

//void SetRegistrySettings()
//{
//    qDebug() << "SetRegistrySettings()";
//#ifdef WIN32
//    {
//    QSettings settings("HKEY_CLASSES_ROOT\\janus", QSettings::NativeFormat);
//    settings.setValue("@", "URL:janus Protocol");
//    settings.setValue("URL Protocol", "");
//    settings.setValue("EditFlags", 0x00000002);
//    }
//    {
//    QSettings settings("HKEY_CLASSES_ROOT\\janus\\DefaultIcon", QSettings::NativeFormat);
//    settings.setValue(".", QString(""));
//    }
//    {
//    QSettings settings("HKEY_CLASSES_ROOT\\janus\\shell", QSettings::NativeFormat);
//    settings.setValue(".", QString("open"));
//    }
//    {
//    QSettings settings("HKEY_CLASSES_ROOT\\janus\\shell\\open", QSettings::NativeFormat);
//    settings.setValue("CommandId", QString("IE.Protocol"));
//    }
//    {
//    QSettings settings("HKEY_CLASSES_ROOT\\janus\\shell\\open\\command", QSettings::NativeFormat);
//    settings.setValue(".", QString("\"" + QCoreApplication::applicationFilePath() + "\" \"%1\""));
//    }
//    {
//    QSettings settings("HKEY_CLASSES_ROOT\\janus\\shell\\open\\ddeexec", QSettings::NativeFormat);
//    settings.setValue(".", QString(""));
//    }
//#endif

//}

void DisplayVersion()
{
    std::cout << QString(__JANUS_VERSION_COMPLETE).toLatin1().data() << "\n";
}

void DisplayHelp()
{
    std::cout << "Janus VR - Help\n\n";
    std::cout << "Command line usage:\n\n";
    std::cout << "  janusvr [-server SERVER] [-port PORT]\n";
    std::cout << "          [-adapter X] [-render MODE] [-window] [-width X] [-height X]\n";
    std::cout << "          [-help] [-pos X Y Z] [-output_cubemap X] [<url>]\n\n";
    std::cout << "    -server     - SERVER specifies the server to create an initial portal\n";
    std::cout << "    -port       - PORT is a number that specifies the port of the server to connect to\n";
    std::cout << "    -adapter    - X specifies the screen that the Janus window will be positioned on\n";
    std::cout << "    -render     - MODE specifies render mode\n";
    std::cout << "                  (can be: 2d, sbs, sbs_reverse, ou3d, cube, equi, rift, vive)\n";
    std::cout << "    -gl         - MODE specifies render mode,\n";
    std::cout << "                  (can be: 3.3, 4.4, 4.4EXT, FORCE4.4EXT\n";
    std::cout << "    -window     - launch as a window instead of fullscreen\n";
    std::cout << "    -width      - set width of JanusVR window in windowed mode\n";
    std::cout << "    -height     - set height of JanusVR window in windowed mode\n";
    std::cout << "    -novsync    - run JanusVR without V-sync\n";
    std::cout << "    -pos        - position in space (X,Y,Z) to start in\n\n";
    std::cout << "    -output_cubemap     - save out 2k x 2k per face cubemap face images with filename\n";
    std::cout << "                          prefix X from [pos] at  [url], then exit\n";
    std::cout << "    -output_equi        - same as output_cubemap but also saves out an 8k x 4k\n";
    std::cout << "                          equirectangular image with filename prefix X from [pos] at \n";
    std::cout << "                          [url], then exit\n";
    std::cout << "    -help       - prints out this help information, then exits\n";
    std::cout << "    -version    - prints version, then exits\n";
    std::cout << "    <url>       - location to start in\n";
}

bool isCommandArg2(QString arg)
{
    bool const isServer = (QString::compare(arg, "-server", Qt::CaseInsensitive) == 0);
    bool const isPort   = (QString::compare(arg, "-port", Qt::CaseInsensitive) == 0);
    bool const isCubemap = (QString::compare(arg, "-output_cubemap", Qt::CaseInsensitive) == 0);
    bool const isEqui = (QString::compare(arg, "-output_equi", Qt::CaseInsensitive) == 0);
    bool const isAd = (QString::compare(arg, "-ad", Qt::CaseInsensitive) == 0);

    return (isServer || isPort || isCubemap || isEqui || isAd);
}

void ProcessCmdLineArgs1(int argc, char *argv[])
{
    //49.45 - Set command line parameters *after* constructors are called, so we can change defaults/override saved settings
    if (argc > 1) {
        for (int i=1; i<argc; ++i) {
            const QString eacharg(argv[i]);
            if (isCommandArg2(eacharg))
            {
                // Skip over 2 args if this one is used later,
                // this avoids reading cubemap paths as a custom URL
                ++i;
                continue;
            }
            else if (QString::compare(eacharg, "-gl", Qt::CaseInsensitive) == 0) {
#ifdef __APPLE__
                MathUtil::m_requested_gl_version = "3.3";
#elif defined(__ANDROID__)
                MathUtil::m_requested_gl_version = "ES";
#else
                MathUtil::m_requested_gl_version = QString(argv[i+1]);
#endif
                ++i;
                continue;
            }
            else if (QString::compare(eacharg, "-adapter", Qt::CaseInsensitive) == 0 && i < argc-1) {
                QString eacharg2(argv[i+1]);
                MainWindow::use_screen = eacharg2.toInt();
                ++i;
                continue;
            }
            else if (QString::compare(eacharg, "-window", Qt::CaseInsensitive) == 0) {
                MainWindow::window_mode = true;
                continue;
            }
            else if (QString::compare(eacharg, "-width", Qt::CaseInsensitive) == 0 && i < argc-1) {
                QString eacharg2(argv[i+1]);
                MainWindow::window_width = eacharg2.toInt();
                ++i;
                continue;
            }
            else if (QString::compare(eacharg, "-height", Qt::CaseInsensitive) == 0 && i < argc-1) {
                QString eacharg2(argv[i+1]);
                MainWindow::window_height = eacharg2.toInt();
                ++i;
                continue;
            }
            else if (QString::compare(eacharg, "-novsync", Qt::CaseInsensitive) == 0) {
                GLWidget::SetNoVSync(true);
                continue;
            }
            else if ((QString::compare(eacharg, "-render", Qt::CaseInsensitive) == 0 ||
                      QString::compare(eacharg, "-mode", Qt::CaseInsensitive) == 0) && i < argc-1) {
                QString eacharg2(argv[i+1]);
                DisplayMode d = MODE_AUTO;
                if (QString::compare(eacharg2, "sbs", Qt::CaseInsensitive) == 0) {
                    d = MODE_SBS;
                }
                else if (QString::compare(eacharg2, "sbs_reverse", Qt::CaseInsensitive) == 0) {
                    d = MODE_SBS_REVERSE;
                }
                else if (QString::compare(eacharg2, "ou3d", Qt::CaseInsensitive) == 0) {
                    d = MODE_OU3D;
                }
                else if (QString::compare(eacharg2, "cube", Qt::CaseInsensitive) == 0) {
                    d = MODE_CUBE;
                }
                else if (QString::compare(eacharg2, "equi", Qt::CaseInsensitive) == 0) {
                    d = MODE_EQUI;
                    MathUtil::m_do_equi = true;
                }
                else if (QString::compare(eacharg2, "rift", Qt::CaseInsensitive) == 0) {
                    d = MODE_RIFT;
                    //m_linear_framebuffer = true;
                }
                else if (QString::compare(eacharg2, "vive", Qt::CaseInsensitive) == 0) {
                    d = MODE_VIVE;
                }                
                else {
                    d = MODE_2D;
                }
                GLWidget::SetDisplayMode(d);
                ++i;
                continue;
            }           
            else if (QString::compare(eacharg, "-help", Qt::CaseInsensitive) == 0 ||
                     QString::compare(eacharg, "-h", Qt::CaseInsensitive) == 0) {
                MainWindow::display_help = true;
                continue;
            }
            else if (QString::compare(eacharg, "-version", Qt::CaseInsensitive) == 0 ||
                     QString::compare(eacharg, "-v", Qt::CaseInsensitive) == 0) {
                MainWindow::display_version = true;
                continue;
            }
            else {
//                qDebug() << "ProcessCmdLineArgs1 url" << argv[i];
                Environment::SetLaunchURLIsCustom(true);
                Environment::SetLaunchURL(QString(argv[i]));
                continue;
            }
        }
    }
    //MainWindow::use_custom_url = true;
    //MainWindow::custom_url = QString("http://janusvr.com");
}

void ProcessCmdLineArgs2(int argc, char *argv[])
{
    //49.45 - Set command line parameters *after* constructors are called, so we can change defaults/override saved settings
    if (argc > 1) {
        for (int i=1; i<argc; ++i) {
            const QString eacharg(argv[i]);
            if (QString::compare(eacharg, "-server", Qt::CaseInsensitive) == 0 && i < argc-1) {
                QString eacharg2(argv[i+1]);
                SettingsManager::SetServer(eacharg2);
                ++i;
            }
            else if (QString::compare(eacharg, "-port", Qt::CaseInsensitive) == 0&& i < argc-1) {
                QString eacharg2(argv[i+1]);
                SettingsManager::SetPort(eacharg2.toInt());
                ++i;
            }  
            else if (QString::compare(eacharg, "-output_cubemap", Qt::CaseInsensitive) == 0 && i < argc-1) {
                MainWindow::output_cubemap = true;
                MainWindow::output_cubemap_filename_prefix = QString(argv[i+1]);
                ++i;
            }
            else if (QString::compare(eacharg, "-output_equi", Qt::CaseInsensitive) == 0 && i < argc-1) {
                MainWindow::output_equi = true;
                MainWindow::output_cubemap_filename_prefix = QString(argv[i+1]);
                ++i;
            }
        }
    }
    //MainWindow::output_cubemap = true;
    //MainWindow::output_cubemap_filename_prefix = QString("example3");
}

#ifdef WIN32 //59.3 - declspec and following definitions are Windows only
extern "C"
{
#ifndef __ANDROID__
    __declspec(dllexport) DWORD NvOptimusEnablement = 1;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
#endif
}
#endif

int main(int argc, char *argv[])
{
    bool    cefIsInitialized; // for storing the result of calling CefInitialize()
    int     cefExecuteResult; // for storing the result of calling CefExecuteProcess()

    //duplicate argv so CEF doesn't modify it, and Janus cmd line parameters continue to work
    char ** argv_copy = new char *[argc];
    for (int i=0; i<argc; ++i) {
        const int len = QString(argv[i]).length()+1; //null-terminated character strings
        argv_copy[i] = new char[len];
        memcpy(argv_copy[i], argv[i], len);
    }

#ifdef WIN32
    CefMainArgs main_args;
#elif !defined(__ANDROID__)
    CefMainArgs main_args(argc, argv_copy);
    //CefMainArgs main_args;
#endif

#ifndef __ANDROID__
    CefRefPtr<CEFApp> janusapp = new CEFApp();

    cefExecuteResult = CefExecuteProcess(main_args, janusapp, nullptr);

    // checkout CefApp, derive it and set it as second parameter, for more control on
    // command args and resources.
    if (cefExecuteResult >= 0) // child proccess has endend, so exit.
    {
        qDebug() << "CefExecuteProcess(): The child has terminated abnormally";
        return cefExecuteResult;
    }
    if (cefExecuteResult == -1)
    {
        // we are here in the father proccess.
        qDebug() << "CefExecuteProcess(): Chromium Embedded Framework Started";
    }


// TODO: Move this to its own initialization function
    CefSettings settings;

// Toggle the verbosity of CefEngine output
#ifndef QT_DEBUG
    settings.log_severity = LOGSEVERITY_DEFAULT;
#else
    settings.log_severity = LOGSEVERITY_DEBUG;
#endif

    settings.multi_threaded_message_loop = false;
    settings.no_sandbox = true;
    settings.ignore_certificate_errors = true;
    settings.persist_session_cookies = true;
    settings.persist_user_preferences = true;
    settings.external_message_pump = true;

    cefIsInitialized = CefInitialize(main_args, settings, janusapp, nullptr);
    // CefInitialize creates a sub-proccess and executes the same executeable, as calling CefInitialize, if not set different in settings.browser_subprocess_path
    // if you create an extra program just for the childproccess you only have to call CefExecuteProcess(...) in it.
    if (!cefIsInitialized) {
        // handle error
        qDebug() << "CefInitialize(): Unable to initialize CefEngine";
        return -1;
    } else {
        qDebug() << "CefInitialize(): CefEngine Initialization successful! Starting the message pump...";
        CefDoMessageLoopWork();
    }
#endif

    QApplication a(argc, argv);
    a.setStyle(QStyleFactory::create("fusion"));
    a.setStyleSheet("QMenu {background: #2F363B; color: #FFFFFF; border: 2px solid #FFFFFF;}"
                #ifdef __ANDROID__
                    "QMenu::item { padding: 18px; font: 20px;}"
                #endif
                    //59.9 - note: use default (padding: 0px) for non-Android builds
                    "QMenu::item:selected { background: #414A51; }"
                    "QMenu::item:disabled { color: #4A5B68; }");

#ifdef WIN32
    //    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
#endif

    int ideal_thread_count = QThread::idealThreadCount();
    if (ideal_thread_count > 2)
    {
        // Leave two threads out of the pool for use in the main and render threads which are created
        // outside of the thread pool
        ideal_thread_count = ideal_thread_count - 2;
    }
    //    QThreadPool::globalInstance()->setMaxThreadCount(ideal_thread_count);
    //    QThreadPool::globalInstance()->setMaxThreadCount(2);

#ifdef __ANDROID__
    qDebug() << "JNI initialize";
    JNIUtil::Initialize();
#endif
    qDebug() << "CookieJar initialize";
    CookieJar::Initialize();
    qDebug() << "WebAsset initialize";
    WebAsset::Initialize();
    qDebug() << "MathUtil initialize";
    MathUtil::Initialize();

    ProcessCmdLineArgs1(argc, argv);

    MainWindow w;

    if (MainWindow::display_help) {
        DisplayHelp();
        return 0;
    }

    if (MainWindow::display_version) {
        DisplayVersion();
        return 0;
    }

    //move the window to the screen if needed
    if (MainWindow::use_screen != -1 && MainWindow::use_screen < QApplication::desktop()->screenCount() ) {
        QRect screenres = QApplication::desktop()->screenGeometry(MainWindow::use_screen);
        w.move(QPoint(screenres.x(), screenres.y()));
    }

    if ((MainWindow::use_screen != -1 &&
         MainWindow::use_screen < QApplication::desktop()->screenCount()) ||
            MainWindow::window_width > 0 ||
            MainWindow::window_height > 0) {
        QRect screenres = QApplication::desktop()->screenGeometry(MainWindow::use_screen);
        w.resize((MainWindow::window_width > 0) ? MainWindow::window_width : screenres.width(),
                 (MainWindow::window_height > 0) ? MainWindow::window_height : screenres.height());
    }

#ifdef __ANDROID__
    JNIUtil::SetMainWindow(&w);

    QSurfaceFormat fmt;
    fmt.setVersion(3,1);
    fmt.setDepthBufferSize(24);
    fmt.setStencilBufferSize(8);
    fmt.setSwapBehavior(QSurfaceFormat::TripleBuffer);
    fmt.setRenderableType(QSurfaceFormat::OpenGLES);
    //fmt.setOption(QSurfaceFormat::DebugContext);
    fmt.setSwapInterval(0);

    if (SettingsManager::GetDemoModeEnabled() && SettingsManager::GetDemoModeWindowMaximize()) {
        w.showMaximized();
    }
    else{
        w.show();
    }
    QSurfaceFormat::setDefaultFormat(fmt);
#else
    if (SettingsManager::GetDemoModeEnabled() && SettingsManager::GetDemoModeWindowMaximize()) {
        w.showMaximized();
    }
    else {
        w.show();
    }
#endif

//#ifdef WIN32
//    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
//#endif

#ifdef __APPLE__
    QDir::setCurrent(QApplication::applicationDirPath());
#endif
    qDebug() << "main(): application path " << MathUtil::GetApplicationPath();

    w.Initialize();
    ProcessCmdLineArgs2(argc, argv);

    qDebug() << "main(): writing settings to path" << MathUtil::GetAppDataPath();
    const int ret_val = a.exec();

//#ifndef __ANDROID__
//    CEFWebView::Shutdown();
//    qDebug() << "CefShutdown() started";
//    CefShutdown(); //shut down CEF (any other janusvr.exe processes launched)
//    qDebug() << "CefShutdown() done";
//
//    Possible fix for the CefShutdown() issue:
//    SetErrorMode(SEM_NOGPFAULTERRORBOX);
//    CefShutdown();
//#endif

    return ret_val;
}
