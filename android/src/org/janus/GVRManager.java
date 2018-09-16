package org.janus;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.opengles.GL10;

import android.R;
import android.app.Activity;
import android.app.ActionBar;
import android.content.Context;
import android.opengl.GLSurfaceView;
import android.opengl.GLES20;
import android.opengl.GLES31;
import android.opengl.EGL14;
import android.os.Handler;
import android.os.Message;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager.LayoutParams;
import android.widget.FrameLayout;

import com.google.vr.ndk.base.AndroidCompat;
import com.google.vr.ndk.base.GvrLayout;

public class GVRManager
{
        Context context;
        SplashView splash_screen;
        boolean gvr_setup = false;
        protected long gvrContext = 0;
        protected GLSurfaceView surfaceView = null;
        protected GvrLayout gvrLayout = null;
        boolean showing_vr = false;
        boolean gvr_was_paused = false;

        public GVRManager(Context c, SplashView s)
        {
            this.context = c;
            splash_screen = s;
        }

        public void setupGvr()
        {
            Message msg = new Message();
            setupGvrHandler.sendMessage(msg);
        }

        protected Handler setupGvrHandler = new Handler() {
                public void handleMessage(Message msg) {
                    // Create and configure the GvrLayout.
                    surfaceView = new GLSurfaceView(context);
                    surfaceView.setEGLContextClientVersion(3);
                    surfaceView.setEGLConfigChooser(8, 8, 8, 8, 24, 8);
                    surfaceView.setPreserveEGLContextOnPause(true);
                    surfaceView.setRenderer(
                        new GLSurfaceView.Renderer() {
                          @Override
                          public void onSurfaceCreated(GL10 gl, EGLConfig config) {
                              //GLES31.glClear(GLES31.GL_COLOR_BUFFER_BIT | GLES31.GL_DEPTH_BUFFER_BIT);
                              //GLES31.glClearColor(0, 0, 0, 1f);
                          }

                          @Override
                          public void onSurfaceChanged(GL10 gl, int width, int height) {}

                          @Override
                          public void onDrawFrame(GL10 gl) {
                              //GLES31.glClear(GLES31.GL_COLOR_BUFFER_BIT | GLES31.GL_DEPTH_BUFFER_BIT);
                              //GLES31.glClearColor(0, 0, 1, .5f);
                              //nativeDrawFrame();
                          }
                        });
                    surfaceView.setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);
                    gvrLayout = new GvrLayout(context);
                    gvrLayout.setPresentationView(surfaceView);

                    // Enable async reprojection for low-latency rendering on supporting
                    // devices. Note that this must be set prior to calling initialize_gl()
                    // on the native gvr_context.
                    gvrLayout.setAsyncReprojectionEnabled(true);

                    // Set up the necessary VR settings.
                    //boolean vrModeEnabled = AndroidCompat.setVrModeEnabled(JanusActivity.this, true);
                    //AndroidCompat.setSustainedPerformanceMode(JanusActivity.this, true);

                    // Plumb the native gvr_context to the native rendering code.
                    gvrContext = gvrLayout.getGvrApi().getNativeGvrContext();

                    gvrLayout.setVisibility(View.INVISIBLE);

                    gvrLayout.getUiLayout().setCloseButtonListener(
                        new Runnable() {
                            @Override
                            public void run() {
                                ((JanusActivity)context).nativeExitVR();
                            }
                    });

                    if (surfaceView != null)
                        surfaceView.onPause();
                    if (gvrLayout != null)
                        gvrLayout.onPause();

                    gvr_setup = true;
                }
        };

        public boolean getShowingVR()
        {
            return showing_vr;
        }

        public long getGvrContext()
        {
            int count = 0;
            while(!gvr_setup && count < 10){
                try {
                    Thread.sleep(50);
                } catch (Exception e) {
                }
                count++;

                if (count == 10) gvrContext = 0;
            }
            return gvrContext;
        }

        public void showGVR(boolean b)
        {
            Message msg = new Message();
            if (b){
                msg.what = 1;

                showing_vr = true;
                AndroidCompat.setVrModeEnabled((Activity) context, true);
            }
            else{
                msg.what = 0;

                showing_vr = false;
                AndroidCompat.setVrModeEnabled((Activity) context, false);
            }
            setGVRLayoutHandler.sendMessage(msg);
        }

        protected Handler setGVRLayoutHandler = new Handler() {
                public void handleMessage(Message msg) {
                    //Log.i("SETVRLAYOUTHANDLER","HERE");
                    if (msg.what == 0) {
                        if (gvrLayout != null && surfaceView != null)
                        {
                            splash_screen.setVisibility(View.VISIBLE);
                            FrameLayout mainLayout = (FrameLayout) ((Activity) context).findViewById(R.id.content);
                            FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(ViewGroup.LayoutParams.FILL_PARENT, ViewGroup.LayoutParams.FILL_PARENT);

                            params.leftMargin = 0;
                            params.bottomMargin = 0;
                            params.gravity = Gravity.TOP | Gravity.LEFT;

                            if (splash_screen.getParent() != mainLayout) {
                                mainLayout.addView(splash_screen, params);
                                splash_screen.start();
                                splash_screen.drawSplash();
                            }

                            gvrLayout.onPause();
                            surfaceView.onPause();

                            //setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR);
                            View decorView = ((Activity) context).getWindow().getDecorView();
                            int uiOptions = View.SYSTEM_UI_FLAG_LAYOUT_STABLE;
                            decorView.setSystemUiVisibility(uiOptions);

                            gvrLayout.setVisibility(View.INVISIBLE);
                            //Log.i("SETVRLAYOUTHANDLER","INVISIBLE");

                            ViewGroup rootView = (ViewGroup) ((Activity) context).findViewById(android.R.id.content);
                            rootView.removeView(gvrLayout);
                        }
                    }
                    else if (msg.what == 1) {
                        if (gvrLayout != null && surfaceView != null)
                        {

                            //setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
                            View decorView = ((Activity) context).getWindow().getDecorView();
                            int uiOptions = View.SYSTEM_UI_FLAG_LAYOUT_STABLE | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY | View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION;
                            decorView.setSystemUiVisibility(uiOptions);

                            ActionBar actionBar = ((Activity) context).getActionBar();
                            if (actionBar != null)
                                actionBar.hide();

                            gvrLayout.setVisibility(View.VISIBLE);
                            //Log.i("SETVRLAYOUTHANDLER","VISIBLE");

                            ((Activity) context).addContentView(gvrLayout, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.FILL_PARENT,
                                                                                 ViewGroup.LayoutParams.FILL_PARENT));

                            gvrLayout.onResume();
                            surfaceView.onResume();
                        }
                    }
                }
        };

        public void onDestroy(){
            if (gvrLayout != null){
                gvrLayout.shutdown();
            }
        }

        public void onPause() {
            if (gvrLayout != null){

                surfaceView.onPause();
                gvrLayout.onPause();

                /*if (gvrLayout.getVisibility() == View.VISIBLE){
                    AndroidCompat.setVrModeEnabled((Activity) context, false);

                    //setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR);
                    View decorView = ((Activity) context).getWindow().getDecorView();
                    int uiOptions = View.SYSTEM_UI_FLAG_LAYOUT_STABLE;
                    decorView.setSystemUiVisibility(uiOptions);

                    gvrLayout.setVisibility(View.INVISIBLE);
                    //Log.i("SETVRLAYOUTHANDLER","INVISIBLE");

                    ViewGroup rootView = (ViewGroup) ((Activity) context).findViewById(android.R.id.content);
                    rootView.removeView(gvrLayout);

                    gvr_was_paused = true;
                }*/
            }
        }

        public void onResume() {
            if (gvrLayout != null && surfaceView != null)
            {
                /*if (gvr_was_paused){
                    //setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
                    View decorView = ((Activity) context).getWindow().getDecorView();
                    int uiOptions = View.SYSTEM_UI_FLAG_LAYOUT_STABLE | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY | View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION;
                    decorView.setSystemUiVisibility(uiOptions);

                    ActionBar actionBar = ((Activity) context).getActionBar();
                    actionBar.hide();

                    gvrLayout.setVisibility(View.VISIBLE);
                    //Log.i("SETVRLAYOUTHANDLER","VISIBLE");

                    ((Activity) context).addContentView(gvrLayout, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.FILL_PARENT,
                                                                         ViewGroup.LayoutParams.FILL_PARENT));

                    gvr_was_paused = false;
                }*/

                gvrLayout.onResume();
                surfaceView.onResume();
            }
        }

        public void onBackPressed()
        {
            /*if (gvrLayout != null && gvrLayout.getVisibility() == View.VISIBLE){
                gvrLayout.onBackPressed();
                return;
            }*/
        }

}
