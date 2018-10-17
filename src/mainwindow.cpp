#include "mainwindow.h"

int MainWindow::use_screen = -1;
bool MainWindow::window_mode = false;
int MainWindow::window_width = 0;
int MainWindow::window_height = 0;
bool MainWindow::display_help = false;
bool MainWindow::display_version = false;
bool MainWindow::output_cubemap = false;
bool MainWindow::output_equi = false;
QString MainWindow::output_cubemap_filename_prefix;

MainWindow::MainWindow()
    : cur_screen(0),
      fullscreened(false)
{
    SettingsManager::LoadSettings();   

    //set application-wide font
    const QString font_path = MathUtil::GetApplicationPath() + "assets/fonts/OpenSans-Regular.ttf";
    int opensans_id = QFontDatabase::addApplicationFont(font_path);
    if (!QFontDatabase::applicationFontFamilies(opensans_id).isEmpty()) {
        QString opensans_family = QFontDatabase::applicationFontFamilies(opensans_id).at(0);
        QFont opensans(opensans_family);
        QApplication::setFont(opensans);
    }
    else {
        qDebug() << "Error: Unable to find" << font_path;
    }

    //set mainwindow style
    QString style = "QHeaderView::section {background-color: #59646B;}"
                    "QScrollBar {border: none; background: #3E4D54;}"
                    "QScrollBar::sub-page, QScrollBar::add-page { background: #3E4D54;}"
                    "QScrollBar::sub-line, QScrollBar::add-line {border: none; background: none;}"
                    "QScrollBar::handle {border:none; background: #59646B;}"
                    "QScrollBar::left-arrow, QScrollBar::right-arrow, QScrollBar::up-arrow, QScrollBar::down-arrow {background: none; border: none;}";
#ifdef __ANDROID__
    asked_permissions = false;
    require_permissions = !JNIUtil::GetPermissions();
    show_splash = true;
    paused = false;

    qApp->installEventFilter(this);

    url_bar_tab = new SlidingTabWidget();
    url_bar_tab->setTabPosition(QTabWidget::South);
    //url_bar_tab->setVisible(false);

    social_window_tab = new SlidingTabWidget();
    social_window_tab->setTabPosition(QTabWidget::West);
    //social_window_tab->setVisible(false);

    url_bar_tab->setIconSize(QSize(100,140));
    social_window_tab->setIconSize(QSize(100,140));

    style += " QWidget {font: 20px;}";

    //Hover: #3E4D54; Click: #1F2227
    url_bar_tab->setStyleSheet("QTabBar::tab {border-bottom-left-radius: 20px; border-bottom-right-radius: 20px; background: #2F363B; height: 160px; width: 120px; padding: 0px; padding-left:20px; margin-left:64px;}"
                               "QTabWidget::pane {background: #2F363B; margin: 0px,0px,0px,0px; padding:0px;}");
                               //"QTabBar::tab:selected,QTabBar::tab:hover  {background: #3E4D54;} "
                               //"QTabWidget::pane:selected,QTabWidget::pane:hover {background: #3E4D54;}");
    social_window_tab->setStyleSheet("QTabBar::tab {border-top-left-radius: 20px; border-bottom-left-radius: 20px; background: #2F363B; height: 120px; width: 160px; padding: 0px; padding-bottom:20px; margin-top:64px;}"
                                     "QTabWidget::pane {background: #2F363B; margin: 0px,0px,0px,0px; padding:0px;}");
                                     //"QTabBar::tab:selected,QTabBar::tab:hover  {background: #3E4D54;} "
                                     //"QTabWidget::pane:selected,QTabWidget::pane:hover {background: #3E4D54;}");

#ifdef OCULUS_SUBMISSION_BUILD
    url_bar_tab->setVisible(false);
    social_window_tab->setVisible(false);
#endif

#endif

    setStyleSheet(style);  //Mainwindow

    setMouseTracking(true);
    setAcceptDrops(true);   

    CloseEventFilter *closeFilter = new CloseEventFilter(this);
    installEventFilter(closeFilter);
    connect(closeFilter, SIGNAL(Closed()), this, SLOT(Closed()));

    //set window title (and optionally, demo_windowtitle override
    if (SettingsManager::GetDemoModeEnabled() && !SettingsManager::GetDemoModeWindowTitle().isEmpty()) {
        setWindowTitle(SettingsManager::GetDemoModeWindowTitle());
    }
    else {
#ifdef OCULUS_SUBMISSION_BUILD
        setWindowTitle(QString("Janus VR (Oculus build "+QString(__JANUS_VERSION_COMPLETE)+")"));
#else
        setWindowTitle(QString("Janus VR ("+QString(__JANUS_VERSION_COMPLETE)+")"));
#endif
    }

    if (SettingsManager::GetDemoModeEnabled() && !SettingsManager::GetDemoModeWindowIcon().isEmpty()) {
        setWindowIcon(QIcon(SettingsManager::GetDemoModeWindowIcon()));
    }

    setMinimumSize(QSize(800, 600));

    //setSurfaceType(QSurface::OpenGLSurface);

    //Initialize HMD (if present)
    DisplayMode disp_mode = GLWidget::GetDisplayMode();
#ifdef WIN32        

#ifdef OCULUS_SUBMISSION_BUILD

    hmd_manager = new RiftManager();
    hmd_manager->Initialize();
    if (disp_mode == MODE_AUTO || disp_mode == MODE_RIFT) {
        disp_mode = MODE_RIFT;
        if (!hmd_manager->GetEnabled()) { //could not init Rift, do 2D
            disp_mode = MODE_2D;
        }        
    }

#else

    if (disp_mode == MODE_AUTO) {
        //Attempt Oculus initialization only on mode auto
        hmd_manager = new RiftManager();
        hmd_manager->Initialize();
        disp_mode = MODE_RIFT;

        if (!hmd_manager->GetEnabled()) { //could not init Rift
            delete hmd_manager;
            hmd_manager = new ViveManager();
            hmd_manager->Initialize();
            if (!hmd_manager->GetEnabled()) { //count not init Vive/WMXR
                disp_mode = MODE_2D;
            }
            else {
                disp_mode = MODE_VIVE;
            }
        }
    }
    else if (disp_mode == MODE_VIVE) {
        hmd_manager = new ViveManager();
        hmd_manager->Initialize();

        if (!hmd_manager->GetEnabled()) { //count not init Vive
            disp_mode = MODE_2D;
        }
    }
    else if (disp_mode == MODE_RIFT) {
        hmd_manager = new RiftManager();
        hmd_manager->Initialize();
        disp_mode = MODE_RIFT;

        if (!hmd_manager->GetEnabled()) { //count not init Vive
            disp_mode = MODE_2D;
        }        
    }

#endif

#elif defined __ANDROID__
    if (disp_mode == MODE_AUTO) {

#ifdef OCULUS_SUBMISSION_BUILD
        hmd_manager = new GearManager();
        bool enabled = hmd_manager->Initialize();

        if (SettingsManager::GetUseVR() || JNIUtil::GetLaunchInVR() || hmd_manager->GetHMDType() == "go" || hmd_manager->GetHMDType() == "gear"){
            disp_mode = MODE_GEAR;

            if (!enabled) {
                qDebug() << "MainWindow::MainWindow() - Failed to initialize Gear";
                hmd_manager.clear();

                JNIUtil::SetVRModeEnabled(false);

                disp_mode = MODE_2D;
            }

            /*if (disp_mode == MODE_GEAR && require_permissions){
                JNIUtil::AskPermissions();
                asked_permissions = true;
            }*/
        }
        else {
            if (!enabled) {
                qDebug() << "MainWindow::MainWindow() - Failed to initialize Gear";
                hmd_manager.clear();
            }

            JNIUtil::SetVRModeEnabled(false);

            disp_mode = MODE_2D;
        }
#elif defined(DAYDREAM_SUBMISSION_BUILD)
        bool enabled = false;
        if (!require_permissions || SettingsManager::GetUseVR() || JNIUtil::GetLaunchInVR()) {
            hmd_manager = new GVRManager();
            enabled = hmd_manager->Initialize();
        }

        if (SettingsManager::GetUseVR() || JNIUtil::GetLaunchInVR()){
            disp_mode = MODE_GVR;

            if (!enabled) {
                qDebug() << "MainWindow::MainWindow() - Failed to initialize GVR";
                hmd_manager.clear();

                JNIUtil::SetVRModeEnabled(false);

                disp_mode = MODE_2D;
            }
        }
        else {
            if (!enabled) {
                qDebug() << "MainWindow::MainWindow() - Failed to initialize GVR";
                hmd_manager.clear();
            }

            JNIUtil::SetVRModeEnabled(false);

            disp_mode = MODE_2D;
        }
#endif
    }
#elif defined __linux__
    if (disp_mode == MODE_AUTO || disp_mode == MODE_VIVE) {
        hmd_manager = new ViveManager();
        hmd_manager->Initialize();

        if (!hmd_manager->GetEnabled()) { //count not init Vive
            hmd_manager.clear();
            disp_mode = MODE_2D;
        }        
        else {
            disp_mode = MODE_VIVE;
        }
    }    
#else
    //mac (always just do 2D - no VR stuff)
    if (disp_mode == MODE_AUTO || disp_mode == MODE_RIFT || disp_mode == MODE_VIVE) {
        disp_mode = MODE_2D;
    }
#endif   

    GLWidget::SetDisplayMode(disp_mode);

    glwidget = new GLWidget();
    glwidget->setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);

