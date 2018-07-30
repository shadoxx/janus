package org.janus;

import java.lang.Thread;
import java.nio.IntBuffer;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.Arrays;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

import android.R;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ColorMatrixColorFilter;
import android.graphics.ColorMatrix;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.Rect;
import android.graphics.SurfaceTexture;
import android.os.Handler;
import android.os.Message;
import android.os.SystemClock;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.SurfaceView;
import android.view.Surface;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager.LayoutParams;
import android.webkit.WebChromeClient;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebView.HitTestResult;
import android.webkit.WebViewClient;
import android.webkit.ValueCallback;
import android.widget.FrameLayout;
import android.graphics.SurfaceTexture;
import android.opengl.GLES11Ext;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.opengl.GLUtils;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

//import org.mozilla.gecko.gfx.CompositorController;
//import org.mozilla.gecko.gfx.GeckoDisplay;
//import org.mozilla.geckoview.GeckoView;
//import org.mozilla.geckoview.GeckoRuntime;
//import org.mozilla.geckoview.GeckoRuntimeSettings;
//import org.mozilla.geckoview.GeckoResponse;
//import org.mozilla.geckoview.GeckoSession;
//import org.mozilla.geckoview.GeckoSessionSettings;

public class WebViewManager
{
    private boolean use_gecko = false;
    private Context context;
    private boolean paused = false;
    private final float[] rgbSwapTransform =
        {
            0,  0,  1f, 0,  0,
            0,  1f, 0,  0,  0,
            1f, 0,  0,  0,  0,
            0,  0,  0,  1f, 0
        };
    private final ColorMatrix colorMatrix = new ColorMatrix(rgbSwapTransform);
    private final ColorMatrixColorFilter colorFilter = new ColorMatrixColorFilter(colorMatrix);
    private final Paint rgbSwapPaint = new Paint();

    class AWebView extends WebView
    {
        int width = 1000;
        int height = 800;

        public AWebView(Context context){
            super(context);
            super.setWillNotDraw(false);
        }

        public int computeHorizontalScrollRange(){
            return super.computeHorizontalScrollRange();
        }

        public int computeVerticalScrollRange(){
            return super.computeVerticalScrollRange();
        }

        // draw magic
        @Override
        public void onDraw( Canvas canvas ) {
            // Requires a try/catch for .lockCanvas( null )

            if (webViewLockList.containsKey((Integer)AWebView.this.getTag())) webViewLockList.get((Integer)AWebView.this.getTag()).lock();
            else {
                return;
            }

            try {
                Bitmap b = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
                Canvas c = new Canvas(b);
                //c.scale(1, -1, w/2, h/2);
                float xScale = c.getWidth() / (float)c.getWidth();
                c.scale(xScale, xScale);
                c.translate(-getScrollX(), -getScrollY());
                super.onDraw(c); // Call the WebView onDraw targetting the canvas

                if (bitmapsList.get((Integer)AWebView.this.getTag()) != null){
                    bitmapsList.get((Integer)AWebView.this.getTag()).recycle();
                }
                bitmapsList.put((Integer)AWebView.this.getTag(), b);
            } catch ( Exception e ) {
            }

            try {
                HitTestResult h = AWebView.this.getHitTestResult();
                if (h.getType() == WebView.HitTestResult.SRC_ANCHOR_TYPE || h.getType() == WebView.HitTestResult.SRC_IMAGE_ANCHOR_TYPE || h.getType() == WebView.HitTestResult.IMAGE_TYPE){
                    //Log.i("HITTESTRESULT", h.getExtra());
                    if (!(hitTestsList.containsKey((Integer)AWebView.this.getTag()) && hitTestsList.get((Integer)AWebView.this.getTag()) == h.getExtra())) {
                        hitTestLockList.get((Integer)AWebView.this.getTag()).lock();
                        if (!hitTestsList.containsKey((Integer)AWebView.this.getTag()) || hitTestsList.get((Integer)AWebView.this.getTag()) == ""){
                            hitTestsList.put((Integer) AWebView.this.getTag(), h.getExtra());
                        }
                        hitTestLockList.get((Integer) AWebView.this.getTag()).unlock();
                        hitTestsList.put((Integer) AWebView.this.getTag(), h.getExtra());
                    }
                }
            }
            finally{
                if (webViewLockList.containsKey((Integer) AWebView.this.getTag())) webViewLockList.get((Integer) AWebView.this.getTag()).unlock();
            }
            // super.onDraw( canvas ); // <- Uncomment this if you want to show the original view
        }

