package com.android.baseaugmentation.activity;

import android.util.Log;

public class Util {
	private static final String TAG = "Util";
	public static boolean loadNative(String library){
		try{
			System.loadLibrary(library);
			return true;
		}
		catch(UnsatisfiedLinkError ulee){
			Log.e(TAG, "Could not load library " + library );
			ulee.printStackTrace();
			return false;
		}
		catch(SecurityException se){
			Log.e(TAG, "Security Exception caused by loading " + library );
			se.printStackTrace();
			return false;
		}
	}
}
