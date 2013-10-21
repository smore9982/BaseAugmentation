#include "Augment.h"

// ----------------------------------------------------------------------------
// Object to receive update callbacks from QCAR SDK
// ----------------------------------------------------------------------------
class AugmentActivity_UpdateCallback : public QCAR::UpdateCallback
{
    virtual void QCAR_onUpdate(QCAR::State& state)
    {
        // Get the tracker manager:
        QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();

        // Get the image tracker:
        QCAR::ImageTracker* imageTracker = static_cast<QCAR::ImageTracker*>(
                trackerManager.getTracker(QCAR::Tracker::IMAGE_TRACKER));

        // Get the target finder:
        QCAR::TargetFinder* finder = imageTracker->getTargetFinder();

        // Check if there are new results available:
        const int statusCode = finder->updateSearchResults();

        // Show a message if we encountered an error:
        if (statusCode < 0)
        {
            //showErrorMessage(statusCode, state.getFrame().getTimeStamp());
        }
        else if (statusCode == QCAR::TargetFinder::UPDATE_RESULTS_AVAILABLE)
        {
            // Process new search results
            if (finder->getResultCount() > 0)
            {
                const QCAR::TargetSearchResult* result = finder->getResult(0);

                // Check if this target is suitable for tracking:
                if (result->getTrackingRating() > 0)
                {
                    // Create a new Trackable from the result:
                    QCAR::Trackable* newTrackable = finder->enableTracking(*result);
                    if (newTrackable != 0)
                    {
                        LOG("Successfully created new trackable '%s' with rating '%d'.",
                                newTrackable->getName(), result->getTrackingRating());

                        // Checks if the targets has changed
                        LOG( "Comparing Strings. currentTargetId: %s  lastTargetId: %s",
                                result->getUniqueTargetId(), lastTargetId);

                        if (strcmp(result->getUniqueTargetId(), lastTargetId) != 0)
                        {
                            // If the target has changed then regenerate the texture
                            // Cleaning this value indicates that the product Texture needs to be generated
                            // again in Java with the new Book data for the new target
                            deleteCurrentProductTexture = true;

                            // Starts the loading state for the product
                            renderState = RS_LOADING;

                            // Copies the new target Metadata
                            snprintf(targetMetadata, CONTENT_MAX, "%s", result->getMetaData());

                            // Calls the Java method with the current product texture
                            createProductTexture(targetMetadata);

                        }
                        else
                            renderState = RS_NORMAL;

                        // Initialize the frames to skip variable, used for waiting
                        // a few frames for getting the chance to tracking before
                        // starting the transition to 2D when there is no target
                        pthread_mutex_lock(&framesToSkipMutex);
                        framesToSkipBeforeRenderingTransition = 10;
                        pthread_mutex_unlock(&framesToSkipMutex);

                        // Initialize state variables
                        showAnimation3Dto2D = true;
                        trackingStarted = false;

                        // Updates the value of the current Target Id with the new target found
                        pthread_mutex_lock(&lastTargetIdMutex);
                        strcpy(lastTargetId, result->getUniqueTargetId());
                        pthread_mutex_unlock(&lastTargetIdMutex);

                        enterContentMode();
                    }
                    else
                        LOG("Failed to create new trackable.");
                }
            }
        }
    }
};

AugmentActivity_UpdateCallback updateCallback;

// ----------------------------------------------------------------------------
// Native functions called from Java
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Sets Activity orientation Variable
// ----------------------------------------------------------------------------
JNIEXPORT void JNICALL
Java_com_android_baseaugmentation_activity_AugmentActivity_nativeSetActivityPortraitMode(JNIEnv *, jobject, jboolean isPortrait)
{
    isActivityInPortraitMode = isPortrait;
}

// ----------------------------------------------------------------------------
// Initialize Tracker
// ----------------------------------------------------------------------------
JNIEXPORT int JNICALL
Java_com_android_baseaugmentation_activity_AugmentActivity_nativeInitTracker(JNIEnv *, jobject)
{
    LOG("Inside native InitTracker");
    QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
    QCAR::Tracker* tracker = trackerManager.initTracker(QCAR::Tracker::IMAGE_TRACKER);
    if (tracker == NULL)
    {
        LOG("Failed to initialize ImageTracker.");
        return 0;
    }

    LOG("Successfully initialized ImageTracker.");
    return 1;
}