        public void setTextureWidth(int w) {
            width = w;
        }

        public void setTextureHeight(int h) {
            height = h;
        }
    }

    protected ArrayList<View> webViewsList = new ArrayList<View>();
    protected Hashtable<Integer, Lock> updatesEnabledList = new Hashtable<Integer, Lock>();
    protected Hashtable<Integer, Bitmap> bitmapsList = new Hashtable<Integer, Bitmap>();
    protected Hashtable<Integer, Integer> texturesList = new Hashtable<Integer, Integer>();
    protected Hashtable<Integer, Integer> widthList = new Hashtable<Integer, Integer>();
    protected Hashtable<Integer, Integer> heightList = new Hashtable<Integer, Integer>();
    protected Hashtable<Integer, Boolean> repaintRequestedList = new Hashtable<Integer, Boolean>();
    protected Hashtable<Integer, String> hitTestsList = new Hashtable<Integer, String>();
    protected Hashtable<Integer, Integer> scrollXList = new Hashtable<Integer, Integer>();
    protected Hashtable<Integer, Integer> scrollYList = new Hashtable<Integer, Integer>();
    protected Hashtable<Integer, Boolean> scrollRequestedList = new Hashtable<Integer, Boolean>();
    protected Hashtable<Integer, String> urlList = new Hashtable<Integer, String>();
    protected Hashtable<Integer, Boolean> urlChangedList = new Hashtable<Integer, Boolean>();
    protected Hashtable<Integer, Integer> horizontalRangeList = new Hashtable<Integer, Integer>();
    protected Hashtable<Integer, Integer> verticalRangeList = new Hashtable<Integer, Integer>();
    protected Hashtable<Integer, Lock> webViewLockList = new Hashtable<Integer, Lock>();
    protected Hashtable<Integer, Lock> hitTestLockList = new Hashtable<Integer, Lock>();

    protected View findWebViewByTag(int tag) {
        View webViewRes = null;
        for (int i = 0; i < webViewsList.size(); i++) {
            View wv = (View) webViewsList.get(i);
            if (((Integer)wv.getTag()).intValue() == tag) {
                webViewRes = wv;
                break;
            }
        }

        return webViewRes;
    }

    public WebViewManager(Context c) {
        this.context = c;
        AWebView.enableSlowWholeDocumentDraw();
        rgbSwapPaint.setColorFilter(colorFilter);
    }

    public void setUseGeckoWebView(boolean b) {
        use_gecko = false; //b;
    }

    public void createNewWebView(int tag) {
        Message msg = new Message();
        msg.what = tag;

        createNewWebViewHandler.sendMessage(msg);
    }