//    glwidget->setMouseTracking(true);

//    QSurfaceFormat format;

//    int major = 4;
//    int minor = 4;

//    if (MathUtil::m_requested_gl_version == "3.3")
//    {
//        major = 3;
//        minor = 3;
//    }

//    format.setMajorVersion(major);
//    format.setMinorVersion(minor);
//    format.setProfile(QSurfaceFormat::CoreProfile);
//    format.setRenderableType(QSurfaceFormat::OpenGL);
//    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);

//#if defined(QT_DEBUG)
//    format.setOption(QSurfaceFormat::DebugContext);
//#endif
//    format.setDepthBufferSize(24);
//    format.setStencilBufferSize(8);

    //this->create() called implicitly on show()
//    m_context = new QOpenGLContext();
//    m_context->setFormat(format);
//    bool creation_result = m_context->create();
//    auto major_version = m_context->format().majorVersion();
//    auto minor_version = m_context->format().minorVersion();
//    bool const is_gl_44_context = (major_version > 4 || (major_version == 4 && minor_version >= 4));


//    if (!creation_result || !is_gl_44_context)
//    {
//        format.setMajorVersion(3);
//        format.setMinorVersion(3);
//        m_context->setFormat(format);
//        creation_result = m_context->create();
//        major_version = m_context->format().majorVersion();
//        minor_version = m_context->format().minorVersion();
//        bool const is_gl_33_context = (major_version > 3 || (major_version == 3 && minor_version >= 3));

//        if (!creation_result || !is_gl_33_context)
//        {
//            MathUtil::ErrorLog(QString("CRITICAL ERROR: Unable to create GL 3.3 context for rendering"));
//            MathUtil::FlushErrorLog();
//        }
//        else {
//            qDebug() << "MainWindow::MainWindow() - Creating GL 3.3 context";
//        }
//    }
//    else {
//        qDebug() << "MainWindow::MainWindow() - Creating GL 4.4 context";
//    }

    //set vsync appropriately
//    if ((hmd_manager && hmd_manager->GetEnabled()) || novsync) {
//        format.setSwapInterval(0); //this disables vsync
//    }

    //setFormat(format);
//    glwidget->setFormat(format);
    //glwidget->initializeOpenGLFunctions();

    //set screen size
    QRect screenSize = QApplication::desktop()->screenGeometry();
    int margin_x = int(float(screenSize.width())*0.1f);
    int margin_y = int(float(screenSize.height())*0.1f);
    setGeometry(margin_x, margin_y, screenSize.width()-margin_x*2, screenSize.height()-margin_y*2);

    if (SettingsManager::GetDemoModeEnabled() && SettingsManager::GetDemoModeGrabCursor()) {
        glwidget->SetGrab(true);
    }

    button_bookmark_state = 0;
    default_window_flags = windowFlags();

#ifdef __ANDROID__
    SetupWidgets();
    SetupMenuWidgets();
#endif
}

MainWindow::~MainWindow()
{
    disconnect(&timer2);
    MathUtil::FlushErrorLog();

    CookieJar::cookie_jar->SaveToDisk();
    SoundManager::Unload();    

#if !defined(__APPLE__) && !defined(__ANDROID__)
    GamepadShutdown();
#endif

    //60.0 - causes a crash in the render thread
//    delete RendererInterface::m_pimpl;
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    event->accept();
}

void MainWindow::dragMoveEvent(QDragMoveEvent* )
{

}

void MainWindow::dragLeaveEvent(QDragLeaveEvent* )
{

}

void MainWindow::dropEvent(QDropEvent* event)
{
#ifndef __ANDROID__
    asset_window->AddAssetGivenPath(event->mimeData()->text());
#endif
    activateWindow();
    glwidget->SetGrab(true);
}

