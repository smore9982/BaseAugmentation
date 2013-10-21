package com.android.baseaugmentation.activity;

import java.lang.ref.WeakReference;
import java.lang.reflect.Field;

import org.json.JSONException;
import org.json.JSONObject;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.GestureDetector;
import android.view.LayoutInflater;
import android.view.View;
import android.view.WindowManager;
import android.view.View.MeasureSpec;
import android.view.View.OnClickListener;
import android.view.ViewGroup.LayoutParams;
import android.widget.Button;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.android.baseaugmentation.R;
import com.android.baseaugmentation.renderer.CloudRenderer;
import com.android.baseaugmentation.task.GetAugmentInformation;
import com.android.baseaugmentation.task.GetAugmentInformation.GetAugmentInformationListener;
import com.android.baseaugmentation.task.InitCloudRecoTrackerTask;
import com.android.baseaugmentation.task.InitCloudRecoTrackerTask.InitTrackerTaskListener;
import com.android.baseaugmentation.task.InitQCARTask;
import com.android.baseaugmentation.task.InitQCARTask.InitQCARTaskListener;
import com.android.baseaugmentation.view.CommentOverlayView;
import com.qualcomm.QCAR.QCAR;

public class AugmentActivity extends Activity implements InitQCARTaskListener, InitTrackerTaskListener, GetAugmentInformationListener {
	private static final String NATIVE_LIB_QCAR = "QCAR";
	private static final String NATIVE_LIB_APP = "AUGMENTLIB";
	
	private static final String TAG = "AugmentActivity";
	    
    /*
     * Methods to define in C++
     */
    public native int nativeInitTracker();
    public native void nativeDeinitTracker();
    private native void nativeStartCamera();
    private native void nativeStopCamera();
    private native void nativeSetProjectionMatrix();
    private native void nativeInitApplicationNative(int width, int height);
    private native void nativeDeinitApplicationNative();
    private native void nativeSwitchDatasetAsap();
    private native boolean nativeAutofocus();
    private native boolean nativeSetFocusMode(int mode);
    private native boolean nativeActivateFlash(boolean flash);
    private native void nativeSetActivityPortraitMode(boolean isPortrait);
    private native void nativeEnterScanningMode();
    public native void nativeSetDeviceDPIScaleFactor(float dpiScaleIndicator);    
    public native int nativeInitCloudReco();
    public native void nativeDeinitCloudReco();
    public native void nativeCleanTargetTrackedId();
    public native void nativeProductTextureIsCreated();
    public native void nativeEnterContentMode();
	
    public static final int SCREEN_ORIENTATION_LANDSCAPE = ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE;
    public static final int SCREEN_ORIENTATION_PORTRAIT = ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;
    public static final int SCREEN_ORIENTATION_AUTOROTATE = ActivityInfo.SCREEN_ORIENTATION_SENSOR;
    // Change this value to switch between different screen orientations
    public static int screenOrientation = SCREEN_ORIENTATION_AUTOROTATE;
    
    // Application status constants:
    private static final int APPSTATUS_UNINITED = -1;
    private static final int APPSTATUS_INIT_APP = 0;
    private static final int APPSTATUS_INIT_QCAR = 1;
    private static final int APPSTATUS_INIT_TRACKER = 2;
    private static final int APPSTATUS_INIT_APP_AR = 3;
    private static final int APPSTATUS_INIT_CLOUDRECO = 4;
    private static final int APPSTATUS_INITED = 5;
    private static final int APPSTATUS_CAMERA_STOPPED = 6;
    private static final int APPSTATUS_CAMERA_RUNNING = 7;
    private static final int HIDE_2D_OVERLAY = 0;
    private static final int SHOW_2D_OVERLAY = 1;
    private static final int HIDE_LOADING_DIALOG = 0;
    private static final int SHOW_LOADING_DIALOG = 1;
    // Focus modes:
    private static final int FOCUS_MODE_NORMAL = 0;
    private static final int FOCUS_MODE_CONTINUOUS_AUTO = 1;
    
