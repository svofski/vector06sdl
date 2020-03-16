package com.svofski.v06x.uiextras;

import android.arch.lifecycle.Observer;
import android.arch.lifecycle.ViewModelProviders;
import android.content.Context;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.support.v4.widget.SwipeRefreshLayout;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;

import com.svofski.v06x.R;
import com.svofski.v06x.uiextras.loaders.RomListItem;

public class RomListerFragment extends Fragment {

    public interface OnListFragmentInteractionListener {
        void onListFragmentInteraction(RomListItem item, int selector);
    }

    private OnListFragmentInteractionListener mListener;
    private Class mModelClass;
    private RecyclerView mRecyclerView;
    private SwipeRefreshLayout mRefreshnik;

    public RomListerFragment() {
        Log.d("fragment", "new RomListerFragment " + hashCode());
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setRetainInstance(true);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        FrameLayout ll = (FrameLayout)inflater.inflate(R.layout.fragment_romitem_list,
                container, false);

        mRecyclerView = ll.findViewById(R.id.list);
        Context context = mRecyclerView.getContext();
        mRecyclerView.setLayoutManager(new LinearLayoutManager(context));

        mRefreshnik = ll.findViewById(R.id.swipe_refreshnik);
        mRefreshnik.setOnRefreshListener(new SwipeRefreshLayout.OnRefreshListener() {
            @Override
            public void onRefresh() {
                final LoaderModel model = (LoaderModel)ViewModelProviders.of(getActivity()).get(mModelClass);
                model.rescan();
            }
        });

        return ll;
    }

    @Override
    public void onResume() {
        super.onResume();

    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        if (context instanceof OnListFragmentInteractionListener) {
            mListener = (OnListFragmentInteractionListener) context;
        } else {
            throw new RuntimeException(context.toString()
                    + " must implement OnListFragmentInteractionListener");
        }
    }

    @Override
    public void onActivityCreated(@Nullable Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        final LoaderModel model = (LoaderModel)ViewModelProviders.of(getActivity()).get(mModelClass);

        final MyRomItemRecyclerViewAdapter adapter = new MyRomItemRecyclerViewAdapter(model,
                new OnListFragmentInteractionListener() {
                    @Override
                    public void onListFragmentInteraction(RomListItem item, int selector) {
                        if (mListener != null) {
                            mListener.onListFragmentInteraction(item, selector);
                        }
                    }
                });

        final Observer<LoaderModel> observer = new Observer<LoaderModel>() {
            @Override
            public void onChanged(@Nullable LoaderModel loaderModel) {
                if (loaderModel == null) {
                    if (mRefreshnik != null) {
                        mRefreshnik.setRefreshing(true);
                    }
                }
                else {
                    if (mRecyclerView != null) {
                        mRecyclerView.setAdapter(adapter);
                    }
                    if (mRefreshnik != null) {
                        mRefreshnik.setRefreshing(false);
                    }
                }
            }
        };
        model.observe(observer);
        mRecyclerView.setAdapter(adapter);
    }

    @Override
    public void onDetach() {
        super.onDetach();
        mListener = null;
    }

    public void setModelClass(Class cls) {
        mModelClass = cls;
    }

}
