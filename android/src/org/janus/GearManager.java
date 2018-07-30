package org.janus;

import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

import android.R;
import android.app.Activity;
import android.app.ActionBar;
import android.content.Context;
import android.os.Handler;
import android.os.Message;
import android.util.DisplayMetrics;
import android.view.Gravity;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager.LayoutParams;
import android.widget.FrameLayout;
import android.util.Log;

public class GearManager implements SurfaceHolder.Callback
{
        Context context;
        SplashView splash_screen;
        SurfaceView surfaceViewGear = null;
        boolean showing_vr = false;
        boolean gear_ready = false;
        boolean gear_was_paused = false;
        boolean gear_setup = false;

        Lock gear_lock = new ReentrantLock();

        public GearManager(Context c, SplashView s)
        {
            this.context = c;
            splash_screen = s;
        }

        public void setupGear()
        {
            Message msg = new Message();
            setupGearHandler.sendMessage(msg);
        }

        protected Handler setupGearHandler = new Handler() {
                public void handleMessage(Message msg) {
                }
        };

        public void showGear(boolean b)
        {
            Message msg = new Message();
            if (b){
                msg.what = 1;
            }
            else{
                msg.what = 0;
            }
            setGearLayoutHandler.sendMessage(msg);
        }

        protected Handler setGearLayoutHandler = new Handler() {
                public void handleMessage(Message msg) {
                    //Log.i("SETVRLAYOUTHANDLER","HERE");
                    gear_lock.lock();
                    if (surfaceViewGear == null && !gear_setup)
                    {
                        // Create and configure the surface view.
                        surfaceViewGear = new SurfaceView(context);

                        DisplayMetrics displayMetrics = new DisplayMetrics();
                        ((Activity) context).getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);

                        FrameLayout mainLayout = (FrameLayout) ((Activity) context).findViewById(R.id.content);
                        FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(ViewGroup.LayoutParams.FILL_PARENT, ViewGroup.LayoutParams.FILL_PARENT);
                        surfaceViewGear.setZOrderOnTop(true);
                        surfaceViewGear.getHolder().setFixedSize(ViewGroup.LayoutParams.FILL_PARENT, ViewGroup.LayoutParams.FILL_PARENT);

                        params.leftMargin = 0;
                        params.bottomMargin = 0;
                        params.gravity = Gravity.TOP | Gravity.LEFT;
                        //surfaceViewGear.setVisibility(View.GONE);

                        mainLayout.addView(surfaceViewGear, params);
                        surfaceViewGear.getHolder().addCallback(GearManager.this);

                        gear_setup = true;
                    }

                    if (msg.what == 0) {

                        if (surfaceViewGear != null && gear_setup)
                        {
                            /*splash_screen.setVisibility(View.VISIBLE);
                            FrameLayout mainLayout = (FrameLayout) ((Activity) context).findViewById(R.id.content);
                            FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(ViewGroup.LayoutParams.FILL_PARENT, ViewGroup.LayoutParams.FILL_PARENT);

                            params.leftMargin = 0;
                            params.bottomMargin = 0;
                            params.gravity = Gravity.TOP | Gravity.LEFT;

                            if (splash_screen.getParent() != mainLayout) {
                                mainLayout.addView(splash_screen, params);
                                splash_screen.start();
                                splash_screen.drawSplash();
                            }*/

                            //setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR);
                            View decorView = ((Activity) context).getWindow().getDecorView();
                            int uiOptions = View.SYSTEM_UI_FLAG_LAYOUT_STABLE;
                            decorView.setSystemUiVisibility(uiOptions);

                            surfaceViewGear.setVisibility(View.GONE);
                            //Log.i("SETVRLAYOUTHANDLER","INVISIBLE");
                        }

                        showing_vr = false;
                        gear_ready = false;
                    }
                    else if (msg.what == 1) {
                        if (surfaceViewGear != null && gear_setup)
                        {
                            //setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
                            View decorView = ((Activity) context).getWindow().getDecorView();
                            int uiOptions = View.SYSTEM_UI_FLAG_LAYOUT_STABLE | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY | View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION;
                            decorView.setSystemUiVisibility(uiOptions);

                            ActionBar actionBar = ((Activity) context).getActionBar();
                            if (actionBar != null)
                                actionBar.hide();

                            surfaceViewGear.setVisibility(View.VISIBLE);
                            //Log.i("SETVRLAYOUTHANDLER","VISIBLE");
                        }

                        showing_vr = true;
                    }
                    gear_lock.unlock();
                }
        };

        public boolean getShowingVR()
        {
            return showing_vr;
        }

        public boolean getGearReady()
        {
            return gear_ready && surfaceViewGear != null && surfaceViewGear.getVisibility() == View.VISIBLE;
        }

        public Surface getWindowSurface() {
            if (surfaceViewGear != null)
            {
                return surfaceViewGear.getHolder().getSurface();
            }
            return null;
        }

        public void onPause()
        {
            if (surfaceViewGear != null && surfaceViewGear.getVisibility() == View.VISIBLE){
                ((JanusActivity)context).nativeExitVR();
                gear_was_paused = true;
            }
        }

        public void onResume()
        {
            if (surfaceViewGear != null && gear_was_paused){
                ((JanusActivity)context).nativeEnterVR();
                gear_was_paused = false;
            }
        }

        public void onBackPressed()
        {
            /*if (surfaceViewGear != null && surfaceViewGear.getVisibility() == View.VISIBLE){
                nativeExitVR();
                return;
            }*/
        }

        @Override
        public void surfaceCreated( SurfaceHolder holder )
        {
            //gear_ready = true;
        }

        @Override
        public void surfaceChanged( SurfaceHolder holder, int format, int width, int height )
        {
            gear_ready = true;
            //((JanusActivity)context).hideSplash();
        }

        @Override
        public void surfaceDestroyed( SurfaceHolder holder )
        {
            gear_ready = false;
        }
}
