#include "jniutil.h"
#include "mainwindow.h"

#ifdef __ANDROID__

jmethodID JNIUtil::m_getLaunchURLMID = NULL;
jmethodID JNIUtil::m_getLaunchInVRMID = NULL;
jmethodID JNIUtil::m_getPermissionsMID = NULL;
jmethodID JNIUtil::m_getAnsweredPermissionsMID = NULL;
jmethodID JNIUtil::m_askPermissionsMID = NULL;

jmethodID JNIUtil::m_hideSplashMID = NULL;
jmethodID JNIUtil::m_setVRModeEnabledMID = NULL;
jmethodID JNIUtil::m_setProgressBarMID = NULL;

jmethodID JNIUtil::m_getCookieMID = NULL;
jmethodID JNIUtil::m_createNewWebViewMID = NULL;
jmethodID JNIUtil::m_removeWebViewMID = NULL;
jmethodID JNIUtil::m_attachWebViewToMainLayoutMID = NULL;
jmethodID JNIUtil::m_moveWebViewMID = NULL;
jmethodID JNIUtil::m_resizeWebViewMID = NULL;
jmethodID JNIUtil::m_loadUrlAtWebViewMID = NULL;
jmethodID JNIUtil::m_goBackOrForwardAtWebViewMID = NULL;
jmethodID JNIUtil::m_reloadMID = NULL;
jmethodID JNIUtil::m_stopMID = NULL;
jmethodID JNIUtil::m_loadHtmlAtWebViewMID = NULL;
jmethodID JNIUtil::m_getBitmapMID = NULL;
jmethodID JNIUtil::m_getHitTestContentMID = NULL;
jmethodID JNIUtil::m_getHorizontalScrollRangeMID = NULL;
jmethodID JNIUtil::m_getVerticalScrollRangeMID = NULL;
jmethodID JNIUtil::m_getWidthWebViewMID;
jmethodID JNIUtil::m_getHeightWebViewMID;
jmethodID JNIUtil::m_getScrollXWebViewMID;
jmethodID JNIUtil::m_getScrollYWebViewMID;
jmethodID JNIUtil::m_getURLWebViewMID;
jmethodID JNIUtil::m_scrollToWebViewMID = NULL;
jmethodID JNIUtil::m_scrollByWebViewMID = NULL;
jmethodID JNIUtil::m_evaluateJavaScriptAtWebViewMID = NULL;
jmethodID JNIUtil::m_setUpdatesEnabledWebViewMID = NULL;
jmethodID JNIUtil::m_mousePressWebViewMID = NULL;
jmethodID JNIUtil::m_mouseMoveWebViewMID = NULL;
jmethodID JNIUtil::m_mouseReleaseWebViewMID = NULL;
jmethodID JNIUtil::m_keyPressWebViewMID = NULL;
jmethodID JNIUtil::m_keyReleaseWebViewMID = NULL;
jmethodID JNIUtil::m_getRepaintRequestedAtWebViewMID = NULL;
jmethodID JNIUtil::m_getScrollRequestedAtWebViewMID = NULL;
jmethodID JNIUtil::m_getURLChangedAtWebViewMID = NULL;

jmethodID JNIUtil::m_getScreenOrientationMID = NULL;
jmethodID JNIUtil::m_setButtonMarginMID = NULL;
jmethodID JNIUtil::m_setControlsVisibleMID = NULL;
jmethodID JNIUtil::m_hideKeyboardMID = NULL;
jmethodID JNIUtil::m_getWalkJoystickXMID = NULL;
jmethodID JNIUtil::m_getWalkJoystickYMID = NULL;
jmethodID JNIUtil::m_getViewJoystickXMID = NULL;
jmethodID JNIUtil::m_getViewJoystickYMID = NULL;
jmethodID JNIUtil::m_getRunningMID = NULL;
jmethodID JNIUtil::m_getSpeakingMID = NULL;
jmethodID JNIUtil::m_getAlwaysSpeakingMID = NULL;
jmethodID JNIUtil::m_getJumpingMID = NULL;
jmethodID JNIUtil::m_getFlyingMID = NULL;
jmethodID JNIUtil::m_onPauseMID = NULL;
jmethodID JNIUtil::m_onResumeMID = NULL;

jmethodID JNIUtil::m_getGVRContextMID = NULL;
jmethodID JNIUtil::m_getGearReadyMID = NULL;
jmethodID JNIUtil::m_setupGVRMID = NULL;
jmethodID JNIUtil::m_setupGearMID = NULL;
jmethodID JNIUtil::m_showGVRMID = NULL;
jmethodID JNIUtil::m_showGearMID = NULL;
jmethodID JNIUtil::m_getShowingVRMID = NULL;
jmethodID JNIUtil::m_getWindowSurfaceMID = NULL;

jmethodID JNIUtil::m_getGamepadConnectedMID = NULL;
jmethodID JNIUtil::m_getLeftStickXMID = NULL;
jmethodID JNIUtil::m_getLeftStickYMID = NULL;
jmethodID JNIUtil::m_getRightStickXMID = NULL;
jmethodID JNIUtil::m_getRightStickYMID = NULL;
jmethodID JNIUtil::m_getDpadUpMID = NULL;
jmethodID JNIUtil::m_getDpadDownMID = NULL;
jmethodID JNIUtil::m_getDpadLeftMID = NULL;
jmethodID JNIUtil::m_getDpadRightMID = NULL;
jmethodID JNIUtil::m_getButtonXMID = NULL;
jmethodID JNIUtil::m_getButtonYMID = NULL;
jmethodID JNIUtil::m_getButtonAMID = NULL;
jmethodID JNIUtil::m_getButtonBMID = NULL;
jmethodID JNIUtil::m_getButtonLeftThumbMID = NULL;
jmethodID JNIUtil::m_getButtonRightThumbMID = NULL;
jmethodID JNIUtil::m_getTriggerLeftMID = NULL;
jmethodID JNIUtil::m_getTriggerRightMID = NULL;
jmethodID JNIUtil::m_getButtonLeftShoulderMID = NULL;
jmethodID JNIUtil::m_getButtonRightShoulderMID = NULL;
jmethodID JNIUtil::m_getButtonStartMID = NULL;
jmethodID JNIUtil::m_getButtonBackMID = NULL;