void MainWindow::keyPressEvent(QKeyEvent* e)
{
    //qDebug() << "KEYPRESSEVENT" << e->key();
    if (e->key()==Qt::Key_T && !game->GetPlayerEnteringText() && !urlbar->hasFocus()
#ifndef __ANDROID__
            && !codeeditor_window->GetHasFocus() && !properties_window->GetHasFocus()
#endif
            ) {
        e->accept();
    }
    else if (e->key() == Qt::Key_F11) {
        e->accept();
    }
    else
    {
        e->ignore();
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent* e)
{
//    qDebug() << "MainWindow::keyReleaseEvent()";
    if (e->key()==Qt::Key_T && !game->GetPlayerEnteringText() && !urlbar->hasFocus()
#ifndef __ANDROID__
            && !codeeditor_window->GetHasFocus() && !properties_window->GetHasFocus()
#endif
            ) {
        if (e->modifiers().testFlag(Qt::ControlModifier)) {
            glwidget->SetGrab(true);
            social_window->setVisible(false);
        }
        else {
            glwidget->SetGrab(false);
            social_window->SetFocusOnChatEntry(true);
        }
        e->accept();
    }
    else if ((e->key()==Qt::Key_L && (e->modifiers().testFlag(Qt::ControlModifier))) ||
             e->key()==Qt::Key_F6) {
        glwidget->SetGrab(false);
        urlbar->setFocus();
        urlbar->selectAll();
        e->accept();
    }
    else if (e->key() == Qt::Key_F11) {
        ActionToggleFullscreen();
        e->accept();
    }
#ifdef __ANDROID__
    else if (e->key() == Qt::Key_Back || e->key() == Qt::Key_Close) {
#if !defined(OCULUS_SUBMISSION_BUILD)
        if (JNIUtil::GetShowingVR() && hmd_manager){
            ExitVR();
        }
#endif
        e->accept();
    }
#endif
    else
    {
        e->ignore();
    }
}

bool MainWindow::focusNextPrevChild(bool )
{
    return false;
}

#ifdef __ANDROID__
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    switch( event->type() ){
        case QEvent::MouseButtonPress: {
            QMouseEvent *e = (QMouseEvent *)event;
            //qDebug() << "MOUSEPRESS"<< url_bar_tab->tabBar()->tabAt(url_bar_tab->tabBar()->mapFromGlobal(e->pos()));
            //qDebug() << "OBJECTCLICKED" << obj->metaObject()->className();
            //if (QApplication::focusWidget()) qDebug() << "CURRENTLYFOCUSED:" << QApplication::focusWidget()->metaObject()->className();

            if (!settings_window->isVisible() && strcmp(obj->metaObject()->className(), "QWidgetWindow") == 0) //Prevent mouse events from firing twice, stops URL bar tab from opening at same time as social window
            {
                if (social_window_tab->tabBar()->tabAt(social_window_tab->tabBar()->mapFromGlobal(mapToGlobal(e->pos()))) >= 0)
                {
                    //qDebug() << "SOCIALWINDOWPRESSED";
                    QMouseEvent e2(QEvent::MouseButtonPress, social_window_tab->tabBar()->mapFromGlobal(mapToGlobal(e->pos())), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                    social_window_tab->mousePressEvent(&e2);
                }
                else if (url_bar_tab->tabBar()->tabAt(url_bar_tab->tabBar()->mapFromGlobal(mapToGlobal(e->pos()))) >= 0)
                {
                    //qDebug() << "URLBARPRESSED";
                    QMouseEvent e2(QEvent::MouseButtonPress, url_bar_tab->tabBar()->mapFromGlobal(mapToGlobal(e->pos())), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                    url_bar_tab->mousePressEvent(&e2);
                }

                if (glwidget->GetGrab() &&
                    (social_window_tab->tabBar()->tabAt(social_window_tab->tabBar()->mapFromGlobal(mapToGlobal(e->pos()))) >= 0 || //Clicked on tab
                     (social_window_tab->rect().contains(social_window_tab->mapFromGlobal(mapToGlobal(e->pos()))) && !social_window_tab->tabBar()->rect().contains(social_window_tab->tabBar()->mapFromGlobal(mapToGlobal(e->pos())))) || //Clicked on tab widget
                    (url_bar_tab->tabBar()->tabAt(url_bar_tab->tabBar()->mapFromGlobal(mapToGlobal(e->pos()))) >= 0 ||
                     (url_bar_tab->rect().contains(url_bar_tab->mapFromGlobal(mapToGlobal(e->pos()))) && !url_bar_tab->tabBar()->rect().contains(url_bar_tab->tabBar()->mapFromGlobal(mapToGlobal(e->pos())))))))

                {
                    glwidget->SetGrab(false); //Reset grab, allow users to register mouse events to URL bar and social window
                                              //Only if user clicks outside of glwidget, inside a tab or at the tab's widget
                }
                else if (!glwidget->GetGrab() &&
                         !(social_window_tab->tabBar()->tabAt(social_window_tab->tabBar()->mapFromGlobal(mapToGlobal(e->pos()))) >= 0 || //Clicked on tab
                          (social_window_tab->rect().contains(social_window_tab->mapFromGlobal(mapToGlobal(e->pos()))) && !social_window_tab->tabBar()->rect().contains(social_window_tab->tabBar()->mapFromGlobal(mapToGlobal(e->pos())))) || //Clicked on tab widget
                         (url_bar_tab->tabBar()->tabAt(url_bar_tab->tabBar()->mapFromGlobal(mapToGlobal(e->pos()))) >= 0 ||
                          (url_bar_tab->rect().contains(url_bar_tab->mapFromGlobal(mapToGlobal(e->pos()))) && !url_bar_tab->tabBar()->rect().contains(url_bar_tab->tabBar()->mapFromGlobal(mapToGlobal(e->pos())))))))
                {
                    glwidget->SetGrab(true); //Grab on mouse press if user doesn't click on any tabs
                }

                JNIUtil::HideKeyboard();
            }
        }
        break;

        case QEvent::MouseMove: {
        }
        break;

        case QEvent::MouseButtonRelease: {
            QMouseEvent *e = (QMouseEvent *)event;
            //qDebug() << "MOUSERELEASE"<< url_bar_tab->tabBar()->tabAt(url_bar_tab->tabBar()->mapFromGlobal(e->pos()));
            //qDebug() << "OBJECTCLICKED" << obj->metaObject()->className();
            //if (QApplication::focusWidget()) qDebug() << "CURRENTLYFOCUSED:" << QApplication::focusWidget()->metaObject()->className();

            if (!settings_window->isVisible() && social_window_tab->GetSliding())
            {
                QMouseEvent e2(QEvent::MouseButtonRelease, social_window_tab->tabBar()->mapFromGlobal(mapToGlobal(e->pos())), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                social_window_tab->mouseReleaseEvent(&e2);
            }
            else if (!settings_window->isVisible() && url_bar_tab->GetSliding())
            {
                QMouseEvent e2(QEvent::MouseButtonPress, url_bar_tab->tabBar()->mapFromGlobal(mapToGlobal(e->pos())), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                url_bar_tab->mouseReleaseEvent(&e2);
            }

            if ((QWidget *) obj != settings_window && !settings_window->children().contains(obj)
                    && (QWidget *) obj != (QWidget *) settings_window->GetTabWidget() && !settings_window->GetTabWidget()->children().contains(obj)
                    && settings_window->isVisible() && (strcmp(obj->metaObject()->className(), "QOpenGLWidget") == 0 || strcmp(obj->metaObject()->className(), "QTabBar") == 0 || strcmp(obj->metaObject()->className(), "QTabWidget") == 0)) {
                SettingsManager::SaveSettings(); //Save settings upon hiding settings window
                settings_window->setVisible(false);
            }
        }
        break;

        default:
        break;
    }
    // pass the event on to the parent class
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    if (settings_window)
    {
        settings_window->setFixedSize(4*width()/5, 4*height()/5);
        settings_window->move(width()/10, height()/10);
    }

    //Reset URL bar / social window size
    url_bar_tab->setFixedSize(2*width()/3 - 64, url_bar_tab->widget(0)->height() + url_bar_tab->tabBar()->height());
    social_window_tab->setFixedSize(width()/3, height());

    url_bar_tab->SetWindowSize(size());
    social_window_tab->SetWindowSize(size());

    QMainWindow::resizeEvent(event);
}
#endif

void MainWindow::Update()
{
    if (game == NULL || game->GetEnvironment() == NULL || game->GetEnvironment()->GetCurRoom() == NULL) {
        return;
    }

#ifdef __ANDROID__
    // Need to always be in VR mode for Gear/Go
    if (!paused && !require_permissions && hmd_manager && (hmd_manager->GetHMDType() == "go" || hmd_manager->GetHMDType() == "gear") && (!hmd_manager->GetEnabled() || !JNIUtil::GetShowingVR())) {
        EnterVR();
    }

    //Request user remove headset to accept permissions if user is still in MODE_GVR
    if (require_permissions){
        if (GLWidget::GetDisplayMode() == MODE_2D && !asked_permissions){
            //If user removed headset, request permissions
            asked_permissions = true;
            JNIUtil::AskPermissions();
            game->SetRemoveHeadset(false);
        }
        else if (GLWidget::GetDisplayMode() == MODE_GVR && !asked_permissions){
            //Only show image requesting user to remove headset if in VR mode
            game->SetRemoveHeadset(true);
        }
        else if (GLWidget::GetDisplayMode() == MODE_GEAR && !asked_permissions){
            asked_permissions = true;
            JNIUtil::AskPermissions();
        }

        if (asked_permissions && JNIUtil::GetAnsweredPermissions()){
            if (JNIUtil::GetPermissions()){
                // Reload Settings and SoundManager if we get the permissions
                SettingsManager::LoadSettings();
                SoundManager::Load(SettingsManager::GetPlaybackDevice(),
                                   SettingsManager::GetCaptureDevice());
            }

            JNIUtil::HideSplash();
            show_splash = false;

            require_permissions = false;
            asked_permissions = false;

            if (hmd_manager && (hmd_manager->GetHMDType() == "go" || hmd_manager->GetHMDType() == "gear")) {
                EnterVR();
            }
#ifndef OCULUS_SUBMISSION_BUILD
            else if (!hmd_manager && GLWidget::GetDisplayMode() == MODE_2D){
                hmd_manager = new GVRManager();
                bool enabled = hmd_manager->Initialize();
                if (enabled){
                    glwidget->SetHMDManager(hmd_manager);
                    game->GetControllerManager()->SetHMDManager(hmd_manager);
                }
                else{
                    hmd_manager.clear();
                }
            }
#endif
        }
    }

    //qDebug() << SettingsManager::GetUseVR();
#ifndef OCULUS_SUBMISSION_BUILD
    if (GLWidget::GetDisplayMode() == MODE_GEAR || GLWidget::GetDisplayMode() == MODE_GVR || ellipsisMenu->isVisible() || settings_window->isVisible() || show_splash)
    {
        JNIUtil::SetControlsVisible(false, SettingsManager::GetShowViewJoystick());
        url_bar_tab_timer.invalidate();
    }
    else
    {
        if (url_bar_tab->GetShowing() &&         //Check if URL bar is open
            !ellipsisMenu->isVisible() && !settings_window->isVisible() &&          //Check if ellipsis menu / settings window is open
            !urlbar->hasFocus()){                                                   //Check if URL line edit is focussed
            if (!url_bar_tab_timer.isValid()){
                url_bar_tab_timer.start();
            }
            else{
                if (url_bar_tab_timer.isValid() && url_bar_tab_timer.elapsed() > 10000){
                    url_bar_tab->Hide();
                }
            }
        }
        else{
            url_bar_tab_timer.invalidate();
        }

        url_bar_tab->Update();
        social_window_tab->Update();

        if (JNIUtil::GetGamepadConnected())
            JNIUtil::SetControlsVisible(false, SettingsManager::GetShowViewJoystick());
        else
            JNIUtil::SetControlsVisible(true, SettingsManager::GetShowViewJoystick());
    }
#endif
    if (!urlbar->hasFocus() && glwidget->GetGrab() && urlbar->text() != game->GetPlayer()->GetProperties()->GetURL()) {
        urlbar->setText(game->GetPlayer()->GetProperties()->GetURL());
    }
#else
    if (!urlbar->hasFocus() && urlbar->text() != game->GetPlayer()->GetProperties()->GetURL()) {
        urlbar->setText(game->GetPlayer()->GetProperties()->GetURL());
    }
#endif

    //60.0 - top bar widget visibility
    const bool vis = !(fullscreened && glwidget->GetGrab());
    if (topbarwidget && topbarwidget->isVisible() != vis) {
        topbarwidget->setVisible(vis);
    }

    //59.3 - disable pocketspace toggle button if there is no other current viewed room to toggle to
    if (game->GetEnvironment()->GetLastRoom().isNull() && button_home->isVisible()) {
        button_home->setVisible(false);
    }
    else if (game->GetEnvironment()->GetLastRoom() && !button_home->isVisible()) {
        button_home->setVisible(true);
    }

    const int value = game->GetEnvironment()->GetCurRoom()->GetProgress()*100.0f;
    if (progressbar->value() != value) {
        progressbar->setValue(value);
    }
#ifdef __ANDROID__
        //qDebug() << progressbar->value();
    if (show_splash){
        JNIUtil::SetProgressBar(value);

        if (value >= 95){
            JNIUtil::HideSplash();
            show_splash = false;
        }
    }
#endif

    const QString bookmarked_path = MathUtil::GetApplicationPath() + "assets/icons/bookmarked.png";
    const QString bookmark_path = MathUtil::GetApplicationPath() + "assets/icons/bookmark.png";

    if (game->GetBookmarkManager()->GetBookmarked(game->GetPlayer()->GetProperties()->GetURL())) {
        if (button_bookmark_state != 1) {
            button_bookmark_state = 1;
            button_bookmark->setIcon(QIcon(bookmarked_path));
#ifndef __ANDROID__
            button_bookmark->setIconSize(QSize(32,32));
#else
            button_bookmark->setIconSize(QSize(92,92));
#endif
            button_bookmark->setToolTip("Remove bookmark");
        }
    }
    else {
        if (button_bookmark_state != 2) {
            button_bookmark_state = 2;
            button_bookmark->setIcon(QIcon(bookmark_path));
#ifndef __ANDROID__
            button_bookmark->setIconSize(QSize(32,32));
#else
            button_bookmark->setIconSize(QSize(92,92));
#endif
            button_bookmark->setToolTip("Add bookmark");
        }
    }

    //grabbing subject to window focus
    const bool active_window = isActiveWindow();
    if (!active_window && glwidget->GetGrab()) {
        glwidget->SetGrab(false);
    }

    //player text entry (URL bar or chat text entry)
    if (social_window && urlbar) {
        game->GetPlayer()->SetTyping(social_window->GetFocusOnChatEntry() || urlbar->hasFocus());
    }

    //Should quit? or not entitled?
    if (hmd_manager && hmd_manager->GetEnabled()) {
        hmd_manager->Platform_ProcessMessages();
        if ((hmd_manager->Platform_GetShouldQuit() || !hmd_manager->Platform_GetEntitled())) {
            game->SetDoExit(true);
        }
    }

    if (social_window->isVisible()) {
        social_window->Update();
    }

#ifdef __ANDROID__
        JNIUtil::SetButtonMargin(width()-social_window_tab->x()-social_window_tab->tabBar()->width());
#endif

#ifndef __ANDROID__
    if (codeeditor_window->isVisible()) {
        codeeditor_window->Update();
    }
    if (navigation_window->isVisible()) {
        navigation_window->Update();
    }
    if (hierarchy_window->isVisible()) {
        hierarchy_window->Update();
    }
    if (properties_window->isVisible()) {
        properties_window->Update();
    }
    if (asset_window->isVisible()) {
        asset_window->Update();
    }
#endif

    //Controllers (Leap Motion)
    UpdateHands();
}

void MainWindow::CEFTimeOut()
{    
#ifndef __ANDROID__  
    if (game && !game->GetDoExit() && SettingsManager::GetUpdateWebsurfaces()) { // && cef_mutex.tryLock()) {
//        qDebug() << "MainWindow::CEFTimeOut() enter";
        CefDoMessageLoopWork();
//        qDebug() << "MainWindow::CEFTimeOut() exit";
//        cef_mutex.unlock();
    }
#endif
}

void MainWindow::TimeOut()
{    
    if (game && game->GetDoExit()) {       
       Closed();
       return;
    }

#ifdef __ANDROID__
    if (JNIUtil::GetMainThreadID() != gettid()) {
        JNIUtil::SetMainThreadID(gettid());
    }

    while (paused && hmd_manager && !JNIUtil::GetShowingVR())
    {
       QThread::sleep(0.5);
    }
#endif

    //59.3 - makeCurrent call not needed/costly
//    glwidget->makeCurrent();

    //Check for changes to antialiasing setting
#ifdef __ANDROID__
    uint32_t sampleCount = (SettingsManager::GetAntialiasingEnabled()) ? 2 : 0;
#else
    uint32_t sampleCount = (SettingsManager::GetAntialiasingEnabled()) ? 4 : 0;
#endif
    RendererInterface::m_pimpl->ConfigureSamples(sampleCount);

    //Check for changes to enhanced depth precision setting
#ifdef __ANDROID__
    RendererInterface::m_pimpl->SetIsUsingEnhancedDepthPrecision(false);
#else
    RendererInterface::m_pimpl->SetIsUsingEnhancedDepthPrecision(SettingsManager::GetEnhancedDepthPrecisionEnabled());
#endif

    Update();    
    glwidget->update();      
}

void MainWindow::Initialize()
{
//    qDebug() << "MainWindow::Initialize()";
    glwidget->makeCurrent();

#ifndef __ANDROID__
    SetupWidgets();
    SetupMenuWidgets();
#else
    if (!SettingsManager::GetShowSplash() && GLWidget::GetDisplayMode() != MODE_GVR && GLWidget::GetDisplayMode() != MODE_GEAR){
        JNIUtil::SetProgressBar(100);
        JNIUtil::HideSplash();
        show_splash = false;
    }
#endif

    game = new Game();

    glwidget->SetGame(game);    

    social_window = new SocialWindow(game);
#ifdef __ANDROID__
    social_window->setStyleSheet("QWidget {font: 24px; color: #FFFFFF; background: #2F363B;}"); //Hover: #3E4D54; Click: #1F2227
#else
    social_window->setStyleSheet("QWidget {color: #FFFFFF; background: #2F363B;}"); //Hover: #3E4D54; Click: #1F2227
#endif

#ifndef __ANDROID__
    splitter->addWidget(social_window);
    social_window->setVisible(false);
#else
    QImage message_icon = QImage("assets:/assets/icons/message_tab_icon.png").scaled(75, 75, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QMatrix r;
    r.rotate(90);
    message_icon = message_icon.transformed(r);

    QPixmap * message_icon_with_ridge = new QPixmap(social_window_tab->iconSize()); // 100x140
    message_icon_with_ridge->fill(QColor(0,0,0,0));
    QPainter * p = new QPainter(message_icon_with_ridge);
    p->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    p->drawImage(QRect(12.5, 50, message_icon.width(), message_icon.height()),
                 message_icon,
                 QRect(0, 0, message_icon.width(), message_icon.height()));
    p->setPen(QPen(QColor(0x59,0x64,0x6B), 3, Qt::SolidLine, Qt::RoundCap));
    p->drawLine(QPoint(5,5),QPoint(95,5));
    p->drawLine(QPoint(5,15),QPoint(95,15));

    social_window_tab->addTab(social_window, QIcon(*message_icon_with_ridge), QString(""));

    //qDebug() << "MainWindow::Initialize() - GetUseVR()" << SettingsManager::GetUseVR();

    url_bar_tab->setFixedSize(2*width()/3 - 64, url_bar_tab->widget(0)->height() + url_bar_tab->tabBar()->height());
    social_window_tab->setFixedSize(width()/3, height());

    url_bar_tab->SetWindowSize(size());
    social_window_tab->SetWindowSize(size());

#ifndef OCULUS_SUBMISSION_BUILD
    url_bar_tab->setVisible(true);
    social_window_tab->setVisible(true);
#endif

#endif

#ifndef __ANDROID__
    codeeditor_window = new CodeEditorWindow(game);
    splitter->addWidget(codeeditor_window);
    codeeditor_window->setVisible(false);
    codeeditor_window->setStyleSheet("QWidget {color: #FFFFFF; background: #2F363B;}"
                                     "QCheckBox::indicator:unchecked {color: #FFFFFF; background: #24292D;}"); //Hover: #3E4D54; Click: #1F2227

    navigation_window = new NavigationWindow(game);
    splitter->addWidget(navigation_window);
    navigation_window->setVisible(false);
    navigation_window->setStyleSheet("QWidget {color: #FFFFFF; background: #2F363B;}"); //Hover: #3E4D54; Click: #1F2227

    asset_window = new AssetWindow(game);
    splitter->addWidget(asset_window);
    asset_window->setVisible(false);
    asset_window->setStyleSheet("QWidget {color: #FFFFFF; background: #2F363B;}"
                                "QTableWidget {color: #FFFFFF; background: #24292D;}"); //Hover: #3E4D54; Click: #1F2227

    hierarchy_window = new HierarchyWindow(game);
    splitter->addWidget(hierarchy_window);
    hierarchy_window->setVisible(false);
    hierarchy_window->setStyleSheet("QWidget {color: #FFFFFF; background: #2F363B;}"
                                    "QTableWidget {color: #FFFFFF; background: #24292D;}"); //Hover: #3E4D54; Click: #1F2227

    properties_window = new PropertiesWindow(game);
    splitter->addWidget(properties_window);
    properties_window->setVisible(false);
    properties_window->setStyleSheet("QWidget {color: #FFFFFF; background: #2F363B;}"
                                     "QLineEdit {color: #FFFFFF; background: #24292D;}"
                                     "QCheckBox::indicator:unchecked {color: #FFFFFF; background: #24292D;}"
                                     "QGroupBox::title {color:#62BD6C;}"); //Hover: #3E4D54; Click: #1F2227
#endif

    settings_window = new SettingsWindow(game);
#ifndef __ANDROID__
    settings_window->setStyleSheet("QWidget {color: #FFFFFF; background: #2F363B;}"
                                   "QCheckBox::indicator:unchecked {color: #FFFFFF; background: #24292D;}"
                                   "QLineEdit {color: #FFFFFF; background: #24292D;}"
                                   "QGroupBox::title {color:#62BD6C;}"); //Hover: #3E4D54; Click: #1F2227
#else
    settings_window->setStyleSheet("QWidget {font: 24px; color: #FFFFFF; background: #2F363B;}"
                                   "QCheckBox::indicator:unchecked {color: #FFFFFF; background: #24292D;}"
                                   "QRadioButton::indicator:unchecked {color: #FFFFFF; background: #24292D;}"
                                   "QLineEdit {color: #FFFFFF; background: #24292D;}"
                                   "QGroupBox::title {color:#62BD6C;}"); //Hover: #3E4D54; Click: #1F2227
#endif

    this->centralWidget()->setAcceptDrops(true);

    //set up anisotropic filtering support
    GLfloat anisotropyMax = 0;

    MathUtil::glFuncs->glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &anisotropyMax);

#ifndef __APPLE__
    if(anisotropyMax >=8)
    {
        //anisotropyMax = 8;
        qDebug() << "MainWindow::InitializeGame() - Anisotropic filtering supported, set to x" << anisotropyMax;
        AssetImage::SetAnisotropicMax(anisotropyMax);
    }   
#endif

    const DisplayMode disp_mode = GLWidget::GetDisplayMode();

    //60.0 - We keep the hmd_manager pointer in MainWindow for Oculus SDK entitlement check, but only pass it to glwidget and others if an HMD is actually in use
    if (disp_mode == MODE_RIFT || disp_mode == MODE_VIVE || disp_mode == MODE_GEAR || disp_mode == MODE_GVR) {
        glwidget->SetHMDManager(hmd_manager);
        game->GetControllerManager()->SetHMDManager(hmd_manager);
    }

    if (disp_mode == MODE_VIVE || disp_mode == MODE_RIFT || disp_mode == MODE_GVR || disp_mode == MODE_GEAR || disp_mode == MODE_CUBE || disp_mode == MODE_EQUI) {
         game->SetMouseDoPitch(false);
    }
    else if (disp_mode == MODE_2D || disp_mode == MODE_SBS || disp_mode == MODE_SBS_REVERSE || disp_mode == MODE_OU3D) {
        game->SetMouseDoPitch(true);
    }   

    game->initializeGL();

    MenuOperations & menu_ops = game->GetMenuOperations();

    switch (disp_mode) {    
    case MODE_VIVE:
        game->GetPlayer()->SetHMDType(hmd_manager->GetHMDString());
        game->GetPlayer()->SetHMDEnabled(true);
        menu_ops.hmd = true;
        break;
    case MODE_RIFT:
        game->GetPlayer()->SetHMDType("rift");
        game->GetPlayer()->SetHMDEnabled(true);
        menu_ops.hmd = true;
        break;
    case MODE_GVR:
        game->GetPlayer()->SetHMDType(hmd_manager->GetHMDType());
        game->GetPlayer()->SetHMDEnabled(true);
        menu_ops.hmd = true;
        break;
    case MODE_GEAR:
        game->GetPlayer()->SetHMDType(hmd_manager->GetHMDType());
        game->GetPlayer()->SetHMDEnabled(true);
        menu_ops.hmd = true;
        break;
    case MODE_SBS:
        game->GetPlayer()->SetHMDType("sbs");
        break;
    case MODE_SBS_REVERSE:
        game->GetPlayer()->SetHMDType("sbs_reverse");
        break;
    case MODE_OU3D:
        game->GetPlayer()->SetHMDType("ou3d");
        break;
    case MODE_CUBE:
        game->GetPlayer()->SetHMDType("cube");
        break;
    default:
        game->GetPlayer()->SetHMDType("2d");
        break;
    }

#ifdef __ANDROID__
    game->GetPlayer()->SetDeviceType("android");
#else
    game->GetPlayer()->SetDeviceType("desktop");
#endif

    qDebug() << "MainWindow::InitializeGame() - HMD/render type:" << game->GetPlayer()->GetHMDType();

    //game->SetPlayerHeight(rift_render.GetPlayerEyeHeight());
    qDebug() << "MainWindow::InitializeGame() - Initializing sound manager...";
    if (SettingsManager::GetDemoModeEnabled()) {
        SoundManager::SetBuiltinSoundsEnabled(SettingsManager::GetDemoModeBuiltinSounds());
    }
    SoundManager::Load(SettingsManager::GetPlaybackDevice(),
                       SettingsManager::GetCaptureDevice());

//    if (rift_render.GetEnabled()) {
//        SoundManager::Play3D(SOUND_RIFT, false, game->GetPlayer()->GetPos(), 1.0f);
//    }
//    else {
//        SoundManager::Play3D(SOUND_NORIFT, false, game->GetPlayer()->GetPos(), 1.0f);
//    }

    //initialize controllers
#if !defined(__APPLE__) && !defined(__ANDROID__)
    GamepadInit();
#endif

//    glwidget->DoGrabMouseStuff();

    if (hmd_manager) {
        glwidget->makeCurrent();
        //hmd_manager->InitializeGL();
        //hmd_manager->ReCentre();

#if defined(__ANDROID__)
        if ((GLWidget::GetDisplayMode() == MODE_GEAR && !require_permissions) || (GLWidget::GetDisplayMode() == MODE_GVR)){
            EnterVR();
        }
#endif
    }

#ifndef __ANDROID__
    connect(&timer2, SIGNAL(timeout()), this, SLOT(CEFTimeOut()), Qt::ConnectionType::QueuedConnection);
#endif
    connect(&timer, SIGNAL(timeout()), this, SLOT(TimeOut()), Qt::ConnectionType::QueuedConnection);

#ifndef __ANDROID__
    timer2.start( 0 );
#endif
    timer.start( 0 );    
}

void MainWindow::UpdateHands()
{
//    qDebug() << "MainWindow::UpdateHands()" << leap_controller.isConnected() << hmd_manager << hmd_manager->GetEnabled();
    QPointer <Player> player = game->GetPlayer();
    player->GetHand(0).is_active = false;
    player->GetHand(1).is_active = false;

    if (game->GetControllerManager()->GetUsingSpatiallyTrackedControllers()) { //spatially tracked controller path
        for (int i=0; i<2; ++i) {
            LeapHand & hand = player->GetHand(i);
            hand.is_active = false;
            if (hmd_manager->GetControllerTracked(i)) {                                                
                hand.is_active = true;          
                hand.basis = hmd_manager->GetControllerTransform(i);
            }
        }
    }
}

void MainWindow::Closed()
{        
    qDebug() << "MainWindow::Closed()";      

    disconnect(&timer, 0, 0, 0);
    disconnect(&timer2, 0, 0, 0);    

    if (social_window) {
        social_window->Shutdown();
    }

    if (game) {
        delete game;
    }

#ifdef __ANDROID__
    JNIUtil::Destroy();
#endif

//    QApplication::closeAllWindows();
    QCoreApplication::quit(); //60.0 - important!  Calling QCoreApplication::quit clears the event loop, so no more events go to CEF

#ifndef __ANDROID__
    //shut down CEF
    CEFWebView::Shutdown();
    qDebug() << "CefShutdown() started";
    CefShutdown();
    qDebug() << "CefShutdown() done";
#endif
}

void MainWindow::SetupWidgets()
{
#ifndef __ANDROID__
    const unsigned int btn_size = 32 * this->devicePixelRatio();
#else
    const unsigned int btn_size = 92;
#endif

    button_back = new QPushButton();
    button_back->setIcon(QIcon(MathUtil::GetApplicationPath() + "assets/icons/back.png"));
    button_back->setIconSize(QSize(btn_size,btn_size));
    button_back->setMaximumWidth(btn_size);
    button_back->setMaximumHeight(btn_size);
    button_back->setToolTip("Back");
    connect(button_back, SIGNAL(clicked(bool)), this, SLOT(ActionBack()));

    button_forward = new QPushButton();
    button_forward->setIcon(QIcon(MathUtil::GetApplicationPath() + "assets/icons/forward.png"));
    button_forward->setIconSize(QSize(btn_size,btn_size));
    button_forward->setMaximumWidth(btn_size);
    button_forward->setMaximumHeight(btn_size);
    button_forward->setToolTip("Forward");
    connect(button_forward, SIGNAL(clicked(bool)), this, SLOT(ActionForward()));

    button_reload = new QPushButton();
    button_reload->setIcon(QIcon(MathUtil::GetApplicationPath() + "assets/icons/reload.png"));
    button_reload->setIconSize(QSize(btn_size,btn_size));
    button_reload->setMaximumWidth(btn_size);
    button_reload->setMaximumHeight(btn_size);
    button_reload->setToolTip("Reload");
    connect(button_reload, SIGNAL(clicked(bool)), this, SLOT(ActionReload()));

    button_home = new QPushButton();
    button_home->setIcon(QIcon(MathUtil::GetApplicationPath() + "assets/icons/home.png"));
    button_home->setIconSize(QSize(btn_size,btn_size));
    button_home->setMaximumWidth(btn_size);
    button_home->setMaximumHeight(btn_size);
    button_home->setToolTip("Go to/from pocketspace");
    connect(button_home, SIGNAL(clicked(bool)), this, SLOT(ActionHome()));

    urlbar = new QLineEdit();
#ifdef __ANDROID__
    urlbar->setFixedHeight(btn_size - 8);
#endif
    urlbar->setStyleSheet("color:#62BD6C; padding-left:16px;");
    connect(urlbar, SIGNAL(returnPressed()), this, SLOT(ActionOpenURL()));

    progressbar = new QProgressBar();
    progressbar->setRange(0, 100);
    progressbar->setValue(0);
#ifndef __ANDROID__
    progressbar->setMaximumHeight(4);
#else
    progressbar->setMaximumHeight(8);
#endif
    progressbar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    progressbar->setTextVisible(false);
    progressbar->setStyleSheet("QProgressBar::chunk {background-color: #62BD6C;}");

    button_bookmark = new QPushButton();
    button_bookmark->setIcon(QIcon(MathUtil::GetApplicationPath() + "assets/icons/bookmark.png"));
    button_bookmark->setIconSize(QSize(btn_size,btn_size));
    button_bookmark->setMaximumWidth(btn_size);
    button_bookmark->setMaximumHeight(btn_size);
    button_bookmark->setToolTip("Bookmark");
    connect(button_bookmark, SIGNAL(clicked(bool)), this, SLOT(ActionBookmark()));

    button_ellipsis = new QPushButton();
    button_ellipsis->setIcon(QIcon(MathUtil::GetApplicationPath() + "assets/icons/menu.png"));
    button_ellipsis->setIconSize(QSize(btn_size,btn_size));
    button_ellipsis->setMaximumWidth(btn_size);
    button_ellipsis->setMaximumHeight(btn_size);
    button_ellipsis->setToolTip("Customize and control Janus VR");
    connect(button_ellipsis, SIGNAL(clicked(bool)), this, SLOT(ActionEllipsisMenu()));

    QWidget * w4 = new QWidget();
    QVBoxLayout * l4 = new QVBoxLayout();
    l4->addWidget(urlbar, 1);
    l4->addWidget(progressbar);
    l4->setSpacing(0);
    l4->setMargin(0);
    w4->setLayout(l4);
#ifndef __ANDROID__
    w4->setMaximumHeight(32);
#else
    w4->setFixedHeight(btn_size);
#endif

    topbarwidget = new QWidget();
    QHBoxLayout * l3 = new QHBoxLayout();
    l3->addWidget(button_back);
    l3->addWidget(button_forward);
    l3->addWidget(button_reload);
    l3->addWidget(button_home);
    l3->addWidget(w4);
    l3->addWidget(button_bookmark);
    l3->addWidget(button_ellipsis);
    l3->setSpacing(0);
    l3->setMargin(0);
    topbarwidget->setLayout(l3);
#ifndef __ANDROID__
    topbarwidget->setMaximumHeight(32);
#else
    topbarwidget->setFixedHeight(btn_size);
#endif
    topbarwidget->setStyleSheet("QWidget {color: #FFFFFF; background: #2F363B;}"
                        "QPushButton:hover, QPushButton:selected {background: #3E4D54;}" //Hover: #3E4D54; Click: #1F2227;
                        "QPushButton::menu-indicator {image: url("");}"); //Hover: #3E4D54; Click: #1F2227;


#ifndef __ANDROID__
    QVBoxLayout * l2 = new QVBoxLayout();
    l2->setSpacing(0);
    l2->setMargin(0);
    //add top panel widget (optionally disabled if demo_enabled is true and demo_ui is false)
    if (!SettingsManager::GetDemoModeEnabled() || SettingsManager::GetDemoModeUI()) {
        l2->addWidget(topbarwidget);
    }
    l2->addWidget(glwidget);
#else
    QWidget * w5 = new QWidget();
    QVBoxLayout * l5 = new QVBoxLayout();
    l5->addWidget(topbarwidget);
    w5->setLayout(l5);
    w5->setFixedHeight(btn_size + 32);
    w5->setStyleSheet("QWidget {color: #FFFFFF; background: #2F363B;}"); //Hover: #3E4D54; Click: #1F2227;

    QImage url_tab_icon = QImage("assets:/assets/icons/url_tab_icon.png").scaled(75, 75, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QPixmap * url_tab_icon_with_ridge = new QPixmap(url_bar_tab->iconSize()); // 100x140
    url_tab_icon_with_ridge->fill(QColor(0,0,0,0));
    QPainter * p = new QPainter(url_tab_icon_with_ridge);
    p->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    p->drawImage(QRect(12.5, 25, url_tab_icon.width(), url_tab_icon.height()),
                 url_tab_icon,
                 QRect(0, 0, url_tab_icon.width(), url_tab_icon.height()));
    p->setPen(QPen(QColor(0x59,0x64,0x6B), 3, Qt::SolidLine, Qt::RoundCap));
    p->drawLine(QPoint(5, url_bar_tab->iconSize().height() - 5),QPoint(95, url_bar_tab->iconSize().height() - 5));
    p->drawLine(QPoint(5, url_bar_tab->iconSize().height() - 15),QPoint(95, url_bar_tab->iconSize().height() - 15));

    url_bar_tab->addTab(w5, QIcon(*url_tab_icon_with_ridge), QString(""));

    QGridLayout * l2 = new QGridLayout();
    l2->addWidget(glwidget, 0, 0);
    l2->addWidget(social_window_tab, 0, 0, Qt::AlignTop | Qt::AlignRight);
    l2->addWidget(url_bar_tab, 0, 0, Qt::AlignTop | Qt::AlignLeft);
    l2->setSpacing(0);
    l2->setMargin(0);
#endif

    splitter = new QSplitter(this);
    splitter->setChildrenCollapsible(false);

    QWidget * w = new QWidget();
    w->setLayout(l2);
    splitter->addWidget(w);

    splitter->setStyleSheet("QWidget {color: #FFFFFF; background: #2F363B;}");
    this->setCentralWidget(splitter);
}

void MainWindow::SetupMenuWidgets()
{
    newAct = new QAction(tr("&New..."), this);
    newAct->setShortcuts(QKeySequence::New);
    newAct->setStatusTip(tr("Create a new workspace"));
    connect(newAct, &QAction::triggered, this, &MainWindow::ActionNew);

    openAct = new QAction(tr("&Open..."), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open file or workspace"));
    connect(openAct, &QAction::triggered, this, &MainWindow::ActionOpen);

    saveAct = new QAction(tr("&Save"), this);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("Save current workspace"));
    connect(saveAct, &QAction::triggered, this, &MainWindow::ActionSave);

    saveAsAct = new QAction(tr("Save &As..."), this);
    saveAsAct->setStatusTip(tr("Save current workspace as"));
    connect(saveAsAct, &QAction::triggered, this, &MainWindow::ActionSaveAs);

    importLocalAct = new QAction(tr("&Import (Local file)..."), this);
    importLocalAct->setStatusTip(tr("Import JML code from a local HTML file"));
    connect(importLocalAct, &QAction::triggered, this, &MainWindow::ActionImportLocal);

    importRemoteAct = new QAction(tr("Import (&Remote URL)..."), this);
    importRemoteAct->setStatusTip(tr("Import JML code from a remote HTML file"));
    connect(importRemoteAct, &QAction::triggered, this, &MainWindow::ActionImportRemote);

    saveThumbAct = new QAction(tr("Save Thumbnail"), this);
    saveThumbAct->setShortcut(QKeySequence(Qt::SHIFT+Qt::Key_F7));
    saveThumbAct->setStatusTip(tr("Save thumbnail image"));
    connect(saveThumbAct, &QAction::triggered, this, &MainWindow::ActionSaveThumb);

#ifndef __ANDROID__
    saveScreenshotAct = new QAction(tr("Save Screenshot"), this);
    saveScreenshotAct->setShortcut(QKeySequence(Qt::SHIFT+Qt::Key_F8));
#else
    saveScreenshotAct = new QAction(tr("Screenshot"), this);
#endif
    saveScreenshotAct->setStatusTip(tr("Save screenshot"));
    connect(saveScreenshotAct, &QAction::triggered, this, &MainWindow::ActionSaveScreenshot);

#ifndef __ANDROID__
    saveEquiAct = new QAction(tr("Save Equirectangular"), this);
    saveEquiAct->setShortcut(QKeySequence(Qt::SHIFT+Qt::CTRL + Qt::Key_F8));
    saveEquiAct->setStatusTip(tr("Save equirectangular image"));
#else
    saveEquiAct = new QAction(tr("Equi Sceenshot"), this);
    saveEquiAct->setStatusTip(tr("Save equi images"));
#endif
    connect(saveEquiAct, &QAction::triggered, this, &MainWindow::ActionSaveEqui);

    exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip(tr("Exit Janus VR"));
    connect(exitAct, &QAction::triggered, this, &MainWindow::ActionExit);

    startRecordingAct = new QAction(tr("Start Recording"), this);
    startRecordingAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_G));
    startRecordingAct->setStatusTip(tr("Start Recording"));
    connect(startRecordingAct, &QAction::triggered, this, &MainWindow::ActionStartRecording);

    startRecordingEveryoneAct = new QAction(tr("Start Recording (All users)"), this);
    startRecordingEveryoneAct->setStatusTip(tr("Start Recording (All users)"));
    connect(startRecordingEveryoneAct, &QAction::triggered, this, &MainWindow::ActionStartRecordingEveryone);

    stopRecordingAct = new QAction(tr("Stop Recording"), this);
    stopRecordingAct->setStatusTip(tr("Stop Recording"));
    connect(stopRecordingAct, &QAction::triggered, this, &MainWindow::ActionStopRecording);

    syncToAct = new QAction(tr("Sync to"), this);
    syncToAct->setStatusTip(tr("Sync to"));
    connect(syncToAct, &QAction::triggered, this, &MainWindow::ActionSyncTo);

    settingsAct = new QAction(tr("&Settings..."), this);    
    settingsAct->setStatusTip(tr("Settings..."));
    connect(settingsAct, &QAction::triggered, this, &MainWindow::ActionSettings);

    toggleFullscreenAct = new QAction(tr("Fullscreen"), this);
    toggleFullscreenAct->setStatusTip(tr("Toggle Fullscreen Mode"));
    connect(toggleFullscreenAct, &QAction::triggered, this, &MainWindow::ActionToggleFullscreen);

    socialAct = new QAction(tr("Social"), this);
    socialAct->setCheckable(true);
    socialAct->setChecked(false);
    socialAct->setStatusTip(tr("Show social pane"));
    connect(socialAct, &QAction::triggered, this, &MainWindow::ActionSocial);

#ifndef __ANDROID__
    codeEditorAct = new QAction(tr("Code Editor"), this);
    codeEditorAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_U));
    codeEditorAct->setCheckable(true);
    codeEditorAct->setChecked(false);
    codeEditorAct->setStatusTip(tr("Show code editor"));
    connect(codeEditorAct, &QAction::triggered, this, &MainWindow::ActionCodeEditor);

    navigationAct = new QAction(tr("Navigation"), this);
    navigationAct->setCheckable(true);
    navigationAct->setChecked(false);
    navigationAct->setStatusTip(tr("Show navigation pane"));
    connect(navigationAct, &QAction::triggered, this, &MainWindow::ActionNavigation);

    assetAct = new QAction(tr("Assets"), this);
    assetAct->setCheckable(true);
    assetAct->setChecked(false);
    assetAct->setStatusTip(tr("Show/hide assets"));
    connect(assetAct, &QAction::triggered, this, &MainWindow::ActionAssets);

    hierarchyAct = new QAction(tr("Room Objects"), this);
    hierarchyAct->setCheckable(true);
    hierarchyAct->setChecked(false);
    hierarchyAct->setStatusTip(tr("Show/hide hierarchy"));
    connect(hierarchyAct, &QAction::triggered, this, &MainWindow::ActionHierarchy);

    propertiesAct = new QAction(tr("Properties"), this);
    propertiesAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_P));
    propertiesAct->setCheckable(true);
    propertiesAct->setChecked(false);
    propertiesAct->setStatusTip(tr("Show/hide properties"));
    connect(propertiesAct, &QAction::triggered, this, &MainWindow::ActionProperties);
