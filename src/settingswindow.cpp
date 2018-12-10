#include "settingswindow.h"

SettingsWindow::SettingsWindow(Game * g) :
    game(g)
{    
    setWindowTitle("Settings"); 

    tab_widget = new QTabWidget();
    tab_widget->addTab(GetAvatarWidget(), "Avatar");
    tab_widget->addTab(GetVRWidget(), "VR");
    tab_widget->addTab(GetInterfaceWidget(), "Interface");
    tab_widget->addTab(GetAudioWidget(), "Audio");
    tab_widget->addTab(GetNetworkWidget(), "Network");
    tab_widget->addTab(GetDeveloperWidget(), "Developer");
    tab_widget->setUsesScrollButtons(false); //62.0 - no scrolling, window should be large enough to show all tabs directly

    connect(tab_widget, SIGNAL(currentChanged(int)), this, SLOT(Update()));    

    setCentralWidget(tab_widget);    
    Update();   
}

QWidget * SettingsWindow::GetAudioWidget()
{
    checkbox_micvoiceactivated = new QCheckBox();
    checkbox_micvoiceactivated->setText("Microphone Voice-Activated");
    connect(checkbox_micvoiceactivated, SIGNAL(clicked(bool)), this, SLOT(SlotSetMicVoiceActivated()));

    label_microphonesensitivity = new QLabel("Microphone Sensitivity");

    slider_microphonesensitivity = new QSlider();
    slider_microphonesensitivity->setRange(-100, 0);
    slider_microphonesensitivity->setValue(-20);
    slider_microphonesensitivity->setOrientation(Qt::Horizontal);
    connect(slider_microphonesensitivity, SIGNAL(valueChanged(int)), this, SLOT(SlotSetMicSensitivity()));

    slider_volumemic = new QSlider();
    slider_volumemic->setRange(0,100);
    slider_volumemic->setValue(50);
    slider_volumemic->setOrientation(Qt::Horizontal);
    connect(slider_volumemic, SIGNAL(valueChanged(int)), this, SLOT(SlotSetVolumeMic()));

    slider_volumeenv = new QSlider();
    slider_volumeenv->setRange(0,100);
    slider_volumeenv->setValue(50);
    slider_volumeenv->setOrientation(Qt::Horizontal);
    connect(slider_volumeenv, SIGNAL(valueChanged(int)), this, SLOT(SlotSetVolumeEnv()));

    checkbox_positionalenv = new QCheckBox();
    checkbox_positionalenv->setText("Positional Audio (Environment)");
    connect(checkbox_positionalenv, SIGNAL(clicked(bool)), this, SLOT(SlotSetPositionalEnv()));

    slider_volumevoip = new QSlider();
    slider_volumevoip->setRange(0,100);
    slider_volumevoip->setValue(50);
    slider_volumevoip->setOrientation(Qt::Horizontal);
    connect(slider_volumevoip, SIGNAL(valueChanged(int)), this, SLOT(SlotSetVolumeVoip()));

    checkbox_positionalvoip = new QCheckBox();
    checkbox_positionalvoip->setText("Positional Audio (Voice)");
    connect(checkbox_positionalvoip, SIGNAL(clicked(bool)), this, SLOT(SlotSetPositionalVoip()));

    QFormLayout * audio_layout = new QFormLayout();
#ifndef __ANDROID__
    audio_layout->addRow(checkbox_micvoiceactivated);
#endif
    audio_layout->addRow(label_microphonesensitivity, slider_microphonesensitivity);
    audio_layout->addRow(new QLabel("Volume (Microphone)"), slider_volumemic);
    audio_layout->addRow(new QLabel("Volume (Environment)"), slider_volumeenv);
#ifndef __ANDROID__
    audio_layout->addRow(checkbox_positionalenv);
#endif
    audio_layout->addRow(new QLabel("Volume (Voice)"), slider_volumevoip);
#ifndef __ANDROID__
    audio_layout->addRow(checkbox_positionalvoip);
#endif

    QWidget * w = new QWidget();
    w->setLayout(audio_layout);
    w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    return w;
}