jobject JNIUtil::m_objectRef = NULL;
jclass JNIUtil::cls = NULL;

MainWindow * JNIUtil::main_window = nullptr;
pid_t JNIUtil::main_thread_id = NULL;

JNIUtil::JNIUtil()
{

}

JNIUtil::~JNIUtil()
{

}

int64_t JNIUtil::GetTimestampNsec()
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (int64_t) now.tv_sec*1000000000LL + now.tv_nsec;
}

void JNIUtil::Initialize()
{
    QAndroidJniEnvironment jniEnv;
    QAndroidJniObject qObjAct = QAndroidJniObject::callStaticObjectMethod("org/qtproject/qt5/android/QtNative",
                                                                           "activity",
                                                                           "()Landroid/app/Activity;");

    jobject objAct = qObjAct.object<jobject>();
    m_objectRef = jniEnv->NewGlobalRef(objAct);
    cls = jniEnv->GetObjectClass(objAct);

    if(cls)
    {
        /*
         * Reference for types:
         * Z - boolean
         * B - byte
         * C - char
         * S - short
         * I - int
         * J - long
         * F - float
         * D - double
         * L fully-qualified-class ; - fully-qualified-class
         * [type - type[]
         * ( arg-types ) ret-type - method type
         */

        //Webview
        m_getCookieMID = jniEnv->GetMethodID(cls, "getCookie", "()Ljava/lang/String;");
        m_createNewWebViewMID = jniEnv->GetMethodID(cls, "createNewWebView", "(I)V");
        m_removeWebViewMID = jniEnv->GetMethodID(cls, "removeWebView", "(I)V");
        m_attachWebViewToMainLayoutMID = jniEnv->GetMethodID(cls, "attachWebViewToMainLayout", "(I)V");
        m_moveWebViewMID = jniEnv->GetMethodID(cls, "moveWebView", "(III)V");
        m_resizeWebViewMID = jniEnv->GetMethodID(cls, "resizeWebView", "(III)V");
        m_loadUrlAtWebViewMID = jniEnv->GetMethodID(cls, "loadUrlAtWebView", "(ILjava/lang/String;)V");
        m_goBackOrForwardAtWebViewMID = jniEnv->GetMethodID(cls, "goBackOrForwardAtWebView", "(II)V");
        m_reloadMID = jniEnv->GetMethodID(cls, "reloadWebView", "(I)V");
        m_stopMID = jniEnv->GetMethodID(cls, "stopWebView", "(I)V");
        m_loadHtmlAtWebViewMID = jniEnv->GetMethodID(cls, "loadHtmlAtWebView", "(ILjava/lang/String;)V");
        m_evaluateJavaScriptAtWebViewMID = jniEnv->GetMethodID(cls, "evaluateJavaScriptAtWebView", "(ILjava/lang/String;)V");
        m_setUpdatesEnabledWebViewMID = jniEnv->GetMethodID(cls, "setUpdatesEnabledWebView", "(IZ)V");
        m_getBitmapMID = jniEnv->GetMethodID(cls, "getBitmap", "(I)Landroid/graphics/Bitmap;");
        m_getHitTestContentMID = jniEnv->GetMethodID(cls, "getHitTestContent", "(I)Ljava/lang/String;");
        m_getHorizontalScrollRangeMID = jniEnv->GetMethodID(cls, "getHorizontalScrollRange", "(I)I");
        m_getVerticalScrollRangeMID = jniEnv->GetMethodID(cls, "getVerticalScrollRange", "(I)I");
        m_getWidthWebViewMID = jniEnv->GetMethodID(cls, "getWidthWebView", "(I)I");
        m_getHeightWebViewMID = jniEnv->GetMethodID(cls, "getHeightWebView", "(I)I");
        m_getScrollXWebViewMID = jniEnv->GetMethodID(cls, "getScrollXWebView", "(I)I");
        m_getScrollYWebViewMID = jniEnv->GetMethodID(cls, "getScrollYWebView", "(I)I");
        m_getURLWebViewMID = jniEnv->GetMethodID(cls, "getURLWebView", "(I)Ljava/lang/String;");
        m_scrollToWebViewMID = jniEnv->GetMethodID(cls, "scrollToWebView", "(III)V");
        m_scrollByWebViewMID = jniEnv->GetMethodID(cls, "scrollByWebView", "(III)V");
        m_mousePressWebViewMID = jniEnv->GetMethodID(cls, "mousePressWebView", "(III)V");
        m_mouseMoveWebViewMID = jniEnv->GetMethodID(cls, "mouseMoveWebView", "(III)V");
        m_mouseReleaseWebViewMID = jniEnv->GetMethodID(cls, "mouseReleaseWebView", "(III)V");
        m_keyPressWebViewMID = jniEnv->GetMethodID(cls, "keyPressWebView", "(III)V");
        m_keyReleaseWebViewMID = jniEnv->GetMethodID(cls, "keyReleaseWebView", "(III)V");
        m_getRepaintRequestedAtWebViewMID = jniEnv->GetMethodID(cls, "getRepaintRequestedAtWebView", "(I)Z");
        m_getScrollRequestedAtWebViewMID = jniEnv->GetMethodID(cls, "getScrollRequestedAtWebView", "(I)Z");
        m_getURLChangedAtWebViewMID = jniEnv->GetMethodID(cls, "getURLChangedAtWebView", "(I)Z");

        //Controls
        m_getLaunchURLMID = jniEnv->GetMethodID(cls, "getLaunchURL", "()Ljava/lang/String;");
        m_getLaunchInVRMID = jniEnv->GetMethodID(cls, "getLaunchInVR", "()Z");
        m_getPermissionsMID = jniEnv->GetMethodID(cls, "getPermissions", "()Z");
        m_getAnsweredPermissionsMID = jniEnv->GetMethodID(cls, "getAnsweredPermissions", "()Z");
        m_askPermissionsMID = jniEnv->GetMethodID(cls, "askPermissions", "()V");
        m_hideSplashMID = jniEnv->GetMethodID(cls, "hideSplash", "()V");
        m_setVRModeEnabledMID = jniEnv->GetMethodID(cls, "setVRModeEnabled", "(Z)V");
        m_setProgressBarMID = jniEnv->GetMethodID(cls, "setProgressBar", "(I)V");
        m_getScreenOrientationMID = jniEnv->GetMethodID(cls, "getScreenOrientation", "()I");
        m_setButtonMarginMID = jniEnv->GetMethodID(cls, "setButtonMargin", "(I)V");
        m_setControlsVisibleMID = jniEnv->GetMethodID(cls, "setControlsVisible", "(ZZ)V");
        m_hideKeyboardMID = jniEnv->GetMethodID(cls, "hideKeyboard", "()V");
        m_getWalkJoystickXMID = jniEnv->GetMethodID(cls, "getWalkJoystickX", "()F");
        m_getWalkJoystickYMID = jniEnv->GetMethodID(cls, "getWalkJoystickY", "()F");
        m_getViewJoystickXMID = jniEnv->GetMethodID(cls, "getViewJoystickX", "()F");
        m_getViewJoystickYMID = jniEnv->GetMethodID(cls, "getViewJoystickY", "()F");
        m_getRunningMID = jniEnv->GetMethodID(cls, "getRunning", "()Z");
        m_getSpeakingMID = jniEnv->GetMethodID(cls, "getSpeaking", "()Z");
        m_getAlwaysSpeakingMID = jniEnv->GetMethodID(cls, "getAlwaysSpeaking", "()Z");
        m_getJumpingMID = jniEnv->GetMethodID(cls, "getJumping", "()Z");
        m_getFlyingMID = jniEnv->GetMethodID(cls, "getFlying", "()Z");
        m_onPauseMID = jniEnv->GetMethodID(cls,"onPause","()V");
        m_onResumeMID = jniEnv->GetMethodID(cls,"onResume","()V");

        //GVR/Gear
        m_getGVRContextMID = jniEnv->GetMethodID(cls, "getGvrContext", "()J");
        m_getGearReadyMID = jniEnv->GetMethodID(cls, "getGearReady", "()Z");
        m_setupGVRMID = jniEnv->GetMethodID(cls, "setupGvr", "()V");
        m_setupGearMID = jniEnv->GetMethodID(cls, "setupGear", "()V");
        m_showGVRMID = jniEnv->GetMethodID(cls, "showGVR", "(Z)V");
        m_showGearMID = jniEnv->GetMethodID(cls, "showGear", "(Z)V");
        m_getShowingVRMID = jniEnv->GetMethodID(cls, "getShowingVR", "()Z");
        m_getWindowSurfaceMID = jniEnv->GetMethodID(cls, "getWindowSurface", "()Landroid/view/Surface;");

        //Gamepad
        m_getGamepadConnectedMID = jniEnv->GetMethodID(cls, "getGamepadConnected", "()Z");
        m_getLeftStickXMID = jniEnv->GetMethodID(cls, "getLeftStickX", "()F");
        m_getLeftStickYMID = jniEnv->GetMethodID(cls, "getLeftStickY", "()F");
        m_getRightStickXMID = jniEnv->GetMethodID(cls, "getRightStickX", "()F");
        m_getRightStickYMID = jniEnv->GetMethodID(cls, "getRightStickY", "()F");
        m_getDpadUpMID = jniEnv->GetMethodID(cls, "getDpadUp", "()Z");
        m_getDpadDownMID = jniEnv->GetMethodID(cls, "getDpadDown", "()Z");
        m_getDpadLeftMID = jniEnv->GetMethodID(cls, "getDpadLeft", "()Z");
        m_getDpadRightMID = jniEnv->GetMethodID(cls, "getDpadRight", "()Z");
        m_getButtonXMID = jniEnv->GetMethodID(cls, "getButtonX", "()Z");
        m_getButtonYMID = jniEnv->GetMethodID(cls, "getButtonY", "()Z");
        m_getButtonAMID = jniEnv->GetMethodID(cls, "getButtonA", "()Z");
        m_getButtonBMID = jniEnv->GetMethodID(cls, "getButtonB", "()Z");
        m_getButtonLeftThumbMID = jniEnv->GetMethodID(cls, "getButtonLeftThumb", "()Z");
        m_getButtonRightThumbMID = jniEnv->GetMethodID(cls, "getButtonRightThumb", "()Z");
        m_getTriggerLeftMID = jniEnv->GetMethodID(cls, "getTriggerLeft", "()F");
        m_getTriggerRightMID = jniEnv->GetMethodID(cls, "getTriggerRight", "()F");
        m_getButtonLeftShoulderMID = jniEnv->GetMethodID(cls, "getButtonLeftShoulder", "()Z");
        m_getButtonRightShoulderMID = jniEnv->GetMethodID(cls, "getButtonRightShoulder", "()Z");
        m_getButtonStartMID = jniEnv->GetMethodID(cls, "getButtonStart", "()Z");
        m_getButtonBackMID = jniEnv->GetMethodID(cls, "getButtonBack", "()Z");
    }
}

