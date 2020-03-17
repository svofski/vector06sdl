package com.svofski.v06x;

import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.os.Build;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.support.annotation.NonNull;
import android.support.v4.app.Fragment;
import android.support.v4.app.NotificationCompat;
import android.support.v4.app.NotificationManagerCompat;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewTreeObserver;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.Toast;

import com.svofski.v06x.cpp.Emulator;
import com.svofski.v06x.cpp.LoadKind;
import com.svofski.v06x.keyboard.OnScreenKeyboard;
import com.svofski.v06x.uiextras.LoaderTabsFragment;
import com.svofski.v06x.uiextras.RomListerFragment;
import com.svofski.v06x.uiextras.loaders.RomListItem;
import com.svofski.v06x.uiextras.loaders.RomListItemAsset;
import com.svofski.v06x.uiextras.loaders.RomListItemDummy;
import com.svofski.v06x.uiextras.loaders.RomListItemStorage;
import com.svofski.v06x.util.PermissionRequester;

public class MainActivity extends AppCompatActivity
    implements RomListerFragment.OnListFragmentInteractionListener,
        LoaderTabsFragment.OnFragmentInteractionListener {
    private static final String CHANNEL_ID = "v06xchan";
    private static final int BRING_TO_FRONT = 101;
    private static final int PERMISSIONS_TOKEN = 1;
    private EmulatorSurfaceView mESV;

    private static Emulator v06x;
    private static EmulatorRunner emulatorRunner;

    private static RomListItem sRomListItem; // notification model should survive between recreations

    private Handler mHandler;

    private ServiceConnection mBrutnikConnection;

    static {
        System.loadLibrary("v06x-lib");
    }

    private boolean mBackButtonQuit = false;

    private LoaderTabsFragment mLoadnik;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        /* If exited by back button, state was not saved but emulator instance could have been kept */
        if (savedInstanceState == null && v06x == null && emulatorRunner == null) {
            v06x = new Emulator();
            v06x.Init();
            emulatorRunner = new EmulatorRunner();
            createNotificationChannel();
            prepareBootNotificationData();
        }

        bindBrutnikService();

        mHandler = new Handler(Looper.myLooper()) {
            @Override
            public void handleMessage(Message msg) {
                if (msg.what == EmulatorRunner.REQUEST_RENDER) {
                    if (mESV != null) {
                        mESV.requestRender();
                    }
                }
            }
        };
    }


    public static Emulator emulator() {
        return v06x;
    }

    @Override
    protected void onStart() {
        super.onStart();
        mESV = findViewById(R.id.surfaceView);

        ViewTreeObserver observer = mESV.getViewTreeObserver();
        if (observer.isAlive()) {
            observer.addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
                @Override
                public void onGlobalLayout() {
                    Log.d("v06x", "onGlobalLayout()");
                    if (mESV != null) {
                        mESV.updateDimensions();
                    }
                    placeMovableButtons();
                }
            });
        }
    }

    private void placeMovableButtons() {
        final OnScreenKeyboard osk = findViewById(R.id.keyboard);
        final Button show_keyboard = findViewById(R.id.show_keyboard);
        final Button loadnik_button = findViewById(R.id.show_files);
        int orientation = this.getResources().getConfiguration().orientation;
        boolean portrait = orientation == Configuration.ORIENTATION_PORTRAIT;
        int yoffset = 0;
        if (osk.getState() == OnScreenKeyboard.STATE_HIDDEN && portrait) {
            yoffset = -show_keyboard.getHeight();
        }
        show_keyboard.setTranslationY(yoffset);
        loadnik_button.setTranslationY(yoffset);
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (!emulatorRunner.isRunning()) {
            new Handler(Looper.myLooper()).postDelayed(new Runnable() {
                @Override
                public void run() {
                    emulatorRunner.startEmulatorThread();
                    boolean stateRestored = emulatorRunner.restoreState(getCacheDir().getPath());
                    if (stateRestored) {
                        restoreCurrentRomDetails();
                    }
                    setLoadnikVisibility(stateRestored ? View.GONE : View.VISIBLE);
                    emulatorRunner.setHandler(mHandler);
                }
            }, 100);
        }
        else {
            emulatorRunner.setHandler(mHandler);
        }
        attachButtonListeners();
        NotificationManagerCompat.from(this).cancelAll();

        /* I'm just not sure at what point it's safe to request permissions. If requested directly,
           the app crashes without any usable diagnostics messages. */
        mHandler.postDelayed(new Runnable() {
                                 @Override
                                 public void run() {
                                     PermissionRequester.requestPermissions(MainActivity.this, PERMISSIONS_TOKEN);
                                 }
                             }, 350);
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (emulatorRunner != null) {
            emulatorRunner.setHandler(null);
            emulatorRunner.saveState(getCacheDir().getPath());
            saveCurrentRomDetails();
        }

        if (mBackButtonQuit) {
            /* Back pressed in the main window, in this case we want to terminate */
            emulatorRunner.fadeOut(new Runnable() {
                @Override
                public void run() {
                    mESV.onPause();
                    mESV = null;
                    emulatorRunner = null;
                    v06x = null;
                }
            });
        }
        else {
            showNotification();
        }
    }

    @Override
    protected void onStop() {
        super.onStop();
        mESV = null;
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        unbindBrutnikService();
        NotificationManagerCompat.from(this).cancelAll();
    }

    private void attachButtonListeners() {
        final Button show_keyboard = findViewById(R.id.show_keyboard);
        if (show_keyboard != null) {
            final OnScreenKeyboard osk = findViewById(R.id.keyboard);
            if (osk != null) {
                osk.setOnKeyListener(new OnScreenKeyboard.KeyListener() {
                    @Override
                    public void onKeyDown(int keycode) {
                        if (keycode == KeyEvent.KEYCODE_F11) {
                            prepareBootNotificationData();
                        }
                    }
                    @Override
                    public void onKeyUp(int keycode) { }
                });
                show_keyboard.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        osk.cycleState();
                    }
                });
            }
        }

        final Button loadnik_button = findViewById(R.id.show_files);
        if (loadnik_button != null) {
            loadnik_button.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    setLoadnikVisibility(View.VISIBLE);
                }
            });
        }
    }

    private void setLoadnikVisibility(int visibility) {
        View loadnik = findViewById(R.id.loader_tabs_fragment);
        if (loadnik != null) {
            loadnik.setVisibility(visibility);
        }
    }

    private void updateLoadnikInfo(int selector, String text) {
        if (mLoadnik == null) {
            return;
        }
        mLoadnik.updateTextView(selector, text);
   }

    @Override
    public void onBackPressed() {
        View loadnik = findViewById(R.id.loader_tabs_fragment);
        if (loadnik != null && loadnik.getVisibility() != View.GONE) {
            loadnik.setVisibility(View.GONE);
            return;
        }

        mBackButtonQuit = true;

        super.onBackPressed();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);

        View decor = getWindow().getDecorView();
        int w = decor.getWidth();
        int h = decor.getHeight();
        if (w > h) {
            if (hasFocus) {
                hideSystemUI();
            }

            if (mESV != null) {
                mESV.setOnTouchListener(new View.OnTouchListener() {
                    @Override
                    public boolean onTouch(View v, MotionEvent event) {
                        //Log.d("touch", event.toString());
                        switch (event.getAction()) {
                            case MotionEvent.ACTION_DOWN:
                                return event.getX() > 100 && event.getY() > 100;
                            case MotionEvent.ACTION_UP:
                                hideSystemUI();
                                break;
                        }
                        return false;
                    }
                });
            }
        }
    }

    private void hideSystemUI() {
        // Enables regular immersive mode.
        // For "lean back" mode, remove SYSTEM_UI_FLAG_IMMERSIVE.
        // Or for "sticky immersive," replace it with SYSTEM_UI_FLAG_IMMERSIVE_STICKY
        View decorView = getWindow().getDecorView();
        decorView.setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_IMMERSIVE
                        // Set the content to appear under the system bars so that the
                        // content doesn't resize when the system bars hide and show.
                        | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                        | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                        // Hide the nav bar and status bar
                        | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_FULLSCREEN);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }

    // Shows the system bars by removing all the flags
    // except for the ones that make the content appear under the system bars.
    private void showSystemUI() {
        View decorView = getWindow().getDecorView();
        decorView.setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                        | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN);
        getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }

    @Override
    public void onListFragmentInteraction(RomListItem item, int selector) {
        final byte[] data = item.load();
        if (data == null || data.length == 0) {
            Toast.makeText(this, "\uD83D\uDCA9", Toast.LENGTH_SHORT).show();
            return;
        }
        final int org;
        final int loadkind = item.getKind();
        final boolean reset;

        if (selector == RomListItem.SELECTOR_A || selector == RomListItem.SELECTOR_B) {
            org = selector;
            reset = false;
            updateLoadnikInfo(selector, item.content);
        }
        else if (selector == RomListItem.SELECTOR_E) {
            org = 0;
            reset = true;
            updateLoadnikInfo(selector, item.content);
        } else {
            // SELECTOR_NONE
            setLoadnikVisibility(View.GONE);

            org = item.getOrg();
            if (loadkind == LoadKind.FDD && sRomListItem.getKind() != LoadKind.UNKIND) {
                /* just swap out the floppy without resetting */
                reset = false;
            }
            else {
                reset = true;
            }
        }

        Runnable load = new Runnable() {
            @Override
            public void run() {
                v06x.LoadAsset(data, loadkind, org);
            }
        };

        if (reset) {
            final boolean blkvvod = loadkind == LoadKind.FDD || loadkind == LoadKind.EDD;
            emulatorRunner.reset(blkvvod, load);
        }
        else {
            emulatorRunner.post(load);
        }

        sRomListItem = item;
    }

    @Override
    public void removeAllMedia() {
        byte[] nil = new byte[0];
        v06x.LoadAsset(nil, LoadKind.FDD, 0);
        v06x.LoadAsset(nil, LoadKind.FDD, 1);
        v06x.LoadAsset(nil, LoadKind.EDD, 0);

        mLoadnik.updateTextView(RomListItem.SELECTOR_A, "");
        mLoadnik.updateTextView(RomListItem.SELECTOR_B, "");
        mLoadnik.updateTextView(RomListItem.SELECTOR_E, "");
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        final OnScreenKeyboard osk = findViewById(R.id.keyboard);
        if (osk != null) {
            osk.saveInstanceState(outState);
        }
    }

    @Override
    protected void onRestoreInstanceState(Bundle savedInstanceState) {
        super.onRestoreInstanceState(savedInstanceState);
        final OnScreenKeyboard osk = findViewById(R.id.keyboard);
        if (osk != null) {
            osk.restoreInstanceState(savedInstanceState);
        }
    }

    @Override
    public void onAttachFragment(Fragment fragment) {
        super.onAttachFragment(fragment);
        Log.d("fragment", "onAttachFragment" + fragment);
        if (fragment instanceof LoaderTabsFragment) {
            mLoadnik = (LoaderTabsFragment)fragment;
        }
    }

    private void createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            CharSequence name = getString(R.string.channel_name);
            String description = getString(R.string.channel_description);
            int importance = NotificationManager.IMPORTANCE_DEFAULT;
            NotificationChannel channel = new NotificationChannel(CHANNEL_ID, name, importance);
            channel.setDescription(description);
            channel.setSound(null,null);
            channel.enableLights(false);
            channel.enableVibration(false);

            NotificationManager notificationManager = getSystemService(NotificationManager.class);
            notificationManager.createNotificationChannel(channel);
        }
    }

    private void prepareBootNotificationData() {
        if (sRomListItem == null || sRomListItem.getKind() != LoadKind.FDD) {
            sRomListItem = new RomListItemDummy("boot", "boot", "BOOT ROM");
        }
    }

    private void showNotification() {
        Intent intent = new Intent(this, MainActivity.class);
        intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP);
        PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, intent,
                PendingIntent.FLAG_UPDATE_CURRENT);
        NotificationCompat.Builder builder = new NotificationCompat.Builder(this, CHANNEL_ID)
                .setDefaults(NotificationCompat.DEFAULT_ALL)
                .setSmallIcon(R.drawable.notification_icon)
                .setContentTitle(sRomListItem.content + getResources().getString(R.string.is_running_in_background))
                .setContentText(sRomListItem.details)
                .setPriority(NotificationCompat.PRIORITY_DEFAULT)
                .setContentIntent(pendingIntent)
                .setOngoing(true)
                .setWhen(0)
                .setAutoCancel(true);
        NotificationManagerCompat notificationManager = NotificationManagerCompat.from(this);
        notificationManager.notify(BRING_TO_FRONT, builder.build());
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        switch (requestCode) {
            case PERMISSIONS_TOKEN:
                if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    Broadcasts.sendInternalBroadcast(this, Broadcasts.RESCAN_LOCAL_STORAGE);
                }
                break;
        }
    }

    private void bindBrutnikService() {
        mBrutnikConnection = new ServiceConnection() {
            @Override
            public void onServiceConnected(ComponentName name, IBinder service) {
                ((Brutnik.BrutnikBinder)service).service
                        .startService(new Intent(MainActivity.this, Brutnik.class));
            }

            @Override
            public void onServiceDisconnected(ComponentName name) {
            }
        };
        bindService(new Intent(MainActivity.this,
                        Brutnik.class), mBrutnikConnection, Context.BIND_AUTO_CREATE);
    }

    private void unbindBrutnikService() {
        unbindService(mBrutnikConnection);
    }

    // TODO: save mounted disks not as roms
    private void saveCurrentRomDetails() {
        SharedPreferences prefs = getSharedPreferences("rom", MODE_PRIVATE);
        SharedPreferences.Editor e = prefs.edit();
        e.putString("rom.id", sRomListItem.id);
        e.putString("rom.content", sRomListItem.content);
        e.putString("rom.details", sRomListItem.details);
        e.putString("rom.class", sRomListItem.getClass().getSimpleName());
        e.commit();
    }

    private void restoreCurrentRomDetails() {
        SharedPreferences prefs = getSharedPreferences("rom", MODE_PRIVATE);

        String klaas = prefs.getString("rom.class", "RomListItemDummy");
        String id = prefs.getString("rom.id", null);
        if (id == null) return;
        String content = prefs.getString("rom.content", null);
        if (content == null) return;
        String details = prefs.getString("rom.details", null);
        if (details == null) return;

        switch (klaas) {
            case "RomListItemAsset":
                sRomListItem = new RomListItemAsset(getApplication(), id, content, details);
                break;
            case "RomListItemStorage":
                sRomListItem = new RomListItemStorage(id, content, details);
                break;
            default:
                sRomListItem = new RomListItemDummy(id, content, details);
                break;
        }

        /* Remount floppy image */
        if (sRomListItem.getKind() == LoadKind.FDD) {
            final byte[] data = sRomListItem.load();
            if (data != null && data.length > 0) {
                emulatorRunner.post(new Runnable() {
                    @Override
                    public void run() {
                        v06x.LoadAsset(data, LoadKind.FDD, 0);
                    }
                });
            }
        }
    }


}