    static final int INIT_ERROR_NO_NETWORK_CONNECTION = -1;
    static final int INIT_ERROR_SERVICE_NOT_AVAILABLE = -2;
    static final int UPDATE_ERROR_AUTHORIZATION_FAILED = -1;
    static final int UPDATE_ERROR_PROJECT_SUSPENDED = -2;
    static final int UPDATE_ERROR_NO_NETWORK_CONNECTION = -3;
    static final int UPDATE_ERROR_SERVICE_NOT_AVAILABLE = -4;
    static final int UPDATE_ERROR_BAD_FRAME_QUALITY = -5;
    static final int UPDATE_ERROR_UPDATE_SDK = -6;
    static final int UPDATE_ERROR_TIMESTAMP_OUT_OF_RANGE = -7;
    static final int UPDATE_ERROR_REQUEST_TIMEOUT = -8;

    public Object mShutdownLock = new Object();
	private int mAppStatus = -1;
    private int mScreenWidth;
    private int mScreenHeight;
    private InitQCARTask mInitQCARTask;
    private QCARSampleGLView mGlView;
	private int mLastScreenRotation;
	private CloudRenderer mRenderer;
	private InitCloudRecoTrackerTask mInitCloudRecoTask;
	private GetAugmentInformation augmentInfoTask;

    private RelativeLayout mUILayout;
    private TextView mStatusBar;
    private Button mCloseButton;
    private View mLoadingDialogContainer;
    private boolean mContAutofocus;
    private static int mTextureSize = 768;
    private Texture mBookDataTexture;
    
    static class Overlay2dHandler extends Handler
    {
        private final WeakReference<AugmentActivity> mAugmentActivity;

        Overlay2dHandler(AugmentActivity cloudReco){        
        	mAugmentActivity = new WeakReference<AugmentActivity>(cloudReco);
        }

        public void handleMessage(Message msg){
        	AugmentActivity augmentAcitivty = mAugmentActivity.get();
            if (augmentAcitivty == null){
                return;
            }

            if (augmentAcitivty.mCloseButton != null){
                if (msg.what == SHOW_2D_OVERLAY){
                	augmentAcitivty.mCloseButton.setVisibility(View.VISIBLE);
                }
                else{
                	augmentAcitivty.mCloseButton.setVisibility(View.GONE);
                }
            }
        }
    }
    
    static class LoadingDialogHandler extends Handler
    {
        private final WeakReference<AugmentActivity> mCloudReco;

        LoadingDialogHandler(AugmentActivity cloudReco)
        {
            mCloudReco = new WeakReference<AugmentActivity>(cloudReco);
        }


        public void handleMessage(Message msg)
        {
        	AugmentActivity cloudReco = mCloudReco.get();
            if (cloudReco == null){
                return;
            }
            if (msg.what == SHOW_LOADING_DIALOG){
                cloudReco.mLoadingDialogContainer.setVisibility(View.VISIBLE);

            }
            else if (msg.what == HIDE_LOADING_DIALOG){
                cloudReco.mLoadingDialogContainer.setVisibility(View.GONE);
            }
        }
    }

    private Handler loadingDialogHandler = new LoadingDialogHandler(this);
    private Handler overlay2DHandler = new Overlay2dHandler(this);
	
	static{
		Util.loadNative(NATIVE_LIB_QCAR);
		Util.loadNative(NATIVE_LIB_APP);
	}
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
        // Update the application status to start initializing application
        updateApplicationStatus(APPSTATUS_INIT_APP);

        // Gets the current device screen density
        float dpiScaleIndicator = getApplicationContext().getResources()
                .getDisplayMetrics().density;
        