// ----------------------------------------------------------------------------
// DeInitialize Tracker
// ----------------------------------------------------------------------------
JNIEXPORT void JNICALL
Java_com_android_baseaugmentation_activity_AugmentActivity_nativeDeinitTracker(JNIEnv *, jobject)
{
	LOG("Inside native DeinitTracker");
    QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
    trackerManager.deinitTracker(QCAR::Tracker::IMAGE_TRACKER);
}

// ----------------------------------------------------------------------------
// Initialize Cloud Reco
// ----------------------------------------------------------------------------
JNIEXPORT int JNICALL
Java_com_android_baseaugmentation_activity_AugmentActivity_nativeInitCloudReco(JNIEnv *, jobject)
{
    LOG("Java_com_qualcomm_QCARSamples_VisualSearch_VisualSearch_initVisualSearch");

    // Get the image tracker:
    QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
    QCAR::ImageTracker* imageTracker = static_cast<QCAR::ImageTracker*>(
            trackerManager.getTracker(QCAR::Tracker::IMAGE_TRACKER));
    assert(imageTracker != NULL);

    // Initialize visual search:
    QCAR::TargetFinder* targetFinder = imageTracker->getTargetFinder();
    assert(targetFinder != NULL);

    // Start initialization:
    if (targetFinder->startInit(kAccessKey, kSecretKey))
    {
        targetFinder->waitUntilInitFinished();
    }

    int resultCode = targetFinder->getInitState();
    if ( resultCode != QCAR::TargetFinder::INIT_SUCCESS)
    {
        LOG("Failed to initialize target finder.");
        return resultCode;
    }

    // Use the following calls if you would like to customize the color of the UI
    // targetFinder->setUIScanlineColor(1.0, 0.0, 0.0);
    // targetFinder->setUIPointColor(0.0, 0.0, 1.0);

    return resultCode;
}

// ----------------------------------------------------------------------------
// DeInitialize Cloud Reco
// ----------------------------------------------------------------------------
JNIEXPORT int JNICALL
Java_com_android_baseaugmentation_activity_AugmentActivity_nativeDeinitCloudReco(JNIEnv *, jobject)
{
    // Get the image tracker:
	LOG("Inside native nativeDeinitCloudReco");
    QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
    QCAR::ImageTracker* imageTracker = static_cast<QCAR::ImageTracker*>(
            trackerManager.getTracker(QCAR::Tracker::IMAGE_TRACKER));
    if (imageTracker == NULL)
    {
        LOG("Failed to destroy the tracking data set because the ImageTracker has not"
                " been initialized.");
        return 0;
    }

    // Deinitialize visual search:
    QCAR::TargetFinder* finder = imageTracker->getTargetFinder();
    finder->deinit();

    return 1;
}

// ----------------------------------------------------------------------------
// This Method is called from Java to indicate the OpenGL Thread that the texture
// is generated and ready to be created in OpenGL
// ----------------------------------------------------------------------------
JNIEXPORT void JNICALL
Java_com_android_baseaugmentation_activity_AugmentActivity_nativeProductTextureIsCreated(JNIEnv *env,jobject)
{
    renderState = RS_TEXTURE_GENERATED;
}

