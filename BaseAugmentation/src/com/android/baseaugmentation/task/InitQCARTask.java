package com.android.baseaugmentation.task;

import com.android.baseaugmentation.activity.AugmentActivity;
import com.qualcomm.QCAR.QCAR;
import android.app.Activity;
import android.os.AsyncTask;

public class InitQCARTask extends AsyncTask<Void, Integer, Boolean> {
	public interface InitQCARTaskListener{
		void onQCARInitSuccess();
		void onQCARInitFailure();
	}
	
	
	public AugmentActivity mActivity;
	public InitQCARTaskListener mListener;
	public InitQCARTask(AugmentActivity activity,InitQCARTaskListener listener){
		mActivity = activity;
		mListener = listener;
	}

	@Override
	protected Boolean doInBackground(Void... params) {
		synchronized(mActivity.mShutdownLock){
			int mProgressValue = 0 ;
			QCAR.setInitParameters(mActivity, QCAR.GL_20);

			do{
				mProgressValue = QCAR.init();

				// Publish the progress value:
				publishProgress(mProgressValue);
			} while (!isCancelled() && mProgressValue >= 0 && mProgressValue < 100);
			return (mProgressValue > 0);
		}		
	}
	
	@Override
	protected void onPostExecute(Boolean result){
		if(result){
			mListener.onQCARInitSuccess();
		}else{
			mListener.onQCARInitFailure();
		}
	}
}