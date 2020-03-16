package com.svofski.v06x;

import android.app.Activity;
import android.content.Intent;

public abstract class Broadcasts {
    public static final String RESCAN_LOCAL_STORAGE = "com.svofski.v06x.rescan";

    public static void sendInternalBroadcast(Activity sender, String action)
    {
        Intent intent = new Intent();
        intent.setAction(action);
        sender.sendBroadcast(intent);
    }
}
