package com.svofski.v06x.uiextras;

import android.arch.lifecycle.ViewModelProviders;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.Uri;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.design.widget.TabLayout;
import android.support.v4.app.Fragment;
import android.support.v4.view.ViewPager;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import com.svofski.v06x.Broadcasts;
import com.svofski.v06x.R;
import com.svofski.v06x.uiextras.loaders.BundledROMLoaderModel;
import com.svofski.v06x.uiextras.loaders.LocalStorageLoaderModel;
import com.svofski.v06x.uiextras.loaders.RomListItem;

public class LoaderTabsFragment extends Fragment {
    private OnFragmentInteractionListener mListener;
    private BroadcastReceiver mBroadcastReceiver;

    private final static String KEY_SLOT_A = "loadertabs.a";
    private final static String KEY_SLOT_B = "loadertabs.b";
    private final static String KEY_SLOT_E = "loadertabs.e";
    private final static String KEY_VISIBLE = "loadertabs.visible";
    private final static String KEY_PAGE = "loadertabs.page";


    public LoaderTabsFragment() {
        Log.d("fragment", "new LoaderTabsFragment " + hashCode());
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setRetainInstance(true);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        RomListerFragment bundled = (RomListerFragment) getChildFragmentManager().findFragmentById(R.id.bundled_roms_fragment);
        bundled.setModelClass(BundledROMLoaderModel.class);
        RomListerFragment local = (RomListerFragment) getChildFragmentManager().findFragmentById(R.id.sdcard_roms_fragment);
        local.setModelClass(LocalStorageLoaderModel.class);

        Button clearAll = view.findViewById(R.id.button_clearall);
        clearAll.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mListener != null) {
                    mListener.removeAllMedia();
                }
            }
        });
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        // Inflate the layout for this fragment
        View view = inflater.inflate(R.layout.fragment_loader_tabs, container, false);

        ViewPager viewPager = view.findViewById(R.id.loader_viewpager);
        CommonPagerAdapter adapter = new CommonPagerAdapter();
        adapter.insertViewId(R.id.loader_page_one,
                view.getResources().getString(R.string.loader_page_one),
                BundledROMLoaderModel.class);
        adapter.insertViewId(R.id.loader_page_two,
                view.getResources().getString(R.string.loader_page_two),
                LocalStorageLoaderModel.class);
        viewPager.setAdapter(adapter);

        TabLayout tabLayout = view.findViewById(R.id.loader_tablayout);
        tabLayout.setupWithViewPager(viewPager);

        if (savedInstanceState != null) {
            int visiboblity = savedInstanceState.getInt(KEY_VISIBLE, View.GONE);
            int page = savedInstanceState.getInt(KEY_PAGE, 0);
            view.setVisibility(visiboblity);
            viewPager.setCurrentItem(page);

            restoreTextView(view, savedInstanceState, KEY_SLOT_A, R.id.slot_disk_a);
            restoreTextView(view, savedInstanceState, KEY_SLOT_B, R.id.slot_disk_b);
            restoreTextView(view, savedInstanceState, KEY_SLOT_E, R.id.slot_kvaz);
        }

        return view;
    }

    private void restoreTextView(View view, Bundle b, String key, int tv_id) {
        TextView tv = view.findViewById(tv_id);
        tv.setText(b.getCharSequence(key, ""));
    }

    private void saveTextView(Bundle b, String key, int tv_id) {
        TextView tv = getView().findViewById(tv_id);
        b.putCharSequence(key, tv.getText());
    }

    public void updateTextView(int selector, String text) {
        TextView textView = null;
        switch (selector) {
            case RomListItem.SELECTOR_A:
                textView = getView().findViewById(R.id.slot_disk_a);
                break;
            case RomListItem.SELECTOR_B:
                textView = getView().findViewById(R.id.slot_disk_b);
                break;
            case RomListItem.SELECTOR_E:
                textView = getView().findViewById(R.id.slot_kvaz);
                break;
        }
        String[] slot_names = getContext().getResources().getStringArray(R.array.slot_names);
        if (selector >= 0 && selector < slot_names.length && textView != null) {
            textView.setText(slot_names[selector] + text);
        }
     }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        registerBroadcastReceiver();
        if (context instanceof OnFragmentInteractionListener) {
            mListener = (OnFragmentInteractionListener) context;
        } else {
            throw new RuntimeException(context.toString()
                    + " must implement OskInteractionListener");
        }
    }

    @Override
    public void onDetach() {
        super.onDetach();
        mListener = null;
        unregisterBroadcastReceiver();
    }


    public interface OnFragmentInteractionListener {
        void removeAllMedia();
    }

    @Override
    public void onSaveInstanceState(@NonNull Bundle outState) {
        super.onSaveInstanceState(outState);
        View v = getView();
        if (v != null) {
            outState.putInt(KEY_VISIBLE, v.getVisibility());
            ViewPager viewPager = v.findViewById(R.id.loader_viewpager);
            outState.putInt(KEY_PAGE, viewPager.getCurrentItem());

            saveTextView(outState, KEY_SLOT_A, R.id.slot_disk_a);
            saveTextView(outState, KEY_SLOT_B, R.id.slot_disk_b);
            saveTextView(outState, KEY_SLOT_E, R.id.slot_kvaz);
        }
    }

    private void registerBroadcastReceiver()
    {
        mBroadcastReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                LoaderModel m = ViewModelProviders.of(getActivity()).get(LocalStorageLoaderModel.class);
                m.rescan();
            }
        };

        getContext().registerReceiver(mBroadcastReceiver,
                new IntentFilter(Broadcasts.RESCAN_LOCAL_STORAGE));
    }

    private void unregisterBroadcastReceiver()
    {
        getContext().unregisterReceiver(mBroadcastReceiver);
        mBroadcastReceiver = null;
    }
}
