package com.svofski.v06x;

import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.os.Process;
import android.os.SystemClock;

import com.svofski.v06x.cpp.Emulator;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;

public class EmulatorRunner implements Handler.Callback {
    public static final int REQUEST_RENDER = 1001;

    private boolean mEngineRunning  = false;
    private byte[] mState = null;

    private AudioTrack mPlayer;
    private Handler mHandler;
    private Handler mRunnerHandler;

    private final Object mLock = new Object();

    private final Object mStateSemaphore = new Object();
    private boolean mRestoreStateResult = false;
    private HandlerThread mHandlerThread;
    private final static int MSG_AUDIO_REQUEST = 1;
    private final static int MSG_RESET = 2;
    private final static int MSG_SAVE_STATE = 3;
    private final static int MSG_RESTORE_STATE = 4;

    private long mNextFrameTime = 0;
    private int frameCount = 0;
    private float mVolumeIncrement  = 0f;
    private float mVolume = 0.0f;

    public EmulatorRunner() {
    }

    public int benchmark() {
        return 0;
    }

    public boolean isRunning() {
        return mEngineRunning;
    }

    public void setHandler(Handler handler) {
        synchronized(mLock) {
            mHandler = handler;
        }
    }


    @Override
    public boolean handleMessage(Message msg) {
        Emulator e = MainActivity.emulator();
        switch (msg.what) {
            case MSG_AUDIO_REQUEST: {
                ++frameCount;
                mNextFrameTime = mNextFrameTime + 1000/50;
                e.executeFrame();
                mPlayer.write(e.samples(), 0,
                            e.samples().length, AudioTrack.WRITE_NON_BLOCKING);
                synchronized (mLock) {
                    if (mHandler != null) {
                        mHandler.obtainMessage(EmulatorRunner.REQUEST_RENDER).sendToTarget();
                    }
                }

                final int first_frame = 6;
                if (frameCount == first_frame) {
                    mPlayer.play();
                    mVolumeIncrement = 0.1f;
                }
                if (mVolumeIncrement != 0f) {
                    mVolume += mVolumeIncrement;
                    if (mVolume >= 1.0f && mVolumeIncrement != 0f) {
                        mVolume = 1.0f;
                        mVolumeIncrement = 0f;
                    }
                    if (mVolume < 0f && mVolumeIncrement != 0f) {
                        mVolume = 0f;
                        mVolumeIncrement = 0f;
                    }
                    mPlayer.setVolume(mVolume);
                }

                if (mHandlerThread.isAlive() && mEngineRunning) {
                    mRunnerHandler.sendEmptyMessageAtTime(MSG_AUDIO_REQUEST, mNextFrameTime);
                }
            }
            break;
            case MSG_RESET: {
                Runnable r = (Runnable) msg.obj;
                boolean blkvvod = msg.arg1 != 0 ? true : false;
                if (r != null) {
                    r.run();
                }
                e.reset(blkvvod);
            }
            break;
            case MSG_SAVE_STATE: {
                mState = MainActivity.emulator().ExportState();
                synchronized(mStateSemaphore) {
                    mStateSemaphore.notifyAll();
                }
            }
            break;
            case MSG_RESTORE_STATE: {
                mRestoreStateResult = MainActivity.emulator().RestoreState(mState);
                synchronized(mStateSemaphore) {
                    mStateSemaphore.notifyAll();
                }
             }
            break;
        }
        return true; // true = no further handling is desired
    }

    public void startEmulatorThread()
    {
        frameCount = 0;
        prepareAudioTrack();

        mEngineRunning = true;
        mHandlerThread = new HandlerThread("Emulator runner", Process.THREAD_PRIORITY_VIDEO);
        mHandlerThread.start();
        mRunnerHandler = new Handler(mHandlerThread.getLooper(), this);

        mNextFrameTime = SystemClock.uptimeMillis() + 1000/50;
        mRunnerHandler.sendEmptyMessageAtTime(MSG_AUDIO_REQUEST, mNextFrameTime);
    }