void JNIUtil::Destroy()
{
    if(m_objectRef != NULL)
    {
        QAndroidJniEnvironment jniEnv;
        jniEnv->DeleteGlobalRef(m_objectRef);
    }
}

QString JNIUtil::GetLaunchURL()
{
    if (m_getLaunchURLMID){
        QAndroidJniEnvironment jniEnv;
        jstring url = (jstring) jniEnv->CallObjectMethod(m_objectRef, m_getLaunchURLMID);
        return QString(jniEnv->GetStringUTFChars(url, 0));
    }
    return "";
}

bool JNIUtil::GetLaunchInVR()
{
    QAndroidJniEnvironment jniEnv;
    return (m_getLaunchInVRMID && (bool)jniEnv->CallBooleanMethod(m_objectRef, m_getLaunchInVRMID));
}

bool JNIUtil::GetPermissions()
{
    QAndroidJniEnvironment jniEnv;
    return (m_getPermissionsMID && (bool)jniEnv->CallBooleanMethod(m_objectRef, m_getPermissionsMID));
}

bool JNIUtil::GetAnsweredPermissions()
{
    QAndroidJniEnvironment jniEnv;
    return (m_getAnsweredPermissionsMID && (bool)jniEnv->CallBooleanMethod(m_objectRef, m_getAnsweredPermissionsMID));
}