// ----------------------------------------------------------------------------
// renderFrame Method - Takes care of drawing in the different render states
// ----------------------------------------------------------------------------
JNIEXPORT void JNICALL
Java_com_android_baseaugmentation_renderer_CloudRenderer_nativeRenderFrame(JNIEnv *, jobject)
{
    // Clear color and depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Get the state from QCAR and mark the beginning of a rendering section
    QCAR::State state = QCAR::Renderer::getInstance().begin();

    // Explicitly render the Video Background
    QCAR::Renderer::getInstance().drawVideoBackground();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    if (deleteCurrentProductTexture)
    {
        // Deletes the product texture if necessary
        if (productTexture != 0)
        {
            glDeleteTextures(1, &(productTexture->mTextureID));
            delete productTexture;
            productTexture = 0;
        }

        deleteCurrentProductTexture = false;
    }

    // If the render state indicates that the texture is generated it generates
    // the OpenGL texture for start drawing the plane with the book data
    LOG("Inside RenderFrame");
    if (renderState == RS_TEXTURE_GENERATED)
    {
    	LOG("RS_TEXTURE_GENERATED");
        generateProductTextureInOpenGL();
    }

    // Did we find any trackables this frame?
    if (state.getNumTrackableResults() > 0)
    {
        trackingStarted = true;

        // If we are already tracking something we don't need
        // to wait any frame before starting the 2D transition
        // when the target gets lost
        pthread_mutex_lock(&framesToSkipMutex);
        framesToSkipBeforeRenderingTransition = 0;
        pthread_mutex_unlock(&framesToSkipMutex);

        // Gets current trackable result
        const QCAR::TrackableResult* trackableResult = state.getTrackableResult(0);

        if (trackableResult == NULL)
        {
            return;
        }

        modelViewMatrix = QCAR::Tool::convertPose2GLMatrix(trackableResult->getPose());

        // Get the size of the ImageTarget
        QCAR::ImageTargetResult *imageResult = (QCAR::ImageTargetResult *)trackableResult;
        targetSize = imageResult->getTrackable().getSize();

        // Renders the Augmentation View with the 3D Book data Panel
        renderAugmentation(trackableResult);

    }
    else
    {
        // Manages the 3D to 2D Transition initialization
        if (!scanningMode && showAnimation3Dto2D && renderState == RS_NORMAL
                && framesToSkipBeforeRenderingTransition == 0)
        {
            startTransitionTo2D();
        }

        // Reduces the number of frames to wait before triggering
        // the transition by 1
        if( framesToSkipBeforeRenderingTransition > 0 && renderState == RS_NORMAL)
        {
            pthread_mutex_lock(&framesToSkipMutex);
            framesToSkipBeforeRenderingTransition -= 1;
            pthread_mutex_unlock(&framesToSkipMutex);
        }

    }

    // Logic for rendering Transition to 2D
    if (renderState == RS_TRANSITION_TO_2D && showAnimation3Dto2D)
    {
        renderTransitionTo2D();
    }

    // Logic for rendering Transition to 3D
    if (renderState == RS_TRANSITION_TO_3D )
    {
        renderTransitionTo3D();
    }

    // Get the tracker manager:
    QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();

    // Get the image tracker:
    QCAR::ImageTracker* imageTracker = static_cast<QCAR::ImageTracker*>(
            trackerManager.getTracker(QCAR::Tracker::IMAGE_TRACKER));

    // Get the target finder:
    QCAR::TargetFinder* finder = imageTracker->getTargetFinder();

    // Renders the current state - User process Feedback
    if (finder->isRequesting())
    {
        // Requesting State - Show Requesting text in Status Bar
        //setStatusBarText("Requesting");
        //showStatusBar();
    }
    else
    {
        // Hiding Status Bar
        //hideStatusBar();
    }

    glDisable(GL_DEPTH_TEST);

    QCAR::Renderer::getInstance().end();

}

// ----------------------------------------------------------------------------
// Initialize Native Application
// ----------------------------------------------------------------------------
JNIEXPORT void JNICALL
Java_com_android_baseaugmentation_activity_AugmentActivity_nativeInitApplicationNative(
        JNIEnv* env, jobject obj, jint width, jint height)
{
	LOG("Inside native nativeInitApplicationNative");
    // Store screen dimensions
    screenWidth = width;
    screenHeight = height;

    // Handle to the activity class:
    jclass activityClass = env->GetObjectClass(obj);

    // Register callback function that gets called every time a tracking cycle
    // has finished and we have a new AR state available
    QCAR::registerCallback(&updateCallback);

    // Screen size:
    QCAR::Vec2F screenSize;
    screenSize.data[0] = (float) screenWidth;
    screenSize.data[1] = (float) screenHeight;

    // Initialise the method handle for showing error messages:
    //showErrorMethodID = env->GetMethodID(activityClass, "showErrorMessage", "(I)V");
    //if (showErrorMethodID == 0)
    //{
    //    LOG("Function showErrorMessage() not found.");
    //    return;
    //}

    // Initialize Java mathods to be called from Native code
    createProductTextureID = env->GetMethodID(activityClass, "createProductTexture", "(Ljava/lang/String;)V");
    getProductTextureID = env->GetMethodID(activityClass, "getProductTexture", "()Lcom/android/baseaugmentation/activity/Texture;");
    //showStatusBarID = env->GetMethodID(activityClass, "showStatusBar", "()V");
    //hideStatusBarID = env->GetMethodID(activityClass, "hideStatusBar", "()V");
    //setStatusBarTextID = env->GetMethodID(activityClass, "setStatusBarText", "(Ljava/lang/String;)V");
    enterContentModeID = env->GetMethodID(activityClass,"enterContentMode", "()V");

    activityObj = env->NewGlobalRef(obj);

    // Reset global variables:
    initStateVariables();
}

// ----------------------------------------------------------------------------
// DeInitialize Native Application
// ----------------------------------------------------------------------------
JNIEXPORT void JNICALL
Java_com_android_baseaugmentation_activity_AugmentActivity_nativeDeinitApplicationNative(
        JNIEnv* env, jobject obj)
{
	LOG("Inside nativeDeinitApplicationNative");
    if (activityObj != 0)
    {
        env->DeleteGlobalRef(activityObj);
        activityObj = 0;
    }

    if (productTexture != 0)
    {
        delete productTexture;
        productTexture = 0;
    }
}