#endif

    addBookmarkAct = new QAction("Add Bookmark", this);
    connect(addBookmarkAct, SIGNAL(triggered(bool)), this, SLOT(ActionAddBookmark()));

    removeBookmarkAct = new QAction("Remove Bookmark", this);
    connect(removeBookmarkAct, SIGNAL(triggered(bool)), this, SLOT(ActionRemoveBookmark()));

#ifdef __ANDROID__
    togglePerfAct = new QAction(tr("Toggle performance info"), this);
    togglePerfAct->setStatusTip(tr("Toggle performance info"));
    connect(togglePerfAct, &QAction::triggered, this, &MainWindow::ActionTogglePerf);

    enterVRAct = new QAction(tr("Enter VR"), this);
    enterVRAct->setStatusTip(tr("Enter VR"));
    connect(enterVRAct, &QAction::triggered, this, &MainWindow::ActionEnterVR);
#endif

    bookmarkMenu = new QMenu("Bookmarks", this);
    connect(bookmarkMenu, SIGNAL(aboutToShow()), this, SLOT(ActionOpenBookmarks()));
    connect(bookmarkMenu, SIGNAL(triggered(QAction*)), this, SLOT(ActionOpenURL(QAction *)));

    fileMenu = new QMenu("File", this);
#ifndef __ANDROID__
    fileMenu->addAction(newAct);
    fileMenu->addAction(openAct);
    fileMenu->addAction(saveAct);
    fileMenu->addAction(saveAsAct);
    fileMenu->addSeparator();
    fileMenu->addAction(importLocalAct);
    fileMenu->addAction(importRemoteAct);
    fileMenu->addSeparator();
    fileMenu->addAction(saveThumbAct);