    private void prepareAudioTrack() {
        int samplerate = 48000;
        //int min_buffer_size = AudioTrack.getMinBufferSize(48000, AudioFormat.CHANNEL_OUT_STEREO,
        //        AudioFormat.ENCODING_PCM_FLOAT);

        final int samples_per_frame = 48000/50;
        MainActivity.emulator().allocSamples(samples_per_frame);

        /* it's possible to use a smaller buffer but *8 seems to be safer on nexus7 */
        int size_in_bytes = samples_per_frame * 4 * 2 * 8; /* sizeof(float) * stereo * 8 just in case */
        //Log.d("v06x", "Minimum buffer size: " + min_buffer_size + " ours: " + size_in_bytes);

        AudioAttributes attributes = new AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_GAME)
                .setContentType(AudioAttributes.CONTENT_TYPE_MOVIE).build();
        AudioFormat format = new AudioFormat.Builder()
                .setEncoding(AudioFormat.ENCODING_PCM_FLOAT)
                .setSampleRate(samplerate)
                .setChannelMask(AudioFormat.CHANNEL_OUT_STEREO)
                .build();
        mPlayer = new AudioTrack(attributes, format,
                size_in_bytes, AudioTrack.MODE_STREAM, AudioManager.AUDIO_SESSION_ID_GENERATE);
        mPlayer.setVolume(0f);
    }

    public void reset(boolean blkvvod, final Runnable before) {
        mRunnerHandler.obtainMessage(MSG_RESET, blkvvod ? 1 : 0, 0, before).sendToTarget();
    }

    public void finish() {
        mPlayer.setPlaybackPositionUpdateListener(null);
        mPlayer.stop();
        mPlayer.flush();
        mEngineRunning = false;
        try {
            Thread.currentThread().sleep(50);
        }
        catch (InterruptedException e) {}
        mHandlerThread.quitSafely();
    }

    public void fadeOut(Runnable onEnd) {
        mVolumeIncrement = -0.2f;
        try {
            Thread.currentThread().sleep(10 * 1000/50);
        }
        catch (InterruptedException e) {}
        mPlayer.stop();
        mPlayer.flush();
        mEngineRunning = false;
        try {
            Thread.currentThread().sleep(50);
        }
        catch (InterruptedException e) {}
        mHandlerThread.quitSafely();
        if (onEnd != null) {
            onEnd.run();
        }
    }

    public byte[] saveState(String cachePath) {
        mRunnerHandler.sendEmptyMessageDelayed(MSG_SAVE_STATE, 10);
        synchronized(mStateSemaphore) {
            try {
                mStateSemaphore.wait();
            } catch (InterruptedException e) {
            }
        }
        saveFile(cachePath + File.separator + "state.bin", mState);
        mState = null;
        return null;
    }


    public boolean restoreState(String cachePath) {
        final String filePath = cachePath + File.separator + "state.bin";
        byte[] state = loadFile(filePath);
        new File(filePath).delete();
        if (state == null || state.length == 0) {
            return false;
        }
        mState = state;
        mRunnerHandler.sendEmptyMessageDelayed(MSG_RESTORE_STATE, 10);
        synchronized (mStateSemaphore) {
            try {
                mStateSemaphore.wait();
            } catch (InterruptedException e) {
            }
        }
        return mRestoreStateResult;
    }

    public void post(Runnable r) {
        if (mHandlerThread.isAlive() && mEngineRunning) {
            mRunnerHandler.post(r);
        }
    }

    private void saveFile(String filePath, byte[] data) {
        try {
            FileOutputStream fos = new FileOutputStream(filePath);
            fos.write(data);
            fos.close();
        }
        catch (FileNotFoundException e) {}
        catch (IOException ioe) {}
    }

    private byte[] loadFile(String filepath) {
        byte[] data = null;
        try {
            FileInputStream fis = new FileInputStream(filepath);
            data = new byte[fis.available()];
            fis.read(data);
        }
        catch(FileNotFoundException e) {
        }
        catch(IOException e) {
        }
        return data;
    }


}
