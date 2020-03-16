package com.svofski.v06x;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.view.SurfaceHolder;
import android.view.View;
import android.view.ViewGroup;

public class EmulatorSurfaceView extends GLSurfaceView {
    private final EmulatorRenderer renderer;

    public EmulatorSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
        renderer = new EmulatorRenderer(context);
        setRenderer(renderer);
        setRenderMode(RENDERMODE_WHEN_DIRTY);
        //setRenderMode(RENDERMODE_CONTINUOUSLY); <-- better on some devices, but taxing on nexus7
    }

    @Override
    public void onPause() {
        super.onPause();
    }

    @Override
    public void onResume() {
        super.onResume();
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        super.surfaceDestroyed(holder);
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
        super.surfaceChanged(holder, format, w, h);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        super.surfaceCreated(holder);
    }

    public void updateDimensions() {
        View racine = getRootView();
        ViewGroup.LayoutParams lp = getLayoutParams();
        boolean update = false;

        if (racine.getWidth() < racine.getHeight()) {
            int w = getWidth();
            int h = w * 4 / 5;
            if (h != lp.height) {
                lp.height = h;
                update = true;
            }
        } else {
            int h = getHeight();
            int w = h * 5 / 4;
            if (w != lp.width) {
                lp.width = w;
                update = true;
            }
        }
        if (update) {
            setLayoutParams(lp);
        }
    }
}