package org.janus;

import android.Manifest;

import android.R;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.PixelFormat;
import android.hardware.input.InputManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.Vibrator;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.inputmethod.InputMethodManager;
import android.view.Gravity;
import android.view.Surface;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.WindowManager.LayoutParams;
import android.widget.FrameLayout;

public class JanusActivity extends org.qtproject.qt5.android.bindings.QtActivity
                           implements JoystickView.JoystickListener, ButtonView.ButtonListener, InputManager.InputDeviceListener
{
        private String launch_url = "";
        private boolean launch_in_vr = false;

        //WebView Manager
        private WebViewManager webViewManager = null;

        //Splash/Control handlers
        private SplashView splash_screen;
        private JoystickView walk_joystick;
        private JoystickView view_joystick;
        private ButtonView talk_button;
        private ButtonView jump_button; //Also flight button
        private float walk_x = 0;
        private float walk_y = 0;
        private float view_x = 0;
        private float view_y = 0;
        private boolean show_view_joystick = false;
        private boolean running = false;
        private boolean talking = false;
        private boolean always_talking = false;
        private boolean jumping = false;
        private boolean flying = false;

        //GoogleVR
        GVRManager gvrManager = null;

        //GearVR
        GearManager gearManager = null;

        //Gamepad flags
        private boolean gamepad_connected = false;
        private float left_stick_x = 0.0f;
        private float left_stick_y = 0.0f;
        private float right_stick_x = 0.0f;
        private float right_stick_y = 0.0f;
        private boolean dpad_up = false;
        private boolean dpad_down = false;
        private boolean dpad_left = false;
        private boolean dpad_right = false;
        private boolean button_x = false;
        private boolean button_y = false;
        private boolean button_a = false;
        private boolean button_b = false;
        private boolean button_left_thumb = false;
        private boolean button_right_thumb = false;
        private float trigger_left = 0.0f;
        private float trigger_right = 0.0f;
        private boolean button_left_shoulder = false;
        private boolean button_right_shoulder = false;
        private boolean button_start = false;
        private boolean button_back = false;

        private InputManager mInputManager;
        private InputDevice mInputDevice;

        public void onCreate(Bundle savedInstanceState){
            super.onCreate(savedInstanceState);

            mInputManager = JanusActivity.this.getSystemService(InputManager.class);
            mInputManager.registerInputDeviceListener(JanusActivity.this, null);

            //Intent example:
            //      <a href="intent:#Intent;action=org.janusvr.launchfrombrowser;category=android.intent.category.DEFAULT;category=android.intent.category.BROWSABLE;category=com.google.intent.category.DAYDREAM;category=com.google.intent.category.CARDBOARD;S.url=http://demos.janusvr.com/joseph/android/images/index.html;end">Click here to launch sample app</a>
            Intent intent = getIntent();
            if(intent != null && intent.getAction() != null && intent.getAction().equals("org.janusvr.launchfrombrowser")){
                Bundle bundle = intent.getExtras();
                if(bundle != null){
                        launch_url = bundle.getString("url");
                }
            }
            if(intent != null && intent.getBooleanExtra("android.intent.extra.VR_LAUNCH", false)){
                launch_in_vr = true;
            }

            webViewManager = new WebViewManager(JanusActivity.this);

            View decorView = getWindow().getDecorView();
            //int uiOptions = View.SYSTEM_UI_FLAG_LAYOUT_STABLE | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY| View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION;
            int uiOptions = View.SYSTEM_UI_FLAG_LAYOUT_STABLE; //| View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION;
            decorView.setSystemUiVisibility(uiOptions);

            DisplayMetrics displayMetrics = new DisplayMetrics();
            getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
            int size =  Math.min(displayMetrics.heightPixels, displayMetrics.widthPixels) / 3;


            FrameLayout mainLayout = (FrameLayout) findViewById(R.id.content);
            FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(ViewGroup.LayoutParams.FILL_PARENT, ViewGroup.LayoutParams.FILL_PARENT);

            splash_screen = new SplashView(this);
            splash_screen.setZOrderOnTop(true);
            splash_screen.setBitmaps(this,"assets/splash/splash.png","assets/splash/janusvr.png");
            splash_screen.getHolder().setFormat(PixelFormat.TRANSLUCENT);
            splash_screen.getHolder().setFixedSize(ViewGroup.LayoutParams.FILL_PARENT, ViewGroup.LayoutParams.FILL_PARENT);

            params.leftMargin = 0;
            params.bottomMargin = 0;
            params.gravity = Gravity.TOP | Gravity.LEFT;

            mainLayout.addView(splash_screen, params);

            walk_joystick = new JoystickView(this);
            walk_joystick.setZOrderOnTop(true);
            walk_joystick.getHolder().setFormat(PixelFormat.TRANSLUCENT);
            walk_joystick.getHolder().setFixedSize(size, size);

            params = new FrameLayout.LayoutParams(size, size);
            params.leftMargin = 0;
            params.bottomMargin = 0;
            params.gravity = Gravity.BOTTOM | Gravity.LEFT;

            mainLayout.addView(walk_joystick, params);

            talk_button = new ButtonView(this);
            talk_button.setZOrderOnTop(true);
            talk_button.setStates(this,"assets/icons/talk_pressed.png","assets/icons/talk_unpressed.png");
            talk_button.setDoubleTapState(this, "assets/icons/always_talk_unpressed.png", "assets/icons/always_talk_pressed.png");
            talk_button.getHolder().setFormat(PixelFormat.TRANSLUCENT);
            talk_button.getHolder().setFixedSize(3*size/4, 3*size/4);

            params = new FrameLayout.LayoutParams(3*size/4, 3*size/4);
            params.rightMargin = displayMetrics.widthPixels/2;
            params.bottomMargin = 0;
            params.gravity = Gravity.BOTTOM | Gravity.RIGHT;

            mainLayout.addView(talk_button, params);

            jump_button = new ButtonView(this);
            jump_button.setZOrderOnTop(true);
            jump_button.setStates(this, "assets/icons/jump_pressed.png","assets/icons/jump_unpressed.png");
            jump_button.setDoubleTapState(this, "assets/icons/flight_unpressed.png", "assets/icons/flight_pressed.png");
            jump_button.getHolder().setFormat(PixelFormat.TRANSLUCENT);
            jump_button.getHolder().setFixedSize(3*size/4, 3*size/4);

            params = new FrameLayout.LayoutParams(3*size/4, 3*size/4);
            params.rightMargin = displayMetrics.widthPixels/2 - 3*size/4;
            params.bottomMargin = 0;
            params.gravity = Gravity.BOTTOM | Gravity.RIGHT;

            mainLayout.addView(jump_button, params);

            view_joystick = new JoystickView(this);
            view_joystick.setZOrderOnTop(true);
            view_joystick.getHolder().setFormat(PixelFormat.TRANSLUCENT);
            view_joystick.getHolder().setFixedSize(size, size);

            params = new FrameLayout.LayoutParams(size, size);
            params.rightMargin = 0;
            params.bottomMargin = 0;
            params.gravity = Gravity.BOTTOM | Gravity.RIGHT;

            mainLayout.addView(view_joystick, params);

            splash_screen.setVisibility(View.VISIBLE);
            walk_joystick.setVisibility(View.INVISIBLE);
            view_joystick.setVisibility(View.INVISIBLE);
            talk_button.setVisibility(View.INVISIBLE);
            jump_button.setVisibility(View.INVISIBLE);

            gvrManager = new GVRManager(JanusActivity.this, splash_screen);
            gearManager = new GearManager(JanusActivity.this, splash_screen);
        }

        public void onDestroy(){
            super.onDestroy();
            gvrManager.onDestroy();
        }

        @Override
        public void onPause() {
            if (gvrManager != null)
                gvrManager.onPause();
            if (gearManager != null)
                gearManager.onPause();

            nativePause();
            webViewManager.setPaused(true);

            super.onPause();
        }

        @Override
        public void onResume() {
            super.onResume();

            nativeResume();
            webViewManager.setPaused(false);

            if (gvrManager != null)
                gvrManager.onResume();
            if (gearManager != null)
                gearManager.onResume();
        }

        @Override
        public void onBackPressed() {
            gvrManager.onBackPressed();
            gearManager.onBackPressed();

            super.onBackPressed();
        }

        public String getLaunchURL()
        {
            return launch_url;
        }

        public boolean getLaunchInVR()
        {
            return launch_in_vr;
        }

        public boolean getShowingVR()
        {
            return gearManager.getShowingVR() || gvrManager.getShowingVR();
        }

        //============================================================================================================
        //Splash
        //============================================================================================================

        public void setProgressBar(int i){
            if (splash_screen != null){
                if (i >= 95){
                    splash_screen.setProgress(100);
                }
                else if (i < 95){
                    splash_screen.setProgress(i);
                }
            }
        }

        public void setVRModeEnabled(boolean b){
            if (splash_screen != null){
                splash_screen.setVRModeEnabled(b);
            }
        }

        public void hideSplash(){
            Message msg = new Message();
            msg.what = 0;

            hideSplashHandler.sendMessage(msg);
        }

        protected Handler hideSplashHandler = new Handler() {
            public void handleMessage(Message msg) {
                if (splash_screen != null && msg.what == 0){
                    splash_screen.setVisibility(View.GONE);
                    FrameLayout mainLayout = (FrameLayout) findViewById(R.id.content);
                    if (splash_screen.getParent() == mainLayout) {
                        mainLayout.removeView(splash_screen);

                        splash_screen.stop();
                    }
                }
            }
        };

        //============================================================================================================
        //Permissions
        //============================================================================================================

        private boolean answered_permissions = false;

        @Override
        public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
            switch (requestCode) {
                case 0: {
                    answered_permissions = true;
                    return;
                }
            }
        }

        public boolean getPermissions(){
            return this.checkSelfPermission(Manifest.permission.RECORD_AUDIO) == PackageManager.PERMISSION_GRANTED &&
                this.checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED &&
                this.checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED;
        }

        public boolean getAnsweredPermissions(){
            return answered_permissions;
        }

        public void askPermissions(){
            Message msg = new Message();
            msg.what = 0;

            askPermissionsHandler.sendMessage(msg);
        }

        protected Handler askPermissionsHandler = new Handler() {
            public void handleMessage(Message msg) {
                if (msg.what == 0){
                    //Ask for permissions
                    if (!getPermissions()){
                        JanusActivity.this.requestPermissions(
                                new String[]{
                                    Manifest.permission.RECORD_AUDIO,
                                    Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.WRITE_EXTERNAL_STORAGE},
                                0);
                    }
                    else{
                        answered_permissions = true;
                    }
                }
            }
        };

        //============================================================================================================
        //WebView
        //============================================================================================================

        public void createNewWebView(int tag) {
            webViewManager.createNewWebView(tag);
        }

        public void removeWebView(int tag) {
            webViewManager.removeWebView(tag);
        }

        public void moveWebView(int tag, int x, int y) {
            webViewManager.moveWebView(tag,x,y);
        }

        public void resizeWebView(int tag, int w, int h) {
            webViewManager.resizeWebView(tag,w,h);
        }

        public void attachWebViewToMainLayout(int tag) {
            webViewManager.attachWebViewToMainLayout(tag);
        }

        public void loadUrlAtWebView(int tag, String url) {
            webViewManager.loadUrlAtWebView(tag,url);
        }

        public void goBackOrForwardAtWebView(int tag, int steps) {
            webViewManager.goBackOrForwardAtWebView(tag,steps);
        }

        public void reloadWebView(int tag) {
            webViewManager.reloadWebView(tag);
        }

        public void stopWebView(int tag) {
            webViewManager.stopWebView(tag);
        }

        public void loadHtmlAtWebView(int tag, String html) {
            webViewManager.loadHtmlAtWebView(tag,html);
        }

        public void evaluateJavaScriptAtWebView(int tag, String javascript) {
            webViewManager.evaluateJavaScriptAtWebView(tag,javascript);
        }

        public void setUpdatesEnabledWebView(int tag, boolean b) {
            webViewManager.setUpdatesEnabledWebView(tag,b);
        }

        public Bitmap getBitmap(int tag) {
            return webViewManager.getBitmap(tag);
        }

        public int getTexture(int tag) {
            return webViewManager.getTexture(tag);
        }

        public String getHitTestContent(int tag) {
            return webViewManager.getHitTestContent(tag);
        }

        public int getHorizontalScrollRange(int tag) {
            return webViewManager.getHorizontalScrollRange(tag);
        }

        public int getVerticalScrollRange(int tag) {
            return webViewManager.getVerticalScrollRange(tag);
        }

        public int getWidthWebView(int tag) {
            return webViewManager.getWidthWebView(tag);
        }

        public int getHeightWebView(int tag) {
            return webViewManager.getHeightWebView(tag);
        }

        public int getScrollXWebView(int tag) {
            return webViewManager.getScrollXWebView(tag);
        }

        public int getScrollYWebView(int tag) {
            return webViewManager.getScrollYWebView(tag);
        }

        public String getURLWebView(int tag) {
            return webViewManager.getURLWebView(tag);
        }

        public void scrollByWebView(int tag, int x, int y) {
            webViewManager.scrollByWebView(tag,x,y);
        }

        public void scrollToWebView(int tag, int x, int y) {
            webViewManager.scrollToWebView(tag,x,y);
        }

        public void mousePressWebView(int tag, int x, int y) {
            webViewManager.mousePressWebView(tag,x,y);
        }

        public void mouseMoveWebView(int tag, int x, int y) {
            webViewManager.mouseMoveWebView(tag,x,y);
        }

        public void mouseReleaseWebView(int tag, int x, int y) {
            webViewManager.mouseReleaseWebView(tag,x,y);
        }

        public boolean getRepaintRequestedAtWebView(int tag) {
            return webViewManager.getRepaintRequestedAtWebView(tag);
        }

        public boolean getScrollRequestedAtWebView(int tag) {
            return webViewManager.getScrollRequestedAtWebView(tag);
        }

        public boolean getURLChangedAtWebView(int tag) {
            return webViewManager.getURLChangedAtWebView(tag);
        }

        //============================================================================================================
        //Controls
        //============================================================================================================

        public void onJoystickMoved(float x, float y, boolean b, int id){
            if (id == walk_joystick.getId()){
                walk_x = x;
                walk_y = y;
                running = b;
            }
            if (id == view_joystick.getId()){
                view_x = x;
                view_y = y;
                //running = b;
            }
        }

        public void onButtonPressed(boolean switched, int id){
            if (id == talk_button.getId()){
                talking = true;
            }
            if (id == jump_button.getId()){
                jumping = true;
            }
        }

        public void onButtonReleased(boolean switched, int id){
            if (id == talk_button.getId()){
                talking = false;
            }
            if (id == jump_button.getId()){
                jumping = false;
            }
        }

        public void onButtonDoubleTapped(boolean switched, int id){
            if (id == jump_button.getId()){
                if (!switched)
                {
                    jumping = false;
                    flying = false;
                }
                else
                {
                    flying = true;
                    jumping = false;
                }
            }
            if (id == talk_button.getId()){
                if (!switched)
                {
                    talking = false;
                    always_talking = false;
                }
                else
                {
                    always_talking = true;
                    talking = false;
                }
            }
        }

        public int getScreenOrientation(){
            int angle = 90;
            /*int rotation =  getWindowManager().getDefaultDisplay().getRotation();
            switch (rotation) {
                case Surface.ROTATION_90:
                    angle = 90;
                    break;
                case Surface.ROTATION_180:
                    angle = 180;
                    break;
                case Surface.ROTATION_270:
                    angle = 270;
                    break;
                default:
                    angle = 0;
                    break;
            }*/
            return angle;
        }

        public float getWalkJoystickX(){
            return walk_x;
        }

        public float getWalkJoystickY(){
            return walk_y;
        }

        public float getViewJoystickX(){
            return view_x;
        }

        public float getViewJoystickY(){
            return view_y;
        }

        public boolean getRunning(){
            return running;
        }

        public boolean getSpeaking(){
            return talking;
        }

        public boolean getAlwaysSpeaking(){
            return always_talking;
        }


        public boolean getJumping(){
            return jumping;
        }

        public boolean getFlying(){
            return flying;
        }

        public void setButtonMargin(int margin){
            Message msg = new Message();
            msg.what = margin;

            moveButtonMarginHandler.sendMessage(msg);
        }

        protected Handler moveButtonMarginHandler = new Handler() {
                public void handleMessage(Message msg) {
                    DisplayMetrics displayMetrics = new DisplayMetrics();
                    getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
                    int size =  Math.min(displayMetrics.heightPixels, displayMetrics.widthPixels) / 3;

                    FrameLayout mainLayout = (FrameLayout) findViewById(R.id.content);
                    FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(size, size);

                    float i = (Math.max(msg.what, 0) == 0) ? 1.0f/2.0f : 2.0f/3.0f - 80.0f/((float)displayMetrics.widthPixels);
                    int margin_value =  (int) (displayMetrics.widthPixels * i);

                    params = new FrameLayout.LayoutParams(3*size/4, 3*size/4);
                    if (show_view_joystick)
                        params.rightMargin = margin_value - size/16; // + Math.max(msg.what, 0);
                    else
                        params.rightMargin = 5*size/8 + Math.max(msg.what, 0);
                    params.bottomMargin = 0;
                    params.gravity = Gravity.BOTTOM | Gravity.RIGHT;

                    talk_button.setLayoutParams(params);

                    params = new FrameLayout.LayoutParams(3*size/4, 3*size/4);
                    if (show_view_joystick)
                        params.rightMargin = margin_value - 3*size/4 + size/16; // + Math.max(msg.what, 0);
                    else
                        params.rightMargin = Math.max(msg.what, 0);
                    params.bottomMargin = 0;
                    params.gravity = Gravity.BOTTOM | Gravity.RIGHT;

                    jump_button.setLayoutParams(params);

                    params = new FrameLayout.LayoutParams(size, size);
                    params.rightMargin = Math.max(msg.what, 0);
                    params.bottomMargin = 0;
                    params.gravity = Gravity.BOTTOM | Gravity.RIGHT;

                    view_joystick.setLayoutParams(params);
                }
        };

        public void setControlsVisible(boolean b, boolean s){
            Message msg = new Message();
            msg.what = b?1:0;

            show_view_joystick = s;

            setControlsVisibleHandler.sendMessage(msg);
        }

        protected Handler setControlsVisibleHandler = new Handler() {
                public void handleMessage(Message msg) {
                    if (msg.what == 0) {
                        walk_joystick.setVisibility(View.INVISIBLE);
                        view_joystick.setVisibility(View.INVISIBLE);
                        talk_button.setVisibility(View.INVISIBLE);
                        jump_button.setVisibility(View.INVISIBLE);
                    }
                    else {
                        walk_joystick.setVisibility(View.VISIBLE);
                        if (show_view_joystick)
                            view_joystick.setVisibility(View.VISIBLE);
                        else
                            view_joystick.setVisibility(View.INVISIBLE);
                        talk_button.setVisibility(View.VISIBLE);
                        jump_button.setVisibility(View.VISIBLE);
                    }

                    if (getShowingVR() || getGamepadConnected())
                    {
                        InputMethodManager imm = (InputMethodManager) getSystemService(Activity.INPUT_METHOD_SERVICE);
                        View view = getCurrentFocus();
                        if (view == null) {
                            return;
                        }
                        imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
                    }
                }
        };

        public void hideKeyboard(){
            Message msg = new Message();

            hideKeyboardHandler.sendMessage(msg);
        }

        protected Handler hideKeyboardHandler = new Handler() {
                public void handleMessage(Message msg) {
                    InputMethodManager imm = (InputMethodManager) getSystemService(Activity.INPUT_METHOD_SERVICE);
                    View view = getCurrentFocus();
                    if (view == null) {
                        return;
                    }
                    imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
                }
        };

        //============================================================================================================
        //GoogleVR
        //============================================================================================================

        public void setupGvr()
        {
            gvrManager.setupGvr();
        }

        public long getGvrContext()
        {
            return gvrManager.getGvrContext();
        }

        public void showGVR(boolean b)
        {
            gvrManager.showGVR(b);
        }

        //============================================================================================================
        //GearVR
        //============================================================================================================

        public void setupGear()
        {
            gearManager.setupGear();
        }

        public boolean getGearReady()
        {
            return gearManager.getGearReady();
        }

        public void showGear(boolean b)
        {
            gearManager.showGear(b);
        }

        public Surface getWindowSurface() {
            return gearManager.getWindowSurface();
        }

        //============================================================================================================
        //Gamepad methods
        //============================================================================================================

        public boolean getGamepadConnected() {
            boolean connected = false;
            int[] devices = mInputManager.getInputDeviceIds();
            for (int i = 0; i < devices.length; i++) {
                if ((mInputManager.getInputDevice(devices[i]).getSources() & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD
                || (mInputManager.getInputDevice(devices[i]).getSources() & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK) {
                    connected = true;
                    break;
                }
            }
            gamepad_connected = connected;
            return gamepad_connected;
        }

        public float getLeftStickX() {
            return left_stick_x;
        }

        public float getLeftStickY() {
            return left_stick_y;
        }

        public float getRightStickX() {
            return right_stick_x;
        }

        public float getRightStickY() {
            return right_stick_y;
        }

        public boolean getDpadUp() {
            return dpad_up;
        }

        public boolean getDpadDown() {
            return dpad_down;
        }

        public boolean getDpadLeft() {
            return dpad_left;
        }

        public boolean getDpadRight() {
            return dpad_right;
        }

        public boolean getButtonX() {
            return button_x;
        }

        public boolean getButtonY() {
            return button_y;
        }

        public boolean getButtonA() {
            return button_a;
        }

        public boolean getButtonB() {
            return button_b;
        }

        public boolean getButtonLeftThumb() {
            return button_left_thumb;
        }

        public boolean getButtonRightThumb() {
            return button_right_thumb;
        }

        public float getTriggerLeft() {
            return trigger_left;
        }

        public float getTriggerRight() {
            return trigger_right;
        }

        public boolean getButtonLeftShoulder() {
            return button_left_shoulder;
        }

        public boolean getButtonRightShoulder() {
            return button_right_shoulder;
        }

        public boolean getButtonStart() {
            return button_start;
        }

        public boolean getButtonBack() {
            return button_back;
        }

        @Override
        public boolean onKeyDown(int keyCode, KeyEvent event) {
            int deviceId = event.getDeviceId();

            if (deviceId != -1) {
                // Handle keys going up.
                boolean handled = false;
                switch (keyCode) {
                    case KeyEvent.KEYCODE_DPAD_LEFT:
                        if ((event.getSource() & InputDevice.SOURCE_DPAD) == InputDevice.SOURCE_DPAD) {
                            dpad_left = true;
                            handled = true;
                        }
                        break;
                    case KeyEvent.KEYCODE_DPAD_RIGHT:
                        if ((event.getSource() & InputDevice.SOURCE_DPAD) == InputDevice.SOURCE_DPAD) {
                            dpad_right = true;
                            handled = true;
                        }
                        break;
                    case KeyEvent.KEYCODE_DPAD_UP:
                        if ((event.getSource() & InputDevice.SOURCE_DPAD) == InputDevice.SOURCE_DPAD) {
                            dpad_up = true;
                            handled = true;
                        }
                        break;
                    case KeyEvent.KEYCODE_DPAD_DOWN:
                        if ((event.getSource() & InputDevice.SOURCE_DPAD) == InputDevice.SOURCE_DPAD) {
                            dpad_down = true;
                            handled = true;
                        }
                        break;
                    case KeyEvent.KEYCODE_BUTTON_X:
                        button_x = true;
                        handled = true;
                        break;
                    case KeyEvent.KEYCODE_BUTTON_Y:
                        button_y = true;
                        handled = true;
                        break;
                    case KeyEvent.KEYCODE_BUTTON_A:
                        button_a = true;
                        handled = true;
                        break;
                    case KeyEvent.KEYCODE_BUTTON_B:
                        button_b = true;
                        handled = true;
                        break;
                    case KeyEvent.KEYCODE_BUTTON_L1:
                        button_left_shoulder = true;
                        handled = true;
                        break;
                    case KeyEvent.KEYCODE_BUTTON_R1:
                        button_right_shoulder = true;
                        handled = true;
                        break;
                    case KeyEvent.KEYCODE_BUTTON_L2:
                        trigger_left = 1.0f;
                        handled = true;
                        break;
                    case KeyEvent.KEYCODE_BUTTON_R2:
                        trigger_right = 1.0f;
                        handled = true;
                        break;
                    case KeyEvent.KEYCODE_BUTTON_THUMBL:
                        button_left_thumb = true;
                        handled = true;
                        break;
                    case KeyEvent.KEYCODE_BUTTON_THUMBR:
                        button_right_thumb = true;
                        handled = true;
                        break;
                    case KeyEvent.KEYCODE_BUTTON_START:
                        button_start = true;
                        handled = true;
                        break;
                    case KeyEvent.KEYCODE_BUTTON_SELECT:
                        button_back = true;
                        handled = true;
                        break;
                    case KeyEvent.KEYCODE_BACK:
                        button_back = true;
                        handled = true;
                        break;
                    default:
                        break;
                }
                return handled;
            }

            return super.onKeyDown(keyCode, event);
        }

        @Override
        public boolean onKeyUp(int keyCode, KeyEvent event) {
            int deviceId = event.getDeviceId();
            if (deviceId != -1) {
                boolean handled = false;
                switch (keyCode) {
                    case KeyEvent.KEYCODE_DPAD_LEFT:
                        if ((event.getSource() & InputDevice.SOURCE_DPAD) == InputDevice.SOURCE_DPAD) {
                            dpad_left = false;
                            handled = true;
                        }
                        break;
                    case KeyEvent.KEYCODE_DPAD_RIGHT:
                        if ((event.getSource() & InputDevice.SOURCE_DPAD) == InputDevice.SOURCE_DPAD) {
                            dpad_right = false;
                            handled = true;
                        }
                        break;
                    case KeyEvent.KEYCODE_DPAD_UP:
                        if ((event.getSource() & InputDevice.SOURCE_DPAD) == InputDevice.SOURCE_DPAD) {
                            dpad_up = false;
                            handled = true;
                        }
                        break;
                    case KeyEvent.KEYCODE_DPAD_DOWN:
                        if ((event.getSource() & InputDevice.SOURCE_DPAD) == InputDevice.SOURCE_DPAD) {
                            dpad_down = false;
                            handled = true;
                        }
                        break;
                    case KeyEvent.KEYCODE_BUTTON_X:
                        button_x = false;
                        handled = true;
                        break;
                    case KeyEvent.KEYCODE_BUTTON_Y:
                        button_y = false;
                        handled = true;
                        break;
                    case KeyEvent.KEYCODE_BUTTON_A:
                        button_a = false;
                        handled = true;
                        break;
                    case KeyEvent.KEYCODE_BUTTON_B:
                        button_b = false;
                        handled = true;
                        break;
                    case KeyEvent.KEYCODE_BUTTON_L1:
                        button_left_shoulder = false;
                        handled = true;
                        break;
                    case KeyEvent.KEYCODE_BUTTON_R1:
                        button_right_shoulder = false;
                        handled = true;
                        break;
                    case KeyEvent.KEYCODE_BUTTON_L2:
                        trigger_left = 0.0f;
                        handled = true;
                        break;
                    case KeyEvent.KEYCODE_BUTTON_R2:
                        trigger_right = 0.0f;
                        handled = true;
                        break;
                    case KeyEvent.KEYCODE_BUTTON_THUMBL:
                        button_left_thumb = false;
                        handled = true;
                        break;
                    case KeyEvent.KEYCODE_BUTTON_THUMBR:
                        button_right_thumb = false;
                        handled = true;
                        break;
                    case KeyEvent.KEYCODE_BUTTON_START:
                        button_start = false;
                        handled = true;
                        break;
                    case KeyEvent.KEYCODE_BUTTON_SELECT:
                        button_back = false;
                        handled = true;
                        break;
                    case KeyEvent.KEYCODE_BACK:
                        button_back = false;
                        handled = true;
                        break;
                    default:
                        break;
                }
                return handled;
            }

            return super.onKeyUp(keyCode, event);
        }

        @Override
        public boolean onGenericMotionEvent(MotionEvent event) {
            //mInputManager.onGenericMotionEvent(event);
            int deviceId = event.getDeviceId();

            int eventSource = event.getSource();
            if ((((eventSource & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD) ||
                    ((eventSource & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK))
                    && event.getAction() == MotionEvent.ACTION_MOVE) {
                int id = event.getDeviceId();
                if (-1 != id) {
                    mInputDevice = event.getDevice();

                    left_stick_x = getCenteredAxis(event, mInputDevice, MotionEvent.AXIS_X);
                    left_stick_y = -getCenteredAxis(event, mInputDevice, MotionEvent.AXIS_Y);
                    right_stick_x = getCenteredAxis(event, mInputDevice, MotionEvent.AXIS_Z);
                    right_stick_y = -getCenteredAxis(event, mInputDevice, MotionEvent.AXIS_RZ);

                    if (event.getAxisValue(MotionEvent.AXIS_LTRIGGER) != 0.0f)
                        trigger_left = event.getAxisValue(MotionEvent.AXIS_LTRIGGER);

                    if (event.getAxisValue(MotionEvent.AXIS_RTRIGGER) != 0.0f)
                        trigger_right = event.getAxisValue(MotionEvent.AXIS_RTRIGGER);

                    if (event.getAxisValue(MotionEvent.AXIS_HAT_Y) == -1.0f) {
                        dpad_up = true;
                    }
                    else if (event.getAxisValue(MotionEvent.AXIS_HAT_Y) == 1.0f) {
                        dpad_down = true;
                    }
                    else if (event.getAxisValue(MotionEvent.AXIS_HAT_Y) == 0.0f) {
                        dpad_up = false;
                        dpad_down = false;
                    }

                    if (event.getAxisValue(MotionEvent.AXIS_HAT_X) == -1.0f) {
                        dpad_left = true;
                    }
                    else if (event.getAxisValue(MotionEvent.AXIS_HAT_X) == 1.0f) {
                        dpad_right = true;
                    }
                    else if (event.getAxisValue(MotionEvent.AXIS_HAT_X) == 0.0f) {
                        dpad_left = false;
                        dpad_right = false;
                    }
                }
            }
            else {
                left_stick_x = 0.0f;
                left_stick_y = 0.0f;
                right_stick_x = 0.0f;
                right_stick_y = 0.0f;
            }

            return super.onGenericMotionEvent(event);
        }

        private static float getCenteredAxis(MotionEvent event, InputDevice device,
                int axis) {
            final InputDevice.MotionRange range = device.getMotionRange(axis, event.getSource());
            if (range != null) {
                final float flat = range.getFlat();
                final float value = event.getAxisValue(axis);

                // Ignore axis values that are within the 'flat' region of the
                // joystick axis center.
                // A joystick at rest does not always report an absolute position of
                // (0,0).
                if (Math.abs(value) > flat) {
                    return value;
                }
            }
            return 0;
        }

        @SuppressLint("NewApi")
        private final Vibrator getVibrator() {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN &&
                    null != mInputDevice) {
                return mInputDevice.getVibrator();
            }
            return null;
        }

        private void vibrateController(int time) {
            Vibrator vibrator = getVibrator();
            if (null != vibrator) {
                vibrator.vibrate(time);
            }
        }

        @Override
        public void onInputDeviceAdded(int deviceId) {
        }

        @Override
        public void onInputDeviceChanged(int deviceId) {
        }

        @Override
        public void onInputDeviceRemoved(int deviceId) {
        }

        //============================================================================================================
        //Native methods
        //============================================================================================================

        public native void nativeExitVR();
        public native void nativeEnterVR();
        public native void nativePause();
        public native void nativeResume();
}
