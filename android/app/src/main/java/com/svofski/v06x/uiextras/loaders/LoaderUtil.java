package com.svofski.v06x.uiextras.loaders;

import android.content.Context;

import com.svofski.v06x.R;

import java.io.IOException;
import java.io.InputStream;

public abstract class LoaderUtil {
    public static String formatDetail(Context c, String res, InputStream is) throws IOException {
        String ilo = res.toLowerCase();
        String detail = null;
        int ksize = (is.available() + 1023) / 1024;
        is.close();

        if (ilo.endsWith(".rom") || ilo.endsWith(".r0m") || ilo.endsWith(".vec")) {
            detail = c.getString(R.string.detail_rom);
        }
        else if (ilo.endsWith("com")) {
            detail = c.getString(R.string.detail_com);
        }
        else if (ilo.endsWith(".fdd")) {
            detail = c.getString(R.string.detail_fdd);
        }
        else if (ilo.endsWith(".edd")) {
            detail = c.getString(R.string.detail_kvaz);
        }
        detail += " " + ksize + "K";
        return detail;
    }
}
