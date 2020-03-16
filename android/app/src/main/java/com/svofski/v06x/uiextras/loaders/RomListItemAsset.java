package com.svofski.v06x.uiextras.loaders;

import android.app.Application;
import android.content.res.AssetManager;

import java.io.IOException;
import java.io.InputStream;

public class RomListItemAsset extends RomListItem {
    Application mContext;

    public RomListItemAsset(Application context, String id, String content, String details) {
        super(id, content, details);
        mContext = context;
    }

    @Override
    public byte[] load() {
        AssetManager assetManager = mContext.getAssets();
        byte[] result = null;
        try {
            InputStream is = assetManager.open(this.id);
            result = new byte[is.available()];
            is.read(result);
            is.close();
        }
        catch(IOException ioe) {
        }

        return result;
    }
}