// ----------------------------------------------------------------------------
// Starts the Camera
// ----------------------------------------------------------------------------
JNIEXPORT void JNICALL
Java_com_android_baseaugmentation_activity_AugmentActivity_nativeStartCamera(JNIEnv *, jobject)
{
    // Initialize the camera:
	LOG("Inside start camera");
    if (!QCAR::CameraDevice::getInstance().init())
    return;

    // Configure the video background
    configureVideoBackground();

    // Select the default mode:
    if (!QCAR::CameraDevice::getInstance().selectVideoMode(
                    QCAR::CameraDevice::MODE_DEFAULT))
    return;

    // Start the camera:
    if (!QCAR::CameraDevice::getInstance().start())
    return;

    // Start the tracker:
    QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
    QCAR::ImageTracker* imageTracker = static_cast<QCAR::ImageTracker*>(
            trackerManager.getTracker(QCAR::Tracker::IMAGE_TRACKER));
    assert(imageTracker != 0);
    imageTracker->start();

    // Start cloud based recognition if we are in scanning mode:
    if(scanningMode)
    {
        QCAR::TargetFinder* targetFinder = imageTracker->getTargetFinder();
        assert (targetFinder != 0);
        crStarted = targetFinder->startRecognition();
    }
}

// ----------------------------------------------------------------------------
// Stops the Camera
// ----------------------------------------------------------------------------
JNIEXPORT void JNICALL
Java_com_android_baseaugmentation_activity_AugmentActivity_nativeStopCamera(JNIEnv *,jobject)
{
    // Stop the tracker:
    QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
    QCAR::ImageTracker* imageTracker = static_cast<QCAR::ImageTracker*>(
            trackerManager.getTracker(QCAR::Tracker::IMAGE_TRACKER));
    assert(imageTracker != 0);
    imageTracker->stop();

    // Stop cloud based recognition:
    QCAR::TargetFinder* targetFinder = imageTracker->getTargetFinder();
    assert (targetFinder != 0);
    crStarted = !targetFinder->stop();

    //Clears the trackables
    targetFinder->clearTrackables();

    QCAR::CameraDevice::getInstance().stop();
    QCAR::CameraDevice::getInstance().deinit();

    // When the camera stops it clears the Product Texture ID so next time textures
    // Are recreated
    deleteCurrentProductTexture = true;

    // Initialize all state Variables
    initStateVariables();
}

// ----------------------------------------------------------------------------
// Sets the projection Matrix
// ----------------------------------------------------------------------------
JNIEXPORT void JNICALL
Java_com_android_baseaugmentation_activity_AugmentActivity_nativeSetProjectionMatrix(JNIEnv *, jobject)
{
    // Cache the projection matrix:
    const QCAR::CameraCalibration& cameraCalibration =
    QCAR::CameraDevice::getInstance().getCameraCalibration();
    projectionMatrix = QCAR::Tool::getProjectionGL(cameraCalibration, 2.0f, 2500.0f);

    // Cache the inverse projection matrix - Used for handling touch events against
    // trackable data
    inverseProjMatrix = SampleMath::Matrix44FInverse(projectionMatrix);

}

// ----------------------------------------------------------------------------
// Activates AutoFocus
// ----------------------------------------------------------------------------
JNIEXPORT jboolean JNICALL
Java_com_android_baseaugmentation_activity_AugmentActivity_nativeAutofocus(JNIEnv*, jobject)
{
    return QCAR::CameraDevice::getInstance().setFocusMode(QCAR::CameraDevice::FOCUS_MODE_TRIGGERAUTO) ? JNI_TRUE : JNI_FALSE;
}

// ----------------------------------------------------------------------------
// Sets the Camera Focus Mode
// ----------------------------------------------------------------------------
JNIEXPORT jboolean JNICALL
Java_com_android_baseaugmentation_activity_AugmentActivity_nativeSetFocusMode(JNIEnv*, jobject, jint mode)
{
    int qcarFocusMode;

    switch ((int)mode)
    {
        case 0:
        qcarFocusMode = QCAR::CameraDevice::FOCUS_MODE_NORMAL;
        break;

        case 1:
        qcarFocusMode = QCAR::CameraDevice::FOCUS_MODE_CONTINUOUSAUTO;
        break;

        case 2:
        qcarFocusMode = QCAR::CameraDevice::FOCUS_MODE_INFINITY;
        break;

        case 3:
        qcarFocusMode = QCAR::CameraDevice::FOCUS_MODE_MACRO;
        break;

        default:
        return JNI_FALSE;
    }

    return QCAR::CameraDevice::getInstance().setFocusMode(qcarFocusMode) ? JNI_TRUE : JNI_FALSE;
}

