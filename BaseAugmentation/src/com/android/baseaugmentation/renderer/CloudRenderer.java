/*==============================================================================
Copyright (c) 2012-2013 QUALCOMM Austria Research Center GmbH.
All Rights Reserved.

@file
    CloudRecoRenderer.java

@brief
    Sample for CloudReco

==============================================================================*/

package com.android.baseaugmentation.renderer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.app.Activity;
import android.opengl.GLSurfaceView;

import com.android.baseaugmentation.activity.AugmentActivity;
import com.qualcomm.QCAR.QCAR;

/** The renderer class for the CloudReco sample. */
public class CloudRenderer implements GLSurfaceView.Renderer
{
    public boolean mIsActive = false;

    /** Reference to main activity **/
    public AugmentActivity mActivity;
    
    public CloudRenderer(AugmentActivity activity){
    	mActivity = activity;
    }

    public native void nativeInitRendering();
    public native void nativeUpdateRendering(int width, int height);    
    public native void nativeRenderFrame();


    /** Called when the surface is created or recreated. */
    public void onSurfaceCreated(GL10 gl, EGLConfig config)
    {
        // Call native function to initialize rendering:
    	nativeInitRendering();

        // Call QCAR function to (re)initialize rendering after first use
        // or after OpenGL ES context was lost (e.g. after onPause/onResume):
        QCAR.onSurfaceCreated();
    }


    /** Called when the surface changed size. */
    public void onSurfaceChanged(GL10 gl, int width, int height)
    {
        // Call native function to update rendering when render surface
        // parameters
        // have changed:
    	nativeUpdateRendering(width, height);

        // Call QCAR function to handle render surface size changes:
        QCAR.onSurfaceChanged(width, height);
    }


    /** Called to draw the current frame. */
    public void onDrawFrame(GL10 gl)
    {
        if (!mIsActive)
        {
            return;
        }

        // Update render view (projection matrix and viewport) if needed:
        mActivity.updateRenderView();

        // Call our native function to render content
        nativeRenderFrame();
    }
}
