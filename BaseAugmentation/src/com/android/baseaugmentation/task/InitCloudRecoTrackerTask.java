package com.android.baseaugmentation.task;

import com.android.baseaugmentation.activity.AugmentActivity;
import com.qualcomm.QCAR.QCAR;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.graphics.Color;
import android.os.AsyncTask;

public class InitCloudRecoTrackerTask extends AsyncTask<Void, Integer, Integer> {
	public interface InitTrackerTaskListener{
		void onLoadTrackerSuccess(int result);
        void onLoadTrackerFailure(int result);
        int loadTrackerData();
    }
        
        
    public AugmentActivity mActivity;
    public InitTrackerTaskListener mListener;
    
    // Initialize with invalid value
    private int mInitResult = -1;
    // These codes match the ones defined in TargetFinder.h
    static final int INIT_SUCCESS = 2;
    
    public InitCloudRecoTrackerTask(AugmentActivity activity, InitTrackerTaskListener listener){
         mListener = listener;
         mActivity = activity;
    }

    @Override
    protected Integer doInBackground(Void... params) {
    	synchronized(mActivity.mShutdownLock){
        	mInitResult = mActivity.nativeInitCloudReco();
        	return mInitResult;        
        }
    
    }
        
    @Override
    protected void onPostExecute(Integer result){
    	if(result == INIT_SUCCESS){
    		this.mListener.onLoadTrackerSuccess(result);
    	}else{
    		this.mListener.onLoadTrackerFailure(result);
    	}
    }	
}