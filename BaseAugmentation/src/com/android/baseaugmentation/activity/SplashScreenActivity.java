package com.android.baseaugmentation.activity;


import com.android.baseaugmentation.R;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;

public class SplashScreenActivity extends Activity {
	Handler _handler; 
	
	
	private Runnable delayRunnable = new Runnable(){
		public void run() {
			SplashScreenActivity.this.loadNextScreen();
		}
	};
	
	private void loadNextScreen(){
		Intent nextScreen = new Intent(this,AugmentActivity.class);
		this.startActivity(nextScreen);
		this.finish();
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		setContentView(R.layout.activity_splashscreen);
		super.onCreate(savedInstanceState);
		_handler = new Handler();
		_handler.postDelayed(delayRunnable, 4500);
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
	}
		
	@Override
	protected void onPause(){
		super.onPause();
	}
		
	@Override
	protected void onStop(){
		super.onStop();
    	if (this._handler != null){
    		this._handler.removeCallbacks(this.delayRunnable);
    	}
	}
		
	@Override
	protected void onDestroy(){
		super.onDestroy();
	}
}
