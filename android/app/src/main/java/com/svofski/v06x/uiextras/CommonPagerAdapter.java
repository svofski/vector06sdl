package com.svofski.v06x.uiextras;

import android.support.annotation.IdRes;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.view.PagerAdapter;
import android.view.View;
import android.view.ViewGroup;

import java.util.ArrayList;
import java.util.List;

public class CommonPagerAdapter extends PagerAdapter {
    private List<Integer> pageIds = new ArrayList<>();
    private List<String> pageTitles = new ArrayList<>();
    private List<Class> modelClasses = new ArrayList<>();

    public void insertViewId(@IdRes int pageId, String title, Class modelClass) {
        pageIds.add(pageId);
        pageTitles.add(title);
        modelClasses.add(modelClass);
    }

    @NonNull
    @Override
    public Object instantiateItem(@NonNull ViewGroup container, int position) {
        return container.findViewById(pageIds.get(position));
    }

    @Override
    public void destroyItem(@NonNull ViewGroup container, int position, @NonNull Object object) {
        container.removeView((View) object);
    }

    @Override
    public int getCount() {
        return pageIds.size();
    }

    @Override
    public boolean isViewFromObject(@NonNull View view, @NonNull Object object) {
        return view == object;
    }

    @Nullable
    @Override
    public CharSequence getPageTitle(int position) {
        return pageTitles.get(position);
    }
}