#else
    fileMenu->addAction(saveScreenshotAct);
    fileMenu->addAction(saveEquiAct);
#endif

#ifndef __ANDROID__
    windowMenu = new QMenu("Window", this);
    windowMenu->addAction(socialAct);
    windowMenu->addAction(navigationAct);
    windowMenu->addSeparator();
    windowMenu->addAction(assetAct);
    windowMenu->addAction(hierarchyAct);
    windowMenu->addAction(propertiesAct);
    windowMenu->addSeparator();
    windowMenu->addAction(codeEditorAct);
    windowMenu->addSeparator();
#ifndef __ANDROID__
    windowMenu->addSeparator();
    windowMenu->addAction(toggleFullscreenAct);
#endif
    connect(windowMenu, SIGNAL(aboutToShow()), this, SLOT(ActionOpenWindow()));

    usersMenu = new QMenu("Users", this);
    usersMenu->addAction(startRecordingAct);
    usersMenu->addAction(startRecordingEveryoneAct);
    usersMenu->addAction(stopRecordingAct);
    usersMenu->addSeparator();
    usersMenu->addAction(syncToAct);
    connect(usersMenu, SIGNAL(aboutToShow()), this, SLOT(ActionOpenEdit()));
#endif

    ellipsisMenu = new QMenu(this);
    ellipsisMenu->addMenu(fileMenu);

