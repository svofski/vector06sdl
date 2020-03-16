package com.svofski.v06x.keyboard;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.content.Context;
import android.content.res.Configuration;
import android.os.Bundle;
import android.text.TextPaint;
import android.util.AttributeSet;
import android.util.SparseArray;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.Space;
import android.widget.ToggleButton;

import com.svofski.v06x.MainActivity;
import com.svofski.v06x.R;

public class OnScreenKeyboard extends LinearLayout {
    public static final int CONFIG_PORTRAIT = 0;
    public static final int CONFIG_LANDSCAPE = 1;
    public static final int STATE_VISIBLE = 0;
    public static final int STATE_TRANSPARENT = 1;
    public static final int STATE_HIDDEN = 2;

    private final static String KEY_STATE = "onscreenkeyboard.state";
    private final static String KEY_BLK = "onscreenkeyboard.blk";

    private static final int BLK = -1000;

    private SparseArray<Button> buttonMap = new SparseArray<>();
    private Space[] spaces = new Space[2];
    private final int[] longKeys = new int[] {300, 311, 400, 401, 403, 404};
    private final int[] greenishKeys = new int[] {402, 50,51,52, 252};
    private final int[] mustardKeys = new int[] {150,151,152, 250,251, 352};
    private final int spaceKey = 402;
    private final float TRANSPARENT_ALPHA = 0.25f;
    private int mState = STATE_VISIBLE;
    private int mConfig = CONFIG_PORTRAIT;
    private KeyListener mKeyListener;

    private final String[] top_text = new String[] {
            "; 1 2 3 4 5 6 7 8 9 0 - /",
            "Й Ц У К Е Н Г Ш Щ З Х :",
            "УС Ф Ы В А П Р О Л Д Ж Э .",
            "СС Я Ч С М И Т Ь Б Ю , ВК",
            "РУС ТАБ ___ ПС ЗБ"};
    private final String[] bottom_text = new String[] {
            "+ ! \" # ¤ % & ' ( ) ___ = ?",
            "J C U K E N G [ ] Z H *",
            "___ F Y W A P R O L D V \\ >",
            "___ Q ^ S M I T X B @ < ___",
            "LAT ___ ___ ___ _"};
    private final String[] num_text = new String[] {
            "ВВОД БЛК СБР",
            "F1 F2 F3",
            "F4 F5 АР2",
            "↖ ↑ СТР",
            "← ↓ →"};
    private final int[][] scancodes = new int[][]{ {
            KeyEvent.KEYCODE_SEMICOLON, KeyEvent.KEYCODE_1, KeyEvent.KEYCODE_2,
            KeyEvent.KEYCODE_3, KeyEvent.KEYCODE_4, KeyEvent.KEYCODE_5, KeyEvent.KEYCODE_6,
            KeyEvent.KEYCODE_7, KeyEvent.KEYCODE_8, KeyEvent.KEYCODE_9, KeyEvent.KEYCODE_0,
            KeyEvent.KEYCODE_EQUALS, KeyEvent.KEYCODE_SLASH},

            {KeyEvent.KEYCODE_J, KeyEvent.KEYCODE_C, KeyEvent.KEYCODE_U, KeyEvent.KEYCODE_K,
             KeyEvent.KEYCODE_E, KeyEvent.KEYCODE_N, KeyEvent.KEYCODE_G, KeyEvent.KEYCODE_LEFT_BRACKET,
             KeyEvent.KEYCODE_RIGHT_BRACKET, KeyEvent.KEYCODE_Z, KeyEvent.KEYCODE_H,
             KeyEvent.KEYCODE_APOSTROPHE},

            {KeyEvent.KEYCODE_CTRL_LEFT, KeyEvent.KEYCODE_F, KeyEvent.KEYCODE_Y, KeyEvent.KEYCODE_W,
             KeyEvent.KEYCODE_A, KeyEvent.KEYCODE_P, KeyEvent.KEYCODE_R, KeyEvent.KEYCODE_O,
             KeyEvent.KEYCODE_L, KeyEvent.KEYCODE_D, KeyEvent.KEYCODE_V, KeyEvent.KEYCODE_BACKSLASH, KeyEvent.KEYCODE_PERIOD},

            {KeyEvent.KEYCODE_SHIFT_LEFT, KeyEvent.KEYCODE_Q, KeyEvent.KEYCODE_GRAVE, KeyEvent.KEYCODE_S,
             KeyEvent.KEYCODE_M, KeyEvent.KEYCODE_I, KeyEvent.KEYCODE_T, KeyEvent.KEYCODE_X,
             KeyEvent.KEYCODE_B, KeyEvent.KEYCODE_MINUS, KeyEvent.KEYCODE_COMMA, KeyEvent.KEYCODE_ENTER},

            {KeyEvent.KEYCODE_F6, KeyEvent.KEYCODE_TAB, KeyEvent.KEYCODE_SPACE,
             KeyEvent.KEYCODE_ALT_RIGHT, KeyEvent.KEYCODE_DEL}};

