package com.svofski.v06x;

import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;
import android.support.v4.app.NotificationManagerCompat;
import android.util.Log;

// see https://stackoverflow.com/questions/12997800/cancel-notification-on-remove-application-from-multitask-panel

public class Brutnik extends Service {
    public class BrutnikBinder extends Binder {
        public final Service service;

        public BrutnikBinder(Service service) {
            this.service = service;
        }

    }

    private IBinder mBinder = new BrutnikBinder(this);

    public Brutnik() {
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    @Override
    public void onTaskRemoved(Intent rootIntent) {
        Log.d("v06x", "Brutnik: onTaskRemoved");
        NotificationManagerCompat.from(this).cancelAll();
        stopSelf();
    }
}