    protected Handler createNewWebViewHandler = new Handler() {
        @SuppressLint("SetJavaScriptEnabled")
        public void handleMessage(Message msg) {
            Lock l = new ReentrantLock();
            l.lock();
            try{
                if (!use_gecko){
                    webViewLockList.put(msg.what, l);

                    AWebView webView = new AWebView(context);
                    webView.setTag(msg.what);

                    webView.getSettings().setJavaScriptEnabled(true);
                    webView.getSettings().setBuiltInZoomControls( true );
                    webView.getSettings().setUseWideViewPort(true);
                    webView.getSettings().setLoadWithOverviewMode(true);
                    //webView.setScrollBarStyle(WebView.SCROLLBARS_OUTSIDE_OVERLAY);
                    //webView.setScrollbarFadingEnabled(false);
                    webView.getSettings().setDomStorageEnabled(true);
                    webView.getSettings().setMixedContentMode(WebSettings.MIXED_CONTENT_ALWAYS_ALLOW);
                    webView.getSettings().setMediaPlaybackRequiresUserGesture(false);
                    webView.getSettings().setOffscreenPreRaster(true);
                    webView.getSettings().setUserAgentString("Desktop"); //Android
                    webView.setWebViewClient(new WebViewClient() {
                        @Override
                        public boolean shouldOverrideUrlLoading(WebView view, String url) {
                            return false;
                        }

                        @Override
                        public void onPageFinished(WebView view, String url) {
                            Integer tag = (Integer) view.getTag();

                            if (webViewLockList.containsKey(tag)) webViewLockList.get(tag).lock();
                            else{
                                return;
                            }

                            try {
                                urlList.put(tag, view.getUrl());
                                urlChangedList.put(tag, true);

                                horizontalRangeList.put(tag, ((AWebView) view).computeHorizontalScrollRange());
                                verticalRangeList.put(tag, ((AWebView) view).computeVerticalScrollRange());
                            }
                            finally {
                                if (webViewLockList.containsKey(tag)) webViewLockList.get(tag).unlock();
                            }
                        }
                    });
                    webView.setWebChromeClient(new WebChromeClient() {
                        //@Override
                        //public void onShowCustomView(View view, WebChromeClient.CustomViewCallback callback) {
                        //    Log.i("SHOWINGCUSTOMVIEW","Here");
                        //}
                    });
                    webView.setBackgroundColor(Color.WHITE); //TRANSPARENT,BLACK
                    webView.setVisibility(View.VISIBLE);
                    //webView.setDrawingCacheQuality(View.DRAWING_CACHE_QUALITY_LOW);
                    //webView.setLayerType(View.LAYER_TYPE_HARDWARE, rgbSwapPaint);

                    webViewsList.add(webView);

                    updatesEnabledList.put(msg.what, new ReentrantLock());
                    repaintRequestedList.put(msg.what, false);
                    urlChangedList.put(msg.what, false);
                    scrollRequestedList.put(msg.what, false);
                    widthList.put(msg.what, 1000);
                    heightList.put(msg.what, 800);
                    horizontalRangeList.put(msg.what, 1000);
                    verticalRangeList.put(msg.what, 800);
                    scrollXList.put(msg.what, 0);
                    scrollYList.put(msg.what, 0);
                    urlList.put(msg.what, "");
                    hitTestsList.put(msg.what,"");
                    hitTestLockList.put(msg.what, new ReentrantLock());
                }
            }
            finally{
                l.unlock();
            }
        }
    };

    public void removeWebView(int tag) {
        Message msg = new Message();
        msg.what = tag;

        removeWebViewHandler.sendMessage(msg);
    }

    protected Handler removeWebViewHandler = new Handler() {
        public void handleMessage(Message msg) {
            if (webViewLockList.containsKey(msg.what)) webViewLockList.get(msg.what).lock();
            else{
                removeWebViewHandler.sendMessage(msg);
                return;
            }
            try{
                View webView = findWebViewByTag(msg.what);
                if (webView != null) {
                    webViewsList.remove(context);
                    FrameLayout mainLayout = (FrameLayout) ((Activity) context).findViewById(R.id.content);
                    mainLayout.removeView(webView);
                }
                if (bitmapsList.contains(msg.what) && bitmapsList.get(msg.what) != null){
                    bitmapsList.get(msg.what).recycle();
                    bitmapsList.remove(msg.what);
                }
                if (repaintRequestedList.contains(msg.what)){
                    repaintRequestedList.remove(msg.what);
                }
                if (scrollRequestedList.contains(msg.what)){
                    scrollRequestedList.remove(msg.what);
                }
                if (urlChangedList.contains(msg.what)){
                    urlChangedList.remove(msg.what);
                }
                if (widthList.contains(msg.what)){
                    widthList.remove(msg.what);
                }
                if (heightList.contains(msg.what)){
                    heightList.remove(msg.what);
                }
                if (scrollXList.contains(msg.what)){
                    scrollXList.remove(msg.what);
                }
                if (scrollYList.contains(msg.what)){
                    scrollYList.remove(msg.what);
                }
                if (urlList.contains(msg.what)){
                    urlList.remove(msg.what);
                }
                if (horizontalRangeList.contains(msg.what)){
                    horizontalRangeList.remove(msg.what);
                }
                if (verticalRangeList.contains(msg.what)){
                    verticalRangeList.remove(msg.what);
                }
            }
            finally{
                if (webViewLockList.containsKey(msg.what) && webViewLockList.containsKey(msg.what)){
                    Lock l = webViewLockList.get(msg.what);
                    webViewLockList.remove(msg.what);

                    createNewWebViewHandler.removeMessages(msg.what);
                    moveWebViewHandler.removeMessages(msg.what);
                    resizeWebViewHandler.removeMessages(msg.what);
                    attachWebViewToMainLayoutHandler.removeMessages(msg.what);
                    loadUrlAtWebViewHandler.removeMessages(msg.what);
                    goBackOrForwardAtWebViewHandler.removeMessages(msg.what);
                    reloadWebViewHandler.removeMessages(msg.what);
                    stopWebViewHandler.removeMessages(msg.what);
                    loadHtmlAtWebViewHandler.removeMessages(msg.what);
                    evaluateJavaScriptAtWebViewHandler.removeMessages(msg.what);
                    scrollByWebViewHandler.removeMessages(msg.what);
                    scrollToWebViewHandler.removeMessages(msg.what);
                    mousePressWebViewHandler.removeMessages(msg.what);
                    mouseMoveWebViewHandler.removeMessages(msg.what);
                    mouseReleaseWebViewHandler.removeMessages(msg.what);
                    removeWebViewHandler.removeMessages(msg.what);

                    l.unlock();
                }
            }
        }
    };

