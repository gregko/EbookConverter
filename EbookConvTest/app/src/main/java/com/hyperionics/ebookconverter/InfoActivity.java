package com.hyperionics.ebookconverter;

import android.app.Activity;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.view.View;
import android.view.Window;
import android.widget.TextView;

/**
 * Created by greg on 1/8/14.
 */
public class InfoActivity extends Activity {
    @Override protected void onCreate(Bundle savedInstanceState) {
        setTheme(android.R.style.Theme_Holo_Light_Dialog);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        super.onCreate(savedInstanceState);
        setContentView(R.layout.info_panel);
        TextView tv = (TextView) findViewById(R.id.vtext);
        try {
            String s = tv.getText().toString() + " " + getPackageManager().getPackageInfo(getPackageName(), 0).versionName;
            ;
            if (BuildConfig.DEBUG) {
                s += " (DEBUG)";
            }
            tv.setText(s);
        } catch (PackageManager.NameNotFoundException ex) {}
    }

    @Override
    public void onPause() {
        super.onPause();
    }

    public void onOK(View view) {
        finish();
    }

}