void JNIUtil::AskPermissions()
{
    if(m_askPermissionsMID)
    {
        QAndroidJniEnvironment jniEnv;
        jniEnv->CallVoidMethod(m_objectRef, m_askPermissionsMID);
    }
}

void JNIUtil::SetVRModeEnabled(bool b)
{
    if(m_setVRModeEnabledMID)
    {
        QAndroidJniEnvironment jniEnv;
        jniEnv->CallVoidMethod(m_objectRef, m_setVRModeEnabledMID, (jboolean) b);
    }
}

void JNIUtil::HideSplash()
{
    if(m_hideSplashMID)
    {
        QAndroidJniEnvironment jniEnv;
        jniEnv->CallVoidMethod(m_objectRef, m_hideSplashMID);
    }
}

void JNIUtil::SetProgressBar(int i)
{
    if(m_setProgressBarMID)
    {
        QAndroidJniEnvironment jniEnv;
        jniEnv->CallVoidMethod(m_objectRef, m_setProgressBarMID, i);
    }
}

void JNIUtil::UpdateCookies()
{
    if (m_getCookieMID){
        QAndroidJniEnvironment jniEnv;
        jstring cookie_jstring = (jstring) jniEnv->CallObjectMethod(m_objectRef, m_getCookieMID);
        QString s = QString(jniEnv->GetStringUTFChars(cookie_jstring, 0));
        if (s != "") {
            //qDebug() << "updating janus-cookie" << s;
            QByteArray ba = s.toLatin1();
            QList<QNetworkCookie> cookies = QNetworkCookie::parseCookies(ba);

            for (QList<QNetworkCookie>::iterator it=cookies.begin(); it!=cookies.end(); ++it) {
                //qDebug() << "jni-janus-cookies" << it->domain() << it->name() << it->value();

                QString domainStr = it->domain();
                if (!domainStr.isEmpty() && domainStr.at(0) == '.') {
                    domainStr.remove(0,1);
                }
                domainStr = QString("http://") + domainStr;

                CookieJar::cookie_jar->setCookiesFromUrl(QList<QNetworkCookie>({*it}), domainStr);
            }
            CookieJar::cookie_jar->SaveToDisk();
        }
        jniEnv->DeleteLocalRef(cookie_jstring);
    }
}

void JNIUtil::CreateNewWebView(int tag)
{
    if(m_createNewWebViewMID)
    {
        QAndroidJniEnvironment jniEnv;
        jniEnv->CallVoidMethod(m_objectRef, m_createNewWebViewMID, tag);
    }
}

void JNIUtil::RemoveWebView(int tag)
{
    if(m_removeWebViewMID)
    {
        QAndroidJniEnvironment jniEnv;
        jniEnv->CallVoidMethod(m_objectRef, m_removeWebViewMID, tag);
    }
}

void JNIUtil::AttachWebViewToMainLayout(int tag)
{
    if(m_attachWebViewToMainLayoutMID)
    {
        QAndroidJniEnvironment jniEnv;
        jniEnv->CallVoidMethod(m_objectRef, m_attachWebViewToMainLayoutMID, tag);
    }
}

void JNIUtil::MoveWebView(int tag, int x, int y)
{
    if(m_moveWebViewMID)
    {
        QAndroidJniEnvironment jniEnv;
        jniEnv->CallVoidMethod(m_objectRef, m_moveWebViewMID, tag, x, y);
    }
}

void JNIUtil::ResizeWebView(int tag, int x, int y)
{
    if(m_resizeWebViewMID)
    {
        QAndroidJniEnvironment jniEnv;
        jniEnv->CallVoidMethod(m_objectRef, m_resizeWebViewMID, tag, x, y);
    }
}