// ----------------------------------------------------------------------------
// Initialize Rendering
// ----------------------------------------------------------------------------
JNIEXPORT void JNICALL
Java_com_android_baseaugmentation_renderer_CloudRenderer_nativeInitRendering(
        JNIEnv* env, jobject obj)
{
    // Define clear color
    glClearColor(0.0f, 0.0f, 0.0f, QCAR::requiresAlpha() ? 0.0f : 1.0f);

    // OpenGL setup for 3D model
    shaderProgramID = SampleUtils::createProgramFromBuffer(cubeMeshVertexShader,
            cubeFragmentShader);

    vertexHandle = glGetAttribLocation(shaderProgramID,
            "vertexPosition");
    normalHandle = glGetAttribLocation(shaderProgramID,
            "vertexNormal");
    textureCoordHandle = glGetAttribLocation(shaderProgramID,
            "vertexTexCoord");
    mvpMatrixHandle = glGetUniformLocation(shaderProgramID,
            "modelViewProjectionMatrix");

    //Initialize the 3D to 2D Transition
    transition3Dto2D = new Transition3Dto2D(screenWidth, screenHeight, isActivityInPortraitMode, dpiScaleIndicator, scaleFactor);
    transition3Dto2D->initializeGL(shaderProgramID);

    //Initialize the 2D to 3D Transition
    transition2Dto3D = new Transition3Dto2D(screenWidth, screenHeight, isActivityInPortraitMode, dpiScaleIndicator, scaleFactor);
    transition2Dto3D->initializeGL(shaderProgramID);
}

// ----------------------------------------------------------------------------
// Updates Rendering
// ----------------------------------------------------------------------------
JNIEXPORT void JNICALL
Java_com_android_baseaugmentation_renderer_CloudRenderer_nativeUpdateRendering(
        JNIEnv* env, jobject obj, jint width, jint height)
{
    // Update screen dimensions
    screenWidth = width;
    screenHeight = height;

    // Reconfigure the video background
    configureVideoBackground();
}

// ----------------------------------------------------------------------------
// Updates the renderSate when transition is finished
// ----------------------------------------------------------------------------
JNIEXPORT void JNICALL
Java_com_android_baseaugmentation_activity_AugmentActivity_finishTransition(JNIEnv* env, jobject obj)
{
    // Initialize the Transition values and returns to the normal
    // rendering state
    renderState = RS_NORMAL;

}

JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM* vm, void* reserved)
{
    LOG("JNI_OnLoad");
    javaVM = vm;
    return JNI_VERSION_1_4;
}

// ----------------------------------------------------------------------------
// Disables the CloudReco service
// ----------------------------------------------------------------------------
JNIEXPORT void JNICALL
Java_com_android_baseaugmentation_activity_AugmentActivity_nativeEnterContentMode(JNIEnv*, jobject)
{
	LOG("Entering Content Mode");
    QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
    QCAR::ImageTracker* imageTracker = static_cast<QCAR::ImageTracker*>(
            trackerManager.getTracker(QCAR::Tracker::IMAGE_TRACKER));
    assert(imageTracker != 0);
    QCAR::TargetFinder* targetFinder = imageTracker->getTargetFinder();
    assert (targetFinder != 0);

    // Stop CloudReco
    crStarted = !targetFinder->stop();

    LOG("Setting Scanning Mode to false");
    // Remember we are in content mode:
    scanningMode = false;
}

// ----------------------------------------------------------------------------
// Enables CloudReco Service
// ----------------------------------------------------------------------------
JNIEXPORT void JNICALL
Java_com_android_baseaugmentation_activity_AugmentActivity_nativeEnterScanningMode(JNIEnv*, jobject)
{
    QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
    QCAR::ImageTracker* imageTracker = static_cast<QCAR::ImageTracker*>(
            trackerManager.getTracker(QCAR::Tracker::IMAGE_TRACKER));
    assert(imageTracker != 0);
    QCAR::TargetFinder* targetFinder = imageTracker->getTargetFinder();
    assert (targetFinder != 0);

    // Start CloudReco
    crStarted = targetFinder->startRecognition();

    // Clear all trackables created previously:
    targetFinder->clearTrackables();

    scanningMode = true;

    // Updates state variables
    showAnimation3Dto2D = false;
    showAnimation2Dto3D = false;
    isShowing2DOverlay = false;
    renderState = RS_SCANNING;

}