#ifndef __ANDROID__
    ellipsisMenu->addMenu(windowMenu);
    ellipsisMenu->addMenu(usersMenu);
#endif

    ellipsisMenu->addMenu(bookmarkMenu);
    ellipsisMenu->addSeparator();
#ifdef __ANDROID__
    ellipsisMenu->addSeparator();
    //ellipsisMenu->addAction(togglePerfAct);
    ellipsisMenu->addAction(enterVRAct);
#endif
    ellipsisMenu->addSeparator();
    ellipsisMenu->addAction(settingsAct);
#ifndef __ANDROID__
    ellipsisMenu->addAction(exitAct);
#endif

    button_ellipsis->setMenu(ellipsisMenu);
}

QString MainWindow::GetNewWorkspaceDirectory()
{
    QDir d;
    int val = 0;
    while (true) {
        ++val;
        //check that workspace directory does not exist
        d.setPath(MathUtil::GetWorkspacePath() + "workspace" +
               QString("%1").arg(val, 3, 10, QChar('0')));
        if (!d.exists()) {
            break;
        }
    }
    return d.path();
}

void MainWindow::ActionNew()
{
    game->CreateNewWorkspace(GetNewWorkspaceDirectory());
}

void MainWindow::ActionOpen()
{
    QString filename = QFileDialog::getOpenFileName(this, "Open...", MathUtil::GetWorkspacePath(), tr("HTML (*.html)"));
    if (!filename.isNull()) {
        QString local_file = QUrl::fromLocalFile(filename).toString();
//        qDebug() << QUrl(filename).isLocalFile() << filename << local_file;
        game->CreatePortal(local_file, false);
    }
}

