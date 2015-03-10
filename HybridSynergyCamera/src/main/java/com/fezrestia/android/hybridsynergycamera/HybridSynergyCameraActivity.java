package com.fezrestia.android.hybridsynergycamera;

import android.app.Activity;
import android.content.Context;
import android.content.res.AssetManager;
import android.graphics.Color;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.os.Bundle;
import android.os.Handler;
import android.view.Surface;
import android.view.TextureView;
import android.webkit.JavascriptInterface;
import android.webkit.ValueCallback;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.FrameLayout;

import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.ThreadFactory;

public class HybridSynergyCameraActivity extends Activity {

    static {
        System.loadLibrary("hybridsynergycamera");
    }

    private static final String TAG = HybridSynergyCameraActivity.class.getSimpleName();

    private static final boolean IS_DEBUG = false;

    private static void logDebug(String tag, String event) {
        android.util.Log.e("TraceLog",
                "[TIME=" + System.currentTimeMillis() + "] "
                + "[TAG=" + tag + "] "
                + "[THREAD=" + Thread.currentThread() + "] "
                + "[EVENT=" + event + "]");
    }

    // Camera.
    private Camera mCamera = null;
    private Camera.Parameters mCameraParams = null;
    private SurfaceTexture mDummySurfaceTexture = null;
    private ByteBufferRing mFrameBufferRing = null;
    private static final int PREVIEW_FRAME_WIDTH = 1280;
    private static final int PREVIEW_FRAME_HEIGHT = 720;
    private static final int PREVIEW_FRAME_RING_BUFFER_SIZE = 5;

    // UI.
    private FrameLayout mRootView = null;
    private TextureView mTextureView = null;
    private UserWebView mUserWebView = null;

    // Thread.
    private Handler mUiWorker = new Handler();
    private ExecutorService mBackWorker = Executors.newSingleThreadExecutor(
            new BackWorkerThreadFactory());

    // Flags.
    private boolean mIsSleep = true;

    /**
     * Life-Cycle interface.
     *
     * @param bundle
     */
    @Override
    public void onCreate(Bundle bundle) {
        if (IS_DEBUG) logDebug(TAG, "onCreate() : E");
        super.onCreate(bundle);

        // Notify to Native.
        nativeOnActivityCreated(this.getResources().getAssets());

        // UI.
        mRootView = new FrameLayout(this);
        mTextureView = new TextureView(this);
        mTextureView.setSurfaceTextureListener(mTextureViewCallback);
        mUserWebView = new UserWebView(this);
        mUserWebView.initialize();
mUserWebView.setAlpha(0.5f);

        // Configure view hierarchy.
        setContentView(mRootView);
        mRootView.addView(mTextureView);
        mRootView.addView(mUserWebView);

        if (IS_DEBUG) logDebug(TAG, "onCreate() : X");
    }

    /**
     * Life-Cycle interface.
     */
    @Override
    public void onResume() {
        if (IS_DEBUG) logDebug(TAG, "onResume() : E");
        super.onResume();

        // Start camera open.
        mBackWorker.execute(mInitializeCameraTask);

        // Check.
        if (mTextureView.isAvailable()) {
            if (IS_DEBUG) logDebug(TAG, "Texture is already available.");
            mTextureViewCallback.onSurfaceTextureAvailable(
                    mTextureView.getSurfaceTexture(),
                    mTextureView.getWidth(),
                    mTextureView.getHeight());
        } else {
            if (IS_DEBUG) logDebug(TAG, "Texture is not available yet. Wait for callback.");
        }

        // Reset flags.
        mIsSleep = false;

        if (IS_DEBUG) logDebug(TAG, "onResume() : X");
    }

    private final TextureViewCallback mTextureViewCallback = new TextureViewCallback();
    private class TextureViewCallback implements TextureView.SurfaceTextureListener {
        // Log tag.
        private final String TAG = TextureViewCallback.class.getSimpleName();

        @Override
        public void onSurfaceTextureAvailable(
                final SurfaceTexture surface,
                final int width,
                final int height) {
            if (IS_DEBUG) logDebug(TAG, "onSurfaceTextureAvailable() : E");

            Runnable task = new NotifySurfaceInitializedTask(new Surface(surface));
            mBackWorker.execute(task);

            if (IS_DEBUG) logDebug(TAG, "onSurfaceTextureAvailable() : X");
        }

        private class NotifySurfaceInitializedTask implements Runnable {
            // Target surface.
            private final Surface mSurface;

            /**
             * CONSTRUCTOR.
             *
             * @param surface
             */
            NotifySurfaceInitializedTask(Surface surface) {
                mSurface = surface;
            }