QWidget * SettingsWindow::GetAvatarWidget()
{
    lineedit_userid = new QLineEdit();
    connect(lineedit_userid, SIGNAL(returnPressed()), this, SLOT(SlotSetUserID()));

    lineedit_customportalshader = new QLineEdit();
    connect(lineedit_customportalshader, SIGNAL(returnPressed()), this, SLOT(SlotSetCustomPortalShader()));

    checkbox_selfavatar = new QCheckBox();
    checkbox_selfavatar->setText("Show Self Avatar");
    connect(checkbox_selfavatar, SIGNAL(clicked(bool)), this, SLOT(SlotSetSelfAvatar()));

    button_resetavatar = new QPushButton();
    button_resetavatar->setText("Reset Avatar");
    connect(button_resetavatar, SIGNAL(clicked(bool)), this, SLOT(SlotResetAvatar()));

    QFormLayout * avatar_layout = new QFormLayout();
    avatar_layout->addRow("userid", lineedit_userid);
#ifndef __ANDROID__
    avatar_layout->addRow("Custom Portal Shader", lineedit_customportalshader);
    avatar_layout->addRow(checkbox_selfavatar);
#endif
    avatar_layout->addRow(button_resetavatar);

    QWidget * w = new QWidget();
    w->setLayout(avatar_layout);
    w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    return w;
}

QWidget * SettingsWindow::GetInterfaceWidget()
{
    checkbox_crosshair = new QCheckBox();
    checkbox_crosshair->setText("Crosshair");
    connect(checkbox_crosshair, SIGNAL(clicked(bool)), this, SLOT(SlotSetCrosshair()));

    checkbox_antialiasing = new QCheckBox();
    checkbox_antialiasing->setText("Antialiasing");
    connect(checkbox_antialiasing, SIGNAL(clicked(bool)), this, SLOT(SlotSetAntialiasing()));

    checkbox_enhanceddepthprecision = new QCheckBox();
    checkbox_enhanceddepthprecision->setText("Enhanced Depth Precision");
    connect(checkbox_enhanceddepthprecision, SIGNAL(clicked(bool)), this, SLOT(SlotSetEnhancedDepthPrecision()));

    checkbox_gamepad = new QCheckBox();
    checkbox_gamepad->setText("Use Gamepad");
    connect(checkbox_gamepad, SIGNAL(clicked(bool)), this, SLOT(SlotSetGamepad()));

    checkbox_invertpitch = new QCheckBox();
    checkbox_invertpitch->setText("Invert Pitch");
    connect(checkbox_invertpitch, SIGNAL(clicked(bool)), this, SLOT(SlotSetInvertPitch()));

#ifdef __ANDROID__
    checkbox_invertyaw = new QCheckBox();
    checkbox_invertyaw->setText("Invert Yaw");
    connect(checkbox_invertyaw, SIGNAL(clicked(bool)), this, SLOT(SlotSetInvertYaw()));

    checkbox_usegyro = new QCheckBox();
    checkbox_usegyro->setText("Use Gyroscope");
    connect(checkbox_usegyro, SIGNAL(clicked(bool)), this, SLOT(SlotSetUseGyro()));

    checkbox_showsplash = new QCheckBox();
    checkbox_showsplash->setText("Show Splash Screen on Launch");
    connect(checkbox_showsplash, SIGNAL(clicked(bool)), this, SLOT(SlotSetShowSplash()));

    checkbox_showloadingicon = new QCheckBox();
    checkbox_showloadingicon->setText("Show Loading Icon For Current Room");
    connect(checkbox_showloadingicon, SIGNAL(clicked(bool)), this, SLOT(SlotSetShowLoadingIcon()));

    checkbox_showviewjoystick = new QCheckBox();
    checkbox_showviewjoystick->setText("Show View Joystick");
    connect(checkbox_showviewjoystick, SIGNAL(clicked(bool)), this, SLOT(SlotSetShowViewJoystick()));
#endif

    checkbox_renderportalrooms = new QCheckBox();
    checkbox_renderportalrooms->setText("Render Rooms through Portals");
    connect(checkbox_renderportalrooms, SIGNAL(clicked(bool)), this, SLOT(SlotSetRenderPortalRooms()));

    slider_fov = new QSlider();
    slider_fov->setRange(50,120);
    slider_fov->setValue(70);
    slider_fov->setOrientation(Qt::Horizontal);
    connect(slider_fov, SIGNAL(valueChanged(int)), this, SLOT(SlotSetFOV()));

    QFormLayout * interface_layout = new QFormLayout();
    interface_layout->addRow(checkbox_crosshair);
#ifndef __ANDROID__
    interface_layout->addRow(checkbox_antialiasing);
    interface_layout->addRow(checkbox_enhanceddepthprecision);
#endif
    interface_layout->addRow(checkbox_invertpitch);
    interface_layout->addRow(checkbox_renderportalrooms);
#ifndef __ANDROID__
    interface_layout->addRow(checkbox_gamepad);
    interface_layout->addRow(new QLabel("FOV"), slider_fov);
#else
    interface_layout->addRow(checkbox_invertyaw);
    interface_layout->addRow(checkbox_showviewjoystick);
    interface_layout->addRow(checkbox_usegyro);
    interface_layout->addRow(checkbox_showsplash);
    interface_layout->addRow(checkbox_showloadingicon);
#endif

    QWidget * w = new QWidget();
    w->setLayout(interface_layout);
    w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    return w;
}