    public void moveWebView(int tag, int x, int y) {
        Message msg = new Message();
        msg.what = tag;
        msg.arg1 = x;
        msg.arg2 = y;

        moveWebViewHandler.sendMessage(msg);
    }

    protected Handler moveWebViewHandler = new Handler() {
        public void handleMessage(Message msg) {
            if (webViewLockList.containsKey(msg.what)) webViewLockList.get(msg.what).lock();
            else{
                moveWebViewHandler.sendMessage(msg);
                return;
            }
            try{
                View viewToMove = null;
                viewToMove = findWebViewByTag(msg.what);

                if (viewToMove != null) {
                        FrameLayout.LayoutParams params = (android.widget.FrameLayout.LayoutParams) viewToMove
                                        .getLayoutParams();
                        params.leftMargin = msg.arg1;
                        params.topMargin = msg.arg2;
                        viewToMove.setLayoutParams(params);
                }
                //repaintRequestedList.put(msg.what, true);
            }
            finally{
                if (webViewLockList.containsKey(msg.what)) webViewLockList.get(msg.what).unlock();
            }
        }
    };

    public void resizeWebView(int tag, int w, int h) {
        Message msg = new Message();
        msg.what = tag;
        msg.arg1 = w;
        msg.arg2 = h;

        resizeWebViewHandler.sendMessage(msg);
    }

    protected Handler resizeWebViewHandler = new Handler() {
        public void handleMessage(Message msg) {
            if (webViewLockList.containsKey(msg.what)) webViewLockList.get(msg.what).lock();
            else{
                resizeWebViewHandler.sendMessage(msg);
                return;
            }
            try{
                View viewToResize = null;
                viewToResize = findWebViewByTag(msg.what);

                if (viewToResize != null) {
                        FrameLayout.LayoutParams params = (android.widget.FrameLayout.LayoutParams) viewToResize
                                        .getLayoutParams();
                        params.width = msg.arg1;
                        params.height = msg.arg2;
                        viewToResize.setLayoutParams(params);

                        ((AWebView) viewToResize).setTextureWidth(msg.arg1);
                        ((AWebView) viewToResize).setTextureHeight(msg.arg2);

                        widthList.put(msg.what, msg.arg1);
                        heightList.put(msg.what, msg.arg2);
                }
                //repaintRequestedList.put(msg.what, true);
            }
            finally{
                if (webViewLockList.containsKey(msg.what)) webViewLockList.get(msg.what).unlock();
            }
        }
    };

    public void attachWebViewToMainLayout(int tag) {
        Message msg = new Message();
        msg.what = tag;

        attachWebViewToMainLayoutHandler.sendMessage(msg);
    }

    protected Handler attachWebViewToMainLayoutHandler = new Handler() {
        public void handleMessage(Message msg) {
            if (webViewLockList.containsKey(msg.what)) webViewLockList.get(msg.what).lock();
            else{
                attachWebViewToMainLayoutHandler.sendMessage(msg);
                return;
            }
            try{
                View viewToAttach = null;
                viewToAttach = findWebViewByTag(msg.what);

                if (viewToAttach != null) {
                        FrameLayout mainLayout = (FrameLayout) ((Activity) context).findViewById(R.id.content);
                        FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(1000, 800); //Default params
                        params.leftMargin = 0;
                        params.topMargin = 0;

                        mainLayout.addView(viewToAttach, params);
                }
            }
            finally{
                if (webViewLockList.containsKey(msg.what)) webViewLockList.get(msg.what).unlock();
            }
        }
    };