            @Override
            public void run() {
                nativeOnSurfaceInitialized(mSurface);
            }
        }

        @Override
        public boolean onSurfaceTextureDestroyed(final SurfaceTexture surface) {
            if (IS_DEBUG) logDebug(TAG, "onSurfaceTextureDestroyed() : E");

            Runnable task = new NotifySurfaceFinalizedTask();
            mBackWorker.execute(task);

            if (IS_DEBUG) logDebug(TAG, "onSurfaceTextureDestroyed() : X");
            return false;
        }

        private class NotifySurfaceFinalizedTask implements Runnable {
            @Override
            public void run() {
                nativeOnSurfaceFinalized();
            }
        }

        @Override
        public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {
            if (IS_DEBUG) logDebug(TAG, "onSurfaceTextureSizeChanged() : E");
            // NOP.
            if (IS_DEBUG) logDebug(TAG, "onSurfaceTextureSizeChanged() : X");
        }

        @Override
        public void onSurfaceTextureUpdated(SurfaceTexture surface) {
            if (IS_DEBUG) logDebug(TAG, "onSurfaceTextureUpdated() : E");
            // NOP.
            if (IS_DEBUG) logDebug(TAG, "onSurfaceTextureUpdated() : X");
        }
    }



    /**
     * Life-Cycle interface.
     */
    @Override
    public void onPause() {
        if (IS_DEBUG) logDebug(TAG, "onPause() : E");

        // Flags.
        mIsSleep = true;

        // Thread.
        mBackWorker.execute(mFinalizeCameraTask);

        super.onPause();
        if (IS_DEBUG) logDebug(TAG, "onPause() : X");
    }

    /**
     * Life-Cycle interface.
     */
    @Override
    public void onDestroy() {
        if (IS_DEBUG) logDebug(TAG, "onDestroy() : E");

        // Await finalize worker thread.
        BackWorkerBlocker blocker = new BackWorkerBlocker();
        mBackWorker.execute(blocker);
        blocker.await();

        // Notify to Native.
        nativeOnActivityDestroyed();

        // Configure view hierarchy.
        mRootView.removeView(mTextureView);
        mRootView.removeView(mUserWebView);

        // UI.
        mTextureView.setSurfaceTextureListener(null);
        mTextureView = null;
        mUserWebView.release();
        mUserWebView = null;
        mRootView = null;

        super.onDestroy();
        if (IS_DEBUG) logDebug(TAG, "onDestroy() : X");
    }

    // Commands.
    private static final int CMD_STORE_NATIVE_APP_CONTEXT_POINTER       = 0x0001;
    private static final int CMD_ACTIVITY_ON_RESUMED                    = 0x0010;
    private static final int CMD_ACTIVITY_ON_PAUSED                     = 0x0020;
    private static final int CMD_WINDOW_INIT                            = 0x0100;
    private static final int CMD_WINDOW_TERM                            = 0x0200;

    public static void sendCommandFromNative(int command, int arg) {
        if (IS_DEBUG) logDebug(TAG, "sendCommandFromNative() : E");

        switch (command) {



            default:
                if (IS_DEBUG) logDebug(TAG, "sendCommandFromNative() : UNEXPECTED");
                // NOP.
                break;
        }
    }

    private final InitializeCameraTask mInitializeCameraTask = new InitializeCameraTask();
    private class InitializeCameraTask implements Runnable {
        // Log tag.
        private final String TAG = InitializeCameraTask.class.getSimpleName();