// ----------------------------------------------------------------------------
// Stores the DPI Scale Factor and initialize a scaleMultiplier for
// different screens dpis
// ----------------------------------------------------------------------------
JNIEXPORT void JNICALL
Java_com_android_baseaugmentation_activity_AugmentActivity_nativeSetDeviceDPIScaleFactor(JNIEnv*, jobject, jfloat dpiSIndicator)
{
    dpiScaleIndicator = dpiSIndicator;

    // MDPI devices
    if ( dpiScaleIndicator == 1.0f )
    {
        scaleFactor = 1.6f;
    }
    // HDPI devices
    else if ( dpiScaleIndicator == 1.5f )
    {
        scaleFactor = 1.3f;
    }
    // XHDPI devices
    else
    {
        scaleFactor = 1.0f;
    }
}

// ----------------------------------------------------------------------------
// Cleans the lastTargetId variable
// ----------------------------------------------------------------------------
JNIEXPORT void JNICALL
Java_com_android_baseaugmentation_activity_AugmentActivity_nativeCleanTargetTrackedId(JNIEnv*, jobject)
{
    pthread_mutex_lock(&lastTargetIdMutex);
    lastTargetId[0] = '\0';
    pthread_mutex_unlock(&lastTargetIdMutex);
}


// ----------------------------------------------------------------------------
// Class Methods
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Shows an error message
// ----------------------------------------------------------------------------
void
showErrorMessage(int statusCode, double frameTime)
{
    // Check that the JNI handles are setup correctly:
    JNIEnv* env = 0;
    if (javaVM != 0 && showErrorMethodID != 0 && activityObj != 0
            && javaVM->GetEnv((void**)&env, JNI_VERSION_1_4) == JNI_OK)
    {
        // Check that we have not shown an error message for the same error
        // recently:
        if (frameTime > lastErrorMessageTime + 5.f ||
                statusCode != lastErrorCode)
        {
            env->CallVoidMethod(activityObj, showErrorMethodID, statusCode);
            lastErrorMessageTime = frameTime;
            lastErrorCode = statusCode;
        }
    }
}

// ----------------------------------------------------------------------------
/** Calls the Java Method to start loading the product texture*/
// ----------------------------------------------------------------------------
void
createProductTexture(const char* targetMetadata)
{
    // Check that the JNI handles are setup correctly:
    JNIEnv* env = 0;

    if (javaVM != 0 && createProductTextureID != 0 && activityObj != 0
            && javaVM->GetEnv((void**)&env, JNI_VERSION_1_4) == JNI_OK)
    {

        env->CallVoidMethod(activityObj, createProductTextureID, env->NewStringUTF(targetMetadata));

    }
}

// ----------------------------------------------------------------------------
/** Generates an OpenGL Texture from the Texture Object generated in Java*/
// ----------------------------------------------------------------------------
void
generateProductTextureInOpenGL()
{
    // Check that the JNI handles are setup correctly:
    JNIEnv* env = 0;

    if (javaVM != 0 && activityObj != 0
            && javaVM->GetEnv((void**)&env, JNI_VERSION_1_4) == JNI_OK)
    {
        // Calls the Java Method to get the generated Texture Object
        jobject textureObject = env->CallObjectMethod(activityObj, getProductTextureID);
        LOG("generateProductTextureInOpenGL");
        if (textureObject != NULL)
        {
        	LOG("GOING TO CREATE TEXTURE");
            productTexture = Texture::create(env, textureObject);
        }else{
        	LOG("TEXTURE IS NULL");
        }

        // Generates the Texture in OpenGL
        glGenTextures(1, &(productTexture->mTextureID));
        glBindTexture(GL_TEXTURE_2D, productTexture->mTextureID);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // We create an empty power of two texture and upload a sub image.
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024,
                1024, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, productTexture->mWidth,
            productTexture->mHeight, GL_RGBA, GL_UNSIGNED_BYTE,
            (GLvoid*) productTexture->mData);

        // Updates the current Render State
        renderState = RS_NORMAL;
    }
}

// ----------------------------------------------------------------------------
// Calls the Java Method to display the Status Bar overlay
// ----------------------------------------------------------------------------
void
showStatusBar( )
{

    // Check that the JNI handles are setup correctly:
    JNIEnv* env = 0;

    if (javaVM != 0 && showStatusBarID != 0 && activityObj != 0
            && javaVM->GetEnv((void**)&env, JNI_VERSION_1_4) == JNI_OK)
    {

        env->CallVoidMethod(activityObj, showStatusBarID);
    }
}

