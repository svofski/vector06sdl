package com.svofski.v06x.uiextras;

import android.app.Application;
import android.arch.lifecycle.AndroidViewModel;
import android.arch.lifecycle.Observer;

import com.svofski.v06x.uiextras.loaders.RomListItem;

abstract public class LoaderModel extends AndroidViewModel {
    protected Observer<LoaderModel> observer;

    public LoaderModel(Application application) {
        super(application);
    }

    public abstract int size();

    public abstract RomListItem get(int position);

    public void rescan() {}

    public void observe(Observer<LoaderModel> observer) {
        this.observer = observer;
    }
}