        @Override
        public void run() {
            if (IS_DEBUG) logDebug(TAG, "run() : E");

            if (mCamera == null) {
                // Initialize camera.
                if (IS_DEBUG) logDebug(TAG, "Camera.open() : E");
                mCamera = Camera.open(Camera.CameraInfo.CAMERA_FACING_BACK);
                if (IS_DEBUG) logDebug(TAG, "Camera.open() : X");
                mCameraParams = mCamera.getParameters();
                mCameraParams.setPreviewSize(PREVIEW_FRAME_WIDTH, PREVIEW_FRAME_HEIGHT);
                mCameraParams.setFocusMode(
                        Camera.Parameters.FOCUS_MODE_CONTINUOUS_PICTURE);
                mCamera.setParameters(mCameraParams);
                mCameraParams = mCamera.getParameters();

                if (IS_DEBUG) logDebug(TAG, "Camera.setPreviewTexture() : E");
                // Dummy surface to gain preview callback.
                mDummySurfaceTexture = new SurfaceTexture(100, true);
                mDummySurfaceTexture.setDefaultBufferSize(8, 8);
                try {
                    mCamera.setPreviewTexture(mDummySurfaceTexture);
                } catch (IOException e) {
                    e.printStackTrace();
                }
                if (IS_DEBUG) logDebug(TAG, "Camera.setPreviewTexture() : X");

                if (IS_DEBUG) logDebug(TAG, "Camera.startPreview() : E");
                mCamera.startPreview();
                if (IS_DEBUG) logDebug(TAG, "Camera.startPreview() : X");

                // Notify.
                nativeOnCameraPrepared(
                        mCameraParams.getPreviewSize().width,
                        mCameraParams.getPreviewSize().height,
                        mCameraParams.getPreviewFormat());

                // Start preview frame handling.
                mFrameBufferRing = new ByteBufferRing(
                        mCameraParams.getPreviewSize().width
                        * mCameraParams.getPreviewSize().height
                        * 3 / 2,
                        PREVIEW_FRAME_RING_BUFFER_SIZE);

                // Request frame.
                mCamera.setPreviewCallbackWithBuffer(mPreviewFrameCallback);
                for (int i = 0; i < PREVIEW_FRAME_RING_BUFFER_SIZE - 2; ++i) {
                    mCamera.addCallbackBuffer(mFrameBufferRing.getCurrent().array());
                    mFrameBufferRing.increment();
                }

            } else {
                if (IS_DEBUG) logDebug(TAG, "Camera is already opened.");
            }

            if (IS_DEBUG) logDebug(TAG, "run() : X");
        }
    }

    private final FinalizeCameraTask mFinalizeCameraTask = new FinalizeCameraTask();
    private class FinalizeCameraTask implements Runnable {
        // Log tag.
        private final String TAG = FinalizeCameraTask.class.getSimpleName();

        @Override
        public void run() {
            if (IS_DEBUG) logDebug(TAG, "run() : E");

            // Finalize camera.
            if (mCamera != null) {
                if (IS_DEBUG) logDebug(TAG, "Close camera : E");
                mCamera.setPreviewCallbackWithBuffer(null);
//                try {
//                    mCamera.setPreviewTexture(null);
//                } catch (IOException e) {
//                    e.printStackTrace();
//                }
                mCamera.stopPreview();
                mCamera.release();
                mCamera = null;
            } else {
                if (IS_DEBUG) logDebug(TAG, "Camera is not opened yet.");
            }
            if (IS_DEBUG) logDebug(TAG, "Close camera : X");

            // Release buffer.
            if (mFrameBufferRing != null) {
                mFrameBufferRing.release();
                mFrameBufferRing = null;
            }

            // Release texture.
            if (mDummySurfaceTexture != null) {
                mDummySurfaceTexture.release();
                mDummySurfaceTexture = null;
            }

            nativeOnCameraReleased();

            if (IS_DEBUG) logDebug(TAG, "run() : X");
        }
    }

    private final PreviewFrameCallback mPreviewFrameCallback = new PreviewFrameCallback();
    private class PreviewFrameCallback implements Camera.PreviewCallback {
        // Log tag.
        private final String TAG = PreviewFrameCallback.class.getSimpleName();

        @Override
        public void onPreviewFrame(final byte[] data, Camera camera) {
            if (IS_DEBUG) logDebug(TAG, "onPreviewFrame() : E");

            if (!mIsSleep) {
                if (!mHandlePreviewCallbackTask.isBusy()) {
                    ByteBuffer buf = mFrameBufferRing.findByByteArray(data);

                    if (IS_DEBUG) logDebug(TAG, "buf.len = " + buf.array().length);
                    if (IS_DEBUG) logDebug(TAG, "buf.offset = " + buf.arrayOffset());
                    if (IS_DEBUG) logDebug(TAG, "buf.capacity = " + buf.capacity());
                    if (IS_DEBUG) logDebug(TAG, "buf.limit = " + buf.limit());
                    if (IS_DEBUG) logDebug(TAG, "buf.position = " + buf.position());

                    mHandlePreviewCallbackTask.setData(
                            buf.array(),
                            buf.arrayOffset(),
                            mFrameBufferRing.getEachBufSize());
                    mBackWorker.execute(mHandlePreviewCallbackTask);
                } else {
                    if (IS_DEBUG) logDebug(TAG, "HandlePreviewCallbackTask is busy.");
                }

                // Request next.
                mCamera.addCallbackBuffer(mFrameBufferRing.getCurrent().array());
                mFrameBufferRing.increment();
            }

            if (IS_DEBUG) logDebug(TAG, "onPreviewFrame() : X");
        }
    }