    public void loadUrlAtWebView(int tag, String url) {
        Message msg = new Message();
        msg.what = tag;
        msg.obj = url;

        loadUrlAtWebViewHandler.sendMessage(msg);
    }

    protected Handler loadUrlAtWebViewHandler = new Handler() {
        public void handleMessage(Message msg) {
            if (webViewLockList.containsKey(msg.what)) webViewLockList.get(msg.what).lock();
            else{
                loadUrlAtWebViewHandler.sendMessage(msg);
                return;
            }
            try{
                if (!use_gecko) {
                    AWebView webView = (AWebView) findWebViewByTag(msg.what);
                    if (webView != null) {
                        String s = "";
                        if (!(msg.obj.toString()).contains(":")) s += "http://";
                        //webView.clearCache(true);
                        webView.loadUrl(s + msg.obj.toString());

                        //Log.i("url", msg.obj.toString());
                    }

                    urlList.put(msg.what, webView.getUrl());
                    urlChangedList.put(msg.what, true);
                }
            }
            finally{
                if (webViewLockList.containsKey(msg.what)) webViewLockList.get(msg.what).unlock();
            }
        }
    };

    public void goBackOrForwardAtWebView(int tag, int steps) {
        Message msg = new Message();
        msg.what = tag;
        msg.arg1 = steps;

        goBackOrForwardAtWebViewHandler.sendMessage(msg);
    }

    protected Handler goBackOrForwardAtWebViewHandler = new Handler() {
        public void handleMessage(Message msg) {
            if (webViewLockList.containsKey(msg.what)) webViewLockList.get(msg.what).lock();
            else{
                goBackOrForwardAtWebViewHandler.sendMessage(msg);
                return;
            }
            try{
                if (!use_gecko){
                    AWebView webView = (AWebView) findWebViewByTag(msg.what);
                    if (webView != null) {
                        if (webView.canGoBackOrForward(msg.arg1))
                        {
                            webView.goBackOrForward(msg.arg1);
                        }
                    }

                    urlList.put(msg.what, webView.getUrl());
                    urlChangedList.put(msg.what, true);
                }
            }
            finally{
                if (webViewLockList.containsKey(msg.what)) webViewLockList.get(msg.what).unlock();
            }
        }
    };

    public void reloadWebView(int tag) {
        Message msg = new Message();
        msg.what = tag;

        reloadWebViewHandler.sendMessage(msg);
    }

    protected Handler reloadWebViewHandler = new Handler() {
        public void handleMessage(Message msg) {
            if (webViewLockList.containsKey(msg.what)) webViewLockList.get(msg.what).lock();
            else{
                reloadWebViewHandler.sendMessage(msg);
                return;
            }
            try{
                if (!use_gecko){
                    AWebView webView = (AWebView) findWebViewByTag(msg.what);
                    if (webView != null) {
                        webView.reload();
                    }

                    urlList.put(msg.what, webView.getUrl());
                    urlChangedList.put(msg.what, true);
                }
            }
            finally{
                if (webViewLockList.containsKey(msg.what)) webViewLockList.get(msg.what).unlock();
            }
        }
    };

    public void stopWebView(int tag) {
        Message msg = new Message();
        msg.what = tag;

        stopWebViewHandler.sendMessage(msg);
    }

    protected Handler stopWebViewHandler = new Handler() {
        public void handleMessage(Message msg) {
            if (webViewLockList.containsKey(msg.what)) webViewLockList.get(msg.what).lock();
            else{
                stopWebViewHandler.sendMessage(msg);
                return;
            }
            try{
                if (!use_gecko){
                    AWebView webView = (AWebView) findWebViewByTag(msg.what);
                    if (webView != null) {
                        webView.stopLoading();
                    }

                    urlList.put(msg.what, webView.getUrl());
                    urlChangedList.put(msg.what, true);
                }
            }
            finally{
                if (webViewLockList.containsKey(msg.what)) webViewLockList.get(msg.what).unlock();
            }
        }
    };

    public void loadHtmlAtWebView(int tag, String html) {
        Message msg = new Message();
        msg.what = tag;
        msg.obj = html;

        loadHtmlAtWebViewHandler.sendMessage(msg);
    }