    private final int[][] scancodes_num = new int[][]{
            {KeyEvent.KEYCODE_F11, BLK, KeyEvent.KEYCODE_F12},
            {KeyEvent.KEYCODE_F1, KeyEvent.KEYCODE_F2, KeyEvent.KEYCODE_F3},
            {KeyEvent.KEYCODE_F4, KeyEvent.KEYCODE_F5, KeyEvent.KEYCODE_ESCAPE},
            {KeyEvent.KEYCODE_HOME, KeyEvent.KEYCODE_DPAD_UP, KeyEvent.KEYCODE_F8},
            {KeyEvent.KEYCODE_DPAD_LEFT, KeyEvent.KEYCODE_DPAD_DOWN, KeyEvent.KEYCODE_DPAD_RIGHT}};

    private final int[] scancodes_blk = {KeyEvent.KEYCODE_F11, BLK, KeyEvent.KEYCODE_F12};

    private boolean mBlkState = false;

    private boolean isBlkKey(int scancode) {
        for (int i = 0; i < scancodes_blk.length; ++i) {
            if (scancode == scancodes_blk[i]) return true;
        }
        return false;
    }

    private void updateBlkState(boolean newState) {
        mBlkState = newState;
        Button blk = buttonMap.get(51, null);
        blk.setActivated(mBlkState);
    }

    private OnTouchListener mButtonTouchListener = new OnTouchListener() {
        @Override
        public boolean onTouch(View v, MotionEvent event) {
            int scancode = (int) v.getTag();
            switch (event.getAction()) {
                case MotionEvent.ACTION_DOWN:
                    if (isBlkKey(scancode)) {
                        if (scancode == BLK) {
                            updateBlkState(mBlkState ^ true);
                        }
                        else if (!mBlkState) {
                            break;
                        }
                    }
                    MainActivity.emulator().KeyDown(scancode);
                    if (mKeyListener != null) {
                        mKeyListener.onKeyDown(scancode);
                    }
                    break;
                case MotionEvent.ACTION_UP:
                case MotionEvent.ACTION_CANCEL:
                    if (scancode != BLK) {
                        updateBlkState(false);
                    }
                    MainActivity.emulator().KeyUp(scancode);
                    if (mKeyListener != null) {
                        mKeyListener.onKeyUp(scancode);
                    }
                    break;
            }
            return false;
        }
    };

    public OnScreenKeyboard(Context context, AttributeSet attrs) {
        super(context, attrs);
        LayoutInflater inflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        inflater.inflate(R.layout.onscreenkeyboard, this, true);

        createKeys();
    }

    private int keys_for_row(int row) {
        if (row == 1 || row == 3) {
            return 12;
        }
        if (row == 4)
            return 5;
        return 13;
    }