        nativeSetDeviceDPIScaleFactor(dpiScaleIndicator);
	}
		
	@Override
	protected void onPostCreate(Bundle savedInstanceState){
	       super.onPostCreate(savedInstanceState);
	}
		
		
	@Override
	protected void onStart(){
		super.onStart();
	}
	    
	@Override
	protected void onRestart(){
		super.onRestart();
	}

	@Override
	protected void onResume(){
		super.onResume();
		QCAR.onResume();
		if (mAppStatus == APPSTATUS_CAMERA_STOPPED){
            updateApplicationStatus(APPSTATUS_CAMERA_RUNNING);
        }

        // Resume the GL view:
        if (mGlView != null){
            mGlView.setVisibility(View.VISIBLE);
            mGlView.onResume();
        }
        // By default the 2D Overlay is hidden
        hide2DOverlay();
	}
		
	@Override
	protected void onPause(){
		super.onPause();
        // Pauses the OpenGLView
        if (mGlView != null)
        {
            mGlView.setVisibility(View.INVISIBLE);
            mGlView.onPause();
        }

        // Updates the Application current Status
        if (mAppStatus == APPSTATUS_CAMERA_RUNNING)
        {
            updateApplicationStatus(APPSTATUS_CAMERA_STOPPED);
        }
        
        // QCAR-specific pause operation
        QCAR.onPause();
	}
		
	@Override
	protected void onStop(){
		super.onStop();
	}
		
	@Override
	protected void onDestroy(){
        super.onDestroy();
        // Cancel potentially running tasks
        if (mInitQCARTask != null
                && mInitQCARTask.getStatus() != InitQCARTask.Status.FINISHED)
        {
            mInitQCARTask.cancel(true);
            mInitQCARTask = null;
        }

        if (mInitCloudRecoTask != null
                && mInitCloudRecoTask.getStatus() != InitCloudRecoTrackerTask.Status.FINISHED)
        {
            mInitCloudRecoTask.cancel(true);
            mInitCloudRecoTask = null;
        }

        // Ensure that all asynchronous operations to initialize QCAR and
        // loading
        // the tracker datasets do not overlap:
        synchronized (mShutdownLock)
        {

            // Do application deinitialization in native code
            nativeDeinitApplicationNative();

            // Destroy the tracking data set:
            nativeDeinitCloudReco();

            // Deinit the tracker:
            nativeDeinitTracker();

            // Deinitialize QCAR SDK
            QCAR.deinit();
        }

        System.gc();
	}
	
    /**
     * NOTE: this method is synchronized because of a potential concurrent
     * access by VisualSearch::onResume() and InitQCARTask::onPostExecute().
     */
    private synchronized void updateApplicationStatus(int appStatus)
    {
        // Exit if there is no change in status
        if (mAppStatus == appStatus)
            return;

        // Store new status value
        mAppStatus = appStatus;

        // Execute application state-specific actions
        switch (mAppStatus)
        {
        case APPSTATUS_INIT_APP:
        	//Setup screen dimensions and rotation
            initApplication();
            updateApplicationStatus(APPSTATUS_INIT_QCAR);
            break;

        case APPSTATUS_INIT_QCAR:
        	//Initialize QCAR
            try
            {
                mInitQCARTask = new InitQCARTask(this,this);
                mInitQCARTask.execute();
            }
            catch (Exception e)
            {
                
            }
            break;

        case APPSTATUS_INIT_TRACKER:
            // Initialize the tracer
            if (nativeInitTracker() > 0)
            {
                // Proceed to next application initialization status
                updateApplicationStatus(APPSTATUS_INIT_APP_AR);

            }
            break;

        case APPSTATUS_INIT_APP_AR:
            // Initalize surface view and overlay.
            initApplicationAR();

            // Proceed to next application initialization status
            updateApplicationStatus(APPSTATUS_INIT_CLOUDRECO);
            break;

        case APPSTATUS_INIT_CLOUDRECO:
            // Initialize visual search. Can only be done once.
            try
            {
                mInitCloudRecoTask = new InitCloudRecoTrackerTask(this,this);
                mInitCloudRecoTask.execute();
            }
            catch (Exception e)
            {
                
            }
            break;

        case APPSTATUS_INITED:
            System.gc();

            // Activate the renderer
            mRenderer.mIsActive = true;
            //Add the surface view before starting camera 
            addContentView(mGlView, new LayoutParams(LayoutParams.MATCH_PARENT,LayoutParams.MATCH_PARENT));
            // Start the camera:
            updateApplicationStatus(APPSTATUS_CAMERA_RUNNING);
            //Bring ui layout to front.
            mUILayout.bringToFront();

            break;

        case APPSTATUS_CAMERA_STOPPED:
            // Call the native function to stop the camera
            nativeStopCamera();
            break;

        case APPSTATUS_CAMERA_RUNNING:
            // Call the native function to start the camera
            nativeStartCamera();

            // Set continuous auto-focus if supported by the device,
            // otherwise default back to regular auto-focus mode.
            // This will be activated by a tap to the screen in this
            // application.
            if (!nativeSetFocusMode(FOCUS_MODE_CONTINUOUS_AUTO))
            {
                nativeSetFocusMode(FOCUS_MODE_NORMAL);
                mContAutofocus = false;
            }
            else
            {
                mContAutofocus = true;
            }
            break;

        default:
            throw new RuntimeException("Invalid application state");
        }
    }
    
    /** Initialize application GUI elements that are not related to AR. */
    private void initApplication()
    {
        // Set the screen orientation from activity setting:
        int screenOrientation = this.screenOrientation;

        // This is necessary for enabling AutoRotation in the Augmented View
        if (screenOrientation == AugmentActivity.SCREEN_ORIENTATION_AUTOROTATE)
        {
            try{
                // SCREEN_ORIENTATION_FULL_SENSOR is required to allow all 
                // 4 screen rotations if API level >= 9:
                Field fullSensorField = ActivityInfo.class.getField("SCREEN_ORIENTATION_FULL_SENSOR");
                screenOrientation = fullSensorField.getInt(null);
            }
            catch (NoSuchFieldException e){
                // App is running on API level < 9, do nothing.
            }
            catch (Exception e){
                e.printStackTrace();
            }
        }
        // Apply screen orientation
        setRequestedOrientation(screenOrientation);
        updateActivityOrientation();
        storeScreenDimensions();
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON,WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }
    
    /** Initializes AR application components. */
    private void initApplicationAR()
    {
        // Do application initialization in native code (e.g. registering
        // callbacks, etc.)
        nativeInitApplicationNative(mScreenWidth, mScreenHeight);

        // Create OpenGL ES view:
        int depthSize = 16;
        int stencilSize = 0;
        boolean translucent = QCAR.requiresAlpha();

        // Initialize the GLView with proper flags
        mGlView = new QCARSampleGLView(this);
        mGlView.init(QCAR.GL_20, translucent, depthSize, stencilSize);

        // Setups the Renderer of the GLView
        mRenderer = new CloudRenderer(this);
        mRenderer.mActivity = this;
        mGlView.setRenderer(mRenderer);

        // Inflates the Overlay Layout to be displayed above the Camera View
        LayoutInflater inflater = LayoutInflater.from(this);
        mUILayout = (RelativeLayout) inflater.inflate(R.layout.view_camera_overlay,null, false);

        mUILayout.setVisibility(View.VISIBLE);
        mUILayout.setBackgroundColor(Color.BLACK);

        addContentView(mUILayout, new LayoutParams(LayoutParams.MATCH_PARENT,LayoutParams.MATCH_PARENT));

        // Gets a Reference to the Bottom Status Bar
        //mStatusBar = (TextView) mUILayout.findViewById(R.id.overlay_status);

        // By default
        mLoadingDialogContainer = mUILayout.findViewById(R.id.loading_layout);
        mLoadingDialogContainer.setVisibility(View.VISIBLE);

        // Gets a reference to the Close Button
        mCloseButton = (Button) mUILayout.findViewById(R.id.overlay_close_button);

        // Sets the Close Button functionality
        mCloseButton.setOnClickListener(new OnClickListener()
        {
            public void onClick(View v)
            {
                // Cleans the Target Tracker Id
            	nativeCleanTargetTrackedId();
                enterScanningMode();
            }
        });

        // As default the 2D overlay and Status bar are hidden when application
        // starts
        hide2DOverlay();
    }
    
    /** Stores screen dimensions */
    private void storeScreenDimensions()
    {
        // Query display dimensions
        DisplayMetrics metrics = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(metrics);
        mScreenWidth = metrics.widthPixels;
        mScreenHeight = metrics.heightPixels;
    }
    
    private void updateActivityOrientation()
    {
        Configuration config = getResources().getConfiguration();

        boolean isPortrait = false;

        switch (config.orientation)
        {
        case Configuration.ORIENTATION_PORTRAIT:
            isPortrait = true;
            break;
        case Configuration.ORIENTATION_LANDSCAPE:
            isPortrait = false;
            break;
        case Configuration.ORIENTATION_UNDEFINED:
        default:
            break;
        }
        nativeSetActivityPortraitMode(isPortrait);
    }
    
    public void updateRenderView()
    {
        int currentScreenRotation =
            getWindowManager().getDefaultDisplay().getRotation();

        if (currentScreenRotation != mLastScreenRotation)
        {
            // Set projection matrix if there is already a valid one:
            if (QCAR.isInitialized() &&
                (mAppStatus == APPSTATUS_CAMERA_RUNNING))
            {                
                // Query display dimensions:
                storeScreenDimensions();

                // Update viewport via renderer:
                mRenderer.nativeUpdateRendering(mScreenWidth, mScreenHeight);

                // Update projection matrix:
                nativeSetProjectionMatrix();

                // Cache last rotation used for setting projection matrix:
                mLastScreenRotation = currentScreenRotation;
            }
        }
    }
    
    /** Hides the 2D Overlay view and starts CloudReco service again */
    private void enterScanningMode()
    {
        // Hides the 2D Overlay
        hide2DOverlay();

        // Enables CloudReco Scanning Mode in Native code
        nativeEnterScanningMode();
    }
    

    /** Displays the 2D Book Overlay */
    public void show2DOverlay()
    {
        // Sends the Message to the Handler in the UI thread
        overlay2DHandler.sendEmptyMessage(SHOW_2D_OVERLAY);
    }


    /** Hides the 2D Book Overlay */
    public void hide2DOverlay()
    {
        // Sends the Message to the Handler in the UI thread
        overlay2DHandler.sendEmptyMessage(HIDE_2D_OVERLAY);
    }
    
	@Override
	public void onQCARInitSuccess() {
		 updateApplicationStatus(APPSTATUS_INIT_TRACKER);
	}
	@Override
	public void onQCARInitFailure() {
		Log.e(TAG, "Failed to InitQCAR");
		
	}
	@Override
	public void onLoadTrackerSuccess(int result) {
        // Done loading the tracker, update application status:
        updateApplicationStatus(APPSTATUS_INITED);
        loadingDialogHandler.sendEmptyMessage(HIDE_LOADING_DIALOG);
        mUILayout.setBackgroundColor(Color.TRANSPARENT);		
	}
	@Override
	public void onLoadTrackerFailure(int result) {
        // Create dialog box for display error:
        AlertDialog dialogError = new AlertDialog.Builder(
                AugmentActivity.this).create();
        dialogError.setButton(DialogInterface.BUTTON_POSITIVE,"Ok",
                new DialogInterface.OnClickListener()
                {
                    public void onClick(DialogInterface dialog,
                            int which)
                    {
                        // Exiting application
                        System.exit(1);
                    }
                });

        // Show dialog box with error message:
        String logMessage = "Failed to initialize CloudReco.";

        // NOTE: Check if initialization failed because the device is
        // not supported. At this point the user should be informed
        // with a message.
        if (result == INIT_ERROR_NO_NETWORK_CONNECTION)
            logMessage = "Failed to initialize CloudReco because "
                    + "the device has no network connection.";
        else if (result == INIT_ERROR_SERVICE_NOT_AVAILABLE)
            logMessage = "Failed to initialize CloudReco because "
                    + "the service is not available.";

        dialogError.setMessage(logMessage);
        dialogError.show();
		
	}
	@Override
	public int loadTrackerData() {
		// TODO Auto-generated method stub
		return 0;
	}
	
    /**
     * Generates a texture for the book data fecthing the book info from
     */
    public void createProductTexture(String targetMetaData)
    {
    	// Cleans old texture reference if necessary
        if (mBookDataTexture != null)
        {
            mBookDataTexture = null;
            System.gc();
        }
        
    	//Retrieve info.
        augmentInfoTask = new GetAugmentInformation(this, this);
        this.loadingDialogHandler.sendEmptyMessage(SHOW_LOADING_DIALOG);
        String[] params = {targetMetaData};
        augmentInfoTask.execute(params);
    }
	
	
	@Override
	public void onSuccess(JSONObject obj) {
        // Generates a View to display the book data
        CommentOverlayView productView = new CommentOverlayView(AugmentActivity.this);

        // Updates the view used as a 3d Texture
        try {
        	
			productView.updateCommentOverlay(obj.getString("title"), obj.getString("description"));
		} catch (JSONException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}

        // Sets the layout params
        productView.setLayoutParams(new LayoutParams(
                RelativeLayout.LayoutParams.WRAP_CONTENT,
                RelativeLayout.LayoutParams.WRAP_CONTENT));

        // Sets View measure - This size should be the same as the
        // texture generated to display the overlay in order for the
        // texture to be centered in screen
        productView.measure(MeasureSpec.makeMeasureSpec(mTextureSize,
                MeasureSpec.EXACTLY), MeasureSpec.makeMeasureSpec(
                mTextureSize, MeasureSpec.EXACTLY));

        // updates layout size
        productView.layout(0, 0, productView.getMeasuredWidth(),
                productView.getMeasuredHeight());

        // Draws the View into a Bitmap. Note we are allocating several
        // large memory buffers thus attempt to clear them as soon as
        // they are no longer required:
        Bitmap bitmap = Bitmap.createBitmap(mTextureSize, mTextureSize,
                Bitmap.Config.ARGB_8888);
        
        Canvas c = new Canvas(bitmap);
        productView.draw(c);

        // Clear the product view as it is no longer needed
        productView = null;
        System.gc();
        
        // Allocate int buffer for pixel conversion and copy pixels
        int width = bitmap.getWidth();
        int height = bitmap.getHeight();
        
        int[] data = new int[bitmap.getWidth() * bitmap.getHeight()];
        bitmap.getPixels(data, 0, bitmap.getWidth(), 0, 0, bitmap.getWidth(),
                bitmap.getHeight());
        
        // Recycle the bitmap object as it is no longer needed
        bitmap.recycle();
        bitmap = null;
        c = null;
        System.gc();   
        
        // Generates the Texture from the int buffer
        mBookDataTexture = Texture.loadTextureFromIntBuffer(data,
                                width, height);

        // Clear the int buffer as it is no longer needed
        data = null;
        System.gc(); 
                        
        // Hides the loading dialog from a UI thread
        loadingDialogHandler.sendEmptyMessage(HIDE_LOADING_DIALOG);
		nativeProductTextureIsCreated();
	}
	@Override
	public void onFailure(Exception e) {

		
	}
	
    /** Returns the current Book Data Texture */
    private Texture getProductTexture()
    {
        return mBookDataTexture;
    }
    
    /**
     * Starts application content Mode Displays UI OVerlays and turns CloudReco
     * off
     */
    public void enterContentMode()
    {
    	Log.i(TAG, "Entering Content Mode");
        // Shows the 2D Overlay
        show2DOverlay();

        // Enters content mode to disable CloudReco in Native
        this.nativeEnterContentMode();
    }
}

