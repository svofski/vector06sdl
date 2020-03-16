package com.svofski.v06x.uiextras.loaders;

import android.app.Application;
import android.content.res.AssetManager;
import android.os.Handler;
import android.os.Looper;
import android.support.annotation.NonNull;

import com.svofski.v06x.uiextras.LoaderModel;

import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;

public class BundledROMLoaderModel extends LoaderModel {
    private List<RomListItem> roms;

    public BundledROMLoaderModel(@NonNull Application application) {
        super(application);
    }

    private List<RomListItem> loadBundledAssets() {
        ArrayList<RomListItem> roms = new ArrayList<>();
        AssetManager assetManager = getApplication().getApplicationContext().getAssets();
        try {
            for (String item: assetManager.list("")) {
                String detail = null;
                try {
                    InputStream is = assetManager.open(item, AssetManager.ACCESS_RANDOM);
                    detail = LoaderUtil.formatDetail(getApplication(), item, is);
                    is.close();
                }
                catch (IOException fail) {}

                if (detail != null) {
                    roms.add(new RomListItemAsset(getApplication(), item, item, detail));
                }
            }
        } catch (IOException ioe) {
        }

        if (observer != null) {
            new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
                @Override
                public void run() {
                    observer.onChanged(BundledROMLoaderModel.this);
                }
            }, 50);
        }
        return roms;
    }

    @Override
    public void rescan() {
        loadBundledAssets();
    }

    @Override
    public int size() {
        if (roms == null) {
            roms = loadBundledAssets();
        }
        return roms.size();
    }

    @Override
    public RomListItem get(int position) {
        if (roms == null) {
            roms = loadBundledAssets();
        }
        return roms.get(position);
    }
}