    private void createKeys()
    {
        LinearLayout main = findViewById(R.id.oskeyboard_main);
        LinearLayout num = findViewById(R.id.oskeyboard_num);
        for (int row = 0; row < 5; ++row) {
            final LinearLayout main_row = (LinearLayout) main.getChildAt(row);
            if (row == 1) {
                Space sp = new Space(getContext());
                main_row.addView(sp);
                spaces[0] = sp;
            }
            String[] texts_top = top_text[row].split(" ");
            String[] texts_bottom = bottom_text[row].split(" ");
            String[] texts_num = num_text[row].split(" ");

            for (int col = 0; col < keys_for_row(row); ++col) {
                Button b = makeKey(texts_top[col], texts_bottom[col], scancodes[row][col]);
                main_row.addView(b);
                buttonMap.put(row * 100 + col, b);
            }
            if (row == 1) {
                Space sp = new Space(getContext());
                main_row.addView(sp);
                spaces[1] = sp;
            }

            final LinearLayout num_row = (LinearLayout) num.getChildAt(row);
            for (int col = 0; col < 3; ++col) {
                Button b = makeKey(texts_num[col], "", scancodes_num[row][col]);
                num_row.addView(b);
                buttonMap.put(row * 100 + 50 + col, b);
            }
        }
        for (int i : longKeys) {
            Button b = buttonMap.get(i, null);
            if (b != null) {
                b.setBackgroundResource(R.drawable.oskkey_brown);
                b.setTextColor(getResources().getColor(R.color.oskkey_brown_textcolor));
            }
        }
        for (int i : greenishKeys) {
            Button b = buttonMap.get(i, null);
            if (b != null) {
                b.setBackgroundResource(R.drawable.oskkey_greenish);
            }
        }
        for (int i : mustardKeys) {
            Button b = buttonMap.get(i, null);
            if (b != null) {
                b.setBackgroundResource(R.drawable.oskkey_mustard);
            }
        }
     }

    private void setViewSize(View v, int w, int h) {
        if (v != null) {
            ViewGroup.LayoutParams l = v.getLayoutParams();
            l.width = w;
            l.height = h;
            v.setLayoutParams(l);
            v.setMinimumHeight(h);
        }
    }

    private int rearrangeLayout(int kw, int kh) {
        LinearLayout top = findViewById(R.id.oskeyboard_top);
        int normal_size = kw / 17;
        LinearLayout main = findViewById(R.id.oskeyboard_main);
        LinearLayout num = findViewById(R.id.oskeyboard_num);
        switch (getContext().getResources().getConfiguration().orientation) {
            case Configuration.ORIENTATION_PORTRAIT:
                mConfig = CONFIG_PORTRAIT;
                top.setOrientation(VERTICAL);
                main.bringToFront();
                /* try to resize to fit width */
                normal_size = kw / 13;
                /* but if the height is too tall, resize to fit height */
                int new_h = normal_size * 10;
                if (new_h > kh) {
                    normal_size = kh / 10;
                }
                /* add padding to center the resulting view */
                LinearLayout.LayoutParams lp = (LinearLayout.LayoutParams)main.getLayoutParams();
                lp.leftMargin = (kw - normal_size*13)/2;
                main.setLayoutParams(lp);
                lp = (LinearLayout.LayoutParams)num.getLayoutParams();
                lp.leftMargin = (kw - normal_size*13)/2;
                num.setLayoutParams(lp);
                break;
            case Configuration.ORIENTATION_LANDSCAPE:
            case Configuration.ORIENTATION_UNDEFINED:
                mConfig = CONFIG_LANDSCAPE;
                top.setOrientation(HORIZONTAL);
                num.bringToFront();
                break;
        }

        return normal_size;
    }

    private void updateKeyDimensions(int kw, int kh) {
        int normal_size = rearrangeLayout(kw, kh);

        int textSize;
        {
            Button probe = buttonMap.get(50, null);
            setViewSize(probe, normal_size, normal_size);
            textSize = measureTextUsing(probe, normal_size);
        }

        for (int i = 0; i < spaces.length; ++i) {
            setViewSize(spaces[0], normal_size / 2, normal_size);
        }
        setViewSize(spaces[spaces.length-1], (int)(0.5 + normal_size*1.5), normal_size);

        for (int r = 0; r < 5; ++r) {
            for (int c = 0; c < 13; ++c) {
                {
                    Button n = buttonMap.get(r * 100 + 50 + c, null);
                    if (n != null) n.setTextSize(textSize);
                    setViewSize(n, normal_size, normal_size);
                }
                {
                    Button b = buttonMap.get(r * 100 + c, null);
                    if (b != null) b.setTextSize(textSize);
                    setViewSize(b, normal_size, normal_size);
                }
            }
        }

        for (int i : longKeys) {
            Button l = buttonMap.get(i, null);
            setViewSize(l, (int) (0.5 + normal_size * 1.5), normal_size);
        }
        {
            Button s = buttonMap.get(spaceKey, null);
            setViewSize(s, 7 * normal_size, normal_size);
        }
    }