// ----------------------------------------------------------------------------
// Calls the Java Method to hide the Status Bar Overlay
// ----------------------------------------------------------------------------
void
hideStatusBar( )
{

    // Check that the JNI handles are setup correctly:
    JNIEnv* env = 0;

    if (javaVM != 0 && hideStatusBarID != 0 && activityObj != 0
            && javaVM->GetEnv((void**)&env, JNI_VERSION_1_4) == JNI_OK)
    {

        env->CallVoidMethod(activityObj, hideStatusBarID);
    }
}

// ----------------------------------------------------------------------------
// Calls the Java Method to update the status Bar View Text
// ----------------------------------------------------------------------------
void setStatusBarText(const char* statusText)
{

    // Check that the JNI handles are setup correctly:
    JNIEnv* env = 0;

    if (javaVM != 0 && setStatusBarTextID != 0 && activityObj != 0
            && javaVM->GetEnv((void**)&env, JNI_VERSION_1_4) == JNI_OK)
    {

        env->CallVoidMethod(activityObj, setStatusBarTextID, env->NewStringUTF(statusText));
    }
}

// ----------------------------------------------------------------------------
// Configures Video Background
// ----------------------------------------------------------------------------
void
configureVideoBackground()
{
    // Get the default video mode:
    QCAR::CameraDevice& cameraDevice = QCAR::CameraDevice::getInstance();
    QCAR::VideoMode videoMode = cameraDevice.
    getVideoMode(QCAR::CameraDevice::MODE_DEFAULT);

    // Configure the video background
    QCAR::VideoBackgroundConfig config;
    config.mEnabled = true;
    config.mSynchronous = false;
    config.mPosition.data[0] = 0.0f;
    config.mPosition.data[1] = 0.0f;

    if (isActivityInPortraitMode)
    {
        config.mSize.data[0] = videoMode.mHeight
        * (screenHeight / (float)videoMode.mWidth);
        config.mSize.data[1] = screenHeight;

        if(config.mSize.data[0] < screenWidth)
        {
            LOG("Correcting rendering background size to handle missmatch between screen and video aspect ratios.");
            config.mSize.data[0] = screenWidth;
            config.mSize.data[1] = screenWidth *
            (videoMode.mWidth / (float)videoMode.mHeight);
        }
    }
    else
    {
        config.mSize.data[0] = screenWidth;
        config.mSize.data[1] = videoMode.mHeight
        * (screenWidth / (float)videoMode.mWidth);

        if(config.mSize.data[1] < screenHeight)
        {
            LOG("Correcting rendering background size to handle missmatch between screen and video aspect ratios.");
            config.mSize.data[0] = screenHeight
            * (videoMode.mWidth / (float)videoMode.mHeight);
            config.mSize.data[1] = screenHeight;
        }
    }

    // Set the config:
    QCAR::Renderer::getInstance().setVideoBackgroundConfig(config);
}


// ----------------------------------------------------------------------------
// Renders the Augmented 3d Panel
// ----------------------------------------------------------------------------
void
renderAugmentation(const QCAR::TrackableResult* trackableResult)
{
    QCAR::Matrix44F modelViewProjection;

    // Scales the plane relative to the target
    SampleUtils::scalePoseMatrix(430.f * scaleFactor,
            430.f * scaleFactor,
            1.0f,
            &modelViewMatrix.data[0]);

    // Applies 3d Transformations to the plane
    SampleUtils::multiplyMatrix(&projectionMatrix.data[0],
            &modelViewMatrix.data[0] ,
            &modelViewProjection.data[0]);

    // Moves the trackable current position to a global variable used for
    // the 3d to 2D animation
    pose = trackableResult->getPose();

    // Shader Program for drawing
    glUseProgram(shaderProgramID);

    // The 3D Plane is only drawn when the texture is loaded and generated
    if (renderState == RS_NORMAL)
    {
        glVertexAttribPointer(vertexHandle, 3, GL_FLOAT, GL_FALSE, 0,
                (const GLvoid*) &planeVertices[0]);
        glVertexAttribPointer(normalHandle, 3, GL_FLOAT, GL_FALSE, 0,
                (const GLvoid*) &planeNormals[0]);
        glVertexAttribPointer(textureCoordHandle, 2, GL_FLOAT, GL_FALSE, 0,
                (const GLvoid*) &planeTexcoords[0]);

        glEnableVertexAttribArray(vertexHandle);
        glEnableVertexAttribArray(normalHandle);
        glEnableVertexAttribArray(textureCoordHandle);

        // Enables Blending State
        glEnable(GL_BLEND);

        // Drawing Textured Plane
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, productTexture->mTextureID);
        glUniformMatrix4fv(mvpMatrixHandle, 1, GL_FALSE,
                (GLfloat*)&modelViewProjection.data[0] );
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT,
                (const GLvoid*) &planeIndices[0]);

        glDisableVertexAttribArray(vertexHandle);
        glDisableVertexAttribArray(normalHandle);
        glDisableVertexAttribArray(textureCoordHandle);

        // Disables Blending State - Its important to disable the blending state
        // after using it for preventing bugs with the Camera Video Background
        glDisable(GL_BLEND);

        // Handles target re-acquisition - Checks if the overlay2D is shown
    }
    else if (isShowing2DOverlay)
    {
        // Initialize the Animation to 3d variables
        startTransition2Dto3D = true;
        isShowing2DOverlay = false;

        // Updates renderState
        renderState = RS_TRANSITION_TO_3D;

    }

    SampleUtils::checkGlError("CloudReco renderFrame");
}