void MainWindow::ActionSave()
{
    //game->SaveRoom(game->getEnvironment()->GetPlayerRoom()->GetSaveFilename());
#ifndef __ANDROID__
    codeeditor_window->SlotSaveChanges();
#endif
}

void MainWindow::ActionSaveAs()
{
    QString filename = QFileDialog::getSaveFileName(this, "Save As...", MathUtil::GetWorkspacePath(), tr("HTML (*.html);; JSON (*.json)"));
    if (!filename.isNull()) {
        game->SaveRoom(filename);
    }
}

void MainWindow::ActionImportLocal()
{
    QString filename = QFileDialog::getOpenFileName(this, "Import (local file)...", MathUtil::GetWorkspacePath(), tr("HTML (*.html)"));
    if (!filename.isNull()) {
        QString local_file = QUrl::fromLocalFile(filename).toString();
        game->DoImport(local_file);
    }
}

void MainWindow::ActionImportRemote()
{
    bool ok;
    QString url = QInputDialog::getText(this, tr("Import (Remote URL)"),
                                         tr("Specify URL of HTML file (local or remote) containing JML code:"),
                                         QLineEdit::Normal,
                                         "http://www.janusvr.com", &ok);
    if (ok && !url.isEmpty()) {
        game->DoImport(url);
    }
}

void MainWindow::ActionSaveThumb()
{
    QString filename = QFileDialog::getSaveFileName(this, "Save Thumbnail As...", MathUtil::GetScreenshotPath(), tr("JPEG (*.jpg)"));
    if (!filename.isNull()) {
        glwidget->DoSaveThumb(filename);
    }
}

void MainWindow::ActionSaveScreenshot()
{
#ifndef __ANDROID__
    QString filename = QFileDialog::getSaveFileName(this, "Save Screenshot As...", MathUtil::GetScreenshotPath(), tr("JPEG (*.jpg)"));
#else
    QString filename = MathUtil::GetScreenshotPath() + "out-" + MathUtil::GetCurrentDateTimeAsString() + ".jpg";
#endif

    if (!filename.isNull()) {
        glwidget->DoSaveScreenshot(filename);
    }
}

void MainWindow::ActionSaveEqui()
{
#ifndef __ANDROID__
    QString filename = QFileDialog::getSaveFileName(this, "Save Equirectangular As...", MathUtil::GetScreenshotPath(), tr("JPEG (*.jpg)"));
#else
    QString filename = MathUtil::GetScreenshotPath() + "out-" + MathUtil::GetCurrentDateTimeAsString() + ".jpg";
#endif

    if (!filename.isNull()) {
        glwidget->DoSaveEqui(filename);
    }
}

void MainWindow::ActionExit()
{
    game->SetDoExit(true);
}

void MainWindow::ActionBack()
{
    game->StartResetPlayer();
}

void MainWindow::ActionForward()
{
    game->StartResetPlayerForward();
}

void MainWindow::ActionReload()
{
    game->GetEnvironment()->ReloadRoom();
}

void MainWindow::ActionHome()
{
    game->StartEscapeToPocketspace();
}

void MainWindow::ActionBookmark()
{
    const QString url = game->GetPlayer()->GetProperties()->GetURL();
    const bool url_bookmarked = game->GetBookmarkManager()->GetBookmarked(url);
    url_bookmarked ? ActionRemoveBookmark() : ActionAddBookmark();
}

void MainWindow::ActionOpenURL()
{
    DoOpenURL(urlbar->text());
}

void MainWindow::ActionOpenURL(QAction * a)
{
    if (a && a != addBookmarkAct && a != removeBookmarkAct) {        
        DoOpenURL(a->text());
    }
}