    private final HandlePreviewCallbackTask mHandlePreviewCallbackTask
            = new HandlePreviewCallbackTask();
    private class HandlePreviewCallbackTask implements Runnable {
        // Log tag.
        private final String TAG = HandlePreviewCallbackTask.class.getSimpleName();

        // Buffer.
        private byte[] mData = null;
        private int mOffset = 0;
        private int mSize = 0;

        // Current task is now running or not.
        private volatile boolean mIsBusy = false;

        // CONSTRUCTOR.
        public HandlePreviewCallbackTask() {
            // NOP.
        }

        /**
         * Set frame data to be processed.
         *
         * @param data
         * @param offset
         * @param size
         */
        public void setData(byte[] data, int offset, int size) {
            mData = data;
            mOffset = offset;
            mSize = size;

            mIsBusy = true;
        }

        /**
         * Task is now running or not.
         *
         * @return
         */
        public boolean isBusy() {
            return mIsBusy;
        }

        @Override
        public void run() {
            if (IS_DEBUG) logDebug(TAG, "run() : E");

            // Check.
            if (mIsSleep) {
                // Already released.
                if (IS_DEBUG) logDebug(TAG, "Already released.");
                return;
            }

            // Send data to native.
            nativeOnPreviewFrameUpdated(
                    mCameraParams.getPreviewSize().width,
                    mCameraParams.getPreviewSize().height,
                    mCameraParams.getPreviewFormat(),
                    mData,
                    mOffset,
                    mSize);

            // Release flag.
            mIsBusy = false;

            if (IS_DEBUG) logDebug(TAG, "run() : X");
        }
    }

    //// RING BUFFER /////////////////////////////////////////////////////////////////////////////

    private static class ByteBufferRing {
        // Buffer list.
        private List<ByteBuffer> mBufferList = new ArrayList<ByteBuffer>();

        // Current index.
        private int mCurrentBufferIndex = 0;

        // Each buffer size.
        private final int mEachBufSize;

        /**
         * CONSTRUCTOR.
         *
         * @param eachBufSize
         * @param ringSize
         */
        public ByteBufferRing(int eachBufSize, int ringSize) {
            mEachBufSize = eachBufSize;

            // Create and add buffers.
            for (int i = 0; i < ringSize; ++i) {
                // Allocate.
                ByteBuffer buf = ByteBuffer.allocateDirect(eachBufSize);
                // Cache.
                mBufferList.add(buf);
            }
        }

        /**
         * Release all resources.
         */
        public synchronized void release() {
            // Clear.
            for (ByteBuffer buf : mBufferList) {
                buf.clear();
            }
            mBufferList.clear();
        }

        /**
         * Get buffer size.
         *
         * @return
         */
        public int getEachBufSize() {
            return mEachBufSize;
        }

        /**
         * Get current active buffer.
         *
         * @return
         */
        public synchronized ByteBuffer getCurrent() {
            return mBufferList.get(mCurrentBufferIndex);
        }

        /**
         * Get next active buffer.
         *
         * @return
         */
        public synchronized ByteBuffer getNext() {
            // Return next item.
            return mBufferList.get(getNextIndex());
        }

        /**
         * Increment active buffer.
         */
        public synchronized void increment() {
            // Change to next index.
            mCurrentBufferIndex = getNextIndex();
        }

        private int getNextIndex() {
            if ((mBufferList.size() - 1) <= mCurrentBufferIndex) {
                // Current index is now on the end of list. Return 0.
                return 0;
            } else {
                return mCurrentBufferIndex + 1;
            }
        }

        /**
         * Get ByteBuffer instance including byteArray.
         *
         * @param byteArray
         * @return
         */
        public synchronized ByteBuffer findByByteArray(byte[] byteArray) {
            for (ByteBuffer eachBuf : mBufferList) {
                if (eachBuf.array() == byteArray) {
                    return eachBuf;
                }
            }
            return null;
        }
    }

    //// THREAD RELATED //////////////////////////////////////////////////////////////////////////

    private static class BackWorkerThreadFactory implements ThreadFactory {
        @Override
        public Thread newThread(Runnable r) {
            Thread thread = new Thread(r);
            thread.setName("BackWorker");
            return thread;
        }
    }

    private static class BackWorkerBlocker implements Runnable {
        // Latch.
        private CountDownLatch mLatch = null;

        /**
         * CONSTRUCTOR.
         */
        public BackWorkerBlocker() {
            mLatch = new CountDownLatch(1);
        }

        /**
         * Block current thread.
         */
        public void await() {
            try {
                mLatch.await();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }

        @Override
        public void run() {
            mLatch.countDown();
        }
    }