    protected Handler loadHtmlAtWebViewHandler = new Handler() {
        public void handleMessage(Message msg) {
            if (webViewLockList.containsKey(msg.what)) webViewLockList.get(msg.what).lock();
            else{
                loadHtmlAtWebViewHandler.sendMessage(msg);
                return;
            }
            try{
                if (!use_gecko){
                    AWebView webView = (AWebView) findWebViewByTag(msg.what);
                    if (webView != null) {
                            webView.loadData(msg.obj.toString(), "text/html; charset=UTF-8", "UTF-8");
                    }
                }
                //repaintRequestedList.put(msg.what, true);
            }
            finally{
                if (webViewLockList.containsKey(msg.what)) webViewLockList.get(msg.what).unlock();
            }
        }
    };

    public void evaluateJavaScriptAtWebView(int tag, String javascript) {
        Message msg = new Message();
        msg.what = tag;
        msg.obj = javascript;

        evaluateJavaScriptAtWebViewHandler.sendMessage(msg);
    }

    protected Handler evaluateJavaScriptAtWebViewHandler = new Handler() {
        public void handleMessage(Message msg) {
            if (webViewLockList.containsKey(msg.what)) webViewLockList.get(msg.what).lock();
            else{
                evaluateJavaScriptAtWebViewHandler.sendMessage(msg);
                return;
            }
            try{
                if (!use_gecko){
                    AWebView webView = (AWebView) findWebViewByTag(msg.what);
                    if (webView != null) {
                        webView.evaluateJavascript(msg.obj.toString(), new ValueCallback<String>() {
                            @Override
                            public void onReceiveValue(String s) {
                                //Log.d("EvaluatedJS", s);
                            }
                        });
                    }
                }
                //repaintRequestedList.put(msg.what, true);
            }
            finally{
                if (webViewLockList.containsKey(msg.what)) webViewLockList.get(msg.what).unlock();
            }
        }
    };

    public void setUpdatesEnabledWebView(int tag, boolean b) {
        if (updatesEnabledList.containsKey(tag) && updatesEnabledList.get(tag) != null) {
            if (!b)
                updatesEnabledList.get(tag).lock();
            else
                updatesEnabledList.get(tag).unlock();
        }
    }

    public Bitmap getBitmap(int tag) {
        if (bitmapsList.containsKey(tag) && bitmapsList.get(tag) != null)
        {
            return bitmapsList.get(tag);
        }
        return null;
    }

    public int getTexture(int tag) {
        if (texturesList.containsKey(tag) && texturesList.get(tag) != null)
        {
            return texturesList.get(tag);
        }
        return 0;
    }

    public String getHitTestContent(int tag) {
        if (hitTestsList.containsKey(tag))
        {
            return hitTestsList.get(tag);
        }
        return "";
    }

    public int getHorizontalScrollRange(int tag) {
        if (horizontalRangeList.containsKey(tag))
        {
            return horizontalRangeList.get(tag);
        }
        return 0;
    }

    public int getVerticalScrollRange(int tag) {
        if (verticalRangeList.containsKey(tag))
        {
            return verticalRangeList.get(tag);
        }
        return 0;
    }

    public int getWidthWebView(int tag) {
        if (widthList.containsKey(tag))
        {
            return widthList.get(tag);
        }
        return 0;
    }

    public int getHeightWebView(int tag) {
        if (heightList.containsKey(tag))
        {
            return heightList.get(tag);
        }
        return 0;
    }

    public int getScrollXWebView(int tag) {
        if (scrollXList.containsKey(tag))
        {
            return scrollXList.get(tag);
        }
        return 0;
    }

    public int getScrollYWebView(int tag) {
        if (scrollYList.containsKey(tag))
        {
            return scrollYList.get(tag);
        }
        return 0;
    }

    public String getURLWebView(int tag) {
        if (urlList.containsKey(tag))
        {
            return urlList.get(tag);
        }
        return "";
    }

    public void scrollByWebView(int tag, int x, int y) {
        Message msg = new Message();
        msg.what = tag;
        msg.arg1 = x;
        msg.arg2 = y;

        scrollByWebViewHandler.sendMessage(msg);
    }

    protected Handler scrollByWebViewHandler = new Handler() {
        public void handleMessage(Message msg) {
            if (webViewLockList.containsKey(msg.what)) webViewLockList.get(msg.what).lock();
            else{
                scrollByWebViewHandler.sendMessage(msg);
                return;
            }
            try{
                View viewToScroll = null;
                viewToScroll = findWebViewByTag(msg.what);

                if (!use_gecko) {
                    viewToScroll.scrollBy(msg.arg1, msg.arg2);
                }

                scrollXList.put(msg.what, viewToScroll.getScrollX());
                scrollYList.put(msg.what, viewToScroll.getScrollY());

                //repaintRequestedList.put(msg.what, true);
                scrollRequestedList.put(msg.what, true);
            }
            finally{
                if (webViewLockList.containsKey(msg.what)) webViewLockList.get(msg.what).unlock();
            }
        }
    };

