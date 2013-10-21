package com.android.baseaugmentation.task;

import org.json.JSONException;
import org.json.JSONObject;

import android.content.Context;
import android.os.AsyncTask;

public class GetAugmentInformation extends AsyncTask<String,Void,String> {
	public interface GetAugmentInformationListener{
		void onSuccess(JSONObject obj);
		void onFailure(Exception e);
	}
	
	private GetAugmentInformationListener mListener;
	private Context mContext;
	private Exception e;
	
	public GetAugmentInformation(Context context,GetAugmentInformationListener listener){
		this.mListener = listener;
		this.mContext = context;
	}
	
	private String testData = "{'Title':'TestTitle','Description':'This is a test description'}";
	@Override
	protected String doInBackground(String... params) {
		String metaData = params[0];
		
		
		return metaData;
	}
	
	protected void onPostExecute(String data){
		if(this.e == null){
			JSONObject obj;
			try {
				obj = new JSONObject(data);
				this.mListener.onSuccess(obj);
			} catch (JSONException e1) {				
				this.mListener.onFailure(e1);
			}
		}else{
			this.mListener.onFailure(e);
		}
	}
}