void JNIUtil::LoadUrlAtWebView(int tag, QUrl url)
{
    if(m_loadUrlAtWebViewMID)
    {
        QAndroidJniEnvironment jniEnv;
        jstring jurl = jniEnv->NewStringUTF(url.toString().toUtf8().constData());
        jniEnv->CallVoidMethod(m_objectRef,
                                m_loadUrlAtWebViewMID,
                                tag,
                                jurl);
        jniEnv->DeleteLocalRef(jurl);
    }
}

void JNIUtil::GoBackOrForwardAtWebView(int tag, int b)
{
    if (m_goBackOrForwardAtWebViewMID)
    {
        QAndroidJniEnvironment jniEnv;
        jniEnv->CallVoidMethod(m_objectRef, m_goBackOrForwardAtWebViewMID, tag, b);
    }
}

void JNIUtil::ReloadWebView(int tag)
{
    if (m_reloadMID)
    {
        QAndroidJniEnvironment jniEnv;
        jniEnv->CallVoidMethod(m_objectRef, m_reloadMID, tag);
    }
}


void JNIUtil::StopWebView(int tag)
{
    if (m_stopMID)
    {
        QAndroidJniEnvironment jniEnv;
        jniEnv->CallVoidMethod(m_objectRef, m_stopMID, tag);
    }
}

void JNIUtil::LoadHtmlAtWebView(int tag, QString s)
{
    if(m_loadUrlAtWebViewMID)
    {
        QAndroidJniEnvironment jniEnv;
        jstring jhtml = jniEnv->NewStringUTF(s.toUtf8().constData());
        jniEnv->CallVoidMethod(m_objectRef,
                                m_loadHtmlAtWebViewMID,
                                tag,
                                jhtml);
        jniEnv->DeleteLocalRef(jhtml);
    }
}

void JNIUtil::EvaluateJavaScriptAtWebView(int tag, QString s)
{
    QAndroidJniEnvironment jniEnv;
    jstring jjs = jniEnv->NewStringUTF(s.toUtf8().constData());
    jniEnv->CallVoidMethod(m_objectRef, m_evaluateJavaScriptAtWebViewMID, tag, jjs);
    jniEnv->DeleteLocalRef(jjs);
}

void JNIUtil::SetUpdatesEnabledWebView(int tag, bool b)
{
    QAndroidJniEnvironment jniEnv;
    jniEnv->CallVoidMethod(m_objectRef, m_setUpdatesEnabledWebViewMID, tag, (jboolean) b);
}

void JNIUtil::UpdateTextureWebView(int tag, TextureHandle * m_texture_handle)
{
    QAndroidJniEnvironment jniEnv;
    jobject bitmap = NULL;

    if(m_getBitmapMID)
    {
        bitmap = jniEnv->CallObjectMethod(m_objectRef, m_getBitmapMID, tag);
    }

    if (bitmap != NULL && !jniEnv->IsSameObject(bitmap, NULL))
    {
        AndroidBitmapInfo info;
        if (AndroidBitmap_getInfo(jniEnv, bitmap, &info) != ANDROID_BITMAP_RESULT_SUCCESS)
            return;

        void *buffer;
        if (AndroidBitmap_lockPixels(jniEnv, bitmap, &buffer) != ANDROID_BITMAP_RESULT_SUCCESS)
            return;

        int w = GetWidthWebView(tag);
        int h = GetHeightWebView(tag);

        if (buffer != nullptr){
            RendererInterface::m_pimpl->UpdateTextureHandleData(m_texture_handle, 0, 0, 0, w, h, (int)GL_RGBA, (int)GL_UNSIGNED_BYTE, buffer, w * h * 4);
            RendererInterface::m_pimpl->GenerateTextureHandleMipMap(m_texture_handle);
        }

        if (AndroidBitmap_unlockPixels(jniEnv, bitmap) != ANDROID_BITMAP_RESULT_SUCCESS)
            return;

        jniEnv->DeleteLocalRef(bitmap);
    }
}

WebHitTestResult JNIUtil::GetHitTestContentWebView(int tag)
{
    WebHitTestResult h;

    if (m_getHitTestContentMID){
        QAndroidJniEnvironment jniEnv;
        jstring url_jstring = (jstring) jniEnv->CallObjectMethod(m_objectRef, m_getHitTestContentMID, tag);
        QString url_string = QString(jniEnv->GetStringUTFChars(url_jstring, 0));
        if (url_string == "janus://content_editable") {
            h.editable = true;
        }
        else {
            h.link_url = QUrl(url_string);
        }
        jniEnv->DeleteLocalRef(url_jstring);
        //TODO: add content editable
        //h.selected = qh.isContentSelected();
        h.bounding_rect = QRect();
        h.is_null = false;
    }

    return h;
}

int JNIUtil::GetHorizontalScrollRange(int tag)
{
    int x = 0;
    if (m_getHorizontalScrollRangeMID){
        QAndroidJniEnvironment jniEnv;
        x = (int)jniEnv->CallIntMethod(m_objectRef, m_getHorizontalScrollRangeMID, tag);
    }
    return x;
}

int JNIUtil::GetVerticalScrollRange(int tag)
{
    int y = 0;
    if (m_getVerticalScrollRangeMID){
        QAndroidJniEnvironment jniEnv;
        y = (int)jniEnv->CallIntMethod(m_objectRef, m_getVerticalScrollRangeMID, tag);
    }
    return y;
}

int JNIUtil::GetWidthWebView(int tag)
{
    if (m_getWidthWebViewMID){
        QAndroidJniEnvironment jniEnv;
        return (int)jniEnv->CallIntMethod(m_objectRef, m_getWidthWebViewMID, tag);
    }
    return 0;
}

int JNIUtil::GetHeightWebView(int tag)
{
    if (m_getHeightWebViewMID){
        QAndroidJniEnvironment jniEnv;
        return (int)jniEnv->CallIntMethod(m_objectRef, m_getHeightWebViewMID, tag);
    }
    return 0;
}