    public void scrollToWebView(int tag, int x, int y) {
        Message msg = new Message();
        msg.what = tag;
        msg.arg1 = x;
        msg.arg2 = y;

        scrollToWebViewHandler.sendMessage(msg);
    }

    protected Handler scrollToWebViewHandler = new Handler() {
        public void handleMessage(Message msg) {
            if (webViewLockList.containsKey(msg.what)) webViewLockList.get(msg.what).lock();
            else{
                scrollToWebViewHandler.sendMessage(msg);
                return;
            }
            try{
                View viewToScroll = null;
                viewToScroll = findWebViewByTag(msg.what);

                if (viewToScroll != null) {
                    if (!use_gecko) {
                        viewToScroll.scrollTo(msg.arg1, msg.arg2);
                    }
                }

                scrollXList.put(msg.what, viewToScroll.getScrollX());
                scrollYList.put(msg.what, viewToScroll.getScrollY());

                //repaintRequestedList.put(msg.what, true);
                scrollRequestedList.put(msg.what, true);
            }
            finally{
                if (webViewLockList.containsKey(msg.what)) webViewLockList.get(msg.what).unlock();
            }
        }
    };

    public void mousePressWebView(int tag, int x, int y) {
        Message msg = new Message();
        msg.what = tag;
        msg.arg1 = x;
        msg.arg2 = y;

        mousePressWebViewHandler.sendMessage(msg);
    }

    protected Handler mousePressWebViewHandler = new Handler() {
        public void handleMessage(Message msg) {
            if (webViewLockList.containsKey(msg.what)) webViewLockList.get(msg.what).lock();
            else{
                mousePressWebViewHandler.sendMessage(msg);
                return;
            }
            try{
                View viewToPress = null;
                viewToPress = findWebViewByTag(msg.what);

                if (viewToPress != null) {
                    if (!use_gecko) {
                        WebView webView = (WebView) viewToPress;
                        webView.setWebViewClient(new WebViewClient(){
                            @Override
                            public boolean shouldOverrideUrlLoading(WebView view, String url) {
                                //Log.i("OVERRIDINGURL",url);
                                hitTestLockList.get((Integer)view.getTag()).lock();

                                view.setWebViewClient(new WebViewClient() {
                                    @Override
                                    public boolean shouldOverrideUrlLoading(WebView view, String url) {
                                        return false;
                                    }

                                    @Override
                                    public void onPageFinished(WebView view, String url) {
                                        Integer tag = (Integer) view.getTag();
                                        urlList.put(tag, view.getUrl());
                                        urlChangedList.put(tag, true);
                                    }
                                });

                                hitTestLockList.get((Integer)view.getTag()).unlock();
                                return true;
                            }
                        });

                        //Simulate touch press/release to get hittest
                        MotionEvent e1 = MotionEvent.obtain(
                                SystemClock.uptimeMillis(), SystemClock.uptimeMillis(),
                                MotionEvent.ACTION_DOWN, msg.arg1, msg.arg2, 0);
                        viewToPress.dispatchTouchEvent(e1);
                        MotionEvent e2 = MotionEvent.obtain(
                                SystemClock.uptimeMillis(), SystemClock.uptimeMillis(),
                                MotionEvent.ACTION_UP, msg.arg1, msg.arg2, 0);
                        viewToPress.dispatchTouchEvent(e2);

                        //Actual mouse press
                        hitTestLockList.get(msg.what).lock();

                        webView.setWebViewClient(new WebViewClient() {
                            @Override
                            public boolean shouldOverrideUrlLoading(WebView view, String url) {
                                return false;
                            }

                            @Override
                            public void onPageFinished(WebView view, String url) {
                                Integer tag = (Integer) view.getTag();
                                urlList.put(tag, view.getUrl());
                                urlChangedList.put(tag, true);
                            }
                        });
                    //repaintRequestedList.put(msg.what, true);

                        MotionEvent e = MotionEvent.obtain(
                                SystemClock.uptimeMillis(), SystemClock.uptimeMillis(),
                                MotionEvent.ACTION_DOWN, msg.arg1, msg.arg2, 0);

                        viewToPress.dispatchTouchEvent(e);
                        hitTestLockList.get(msg.what).unlock();
                    }
                }
            }
            finally{
                if (webViewLockList.containsKey(msg.what)) webViewLockList.get(msg.what).unlock();
            }
        }
    };