QWidget * SettingsWindow::GetNetworkWidget()
{
    checkbox_partymode = new QCheckBox();
    checkbox_partymode->setText("Party Mode");
    connect(checkbox_partymode, SIGNAL(clicked(bool)), this, SLOT(SlotSetPartyMode()));  

    checkbox_multiplayer = new QCheckBox();
    checkbox_multiplayer->setText("Multiplayer");
    connect(checkbox_multiplayer, SIGNAL(clicked(bool)), this, SLOT(SlotSetMultiplayer()));

    lineedit_homeurl = new QLineEdit();
    connect(lineedit_homeurl, SIGNAL(editingFinished()), this, SLOT(SlotSetHomeURL()));

    lineedit_websurfaceurl = new QLineEdit();
    connect(lineedit_websurfaceurl, SIGNAL(editingFinished()), this, SLOT(SlotSetWebsurfaceURL()));

    QFormLayout * network_layout = new QFormLayout();
#ifndef __ANDROID__
    network_layout->addRow(checkbox_partymode);    
#endif    
    network_layout->addRow(checkbox_multiplayer);
    network_layout->addRow("Home URL", lineedit_homeurl);
    network_layout->addRow("Websurface URL", lineedit_websurfaceurl);

    QWidget * w = new QWidget();
    w->setLayout(network_layout);
    w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    return w;
}

QWidget * SettingsWindow::GetVRWidget()
{
    checkbox_comfortmode = new QCheckBox();
    checkbox_comfortmode->setText("Comfort Mode");
    connect(checkbox_comfortmode, SIGNAL(clicked(bool)), this, SLOT(SlotSetComfortMode()));

    checkbox_haptics = new QCheckBox();
    checkbox_haptics->setText("Haptics (Vibration)");
    connect(checkbox_haptics, SIGNAL(clicked(bool)), this, SLOT(SlotSetHaptics()));

    checkbox_vivetrackpadmovement = new QCheckBox();
    checkbox_vivetrackpadmovement->setText("Trackpad Movement");
    connect(checkbox_vivetrackpadmovement, SIGNAL(clicked(bool)), this, SLOT(SlotSetViveTrackpadMovement()));

#ifdef __ANDROID__
    checkbox_usevr = new QCheckBox();
    checkbox_usevr->setText("Launch in VR");
    connect(checkbox_usevr, SIGNAL(clicked(bool)), this, SLOT(SlotSetLaunchInVR()));
#endif

    QFormLayout * vr_layout = new QFormLayout();
#ifndef __ANDROID__
    vr_layout->addRow(checkbox_comfortmode);
    vr_layout->addRow(checkbox_haptics);
    vr_layout->addRow(checkbox_vivetrackpadmovement);
#else
    vr_layout->addRow(checkbox_usevr);
#endif

    QWidget * w = new QWidget();
    w->setLayout(vr_layout);
    w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    return w;
}