// ----------------------------------------------------------------------------
// Renders the transition from 3D plane to 2D overlay
// ----------------------------------------------------------------------------
void
renderTransitionTo2D()
{

    if (startTransition3Dto2D)
    {
        // Starts the Transition
        transition3Dto2D->startTransition(transitionDuration, false, true);

        // Initialize control state variables
        startTransition3Dto2D = false;

    }
    else
    {

        if (productTexture != 0 )
        {
            transition3Dto2D->render(projectionMatrix, pose, productTexture->mTextureID );

            // check if transition is finished
            if (transition3Dto2D->transitionFinished() )
            {
                isShowing2DOverlay = true;

            }
        }
    }
}

// ----------------------------------------------------------------------------
// Renders the transition from 2D overlay to 3D Plane
// ----------------------------------------------------------------------------
void
renderTransitionTo3D()
{
    if (startTransition2Dto3D)
    {
        transitionDuration = 0.5f;

        // Starts the Transition
        transition2Dto3D->startTransition(transitionDuration, true, true);

        // Initialize control state variables
        startTransition2Dto3D = false;

    }
    else
    {

        if ( productTexture != 0)
        {
            // Renders the transition
            transition2Dto3D->render(projectionMatrix, pose, productTexture->mTextureID );

            // check if transition is finished
            if (transition2Dto3D->transitionFinished())
            {
                // Updates state values
                isShowing2DOverlay = false;
                showAnimation3Dto2D = true;

                // Updates current renderState when the transition is finished
                // to go back to normal rendering
                renderState = RS_NORMAL;
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Handles logic for starting transition from 3D plane to 2D Overlay
// ----------------------------------------------------------------------------
void
startTransitionTo2D()
{

    // Initialize the animation values when the book data
    // is displayed normally
    if (renderState == RS_NORMAL && trackingStarted)
    {
        transitionDuration = 0.5f;

        // Updates Render State
        renderState = RS_TRANSITION_TO_2D;
        startTransition3Dto2D = true;

    }
    else if ( renderState == RS_NORMAL && !trackingStarted && productTexture != 0)
    {
        // Triggers the transition in case you loose the target while the loading process
        transitionDuration = 0.0f;

        // Updates RenderState
        renderState = RS_TRANSITION_TO_2D;
        startTransition3Dto2D = true;

    }
}

// ----------------------------------------------------------------------------
// Initialize State Variables
// ----------------------------------------------------------------------------
void
initStateVariables()
{
    // Initialize transition state variables
    renderState = RS_SCANNING;

    // Initialize textures state variables
    productTexture = 0;

    pthread_mutex_lock(&lastTargetIdMutex);
    lastTargetId[0] = '\0';
    pthread_mutex_unlock(&lastTargetIdMutex);

    // Initialize state values
    scanningMode = true;
    isShowing2DOverlay = false;
    startTransition3Dto2D = false;
}

// ----------------------------------------------------------------------------
// Transitions the application to content mode:
// ----------------------------------------------------------------------------
void
enterContentMode()
{
    // Check that the JNI handles are setup correctly:
    JNIEnv* env = 0;
    if (javaVM != 0 && enterContentModeID != 0 && activityObj != 0
            && javaVM->GetEnv((void**)&env, JNI_VERSION_1_4) == JNI_OK)
    {
        env->CallVoidMethod(activityObj, enterContentModeID);
    }
}

#ifdef __cplusplus
}

#endif