int JNIUtil::GetScrollXWebView(int tag)
{
    if (m_getScrollXWebViewMID){
        QAndroidJniEnvironment jniEnv;
        return (int)jniEnv->CallIntMethod(m_objectRef, m_getScrollXWebViewMID, tag);
    }
    return 0;
}

int JNIUtil::GetScrollYWebView(int tag)
{
    if (m_getScrollYWebViewMID){
        QAndroidJniEnvironment jniEnv;
        return (int)jniEnv->CallIntMethod(m_objectRef, m_getScrollYWebViewMID, tag);
    }
    return 0;
}

QString JNIUtil::GetURLWebView(int tag)
{
    if (m_getURLWebViewMID){
        QAndroidJniEnvironment jniEnv;
        jstring url_jstring = (jstring) jniEnv->CallObjectMethod(m_objectRef, m_getURLWebViewMID, tag);
        QString s = QString(jniEnv->GetStringUTFChars(url_jstring, 0));
        jniEnv->DeleteLocalRef(url_jstring);
        return s;
    }
    return "";
}

void JNIUtil::ScrollByWebView(int tag, int x, int y)
{
    if (m_scrollByWebViewMID){
        QAndroidJniEnvironment jniEnv;
        jniEnv->CallVoidMethod(m_objectRef, m_scrollByWebViewMID, tag, x, y);
    }
}

void JNIUtil::ScrollToWebView(int tag, int x, int y)
{
    if (m_scrollToWebViewMID){
        QAndroidJniEnvironment jniEnv;
        jniEnv->CallVoidMethod(m_objectRef, m_scrollToWebViewMID, tag, x, y);
    }
}

void JNIUtil::MousePressWebView(int tag, int x, int y)
{
    if (m_mousePressWebViewMID){
        QAndroidJniEnvironment jniEnv;
        jniEnv->CallVoidMethod(m_objectRef, m_mousePressWebViewMID, tag, x, y);
    }
}

void JNIUtil::MouseMoveWebView(int tag, int x, int y)
{
    if (m_mouseMoveWebViewMID){
        QAndroidJniEnvironment jniEnv;
        jniEnv->CallVoidMethod(m_objectRef, m_mouseMoveWebViewMID, tag, x, y);
    }
}

void JNIUtil::MouseReleaseWebView(int tag, int x, int y)
{
    if (m_mouseReleaseWebViewMID){
        QAndroidJniEnvironment jniEnv;
        jniEnv->CallVoidMethod(m_objectRef, m_mouseReleaseWebViewMID, tag, x, y);
    }
}

void JNIUtil::KeyPressWebView(int tag, int code, int state)
{
    if (m_keyPressWebViewMID){
        QAndroidJniEnvironment jniEnv;
        jniEnv->CallVoidMethod(m_objectRef, m_keyPressWebViewMID, tag, code, state);
    }
}

void JNIUtil::KeyReleaseWebView(int tag, int code, int state)
{
    if (m_keyReleaseWebViewMID){
        QAndroidJniEnvironment jniEnv;
        jniEnv->CallVoidMethod(m_objectRef, m_keyReleaseWebViewMID, tag, code, state);
    }
}

bool JNIUtil::GetRepaintRequestedAtWebView(int tag)
{
    QAndroidJniEnvironment jniEnv;
    return (m_getRepaintRequestedAtWebViewMID && (bool)jniEnv->CallBooleanMethod(m_objectRef, m_getRepaintRequestedAtWebViewMID, tag));
}

bool JNIUtil::GetScrollRequestedAtWebView(int tag)
{
    QAndroidJniEnvironment jniEnv;
    return (m_getScrollRequestedAtWebViewMID && (bool)jniEnv->CallBooleanMethod(m_objectRef, m_getScrollRequestedAtWebViewMID, tag));
}

bool JNIUtil::GetURLChangedAtWebView(int tag)
{
    QAndroidJniEnvironment jniEnv;
    return (m_getURLChangedAtWebViewMID && (bool)jniEnv->CallBooleanMethod(m_objectRef, m_getURLChangedAtWebViewMID, tag));
}

int JNIUtil::GetScreenOrientation()
{
    int r = 0;
    if (m_getScreenOrientationMID){
        QAndroidJniEnvironment jniEnv;
        r = (int)jniEnv->CallIntMethod(m_objectRef, m_getScreenOrientationMID);
    }
    return r;
}

void JNIUtil::SetButtonMargin(int margin)
{
    if(m_setButtonMarginMID)
    {
        QAndroidJniEnvironment jniEnv;
        jniEnv->CallVoidMethod(m_objectRef, m_setButtonMarginMID, margin);
    }
}

void JNIUtil::SetControlsVisible(bool b, bool show_view_joystick)
{
    if(m_setControlsVisibleMID)
    {
        QAndroidJniEnvironment jniEnv;
        jniEnv->CallVoidMethod(m_objectRef, m_setControlsVisibleMID, (jboolean) b, (jboolean) show_view_joystick);
    }
}

void JNIUtil::HideKeyboard()
{
    if(m_hideKeyboardMID)
    {
        QAndroidJniEnvironment jniEnv;
        jniEnv->CallVoidMethod(m_objectRef, m_hideKeyboardMID);
    }
}

float JNIUtil::GetWalkJoystickX()
{
    float x = 0;
    if (m_getWalkJoystickXMID){
        QAndroidJniEnvironment jniEnv;
        x = (float)jniEnv->CallFloatMethod(m_objectRef, m_getWalkJoystickXMID);
    }
    return x;
}