    private int measureTextUsing(Button probe, int outerWidth) {
        TextPaint paint = probe.getPaint();
        float innerWidth = outerWidth - probe.getPaddingLeft() - probe.getPaddingRight();

        int textSize = 8;
        for(; textSize < 22;) {
            int nextSize = textSize + 1;
            probe.setTextSize(nextSize);
            float width = paint.measureText("ВВОД,");
            if (width > innerWidth) {
                break;
            }
            textSize = nextSize;
        }

        return textSize;
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        super.onLayout(changed, l, t, r, b);
        //Log.d("osk", "onLayout: l=" + l + " t=" + t + " r=" + r + " b=" + b);
        final int w = r - l;
        final int h = b - t;
        if (changed && w > 0 && h > 0) {
            updateKeyDimensions(w, h);
        }
    }

    private Button makeKey(String top, String bottom, int scancode) {
        Button b = new Button(getContext());
        String s1 = top.length() == 0 || top.equals("___") ? "" : top;
        String s2 = bottom.length() == 0 || bottom.equals("___") ? "" : bottom;
        if (s1.length() > 0 && s2.length() > 0) {
            b.setText(s1 + "\n" + s2);
        }
        else if (s1.length() > 0) {
            b.setText(s1);
        }
        else if (s2.length() > 0) {
            b.setText(s2);
        }
        else {
            b.setText("");
        }
        b.setBackgroundResource(R.drawable.oskkey);
        b.setTag(scancode);
        b.setOnTouchListener(mButtonTouchListener);

        return b;
    }

    public void makeTranslucent(float to) {
        animate().alpha(to);
    }

    public void cycleState() {
        switch (mState) {
            case STATE_VISIBLE:
                makeTranslucent(TRANSPARENT_ALPHA);
                mState = STATE_TRANSPARENT;
                break;
            case STATE_TRANSPARENT:
                makeTranslucent(1.0f);
                changeVisibility(GONE, new Runnable() {
                    @Override
                    public void run() {
                    }
                });
                mState = STATE_HIDDEN;
                break;
            case STATE_HIDDEN:
                changeVisibility(VISIBLE, new Runnable() {
                            @Override
                            public void run() {
                            }
                        });
                mState = STATE_VISIBLE;
                break;
        }
    }

    public void changeVisibility(int newVisibility, final Runnable runnik) {
        if (newVisibility == GONE) {
            animate().translationY(getHeight()).setListener(new AnimatorListenerAdapter() {
                @Override
                public void onAnimationEnd(Animator animation) {
                    setVisibility(GONE);
                    runnik.run();
                }
            });
        }
        else {
            setVisibility(VISIBLE);
            animate().translationY(0).setListener(new AnimatorListenerAdapter() {
                @Override
                public void onAnimationEnd(Animator animation) {
                    setVisibility(VISIBLE);
                    runnik.run();
                }
            });
        }
    }

   public void saveInstanceState(Bundle outState) {
        outState.putInt(KEY_STATE, mState);
        outState.putBoolean(KEY_BLK, mBlkState);
    }

    public void restoreInstanceState(Bundle savedInstanceState) {
        if (savedInstanceState != null) {
            boolean blk = savedInstanceState.getBoolean(KEY_BLK, false);
            updateBlkState(blk);
            mState = savedInstanceState.getInt(KEY_STATE, STATE_HIDDEN);

            switch (mState) {
                case STATE_HIDDEN:
                    setVisibility(View.GONE);
                    setAlpha(1f);
                    break;
                case STATE_TRANSPARENT:
                    setVisibility(View.VISIBLE);
                    setAlpha(TRANSPARENT_ALPHA);
                    break;
                case STATE_VISIBLE:
                    setVisibility(View.VISIBLE);
                    setAlpha(1f);
                    break;
            }
        }
    }

    public void setOnKeyListener(KeyListener kl) {
        mKeyListener = kl;
    }

    public int getConfig() {
        return mConfig;
    }

    public int getState() {
        return mState;
    }

    public interface KeyListener {
        void onKeyDown(int keycode);
        void onKeyUp(int keycode);
    }
}
