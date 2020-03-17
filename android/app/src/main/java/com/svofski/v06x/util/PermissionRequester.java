package com.svofski.v06x.util;

import android.Manifest;
import android.app.Activity;
import android.content.pm.PackageManager;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;

public class PermissionRequester {
    // remember when we were refused, but only once per session
    static boolean sAlreadyAsked = false;
    
    static public void requestPermissions(Activity thisActivity, int token)
    {
        if (!sAlreadyAsked && ContextCompat.checkSelfPermission(thisActivity,
                Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            sAlreadyAsked = true;
            if (false && ActivityCompat.shouldShowRequestPermissionRationale(thisActivity,
                    Manifest.permission.READ_EXTERNAL_STORAGE)) {
                // TODO: show a dialog why permission is needed
            } else {
                ActivityCompat.requestPermissions(thisActivity,
                        new String[]{Manifest.permission.READ_EXTERNAL_STORAGE}, token);
            }
        } else {
            // Permission has already been granted
        }
    }
}