float JNIUtil::GetWalkJoystickY()
{
    float y = 0;
    if (m_getWalkJoystickYMID){
        QAndroidJniEnvironment jniEnv;
        y = (float)jniEnv->CallFloatMethod(m_objectRef, m_getWalkJoystickYMID);
    }
    return y;
}

float JNIUtil::GetViewJoystickX()
{
    float x = 0;
    if (m_getViewJoystickXMID){
        QAndroidJniEnvironment jniEnv;
        x = (float)jniEnv->CallFloatMethod(m_objectRef, m_getViewJoystickXMID);
    }
    return x;
}

float JNIUtil::GetViewJoystickY()
{
    float y = 0;
    if (m_getViewJoystickYMID){
        QAndroidJniEnvironment jniEnv;
        y = (float)jniEnv->CallFloatMethod(m_objectRef, m_getViewJoystickYMID);
    }
    return y;
}

bool JNIUtil::GetRunning()
{
    QAndroidJniEnvironment jniEnv;
    return (m_getRunningMID && (bool)jniEnv->CallBooleanMethod(m_objectRef, m_getRunningMID));
}

bool JNIUtil::GetSpeaking()
{
    QAndroidJniEnvironment jniEnv;
    return (m_getSpeakingMID && (bool)jniEnv->CallBooleanMethod(m_objectRef, m_getSpeakingMID));
}

bool JNIUtil::GetAlwaysSpeaking()
{
    QAndroidJniEnvironment jniEnv;
    return (m_getAlwaysSpeakingMID && (bool)jniEnv->CallBooleanMethod(m_objectRef, m_getAlwaysSpeakingMID));
}

bool JNIUtil::GetJumping()
{
    QAndroidJniEnvironment jniEnv;
    return (m_getJumpingMID && (bool)jniEnv->CallBooleanMethod(m_objectRef, m_getJumpingMID));
}

bool JNIUtil::GetFlying()
{
    QAndroidJniEnvironment jniEnv;
    return (m_getFlyingMID && (bool)jniEnv->CallBooleanMethod(m_objectRef, m_getFlyingMID));
}

long long JNIUtil::GetGVRContext()
{
    if(m_getGVRContextMID)
    {
        QAndroidJniEnvironment jniEnv;
        jlong context = jniEnv->CallLongMethod(m_objectRef, m_getGVRContextMID);
        return ((long long) context);
    }
    return 0;
}

bool JNIUtil::GetGearReady()
{
    QAndroidJniEnvironment jniEnv;
    return (m_getGearReadyMID && (bool)jniEnv->CallBooleanMethod(m_objectRef, m_getGearReadyMID));
}

void JNIUtil::SetupGVR()
{
    if(m_setupGVRMID)
    {
        QAndroidJniEnvironment jniEnv;
        jniEnv->CallVoidMethod(m_objectRef, m_setupGVRMID);
    }

}

void JNIUtil::SetupGear()
{
    if(m_setupGearMID)
    {
        QAndroidJniEnvironment jniEnv;
        jniEnv->CallVoidMethod(m_objectRef, m_setupGearMID);
    }
}

void JNIUtil::ShowGVR(bool b)
{
    if(m_showGVRMID)
    {
        QAndroidJniEnvironment jniEnv;
        jniEnv->CallVoidMethod(m_objectRef, m_showGVRMID, (jboolean) b);
    }
}

void JNIUtil::ShowGear(bool b)
{
    if(m_showGearMID)
    {
        QAndroidJniEnvironment jniEnv;
        jniEnv->CallVoidMethod(m_objectRef, m_showGearMID, (jboolean) b);
    }
}

bool JNIUtil::GetShowingVR()
{
    QAndroidJniEnvironment jniEnv;
    return (m_getShowingVRMID && (bool)jniEnv->CallBooleanMethod(m_objectRef, m_getShowingVRMID));
}

jobject JNIUtil::GetWindowSurface()
{
    if(m_getWindowSurfaceMID)
    {
        QAndroidJniEnvironment jniEnv;
        jobject surface = jniEnv->CallObjectMethod(m_objectRef, m_getWindowSurfaceMID);
        //ANativeWindow* awindow = ANativeWindow_fromSurface(jniEnv, surface);
        //ANativeWindow_acquire(awindow);
        //return ((size_t) awindow);
        return surface;
    }
    return 0;
}

bool JNIUtil::GetGamepadConnected()
{
    QAndroidJniEnvironment jniEnv;
    return (m_getGamepadConnectedMID && (bool)jniEnv->CallBooleanMethod(m_objectRef, m_getGamepadConnectedMID));
}

float JNIUtil::GetLeftStickX()
{
    float x = 0;
    if (m_getLeftStickXMID){
        QAndroidJniEnvironment jniEnv;
        x = (float)jniEnv->CallFloatMethod(m_objectRef, m_getLeftStickXMID);
    }
    return x;
}

float JNIUtil::GetLeftStickY()
{
    float y = 0;
    if (m_getLeftStickYMID){
        QAndroidJniEnvironment jniEnv;
        y = (float)jniEnv->CallFloatMethod(m_objectRef, m_getLeftStickYMID);
    }
    return y;
}

float JNIUtil::GetRightStickX()
{
    float x = 0;
    if (m_getRightStickXMID){
        QAndroidJniEnvironment jniEnv;
        x = (float)jniEnv->CallFloatMethod(m_objectRef, m_getRightStickXMID);
    }
    return x;
}

float JNIUtil::GetRightStickY()
{
    float y = 0;
    if (m_getRightStickYMID){
        QAndroidJniEnvironment jniEnv;
        y = (float)jniEnv->CallFloatMethod(m_objectRef, m_getRightStickYMID);
    }
    return y;
}