    //// NATIVE FUNCTION ACCESSOR ////////////////////////////////////////////////////////////////

    private static final native int nativeOnActivityCreated(
            AssetManager assetManager);
    private static final native int nativeOnActivityDestroyed();

    private static final native int nativeOnCameraPrepared(
            int frameWidth,
            int frameHeight,
            int imageFormat);
    private static final native int nativeOnCameraReleased();
    private static final native int nativeOnPreviewFrameUpdated(
            int frameWidth,
            int frameHeight,
            int imageFormat,
            byte[] frameBuffer,
            int frameBufferOffset,
            int frameBufferSize);

    private static final native int nativeOnSurfaceInitialized(Surface surface);
    private static final native int nativeOnSurfaceFinalized();

    //// UI WEB VIEW /////////////////////////////////////////////////////////////////////////////

    private class UserWebView extends WebView {
        // Log tag.
        private final String TAG = UserWebView.class.getSimpleName();

        // JavaScript native interface name.
        private final String INJECTED_JAVA_SCRIPT_NATIVE_INTERFACE_OBJECT_NAME = "JSNI";

        /**
         * CONSTRUCTOR.
         *
         * @param context
         */
        public UserWebView(Context context) {
            super(context);
            // NOP.
        }

        /**
         * Initialize.
         */
        public void initialize() {
            // Background.
            setBackgroundColor(Color.TRANSPARENT);
            setLayerType(LAYER_TYPE_SOFTWARE, null);

            // Callback.
            setWebViewClient(mUserWebViewClient);

            // Debug.
            setWebContentsDebuggingEnabled(true);

            // Web setting.
            WebSettings webSettings = getSettings();
            webSettings.setBuiltInZoomControls(false);
            webSettings.setDisplayZoomControls(false);
            webSettings.setJavaScriptEnabled(true);

            addJavascriptInterface(mJSNI, INJECTED_JAVA_SCRIPT_NATIVE_INTERFACE_OBJECT_NAME);

            // Load.
            loadUrl("file:///android_asset/web_based_ui/camera_ui.html");
        }

        private String loadJs(String assetsName) {
            String script = "";

            try {
                InputStream fis = getContext().getAssets().open(assetsName);
                BufferedReader reader = new BufferedReader(new InputStreamReader(fis, "UTF-8"));
                String tmp;
                while ((tmp = reader.readLine()) != null) {
                    script = script + tmp + '\n';
                }
                reader.close();
                fis.close();
            } catch (FileNotFoundException e) {
                e.printStackTrace();
            } catch (IOException e) {
                e.printStackTrace();
            }

            return script;
        }

        /**
         * Release all references.
         */
        public void release() {
            stopLoading();
            clearCache(true);
            destroy();

            setWebViewClient(null);
            setWebChromeClient(null);
        }

        private class ExecuteJavaScriptTask implements Runnable {
            // Target script.
            private final String mScript;

            /**
             * CONSTRUCTOR.
             *
             * @param script
             */
            public ExecuteJavaScriptTask(String script) {
                mScript = script;
            }

            @Override
            public void run() {
                evaluateJavascript(mScript, new JavaScriptExecutionCallback());
            }
        }

        private class JavaScriptExecutionCallback implements ValueCallback<String> {
            // Log tag.
            private final String TAG = JavaScriptExecutionCallback.class.getSimpleName();

            @Override
            public void onReceiveValue(String value) {
                if (IS_DEBUG) logDebug(TAG, "onReceiveValue() : " + value);
            }
        }

        private final JavaScriptNativeInterface mJSNI = new JavaScriptNativeInterface();
        private class JavaScriptNativeInterface {
            // Log tag.
            private final String TAG = JavaScriptNativeInterface.class.getSimpleName();

            /**
             * Called on content HTML loaded.
             *
             * @param htmlSrc
             */
            @JavascriptInterface
            public final void onContentHtmlLoaded(final String htmlSrc) {
                // NOP.
            }
        }

        private final UserWebViewClient mUserWebViewClient = new UserWebViewClient();
        private class UserWebViewClient extends WebViewClient {
            @Override
            public void onPageFinished(WebView view, String url) {
                if (IS_DEBUG) logDebug(TAG, "onPageFinished()");
                // NOP.
            }

            @Override
            public boolean shouldOverrideUrlLoading(WebView view, String url) {
                if (IS_DEBUG) logDebug(TAG, "shouldOverrideUrlLoading()");
                if (IS_DEBUG) logDebug(TAG, "URL=" + url);

                UserWebView.this.loadUrl(url);

                return true;
            }
        }



    }



}
