package com.svofski.v06x.uiextras.loaders;

import android.app.Application;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.support.annotation.NonNull;

import com.svofski.v06x.uiextras.LoaderModel;

import org.apache.commons.io.FileUtils;
import org.apache.commons.io.IOCase;
import org.apache.commons.io.filefilter.FileFilterUtils;
import org.apache.commons.io.filefilter.IOFileFilter;
import org.apache.commons.io.filefilter.TrueFileFilter;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.Iterator;

public class LocalStorageLoaderModel extends LoaderModel {
    private ArrayList<RomListItem> roms;
    private final Object mLock = new Object();
    private Thread mThread = null;

    public LocalStorageLoaderModel(@NonNull Application application) {
        super(application);
    }

    @Override
    public int size() {
        scanFiles();
        synchronized(mLock) {
            return roms != null ? roms.size() : 0;
        }
    }

    @Override
    public RomListItem get(int position) {
        synchronized(mLock) {
            return roms != null ? roms.get(position) : null;
        }
    }

    @Override
    public void rescan() {
        synchronized(mLock) {
            roms = null;
        }
        scanFiles();
    }

    private void scanFiles() {
        if (roms == null) {
           synchronized(mLock) {
               if (mThread == null) {
                   mThread = new Thread(new Runnable() {
                       @Override
                       public void run() {
                            scan();
                            mThread = null;
                       }
                   });
                   mThread.start();
               }
           }
        }
    }

    private void scan() {
        new Handler(Looper.getMainLooper()).post(new Runnable() {
            @Override
            public void run() {
                if (observer != null) {
                    observer.onChanged(null);
                }
            }
        });

        ArrayList<RomListItem> r = new ArrayList<>();

        IOFileFilter filter = FileFilterUtils.or(
                FileFilterUtils.suffixFileFilter("vec", IOCase.INSENSITIVE),
                FileFilterUtils.suffixFileFilter("rom", IOCase.INSENSITIVE),
                FileFilterUtils.suffixFileFilter("r0m", IOCase.INSENSITIVE),
                FileFilterUtils.suffixFileFilter("com", IOCase.INSENSITIVE),
                FileFilterUtils.suffixFileFilter("fdd", IOCase.INSENSITIVE),
                FileFilterUtils.suffixFileFilter("edd", IOCase.INSENSITIVE));

        Iterator<File> iterator = FileUtils.iterateFiles(
                Environment.getExternalStorageDirectory(),
                filter, TrueFileFilter.INSTANCE);
        while (iterator.hasNext()) {
            File romfile = iterator.next();
            try {
                InputStream is = new FileInputStream(romfile);
                r.add(new RomListItemStorage(romfile.getPath(), romfile.getName(),
                        LoaderUtil.formatDetail(getApplication(), romfile.getPath(), is)));
                is.close();
            } catch (FileNotFoundException fnf) {
            } catch (IOException ioe) {}
        }

        Collections.sort(r, new Comparator<RomListItem>() {
            @Override
            public int compare(RomListItem o1, RomListItem o2) {
                return o1.content.compareToIgnoreCase(o2.content);
            }
        });

        synchronized(mLock) {
            roms = r;
        }

        new Handler(Looper.getMainLooper()).post(new Runnable() {
            @Override
            public void run() {
                if (observer != null) {
                    observer.onChanged(LocalStorageLoaderModel.this);
                }
            }
        });
    }
}
