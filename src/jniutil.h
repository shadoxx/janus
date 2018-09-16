#ifndef JNIUTIL_H
#define JNIUTIL_H

#ifdef __ANDROID__
#include <QtGui>
#include <QtCore>

#include <jni.h>
#include <QtAndroidExtras/QtAndroidExtras>
#include <QAndroidJniEnvironment>
#include <QAndroidJniObject>
#include <android/bitmap.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <unistd.h>

#include "mathutil.h"
#include "rendererinterface.h"
#include "cookiejar.h"
#include "abstractwebview.h"

class MainWindow;

class JNIUtil
{

public:

    JNIUtil();
    ~JNIUtil();

    static int64_t GetTimestampNsec();

    static void Initialize();
    static void Destroy();

    static QString GetLaunchURL();
    static bool GetLaunchInVR();
    static bool GetPermissions();
    static bool GetAnsweredPermissions();
    static void AskPermissions();

    static void SetVRModeEnabled(bool b);
    static void HideSplash();
    static void SetProgressBar(int i);

    static void UpdateCookies();

    static void CreateNewWebView(int tag);
    static void RemoveWebView(int tag);
    static void AttachWebViewToMainLayout(int tag);
    static void MoveWebView(int tag, int x, int y);
    static void ResizeWebView(int tag, int x, int y);
    static void LoadUrlAtWebView(int tag, QUrl url);
    static void GoBackOrForwardAtWebView(int tag, int b) ;
    static void ReloadWebView(int tag);
    static void StopWebView(int tag);
    static void LoadHtmlAtWebView(int tag, QString s);
    static void SetUpdatesEnabledWebView(int tag, bool b);
    static void EvaluateJavaScriptAtWebView(int tag, QString s);
    static void UpdateTextureWebView(int tag, TextureHandle * m_texture_handle);
    static WebHitTestResult GetHitTestContentWebView(int tag);
    static int GetHorizontalScrollRange(int tag);
    static int GetVerticalScrollRange(int tag);
    static int GetWidthWebView(int tag);
    static int GetHeightWebView(int tag);
    static int GetScrollXWebView(int tag);
    static int GetScrollYWebView(int tag);
    static QString GetURLWebView(int tag);
    static void ScrollByWebView(int tag, int x, int y);
    static void ScrollToWebView(int tag, int x, int y);
    static void MousePressWebView(int tag, int x, int y);
    static void MouseMoveWebView(int tag, int x, int y);
    static void MouseReleaseWebView(int tag, int x, int y);
    static void KeyPressWebView(int tag, int code, int state);
    static void KeyReleaseWebView(int tag, int code, int state);
    static bool GetRepaintRequestedAtWebView(int tag);
    static bool GetScrollRequestedAtWebView(int tag);
    static bool GetURLChangedAtWebView(int tag);

    static int GetScreenOrientation();
    static void SetButtonMargin(int margin);
    static void SetControlsVisible(bool b, bool show_view_joystick);
    static void HideKeyboard();
    static float GetWalkJoystickX();
    static float GetWalkJoystickY();
    static float GetViewJoystickX();
    static float GetViewJoystickY();
    static bool GetRunning();
    static bool GetSpeaking();
    static bool GetAlwaysSpeaking();
    static bool GetJumping();
    static bool GetFlying();

    static long long GetGVRContext();
    static bool GetGearReady();
    static void SetupGVR();
    static void SetupGear();
    static void ShowGVR(bool b);
    static void ShowGear(bool b);
    static bool GetShowingVR();
    static jobject GetWindowSurface();

    static bool GetGamepadConnected();
    static float GetLeftStickX();
    static float GetLeftStickY();
    static float GetRightStickX();
    static float GetRightStickY();
    static bool GetDpadUp();
    static bool GetDpadDown();
    static bool GetDpadLeft();
    static bool GetDpadRight();
    static bool GetButtonX();
    static bool GetButtonY();
    static bool GetButtonA();
    static bool GetButtonB();
    static bool GetButtonLeftThumb();
    static bool GetButtonRightThumb();
    static float GetTriggerLeft();
    static float GetTriggerRight();
    static bool GetButtonLeftShoulder();
    static bool GetButtonRightShoulder();
    static bool GetButtonStart();
    static bool GetButtonBack();

    static void SetMainWindow(MainWindow* w);
    static MainWindow * GetMainWindow();

    static void SetMainThreadID(pid_t id);
    static pid_t GetMainThreadID();

    static void ActivityOnPause();
    static void ActivityOnResume();

private:

    static jmethodID m_getLaunchInVRMID;
    static jmethodID m_getLaunchURLMID;
    static jmethodID m_getPermissionsMID;
    static jmethodID m_getAnsweredPermissionsMID;
    static jmethodID m_askPermissionsMID;
    static jmethodID m_setVRModeEnabledMID;
    static jmethodID m_hideSplashMID;
    static jmethodID m_setProgressBarMID;

    static jmethodID m_getCookieMID;
    static jmethodID m_createNewWebViewMID;
    static jmethodID m_removeWebViewMID;
    static jmethodID m_attachWebViewToMainLayoutMID;
    static jmethodID m_moveWebViewMID;
    static jmethodID m_resizeWebViewMID;
    static jmethodID m_loadUrlAtWebViewMID;
    static jmethodID m_goBackOrForwardAtWebViewMID;
    static jmethodID m_reloadMID;
    static jmethodID m_stopMID;
    static jmethodID m_loadHtmlAtWebViewMID;
    static jmethodID m_getBitmapMID;
    static jmethodID m_getHitTestContentMID;
    static jmethodID m_getHorizontalScrollRangeMID;
    static jmethodID m_getVerticalScrollRangeMID;
    static jmethodID m_getWidthWebViewMID;
    static jmethodID m_getHeightWebViewMID;
    static jmethodID m_getScrollXWebViewMID;
    static jmethodID m_getScrollYWebViewMID;
    static jmethodID m_getURLWebViewMID;
    static jmethodID m_scrollByWebViewMID;
    static jmethodID m_scrollToWebViewMID;
    static jmethodID m_evaluateJavaScriptAtWebViewMID;
    static jmethodID m_setUpdatesEnabledWebViewMID;
    static jmethodID m_mousePressWebViewMID;
    static jmethodID m_mouseMoveWebViewMID;
    static jmethodID m_mouseReleaseWebViewMID;
    static jmethodID m_keyPressWebViewMID;
    static jmethodID m_keyReleaseWebViewMID;
    static jmethodID m_getRepaintRequestedAtWebViewMID;
    static jmethodID m_getScrollRequestedAtWebViewMID;
    static jmethodID m_getURLChangedAtWebViewMID;

    static jmethodID m_getScreenOrientationMID;
    static jmethodID m_setButtonMarginMID;
    static jmethodID m_setControlsVisibleMID;
    static jmethodID m_hideKeyboardMID;
    static jmethodID m_getWalkJoystickXMID;
    static jmethodID m_getWalkJoystickYMID;
    static jmethodID m_getViewJoystickXMID;
    static jmethodID m_getViewJoystickYMID;
    static jmethodID m_getRunningMID;
    static jmethodID m_getSpeakingMID;
    static jmethodID m_getAlwaysSpeakingMID;
    static jmethodID m_getJumpingMID;
    static jmethodID m_getFlyingMID;
    static jmethodID m_onPauseMID;
    static jmethodID m_onResumeMID;

    static jmethodID m_getGVRContextMID;
    static jmethodID m_getGearReadyMID;
    static jmethodID m_setupGVRMID;
    static jmethodID m_setupGearMID;
    static jmethodID m_showGVRMID;
    static jmethodID m_showGearMID;
    static jmethodID m_getShowingVRMID;
    static jmethodID m_getWindowSurfaceMID;

    static jmethodID m_getGamepadConnectedMID;
    static jmethodID m_getLeftStickXMID;
    static jmethodID m_getLeftStickYMID;
    static jmethodID m_getRightStickXMID;
    static jmethodID m_getRightStickYMID;
    static jmethodID m_getDpadUpMID;
    static jmethodID m_getDpadDownMID;
    static jmethodID m_getDpadLeftMID;
    static jmethodID m_getDpadRightMID;
    static jmethodID m_getButtonXMID;
    static jmethodID m_getButtonYMID;
    static jmethodID m_getButtonAMID;
    static jmethodID m_getButtonBMID;
    static jmethodID m_getButtonLeftThumbMID;
    static jmethodID m_getButtonRightThumbMID;
    static jmethodID m_getTriggerLeftMID;
    static jmethodID m_getTriggerRightMID;
    static jmethodID m_getButtonLeftShoulderMID;
    static jmethodID m_getButtonRightShoulderMID;
    static jmethodID m_getButtonStartMID;
    static jmethodID m_getButtonBackMID;

    static jobject m_objectRef;
    static jclass cls;

    static bool gear_initialized;

    static MainWindow * main_window;
    static pid_t main_thread_id;
};
#endif
#endif // JNIUTIL_H
