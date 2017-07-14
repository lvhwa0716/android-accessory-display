package com.lvh.usbhumaninterface;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;

public class MainActivity extends Activity implements View.OnClickListener {
	private final static String TAG = "MainActivity";

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		Button mButton = (Button) findViewById(R.id.PresentationActivityButton);
		mButton.setOnClickListener(this);
	}

	@Override
	public void onClick(View v) {
		// TODO Auto-generated method stub
		switch (v.getId()) {
		case R.id.PresentationActivityButton: {
			Intent intent = new Intent(MainActivity.this, PresentationActivity.class);
			try {
				startActivity(intent);
			} catch (Exception e) {
				Logger.logError(TAG, e.toString());
			}
			break;
		}

		}
	}

}
