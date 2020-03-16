package com.svofski.v06x.cpp;

public class Emulator {
    public final static int PIXELS_WIDTH = 576;
    public final static int PIXELS_HEIGHT = 288;


    private byte[][] mPixels2 = new byte[2][PIXELS_WIDTH*PIXELS_HEIGHT*4];
    private float[] mSamples = null;
    private int mWritePixels = 0;

    public byte[] getPixels() {
        return mPixels2[mWritePixels ^ 1];
    }

    public int getPixelsWidth() {
        return PIXELS_WIDTH;
    }

    public int getPixelsHeight() {
        return PIXELS_HEIGHT;
    }

    public float[] samples() {
        return mSamples;
    }

    public void allocSamples(int samples_per_frame) {
        mSamples = new float[samples_per_frame * 2];
    }

    public int executeFrame() {
        return ExecuteFrame(mPixels2[mWritePixels ^= 1], mSamples);
    }

    public native int Init();
    public native void KeyDown(int scancode);
    public native void KeyUp(int scancode);
    public native void LoadAsset(byte[] content, int kind, int org);
    public native void reset(boolean blkvvod);
    public native byte[] ExportState();
    public native boolean RestoreState(byte[] state);

    private native int ExecuteFrame(byte[] pixels, float[] samples);
}
