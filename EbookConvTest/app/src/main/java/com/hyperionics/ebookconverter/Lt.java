package com.hyperionics.ebookconverter;

/**
 *  Copyright (C) 2012 Hyperionics Technology LLC <http://www.hyperionics.com>
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
import android.app.AlertDialog;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.util.Log;

public class Lt {
//    private static String myPackageName;
//    private static PackageManager myPackageManager;
    private static ApplicationInfo myAppInfo;
    private static String myTag = "Hyperionics";

    private Lt() {}

    public static void init(Context context, String tag)
    {
//        myPackageManager = context.getPackageManager();
//        myPackageName = context.getPackageName();
        myAppInfo = context.getApplicationInfo();
        myTag = tag;
    }

    public static boolean isDebugBuild() {
        return (myAppInfo.flags & ApplicationInfo.FLAG_DEBUGGABLE) != 0;
    }

    public static void d(String msg) {
        // Uncomment line below to turn on debug output
        if (BuildConfig.DEBUG)
            Log.d(myTag, msg == null ? "(null)" : msg);
    }

    public static void df(String msg) {
        // Forced output, do not comment out - for exceptions etc.
        Log.d(myTag, msg == null ? "(null)" : msg);
    }

    public static void e(String msg) {
        Log.e(myTag, msg == null ? "(null)" : msg);
    }

    public static void w(String msg) {
        if (BuildConfig.DEBUG)
            Log.w(myTag, msg == null ? "(null)" : msg); // #WARN
    }

    public static void i(String msg) {
        if (BuildConfig.DEBUG)
            Log.i(myTag, msg == null ? "(null)" : msg); // #WARN
    }

    public static void alert(Context context, String message) {
        if (context == null)
            return;
        AlertDialog.Builder bld = new AlertDialog.Builder(context);
        bld.setMessage(message);
        bld.setNeutralButton("OK", null);
        bld.create().show();
    }

    public static void alert(Context context, int resId) {
        if (context != null)
            alert(context, context.getResources().getString(resId));
    }
}