bool JNIUtil::GetDpadUp()
{
    QAndroidJniEnvironment jniEnv;
    return (m_getDpadUpMID && (bool)jniEnv->CallBooleanMethod(m_objectRef, m_getDpadUpMID));
}

bool JNIUtil::GetDpadDown()
{
    QAndroidJniEnvironment jniEnv;
    return (m_getDpadDownMID && (bool)jniEnv->CallBooleanMethod(m_objectRef, m_getDpadDownMID));
}

bool JNIUtil::GetDpadLeft()
{
    QAndroidJniEnvironment jniEnv;
    return (m_getDpadLeftMID && (bool)jniEnv->CallBooleanMethod(m_objectRef, m_getDpadLeftMID));
}

bool JNIUtil::GetDpadRight()
{
    QAndroidJniEnvironment jniEnv;
    return (m_getDpadRightMID && (bool)jniEnv->CallBooleanMethod(m_objectRef, m_getDpadRightMID));
}

bool JNIUtil::GetButtonX()
{
    QAndroidJniEnvironment jniEnv;
    return (m_getButtonXMID && (bool)jniEnv->CallBooleanMethod(m_objectRef, m_getButtonXMID));
}

bool JNIUtil::GetButtonY()
{
    QAndroidJniEnvironment jniEnv;
    return (m_getButtonYMID && (bool)jniEnv->CallBooleanMethod(m_objectRef, m_getButtonYMID));
}

bool JNIUtil::GetButtonA()
{
    QAndroidJniEnvironment jniEnv;
    return (m_getButtonAMID && (bool)jniEnv->CallBooleanMethod(m_objectRef, m_getButtonAMID));
}

bool JNIUtil::GetButtonB()
{
    QAndroidJniEnvironment jniEnv;
    return (m_getButtonBMID && (bool)jniEnv->CallBooleanMethod(m_objectRef, m_getButtonBMID));
}

bool JNIUtil::GetButtonLeftThumb()
{
    QAndroidJniEnvironment jniEnv;
    return (m_getButtonLeftThumbMID && (bool)jniEnv->CallBooleanMethod(m_objectRef, m_getButtonLeftThumbMID));
}

bool JNIUtil::GetButtonRightThumb()
{
    QAndroidJniEnvironment jniEnv;
    return (m_getButtonRightThumbMID && (bool)jniEnv->CallBooleanMethod(m_objectRef, m_getButtonRightThumbMID));
}

float JNIUtil::GetTriggerLeft()
{
    float l = 0;
    if (m_getTriggerLeftMID){
        QAndroidJniEnvironment jniEnv;
        l = (float)jniEnv->CallFloatMethod(m_objectRef, m_getTriggerLeftMID);
    }
    return l;
}

float JNIUtil::GetTriggerRight()
{
    float r = 0;
    if (m_getTriggerRightMID){
        QAndroidJniEnvironment jniEnv;
        r = (float)jniEnv->CallFloatMethod(m_objectRef, m_getTriggerRightMID);
    }
    return r;
}

bool JNIUtil::GetButtonLeftShoulder()
{
    QAndroidJniEnvironment jniEnv;
    return (m_getButtonLeftShoulderMID && (bool)jniEnv->CallBooleanMethod(m_objectRef, m_getButtonLeftShoulderMID));
}

bool JNIUtil::GetButtonRightShoulder()
{
    QAndroidJniEnvironment jniEnv;
    return (m_getButtonRightShoulderMID && (bool)jniEnv->CallBooleanMethod(m_objectRef, m_getButtonRightShoulderMID));
}

bool JNIUtil::GetButtonStart()
{
    QAndroidJniEnvironment jniEnv;
    return (m_getButtonStartMID && (bool)jniEnv->CallBooleanMethod(m_objectRef, m_getButtonStartMID));
}

bool JNIUtil::GetButtonBack()
{
    QAndroidJniEnvironment jniEnv;
    return (m_getButtonBackMID && (bool)jniEnv->CallBooleanMethod(m_objectRef, m_getButtonBackMID));
}

void JNIUtil::SetMainWindow(MainWindow* w)
{
    main_window = w;
}

MainWindow * JNIUtil::GetMainWindow()
{
    return main_window;
}

void JNIUtil::SetMainThreadID(pid_t id)
{
    main_thread_id = id;
}

pid_t JNIUtil::GetMainThreadID()
{
    return main_thread_id;
}

void JNIUtil::ActivityOnPause()
{
    if(m_onPauseMID)
    {
        QAndroidJniEnvironment jniEnv;
        jniEnv->CallVoidMethod(m_objectRef, m_onPauseMID);
    }
}

void JNIUtil::ActivityOnResume()
{
    if(m_onResumeMID)
    {
        QAndroidJniEnvironment jniEnv;
        jniEnv->CallVoidMethod(m_objectRef, m_onResumeMID);
    }
}


#define JNI_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL              \
      Java_org_janus_JanusActivity_##method_name

namespace {
inline MainWindow *mainwindow() {
  return JNIUtil::GetMainWindow();
}
}  // anonymous namespace

extern "C"
{

JNI_METHOD(void, nativeExitVR)
() {
  if (mainwindow()) mainwindow()->ExitVR();
}

JNI_METHOD(void, nativeEnterVR)
() {
  if (mainwindow()) mainwindow()->EnterVR();
}

JNI_METHOD(void, nativePause)
() {
  if (mainwindow()) mainwindow()->Pause();
}

JNI_METHOD(void, nativeResume)
() {
  if (mainwindow()) mainwindow()->Resume();
}

}  // extern "C"
#endif
