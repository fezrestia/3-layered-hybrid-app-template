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
    private SurfaceTexture mCameraPreviewStream = null;
    private static final int PREVIEW_FRAME_WIDTH = 1280;
    private static final int PREVIEW_FRAME_HEIGHT = 720;
    private static final int PICTURE_FRAME_WIDTH = 1280;
    private static final int PICTURE_FRAME_HEIGHT = 720;

    // UI.
    private FrameLayout mRootView = null;
    private TextureView mTextureView = null;
    private UserWebView mUserWebView = null;

    // Thread.
    private Handler mUiWorker = new Handler();
    private ExecutorService mCameraBackWorker = Executors.newSingleThreadExecutor(
            new CameraBackWorkerThreadFactory());
    private ExecutorService mStateBackWorker = Executors.newSingleThreadExecutor(
            new StateBackWorkerThreadFactory());

    // State.
    private ControlState mCurrentState = new StateNone();

    //// ANDROID APPLICATION LIFE CYCLE RELATED ///////////////////////////////////////////////////

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
        mUserWebView.setAlpha(1.0f);

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

        // State.
        mControlEventInterface.onResume();

        // Camera.
        mCameraBackWorker.execute(mInitializeCameraTask);

        // Surface.
        mUiWorker.post(mCheckSurfaceTextureTask);

        if (IS_DEBUG) logDebug(TAG, "onResume() : X");
    }

    /**
     * Life-Cycle interface.
     */
    @Override
    public void onPause() {
        if (IS_DEBUG) logDebug(TAG, "onPause() : E");

        // Camera.
        mCameraBackWorker.execute(mFinalizeCameraTask);

        // State.
        mControlEventInterface.onPause();

        super.onPause();
        if (IS_DEBUG) logDebug(TAG, "onPause() : X");
    }

    /**
     * Life-Cycle interface.
     */
    @Override
    public void onDestroy() {
        if (IS_DEBUG) logDebug(TAG, "onDestroy() : E");

        // Await finalize camera worker thread.
        BackWorkerBlocker cameraBlocker = new BackWorkerBlocker();
        mCameraBackWorker.execute(cameraBlocker);
        cameraBlocker.await();

        // Await finalize state worker thread.
        BackWorkerBlocker stateBlocker = new BackWorkerBlocker();
        mStateBackWorker.execute(stateBlocker);
        stateBlocker.await();

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

    //// UI SURFACE TEXTURE RELATED ///////////////////////////////////////////////////////////////

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

            // Notify to native.
            Runnable task = new NotifySurfaceInitializedTask(new Surface(surface));
            mCameraBackWorker.execute(task);

            // Register camera preview stream.
            mCameraBackWorker.execute(mRegisterCameraPreviewStreamTask);

            // Notify to state.
            mControlEventInterface.onUiSurfaceInitialized();

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
                nativeOnUiSurfaceInitialized(mSurface);
            }
        }

        @Override
        public boolean onSurfaceTextureDestroyed(final SurfaceTexture surface) {
            if (IS_DEBUG) logDebug(TAG, "onSurfaceTextureDestroyed() : E");

            // Unregister camera preview stream
            mCameraBackWorker.execute(mUnregisterCameraPreviewStreamTask);

            // Notify to native.
            Runnable task = new NotifySurfaceFinalizedTask();
            mCameraBackWorker.execute(task);

            // Notify to state.
            mControlEventInterface.onUiSurfaceFinalized();

            if (IS_DEBUG) logDebug(TAG, "onSurfaceTextureDestroyed() : X");
            return false;
        }

        private class NotifySurfaceFinalizedTask implements Runnable {
            @Override
            public void run() {
                nativeOnUiSurfaceFinalized();
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
//            if (IS_DEBUG) logDebug(TAG, "onSurfaceTextureUpdated() : E");
            // NOP.
//            if (IS_DEBUG) logDebug(TAG, "onSurfaceTextureUpdated() : X");
        }
    }

    private final CheckSurfaceTextureTask mCheckSurfaceTextureTask = new CheckSurfaceTextureTask();
    private class CheckSurfaceTextureTask implements Runnable {
        // Log tag.
        private final String TAG = CheckSurfaceTextureTask.class.getSimpleName();

        @Override
        public void run() {
            if (IS_DEBUG) logDebug(TAG, "run() : E");

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

            if (IS_DEBUG) logDebug(TAG, "run() : X");
        }
    }

    //// NATIVE TO JAVA COMMAND RELATED ///////////////////////////////////////////////////////////

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

    //// CAMERA INITIALIZE / FINALIZE RELATED /////////////////////////////////////////////////////

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
                mCameraParams.setPictureSize(PICTURE_FRAME_WIDTH, PICTURE_FRAME_HEIGHT);
                mCameraParams.setFocusMode(Camera.Parameters.FOCUS_MODE_AUTO);
                mCamera.setParameters(mCameraParams);
                mCameraParams = mCamera.getParameters();

                if (IS_DEBUG) logDebug(TAG, "Camera.startPreview() : E");
                mCamera.startPreview();
                if (IS_DEBUG) logDebug(TAG, "Camera.startPreview() : X");

                // Notify.
                nativeOnCameraPrepared(
                        mCameraParams.getPreviewSize().width,
                        mCameraParams.getPreviewSize().height,
                        mCameraParams.getPreviewFormat());

                // Notify to state.
                mControlEventInterface.onCameraInitialized();
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

            // Notify.
            nativeOnCameraReleased();

            // Notify to state.
            mControlEventInterface.onCameraFinalized();

            if (IS_DEBUG) logDebug(TAG, "run() : X");
        }
    }

    //// CAMERA PREVIEW STREAM RELATED ///////////////////////////////////////////////////////////

    private final RegisterCameraPreviewStreamTask mRegisterCameraPreviewStreamTask
            = new RegisterCameraPreviewStreamTask();
    private class RegisterCameraPreviewStreamTask implements Runnable {
        // Log tag.
        private final String TAG = RegisterCameraPreviewStreamTask.class.getSimpleName();

        @Override
        public void run() {
            if (IS_DEBUG) logDebug(TAG, "run() : E");

            if (mCamera != null) {
                // Surface to gain preview stream.
                final int previewTexName = nativeGetTextureNameOfCameraPreviewStream();
                if (IS_DEBUG) logDebug(TAG, "PreviewTex = " + previewTexName);

                // Surface texture.
                mCameraPreviewStream = new SurfaceTexture(previewTexName);
                mCameraPreviewStream.setOnFrameAvailableListener(mCameraPreviewStreamCallback);

                // Notify to native.
                nativeOnCameraPreviewStreamInitialized(
                        new Surface(mCameraPreviewStream),
                        PREVIEW_FRAME_WIDTH,
                        PREVIEW_FRAME_HEIGHT);

                try {
                    mCamera.setPreviewTexture(mCameraPreviewStream);
                } catch (IOException e) {
                    e.printStackTrace();
                }
            } else {
                if (IS_DEBUG) logDebug(TAG, "Camera is already released.");
            }

            if (IS_DEBUG) logDebug(TAG, "run() : X");
        }
    }

    private final UnregisterCameraPreviewStreamTask mUnregisterCameraPreviewStreamTask
            = new UnregisterCameraPreviewStreamTask();
    private class UnregisterCameraPreviewStreamTask implements Runnable {
        // Log tag.
        private final String TAG = UnregisterCameraPreviewStreamTask.class.getSimpleName();

        @Override
        public void run() {
            if (IS_DEBUG) logDebug(TAG, "run() : E");

            // Release texture.
            if (mCameraPreviewStream != null) {
                // Notify to native.
                nativeOnCameraPreviewStreamFinalized();

                // Release.
                mCameraPreviewStream.setOnFrameAvailableListener(null);
                mCameraPreviewStream.release();
                mCameraPreviewStream = null;
            }

            if (IS_DEBUG) logDebug(TAG, "run() : X");
        }
    }

    private final CameraPreviewStreamCallback mCameraPreviewStreamCallback
            = new CameraPreviewStreamCallback();
    private class CameraPreviewStreamCallback implements SurfaceTexture.OnFrameAvailableListener {
        // Log tag.
        private final String TAG = CameraPreviewStreamCallback.class.getSimpleName();

        @Override
        public void onFrameAvailable(SurfaceTexture surfaceTexture) {
//            if (IS_DEBUG) logDebug(TAG, "onFrameAvailable() : E");

            mCameraBackWorker.execute(mHandleCameraPreviewStreamCallbackTask);

//            if (IS_DEBUG) logDebug(TAG, "onFrameAvailable() : X");
        }
    }

    private final HandleCameraPreviewStreamCallbackTask mHandleCameraPreviewStreamCallbackTask
            = new HandleCameraPreviewStreamCallbackTask();
    private class HandleCameraPreviewStreamCallbackTask implements Runnable {
        // Log tag.
        private final String TAG = HandleCameraPreviewStreamCallbackTask.class.getSimpleName();

        @Override
        public void run() {
//            if (IS_DEBUG) logDebug(TAG, "run() : E");

            // Bind to application EGL.
            nativeBindApplicationEglContext();

            // Update texture.
            mCameraPreviewStream.updateTexImage();

            // Update matrix.
            float[] matrix = new float[16];
            mCameraPreviewStream.getTransformMatrix(matrix);
            nativeSetSurfaceTextureTransformMatrix(matrix);

            // Notify to native.
            nativeOnCameraPreviewStreamUpdated();

            // Unbind from application EGL.
            nativeUnbindApplicationEglContext();

//            if (IS_DEBUG) logDebug(TAG, "run() : X");
        }
    }

    private final StartScanTask mStartScanTask = new StartScanTask();
    private class StartScanTask implements Runnable {
        // Log tag.
        private final String TAG = StartScanTask.class.getSimpleName();

        @Override
        public void run(){
            if (IS_DEBUG) logDebug(TAG, "run() : E");

            if (mCamera != null) {
                mCamera.autoFocus(new ScanDoneCallback());
            } else {
                if (IS_DEBUG) logDebug(TAG, "Camera is null.");
            }

            if (IS_DEBUG) logDebug(TAG, "run() : X");
        }

        private class ScanDoneCallback implements Camera.AutoFocusCallback {
            @Override
            public void onAutoFocus(boolean isSuccess, Camera camera) {
                mControlEventInterface.onScanLockDone();
            }
        }
    }

    private final CancelScanTask mCancelScanTask = new CancelScanTask();
    private class CancelScanTask implements Runnable {
        // Log tag.
        private final String TAG = CancelScanTask.class.getSimpleName();

        @Override
        public void run(){
            if (IS_DEBUG) logDebug(TAG, "run() : E");

            if (mCamera != null) {
                mCamera.cancelAutoFocus();
            } else {
                if (IS_DEBUG) logDebug(TAG, "Camera is null.");
            }

            if (IS_DEBUG) logDebug(TAG, "run() : X");
        }
    }

    private final StillCaptureTask mStillCaptureTask = new StillCaptureTask();
    private class StillCaptureTask implements Runnable {
        // Log tag.
        private final String TAG = StillCaptureTask.class.getSimpleName();

        @Override
        public void run(){
            if (IS_DEBUG) logDebug(TAG, "run() : E");

            if (mCamera != null) {
                mCamera.takePicture(
                    new StillCaptureShutterCallback(),
                    null,
                    new StillCaptureDoneCallback());
            } else {
                if (IS_DEBUG) logDebug(TAG, "Camera is null.");
            }

            if (IS_DEBUG) logDebug(TAG, "run() : X");
        }

        private class StillCaptureShutterCallback implements Camera.ShutterCallback {
            // Log tag.
            private final String TAG = StillCaptureShutterCallback.class.getSimpleName();

            @Override
            public void onShutter() {
                mControlEventInterface.onStillShutterDone();
            }
        }

        private class StillCaptureDoneCallback implements Camera.PictureCallback {
            // Log tag.
            private final String TAG = StillCaptureDoneCallback.class.getSimpleName();

            @Override
            public void onPictureTaken(byte[] data, Camera camera) {
                mControlEventInterface.onStillCaptureDone();

                if (mCamera != null) {
                    // Restart preview.
                    mCamera.startPreview();
                }
            }
        }
    }

    //// THREAD RELATED //////////////////////////////////////////////////////////////////////////

    private static class CameraBackWorkerThreadFactory implements ThreadFactory {
        @Override
        public Thread newThread(Runnable r) {
            Thread thread = new Thread(r);
            thread.setName("CameraWorker");
            return thread;
        }
    }

    private static class StateBackWorkerThreadFactory implements ThreadFactory {
        @Override
        public Thread newThread(Runnable r) {
            Thread thread = new Thread(r);
            thread.setName("StateWorker");
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

    private static final native int nativeOnCameraPreviewStreamInitialized(
            Surface surface,
            int width,
            int height);
    private static final native int nativeOnCameraPreviewStreamFinalized();

    private static final native int nativeOnUiSurfaceInitialized(Surface surface);
    private static final native int nativeOnUiSurfaceFinalized();

    private static final native int nativeGetTextureNameOfCameraPreviewStream();
    private static final native int nativeSetSurfaceTextureTransformMatrix(float[] matrix);

    private static final native int nativeOnCameraPreviewStreamUpdated();

    private static final native int nativeBindApplicationEglContext();
    private static final native int nativeUnbindApplicationEglContext();

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
            loadUrl("file:///android_asset/web_based_ui/web_based_ui.html");
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

            // Touch event.
            private final String TOUCH_EVENT_DOWN = "down";
            private final String TOUCH_EVENT_MOVE = "move";
            private final String TOUCH_EVENT_UP = "up";

            // View ID.
            private final String LEFT_TOP_ICON = "left-top-icon";
            private final String LEFT_BOTTOM_ICON = "left-bottom-icon";
            private final String RIGHT_TOP_ICON = "right-top-icon";
            private final String RIGHT_BOTTOM_ICON = "right-bottom-icon";

            /**
             * Main capture icon clicked.
             */
            @JavascriptInterface
            public final void onFourCornerIconTouched(String viewId, String event) {
                if (IS_DEBUG) logDebug(TAG, "onFourCornerIconTouched() : E");
                if (IS_DEBUG) logDebug(TAG, "ViewID = " + viewId);
                if (IS_DEBUG) logDebug(TAG, "Event = " + event);

                switch(event) {
                    case TOUCH_EVENT_DOWN:
                        mControlEventInterface.onScanLockRequested();
                        break;

                    case TOUCH_EVENT_MOVE:
                        // NOP.
                        break;

                    case TOUCH_EVENT_UP:
                        mControlEventInterface.onStillCaptureRequested();
                        break;

                    default:
                        throw new IllegalArgumentException("Unexpected event : " + event);
                }

                if (IS_DEBUG) logDebug(TAG, "onFourCornerIconTouched() : X");
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

    //// STATE MACHINE ////////////////////////////////////////////////////////////////////////////

    private void changeStateTo(ControlState next) {
        if (IS_DEBUG) logDebug(TAG, "changeStateTo() : NEXT = " + next.getClass().getSimpleName());

        mCurrentState.exit();
        mCurrentState = next;
        mCurrentState.entry();
    }

    private enum ControlEvent {
        // Life-cycle.
        ON_RESUME,
        ON_PAUSE,

        // Device.
        ON_CAMERA_INITIALIZED,
        ON_CAMERA_FINALIZED,

        // Surface.
        ON_UI_SURFACE_INITIALIZED,
        ON_UI_SURFACE_FINALIZED,

        // Still capture.
        ON_SCAN_LOCK_REQUESTED,
        ON_SCAN_LOCK_CANCELED,
        ON_SCAN_LOCK_DONE,
        ON_STILL_CAPTURE_REQUESTED,
        ON_STILL_SHUTTER_DONE,
        ON_STILL_CAPTURE_DONE,



    }

    private abstract class ControlState {
        // Log tag.
        private final String TAG = ControlState.class.getSimpleName();

        protected void entry() {
            if (IS_DEBUG) logDebug(TAG, "entry()");
        }

        protected void exit() {
            if (IS_DEBUG) logDebug(TAG, "exit()");
        }

        protected void onResume() {
            if (IS_DEBUG) logDebug(TAG, "onResume()");
        }

        protected void onPause() {
            if (IS_DEBUG) logDebug(TAG, "onPause()");
        }

        protected void onCameraInitialized() {
            if (IS_DEBUG) logDebug(TAG, "onCameraInitialized()");
        }

        protected void onCameraFinalized() {
            if (IS_DEBUG) logDebug(TAG, "onCameraFinalized()");
        }

        protected void onUiSurfaceInitialized() {
            if (IS_DEBUG) logDebug(TAG, "onUiSurfaceInitialized()");
        }

        protected void onUiSurfaceFinalized() {
            if (IS_DEBUG) logDebug(TAG, "onUiSurfaceFinalized()");
        }

        protected void onScanLockRequested() {
            if (IS_DEBUG) logDebug(TAG, "onScanLockRequested()");
        }

        protected void onScanLockCanceled() {
            if (IS_DEBUG) logDebug(TAG, "onScanLockCanceled()");
        }

        protected void onScanLockDone() {
            if (IS_DEBUG) logDebug(TAG, "onScanLockDone()");
        }

        protected void onStillCaptureRequested() {
            if (IS_DEBUG) logDebug(TAG, "onStillCaptureRequested()");
        }

        protected void onStillShutterDone() {
            if (IS_DEBUG) logDebug(TAG, "onStillShutterDone()");
        }

        protected void onStillCaptureDone() {
            if (IS_DEBUG) logDebug(TAG, "onStillCaptureDone()");
        }



    }

    private class StateNone extends ControlState {
        // Log tag.
        private final String TAG = StateNone.class.getSimpleName();

        @Override
        public void onResume() {
            if (IS_DEBUG) logDebug(TAG, "onResume() : E");

            changeStateTo(new StateResume());

            if (IS_DEBUG) logDebug(TAG, "onResume() : X");
        }
    }

    private class StateResume extends ControlState {
        // Log tag.
        private final String TAG = StateResume.class.getSimpleName();

        // Flags.
        private boolean mIsUiSurfaceAlreadyReady = false;
        private boolean mIsCameraAlreadyReady = false;

        @Override
        public void onPause() {
            if (IS_DEBUG) logDebug(TAG, "onPause()");

            changeStateTo(new StatePause());
        }

        @Override
        public void onCameraInitialized() {
            if (IS_DEBUG) logDebug(TAG, "onCameraInitialized()");

            mIsCameraAlreadyReady = true;

            checkCameraAndSurfaceAreReadyOrNot();
        }

        @Override
        public void onUiSurfaceInitialized() {
            if (IS_DEBUG) logDebug(TAG, "onUiSurfaceInitialized()");

            mIsUiSurfaceAlreadyReady = true;

            checkCameraAndSurfaceAreReadyOrNot();
        }

        private void checkCameraAndSurfaceAreReadyOrNot() {
            if (mIsCameraAlreadyReady && mIsUiSurfaceAlreadyReady) {
                if (IS_DEBUG) logDebug(TAG, "Camera and Surface are already ready.");

                changeStateTo(new StateIdle());
            } else {
                if (IS_DEBUG) logDebug(TAG, "Camera or Surface is NOT ready yet.");
                // NOP. Wait for ready.
            }
        }
    }

    private class StateIdle extends ControlState {
        // Log tag.
        private final String TAG = StateIdle.class.getSimpleName();

        @Override
        public void onPause() {
            if (IS_DEBUG) logDebug(TAG, "onPause()");

            changeStateTo(new StatePause());
        }

        @Override
        public void onScanLockRequested() {
            if (IS_DEBUG) logDebug(TAG, "onScanLockRequested()");

            // Start scan.
            mCameraBackWorker.execute(mStartScanTask);

            changeStateTo(new StateScan());
        }
    }

    private class StateScan extends ControlState {
        // Log tag.
        private final String TAG = StateScan.class.getSimpleName();

        // Flags.
        private boolean mIsAlreadyScanDone = false;
        private boolean mIsAlreadyCaptureRequested = false;

        @Override
        public void onPause() {
            if (IS_DEBUG) logDebug(TAG, "onPause()");

            changeStateTo(new StatePause());
        }

        @Override
        public void onScanLockCanceled() {
            if (IS_DEBUG) logDebug(TAG, "onScanLockCanceled()");

            // Cancel scan.
            mCameraBackWorker.execute(mCancelScanTask);

            changeStateTo(new StateIdle());
        }

        @Override
        public void onScanLockDone() {
            if (IS_DEBUG) logDebug(TAG, "onScanLockDone()");

            mIsAlreadyScanDone = true;

            checkScanStateAndCaptureRequest();
        }

        @Override
        public void onStillCaptureRequested() {
            if (IS_DEBUG) logDebug(TAG, "onStillCaptureRequested()");

            mIsAlreadyCaptureRequested = true;

            checkScanStateAndCaptureRequest();
        }

        private void checkScanStateAndCaptureRequest() {
            if (mIsAlreadyScanDone && !mIsAlreadyCaptureRequested) {
                // Only scan done. User does not request capture.

                changeStateTo(new StateScanDone());
            } else if (!mIsAlreadyScanDone && mIsAlreadyCaptureRequested) {
                // NOP. Scan is not done yet, but user request capture.
            } else if (mIsAlreadyScanDone && mIsAlreadyCaptureRequested) {
                // Scan is already done, and user requests capture.

                // Capture.
                mCameraBackWorker.execute(mStillCaptureTask);

                changeStateTo(new StateStillCapture());
            } else {
                throw new IllegalStateException("Scan not done, user does not request capture.");
            }
        }
    }

    private class StateScanDone extends ControlState {
        // Log tag.
        private final String TAG = StateScan.class.getSimpleName();

        @Override
        public void onPause() {
            if (IS_DEBUG) logDebug(TAG, "onPause()");

            changeStateTo(new StatePause());
        }

        @Override
        public void onScanLockCanceled() {
            if (IS_DEBUG) logDebug(TAG, "onScanLockCanceled()");

            // Cancel scan.
            mCameraBackWorker.execute(mCancelScanTask);

            changeStateTo(new StateIdle());
        }

        @Override
        public void onStillCaptureRequested() {
            if (IS_DEBUG) logDebug(TAG, "onStillCaptureRequested()");

            // Capture.
            mCameraBackWorker.execute(mStillCaptureTask);

            changeStateTo(new StateStillCapture());
        }
    }

    private class StateStillCapture extends ControlState {
        // Log tag.
        private final String TAG = StateStillCapture.class.getSimpleName();

        @Override
        public void onPause() {
            if (IS_DEBUG) logDebug(TAG, "onPause()");

            changeStateTo(new StatePause());
        }

        @Override
        public void onStillShutterDone() {
            if (IS_DEBUG) logDebug(TAG, "onStillShutterDone()");



        }

        @Override
        public void onStillCaptureDone() {
            if (IS_DEBUG) logDebug(TAG, "onStillCaptureDone()");



            changeStateTo(new StateIdle());
        }
    }

    private class StatePause extends ControlState {
        // Log tag.
        private final String TAG = StatePause.class.getSimpleName();

        @Override
        protected void onCameraFinalized() {
            if (IS_DEBUG) logDebug(TAG, "onCameraFinalized()");

            changeStateTo(new StateNone());
        }

//        @Override
//        protected void onUiSurfaceFinalized() {
//            if (IS_DEBUG) logDebug(TAG, "onUiSurfaceFinalized()");
//        }
    }

    private final ControlEventInterface mControlEventInterface = new ControlEventInterface();
    private class ControlEventInterface extends ControlState {
        // Send event task.
        private class HandleEventTask implements Runnable {
            // Event.
            private final ControlEvent mEvent;

            /**
             * CONSTRUCTOR.
             *
             * @param event
             */
            public HandleEventTask(ControlEvent event) {
                mEvent = event;
            }

            @Override
            public void run() {
                handleEvent(mEvent);
            }
        }

        protected void onResume() {
            mStateBackWorker.execute(new HandleEventTask(ControlEvent.ON_RESUME));
        }

        protected void onPause() {
            mStateBackWorker.execute(new HandleEventTask(ControlEvent.ON_PAUSE));
        }

        protected void onCameraInitialized() {
            mStateBackWorker.execute(new HandleEventTask(ControlEvent.ON_CAMERA_INITIALIZED));
        }

        protected void onCameraFinalized() {
            mStateBackWorker.execute(new HandleEventTask(ControlEvent.ON_CAMERA_FINALIZED));
        }

        protected void onUiSurfaceInitialized() {
            mStateBackWorker.execute(new HandleEventTask(ControlEvent.ON_UI_SURFACE_INITIALIZED));
        }

        protected void onUiSurfaceFinalized() {
            mStateBackWorker.execute(new HandleEventTask(ControlEvent.ON_UI_SURFACE_FINALIZED));
        }

        protected void onScanLockRequested() {
            mStateBackWorker.execute(new HandleEventTask(ControlEvent.ON_SCAN_LOCK_REQUESTED));
        }

        protected void onScanLockCanceled() {
            mStateBackWorker.execute(new HandleEventTask(ControlEvent.ON_SCAN_LOCK_CANCELED));
        }

        protected void onScanLockDone() {
            mStateBackWorker.execute(new HandleEventTask(ControlEvent.ON_SCAN_LOCK_DONE));
        }

        protected void onStillCaptureRequested() {
            mStateBackWorker.execute(new HandleEventTask(ControlEvent.ON_STILL_CAPTURE_REQUESTED));
        }

        protected void onStillShutterDone() {
            mStateBackWorker.execute(new HandleEventTask(ControlEvent.ON_STILL_SHUTTER_DONE));
        }

        protected void onStillCaptureDone() {
            mStateBackWorker.execute(new HandleEventTask(ControlEvent.ON_STILL_CAPTURE_DONE));
        }

        // Event handler.
        private void handleEvent(ControlEvent event) {
            if (IS_DEBUG) logDebug(TAG, "handleEvent() : EVENT=" + event.name());

            switch (event) {
                case ON_RESUME:
                    mCurrentState.onResume();
                    break;

                case ON_PAUSE:
                    mCurrentState.onPause();
                    break;

                case ON_CAMERA_INITIALIZED:
                    mCurrentState.onCameraInitialized();
                    break;

                case ON_CAMERA_FINALIZED:
                    mCurrentState.onCameraFinalized();
                    break;

                case ON_UI_SURFACE_INITIALIZED:
                    mCurrentState.onUiSurfaceInitialized();
                    break;

                case ON_UI_SURFACE_FINALIZED:
                    mCurrentState.onUiSurfaceFinalized();
                    break;

                case ON_SCAN_LOCK_REQUESTED:
                    mCurrentState.onScanLockRequested();
                    break;

                case ON_SCAN_LOCK_CANCELED:
                    mCurrentState.onScanLockCanceled();
                    break;

                case ON_SCAN_LOCK_DONE:
                    mCurrentState.onScanLockDone();
                    break;

                case ON_STILL_CAPTURE_REQUESTED:
                    mCurrentState.onStillCaptureRequested();
                    break;

                case ON_STILL_SHUTTER_DONE:
                    mCurrentState.onStillShutterDone();
                    break;

                case ON_STILL_CAPTURE_DONE:
                    mCurrentState.onStillCaptureDone();
                    break;

                default:
                    throw new IllegalArgumentException("Unexpected event : " + event.name());
            }


        }


    }


}
