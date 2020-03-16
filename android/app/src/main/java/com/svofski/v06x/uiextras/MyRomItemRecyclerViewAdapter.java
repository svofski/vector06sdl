package com.svofski.v06x.uiextras;

import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import com.svofski.v06x.R;
import com.svofski.v06x.cpp.LoadKind;
import com.svofski.v06x.uiextras.RomListerFragment.OnListFragmentInteractionListener;
import com.svofski.v06x.uiextras.loaders.RomListItem;

import static com.svofski.v06x.uiextras.loaders.RomListItem.SELECTOR_A;
import static com.svofski.v06x.uiextras.loaders.RomListItem.SELECTOR_B;
import static com.svofski.v06x.uiextras.loaders.RomListItem.SELECTOR_E;
import static com.svofski.v06x.uiextras.loaders.RomListItem.SELECTOR_NONE;

public class MyRomItemRecyclerViewAdapter extends
        RecyclerView.Adapter<MyRomItemRecyclerViewAdapter.ViewHolder> {
    private final OnListFragmentInteractionListener mListener;
    private LoaderModel mModel;

    public MyRomItemRecyclerViewAdapter(LoaderModel model,
                                        OnListFragmentInteractionListener listener) {
        mModel = model;
        mListener = listener;
    }

    @Override
    public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        View view = LayoutInflater.from(parent.getContext())
                .inflate(R.layout.fragment_romitem, parent, false);
        return new ViewHolder(view);
    }

    @Override
    public void onBindViewHolder(final ViewHolder holder, int position) {
        RomListItem item = mModel.get(position);
        if (item != null) {
            holder.mItem = item;
            holder.mContentView.setText(item.content);
            holder.mDetailsView.setText(item.details);

            holder.mView.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    if (null != mListener) {
                        mListener.onListFragmentInteraction(holder.mItem, SELECTOR_NONE);
                    }
                }
            });

            for (int i = 0; i < holder.mButtons.length; ++i) {
                final int selector = i;
                holder.mButtons[i].setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        if (null != mListener) {
                            mListener.onListFragmentInteraction(holder.mItem, selector);
                        }
                    }
                });
            }

            if (holder.mItem.getKind() == LoadKind.FDD) {
                holder.mButtons[SELECTOR_A].setVisibility(View.VISIBLE);
                holder.mButtons[SELECTOR_B].setVisibility(View.VISIBLE);
                holder.mButtons[SELECTOR_E].setVisibility(View.GONE);
            }
            else if (holder.mItem.getKind() == LoadKind.EDD) {
                holder.mButtons[SELECTOR_A].setVisibility(View.GONE);
                holder.mButtons[SELECTOR_B].setVisibility(View.GONE);
                holder.mButtons[SELECTOR_E].setVisibility(View.VISIBLE);
            }
            else {
                holder.mButtons[SELECTOR_A].setVisibility(View.GONE);
                holder.mButtons[SELECTOR_B].setVisibility(View.GONE);
                holder.mButtons[SELECTOR_E].setVisibility(View.INVISIBLE);
            }
        }
    }

    @Override
    public int getItemCount() {
        return mModel.size();
    }

    public class ViewHolder extends RecyclerView.ViewHolder {
        public final View mView;
        public final TextView mContentView;
        public final TextView mDetailsView;
        public final Button[] mButtons = new Button[3];
        public RomListItem mItem;

        public ViewHolder(View view) {
            super(view);
            mView = view;
            mContentView = view.findViewById(R.id.content);
            mDetailsView = view.findViewById(R.id.details);

            mButtons[0] = view.findViewById(R.id.button_a);
            mButtons[1] = view.findViewById(R.id.button_b);
            mButtons[2] = view.findViewById(R.id.button_e);
        }

        @Override
        public String toString() {
            return super.toString() + " '" + mContentView.getText() + "'";
        }
    }
}