void MainWindow::DoOpenURL(const QString url)
{    
    //59.0 - path might be copy-pasted from e.g. windows explorer
    //as a local (non URL) path, correct accordingly
    QString s = url;
    QFileInfo f(QUrl(url).toLocalFile());
    const bool local_file = f.exists();
    if (local_file) {
        //s = QUrl::fromLocalFile(s).toString();
    }
    else if (s.toLower() == "home") {
        s = SettingsManager::GetLaunchURL();
    }
    else if (s.toLower() == "bookmarks" || s.toLower() == "workspaces") {

    }
    else if (s.left(4).toLower() != "http") {
        s = "http://" + s;
    }

//    qDebug() << "MainWindow::DoOpenURL" << url << s;
    game->CreatePortal(s, !local_file);
    glwidget->SetGrab(true);
}

void MainWindow::ActionSocial()
{
    social_window->setVisible(socialAct->isChecked());
}

#ifndef __ANDROID__
void MainWindow::ActionCodeEditor()
{
    codeeditor_window->setVisible(codeEditorAct->isChecked());
}

void MainWindow::ActionNavigation()
{
    navigation_window->setVisible(navigationAct->isChecked());
}

void MainWindow::ActionAssets()
{
    asset_window->setVisible(assetAct->isChecked());
}

void MainWindow::ActionHierarchy()
{
    hierarchy_window->setVisible(hierarchyAct->isChecked());
}

void MainWindow::ActionProperties()
{
    properties_window->setVisible(propertiesAct->isChecked());
}
#endif

void MainWindow::ActionOpenEdit()
{
    const bool rec = game->GetRecording();
    if (rec) {
        startRecordingAct->setShortcut(QKeySequence());
        stopRecordingAct->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_G));
    }
    else {
        startRecordingAct->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_G));
        stopRecordingAct->setShortcut(QKeySequence());
    }
    startRecordingAct->setEnabled(!rec);
    startRecordingEveryoneAct->setEnabled(!rec);
    stopRecordingAct->setEnabled(rec);
}

void MainWindow::ActionStartRecording()
{
    game->StartRecording(false);
    ActionOpenEdit();
}

void MainWindow::ActionStartRecordingEveryone()
{
    game->StartRecording(true);
    ActionOpenEdit();
}

void MainWindow::ActionStopRecording()
{
    game->StopRecording();
    ActionOpenEdit();
}

void MainWindow::ActionSyncTo()
{
    game->GetEnvironment()->GetCurRoom()->SyncAll();
}

void MainWindow::ActionOpenWindow()
{
    socialAct->setChecked(social_window->isVisible());
#ifndef __ANDROID__
    codeEditorAct->setChecked(codeeditor_window->isVisible());
    hierarchyAct->setChecked(hierarchy_window->isVisible());
    propertiesAct->setChecked(properties_window->isVisible());
    assetAct->setChecked(asset_window->isVisible());
    navigationAct->setChecked(navigation_window->isVisible());
#endif
}

void MainWindow::ActionAddBookmark()
{
    glwidget->DoBookmark();
}

void MainWindow::ActionRemoveBookmark()
{
    glwidget->DoBookmark();
}

void MainWindow::ActionOpenBookmarks()
{
    const QString url = game->GetPlayer()->GetProperties()->GetURL();
    const bool url_bookmarked = game->GetBookmarkManager()->GetBookmarked(url);
    if (url_bookmarked) {
        addBookmarkAct->setShortcut(QKeySequence());
#ifndef __ANDROID__
        removeBookmarkAct->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_B));
#endif
    }
    else {
#ifndef __ANDROID__
        addBookmarkAct->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_B));
#endif
        removeBookmarkAct->setShortcut(QKeySequence());
    }
    addBookmarkAct->setEnabled(!url_bookmarked);
    removeBookmarkAct->setEnabled(url_bookmarked);

    bookmarkMenu->clear(); //existing actions are deleted!
    bookmarkMenu->addAction(addBookmarkAct);
    bookmarkMenu->addAction(removeBookmarkAct);
    bookmarkMenu->addSeparator();

    QVariantList list = game->GetBookmarkManager()->GetBookmarks();
    for (int i=0; i<list.size(); ++i) {
        QMap <QString, QVariant> o = list[i].toMap();
        QString url = o["url"].toString();
#ifdef __ANDROID__
        QString thumbnail = "";
#else
        QString thumbnail = o["thumbnail"].toString();
#endif

        QAction * a = new QAction(QIcon(QUrl(thumbnail).toLocalFile()), url, bookmarkMenu);
//        connect(a, SIGNAL(triggered(bool)), this, SLOT(ActionOpenURL(QString)));
        bookmarkMenu->addAction(a);
    }
    bookmarkMenu->addSeparator();
    list = game->GetBookmarkManager()->GetWorkspaces();
    for (int i=0; i<list.size(); ++i) {
        QMap <QString, QVariant> o = list[i].toMap();
        QString url = o["url"].toString();
#ifdef __ANDROID__
        QString thumbnail = "";
#else
        QString thumbnail = o["thumbnail"].toString();
#endif

        QAction * a = new QAction(QIcon(QUrl(thumbnail).toLocalFile()), QUrl::fromLocalFile(url).toString(), bookmarkMenu);
//        connect(a, SIGNAL(triggered(bool)), this, SLOT(ActionOpenURL(QString)));
        bookmarkMenu->addAction(a);
    }

}

void MainWindow::ActionSettings()
{
#ifdef __ANDROID__
    settings_window->setFixedSize(width()/2, height()/2);
    settings_window->move(width()/4, height()/4);
#endif
    if (fullscreened) {
        ActionToggleFullscreen();
    }
    settings_window->setVisible(true);
    settings_window->Update();
}

void MainWindow::ActionToggleFullscreen()
{
    QScreen * s = QApplication::primaryScreen();
    if (s == NULL) {
        return;
    }

    const QRect r = s->geometry();
    if (fullscreened) {
        setWindowFlags( default_window_flags );
        show();
//        setWindowFlags( Qt::CustomizeWindowHint );
        setGeometry(r.left()+100, r.top()+100, r.width()-200, r.height()-200);
    }
    else {
        default_window_flags = windowFlags();
        setWindowFlags( Qt::CustomizeWindowHint); // | Qt::FramelessWindowHint); // | Qt::WindowStaysOnTopHint);
        show();
        //setGeometry(r.left()+1, r.top(), r.width()-1, r.height());
        setGeometry(s->availableGeometry());
    }
    fullscreened = !fullscreened;
}

void MainWindow::ActionEllipsisMenu()
{
    ellipsisMenu->show();
}

#ifdef __ANDROID__
void MainWindow::ActionTogglePerf()
{
    QKeyEvent e((QEvent::Type)6, Qt::Key_Slash, 0);
    game->keyPressEvent(&e);
}

void MainWindow::ActionEnterVR()
{
    EnterVR();
}

void MainWindow::EnterVR()
{
    //qDebug() << "MainWindow::ENTERINGVR";
    if (hmd_manager)
    {
        if (settings_window->isVisible()){
            SettingsManager::SaveSettings(); //Save settings upon hiding settings window
            settings_window->setVisible(false);
        }

        //glwidget->makeCurrent();
        glwidget->SetHMDManager(hmd_manager);
        game->GetControllerManager()->SetHMDManager(hmd_manager);

        if (hmd_manager->GetHMDType() == "daydream" || hmd_manager->GetHMDType() == "cardboard")
        {
            GLWidget::SetDisplayMode(MODE_GVR);
        }
        else
        {
            GLWidget::SetDisplayMode(MODE_GEAR);
        }
        game->GetPlayer()->SetHMDType(hmd_manager->GetHMDType());

        game->SetMouseDoPitch(false);
        game->GetMenuOperations().hmd = true;

        glwidget->SetupFramebuffer();

        hmd_manager->EnterVR();
    }
}

void MainWindow::ExitVR()
{
   //qDebug() << "MainWindow::EXITINGVR";
    if (hmd_manager && hmd_manager->GetEnabled() && !asked_permissions)
    {
        //glwidget->makeCurrent();
        hmd_manager->ExitVR();
        GLWidget::SetDisplayMode(MODE_2D);

        game->SetMouseDoPitch(true);
        game->GetPlayer()->SetHMDType("2d");
        game->GetMenuOperations().hmd = false;

        glwidget->SetupFramebuffer();

        JNIUtil::SetButtonMargin(width()-social_window_tab->x()-social_window_tab->tabBar()->width());
    }
}

void MainWindow::Pause()
{
    //Pause room
    if (!asked_permissions) {
        QPointer <RoomPhysics> phys = game->GetEnvironment()->GetCurRoom()->GetPhysics();
        if (phys) {
            phys->SetPlayerGravity(0.0f);
        }

        RendererInterface::m_pimpl->Pause();
        game->GetEnvironment()->GetCurRoom()->Pause();
        paused = true;
    }
}

void MainWindow::Resume()
{
    //Resume room
    paused = false;
    game->GetEnvironment()->GetCurRoom()->Resume();
    RendererInterface::m_pimpl->Resume();

    QPointer <RoomPhysics> phys = game->GetEnvironment()->GetCurRoom()->GetPhysics();
    if (phys) {
        phys->SetPlayerGravity(game->GetEnvironment()->GetCurRoom()->GetProperties()->GetGravity());
    }

    //game->GetEnvironment()->ReloadRoom();
}
#endif