QWidget * SettingsWindow::GetDeveloperWidget()
{
    checkbox_downloadcache = new QCheckBox();
    checkbox_downloadcache->setText("Download Cache");
    connect(checkbox_downloadcache, SIGNAL(clicked(bool)), this, SLOT(SlotSetDownloadCache()));

    checkbox_editmode = new QCheckBox();
    checkbox_editmode->setText("Edit Mode");
    connect(checkbox_editmode, SIGNAL(clicked(bool)), this, SLOT(SlotSetEditMode()));

    checkbox_editmodeicons = new QCheckBox();
    checkbox_editmodeicons->setText("Show Edit Mode Icons");
    connect(checkbox_editmodeicons, SIGNAL(clicked(bool)), this, SLOT(SlotSetEditModeIcons()));

    checkbox_updatewebsurfaces = new QCheckBox();
    checkbox_updatewebsurfaces->setText("Update Websurfaces");
    connect(checkbox_updatewebsurfaces, SIGNAL(clicked(bool)), this, SLOT(SlotSetUpdateWebsurfaces()));

    checkbox_updatecmft = new QCheckBox();
    checkbox_updatecmft->setText("Update CMFT");
    connect(checkbox_updatecmft, SIGNAL(clicked(bool)), this, SLOT(SlotSetUpdateCMFT()));

    checkbox_updatevoip = new QCheckBox();
    checkbox_updatevoip->setText("Update VOIP");
    connect(checkbox_updatevoip, SIGNAL(clicked(bool)), this, SLOT(SlotSetUpdateVOIP()));

    checkbox_updatecustomavatars = new QCheckBox();
    checkbox_updatecustomavatars->setText("Update Custom Avatars");
    connect(checkbox_updatecustomavatars, SIGNAL(clicked(bool)), this, SLOT(SlotSetUpdateCustomAvatars()));

    checkbox_updateassetimages = new QCheckBox();
    checkbox_updateassetimages->setText("Update AssetImages");
    connect(checkbox_updateassetimages, SIGNAL(clicked(bool)), this, SLOT(SlotSetUpdateAssetImages()));

    QFormLayout * dev_layout = new QFormLayout();
    dev_layout->addRow(checkbox_editmode);
    dev_layout->addRow(checkbox_editmodeicons);
    dev_layout->addRow(checkbox_downloadcache);
    dev_layout->addRow(checkbox_updatewebsurfaces);
    dev_layout->addRow(checkbox_updatecmft);
    dev_layout->addRow(checkbox_updatevoip);
    dev_layout->addRow(checkbox_updatecustomavatars);
    dev_layout->addRow(checkbox_updateassetimages);

    QWidget * w = new QWidget();
    w->setLayout(dev_layout);
    w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    return w;
}

void SettingsWindow::Update()
{
    lineedit_userid->setText(game->GetPlayer()->GetProperties()->GetUserID());
    lineedit_customportalshader->setText(game->GetMultiPlayerManager()->GetCustomPortalShader());
    checkbox_editmode->setChecked(SettingsManager::GetEditModeEnabled());
    checkbox_editmodeicons->setChecked(SettingsManager::GetEditModeIconsEnabled());
    checkbox_updatewebsurfaces->setChecked(SettingsManager::GetUpdateWebsurfaces());
    checkbox_updatecmft->setChecked(SettingsManager::GetUpdateCMFT());
    checkbox_updatevoip->setChecked(SettingsManager::GetUpdateVOIP());
    checkbox_updatecustomavatars->setChecked(SettingsManager::GetUpdateCustomAvatars());
    checkbox_updateassetimages->setChecked(SettingsManager::GetAssetImagesEnabled());
    checkbox_crosshair->setChecked(SettingsManager::GetCrosshairEnabled());
    slider_fov->setValue(SettingsManager::GetFOV());
    checkbox_comfortmode->setChecked(SettingsManager::GetComfortMode());
    checkbox_haptics->setChecked(SettingsManager::GetHapticsEnabled());
    checkbox_gamepad->setChecked(SettingsManager::GetGamepadEnabled());
    checkbox_micvoiceactivated->setChecked(SettingsManager::GetMicAlwaysOn());

    slider_microphonesensitivity->setValue(SettingsManager::GetMicSensitivity());
    label_microphonesensitivity->setVisible(checkbox_micvoiceactivated->isChecked());
    slider_microphonesensitivity->setVisible(checkbox_micvoiceactivated->isChecked());

    checkbox_positionalenv->setChecked(SettingsManager::GetPositionalEnvEnabled());
    checkbox_positionalvoip->setChecked(SettingsManager::GetPositionalVOIPEnabled());

    checkbox_invertpitch->setChecked(SettingsManager::GetInvertYEnabled());
    checkbox_partymode->setChecked(SettingsManager::GetPartyModeEnabled());
    checkbox_selfavatar->setChecked(SettingsManager::GetSelfAvatar());    
    checkbox_multiplayer->setChecked(SettingsManager::GetMultiplayerEnabled());
    checkbox_downloadcache->setChecked(SettingsManager::GetCacheEnabled());
    checkbox_antialiasing->setChecked(SettingsManager::GetAntialiasingEnabled());
    checkbox_enhanceddepthprecision->setChecked(SettingsManager::GetEnhancedDepthPrecisionEnabled());
    checkbox_renderportalrooms->setChecked(SettingsManager::GetRenderPortalRooms());
    slider_volumemic->setValue(SettingsManager::GetVolumeMic());
    slider_volumeenv->setValue(SettingsManager::GetVolumeEnv());
    slider_volumevoip->setValue(SettingsManager::GetVolumeVOIP());
    lineedit_homeurl->setText(SettingsManager::GetHomeURL());
    lineedit_websurfaceurl->setText(SettingsManager::GetWebsurfaceURL());

#ifdef __ANDROID__
    checkbox_invertyaw->setChecked(SettingsManager::GetInvertXEnabled());
    checkbox_usegyro->setChecked(SettingsManager::GetUseGyroscope());
    checkbox_showsplash->setChecked(SettingsManager::GetShowSplash());
    checkbox_showloadingicon->setChecked(SettingsManager::GetShowLoadingIcon());
    checkbox_showviewjoystick->setChecked(SettingsManager::GetShowViewJoystick());
    checkbox_usevr->setChecked(SettingsManager::GetUseVR());
#endif
}