    public void mouseMoveWebView(int tag, int x, int y) {
        Message msg = new Message();
        msg.what = tag;
        msg.arg1 = x;
        msg.arg2 = y;

        mouseMoveWebViewHandler.sendMessage(msg);
    }

    protected Handler mouseMoveWebViewHandler = new Handler() {
        public void handleMessage(Message msg) {
            if (webViewLockList.containsKey(msg.what)) webViewLockList.get(msg.what).lock();
            else{
                mouseMoveWebViewHandler.sendMessage(msg);
                return;
            }
            try{
                View viewToMove = null;
                viewToMove = findWebViewByTag(msg.what);

                if (viewToMove != null) {
                    if (!use_gecko){
                        MotionEvent e = MotionEvent.obtain(
                                SystemClock.uptimeMillis(), SystemClock.uptimeMillis(),
                                MotionEvent.ACTION_MOVE, msg.arg1, msg.arg2, 0);
                        viewToMove.dispatchTouchEvent(e);
                    }
                }
                //repaintRequestedList.put(msg.what, true);
            }
            finally{
                if (webViewLockList.containsKey(msg.what)) webViewLockList.get(msg.what).unlock();
            }
        }
    };

    public void mouseReleaseWebView(int tag, int x, int y) {
        Message msg = new Message();
        msg.what = tag;
        msg.arg1 = x;
        msg.arg2 = y;

        mouseReleaseWebViewHandler.sendMessage(msg);
    }

    protected Handler mouseReleaseWebViewHandler = new Handler() {
        public void handleMessage(Message msg) {
            if (webViewLockList.containsKey(msg.what)) webViewLockList.get(msg.what).lock();
            else{
                mouseReleaseWebViewHandler.sendMessage(msg);
                return;
            }
            try{
                View viewToRelease = null;
                viewToRelease = findWebViewByTag(msg.what);

                if (viewToRelease != null) {
                    if (!use_gecko){
                        MotionEvent e = MotionEvent.obtain(
                                SystemClock.uptimeMillis(), SystemClock.uptimeMillis(),
                                MotionEvent.ACTION_UP, msg.arg1, msg.arg2, 0);
                        viewToRelease.dispatchTouchEvent(e);
                    }
                }
                //repaintRequestedList.put(msg.what, true);
            }
            finally{
                if (webViewLockList.containsKey(msg.what)) webViewLockList.get(msg.what).unlock();
            }
        }
    };

    public boolean getRepaintRequestedAtWebView(int tag) {
        boolean b = false;
        if (repaintRequestedList.containsKey(tag))
        {
            b = repaintRequestedList.get(tag);
            if (b){
                repaintRequestedList.put(tag, false);
            }
        }
        return b;
    }

    public boolean getScrollRequestedAtWebView(int tag) {
        boolean b = false;
        if (scrollRequestedList.containsKey(tag))
        {
            b = scrollRequestedList.get(tag);
            if (b){
                scrollRequestedList.put(tag, false);
            }
        }
        return b;
    }

    public boolean getURLChangedAtWebView(int tag) {
        boolean b = false;
        if (urlChangedList.containsKey(tag))
        {
            b = urlChangedList.get(tag);
            if (b){
                urlChangedList.put(tag, false);
            }
        }
        return b;
    }

    public void setPaused(boolean b) {
        paused = b;

        for (int i = 0; i < webViewsList.size(); i++) {
            View wv = (View) webViewsList.get(i);
            if (paused){
                stopWebView(((Integer)wv.getTag()).intValue());
                if (!use_gecko){
                    ((AWebView) wv).loadUrl("about:blank");
                    ((AWebView) wv).onPause();
                }
            }
            else {
                if (!use_gecko){
                    int tag = ((Integer)wv.getTag()).intValue();
                    ((AWebView) wv).loadUrl(urlList.get(tag));
                    ((AWebView) wv).onResume();
                }
                reloadWebView(((Integer)wv.getTag()).intValue());
            }
        }
    }
}