void SettingsWindow::SlotSetUserID()
{
    game->SetUserID(lineedit_userid->text());
}

void SettingsWindow::SlotSetCustomPortalShader()
{
    const QString src = lineedit_customportalshader->text();
    game->GetMultiPlayerManager()->SetCustomPortalShader(src);
}

void SettingsWindow::SlotSetEditMode()
{
    SettingsManager::settings["editmode"] = checkbox_editmode->isChecked();
}

void SettingsWindow::SlotSetEditModeIcons()
{
    SettingsManager::settings["editmodeicons"] = checkbox_editmodeicons->isChecked();
}

void SettingsWindow::SlotSetUpdateWebsurfaces()
{
    SettingsManager::settings["updatewebsurfaces"] = checkbox_updatewebsurfaces->isChecked();
}

void SettingsWindow::SlotSetUpdateCMFT()
{
    SettingsManager::settings["updatecmft"] = checkbox_updatecmft->isChecked();
}

void SettingsWindow::SlotSetUpdateVOIP()
{
    SettingsManager::settings["updatevoip"] = checkbox_updatevoip->isChecked();
}

void SettingsWindow::SlotSetUpdateCustomAvatars()
{
    SettingsManager::settings["updatecustomavatars"] = checkbox_updatecustomavatars->isChecked();
}

void SettingsWindow::SlotSetUpdateAssetImages()
{
    SettingsManager::settings["assetimages"] = checkbox_updateassetimages->isChecked();
}

void SettingsWindow::SlotSetCrosshair()
{
    SettingsManager::settings["crosshair"] = checkbox_crosshair->isChecked();
#ifdef __ANDROID__
    SettingsManager::SaveSettings();
#endif
}

void SettingsWindow::SlotSetFOV()
{
    SettingsManager::settings["fov"] = slider_fov->value();
}

void SettingsWindow::SlotSetSelfAvatar()
{
    SettingsManager::settings["selfavatar"] = checkbox_selfavatar->isChecked();
}

void SettingsWindow::SlotSetPartyMode()
{
    SettingsManager::settings["partymode"] = checkbox_partymode->isChecked();
}

void SettingsWindow::SlotSetMultiplayer()
{
    SettingsManager::settings["multiplayer"] = checkbox_multiplayer->isChecked();
#ifdef __ANDROID__
    SettingsManager::SaveSettings();
#endif
}

void SettingsWindow::SlotSetHomeURL()
{
    SettingsManager::settings["homeurl"] = lineedit_homeurl->text();
}

void SettingsWindow::SlotSetWebsurfaceURL()
{
    SettingsManager::settings["websurfaceurl"] = lineedit_websurfaceurl->text();
}

void SettingsWindow::SlotSetDownloadCache()
{
    SettingsManager::settings["downloadcache"] = checkbox_downloadcache->isChecked();
}

void SettingsWindow::SlotSetComfortMode()
{
    SettingsManager::settings["comfortmode"] = checkbox_comfortmode->isChecked();
}

void SettingsWindow::SlotSetHaptics()
{
    SettingsManager::settings["haptics"] = checkbox_haptics->isChecked();
}

void SettingsWindow::SlotSetAntialiasing()
{
    SettingsManager::settings["antialiasing"] = checkbox_antialiasing->isChecked();
}

void SettingsWindow::SlotSetEnhancedDepthPrecision()
{
    SettingsManager::settings["enhanceddepthprecision"] = checkbox_enhanceddepthprecision->isChecked();
}

void SettingsWindow::SlotSetGamepad()
{
    SettingsManager::settings["gamepad"] = checkbox_gamepad->isChecked();
}

void SettingsWindow::SlotSetMicVoiceActivated()
{
    SettingsManager::settings["micalwayson"] = checkbox_micvoiceactivated->isChecked();
    Update();
}

void SettingsWindow::SlotSetMicSensitivity()
{
    SettingsManager::settings["micsensitivity"] = slider_microphonesensitivity->value();
}

void SettingsWindow::SlotSetVolumeMic()
{
    SettingsManager::settings["volumemic"] = slider_volumemic->value();
#ifdef __ANDROID__
    SettingsManager::SaveSettings();
#endif
}

void SettingsWindow::SlotSetVolumeEnv()
{
    SettingsManager::settings["volumeenv"] = slider_volumeenv->value();
#ifdef __ANDROID__
    SettingsManager::SaveSettings();
#endif
}

void SettingsWindow::SlotSetPositionalEnv()
{
    SettingsManager::settings["positionalenv"] = checkbox_positionalenv->isChecked();
}

void SettingsWindow::SlotSetVolumeVoip()
{
    SettingsManager::settings["volumevoip"] = slider_volumevoip->value();
#ifdef __ANDROID__
    SettingsManager::SaveSettings();
#endif
}

void SettingsWindow::SlotSetPositionalVoip()
{
    SettingsManager::settings["positionalvoip"] = checkbox_positionalvoip->isChecked();
}

void SettingsWindow::SlotSetInvertPitch()
{
    SettingsManager::settings["invertpitch"] = checkbox_invertpitch->isChecked();
#ifdef __ANDROID__
    SettingsManager::SaveSettings();
#endif
}

void SettingsWindow::SlotResetAvatar()
{
    game->ResetAvatar();
    Update();
}

void SettingsWindow::SlotSetViveTrackpadMovement()
{
    SettingsManager::settings["vivetrackpadmovement"] = checkbox_vivetrackpadmovement->isChecked();
}

void SettingsWindow::SlotSetRenderPortalRooms()
{
    SettingsManager::settings["renderportalrooms"] = checkbox_renderportalrooms->isChecked();
}

void SettingsWindow::closeEvent(QCloseEvent *event)
{
    SettingsManager::SaveSettings();
}

#ifdef __ANDROID__
void SettingsWindow::SlotSetInvertYaw()
{
    SettingsManager::settings["invertyaw"] = checkbox_invertyaw->isChecked();
    SettingsManager::SaveSettings();
}

void SettingsWindow::SlotSetUseGyro()
{
    SettingsManager::settings["usegyroscope"] = checkbox_usegyro->isChecked();
    SettingsManager::SaveSettings();
}

void SettingsWindow::SlotSetShowSplash()
{
    SettingsManager::settings["showsplash"] = checkbox_showsplash->isChecked();
    SettingsManager::SaveSettings();
}

void SettingsWindow::SlotSetShowLoadingIcon()
{
    SettingsManager::settings["showloadingicon"] = checkbox_showloadingicon->isChecked();
    SettingsManager::SaveSettings();
}

void SettingsWindow::SlotSetShowViewJoystick()
{
    SettingsManager::settings["showviewjoystick"] = checkbox_showviewjoystick->isChecked();
    SettingsManager::SaveSettings();
}

void SettingsWindow::SlotSetLaunchInVR()
{
    SettingsManager::settings["usevr"] = checkbox_usevr->isChecked();
    SettingsManager::SaveSettings();
}

QTabWidget * SettingsWindow::GetTabWidget()
{
    return tab_widget;
}

#endif
