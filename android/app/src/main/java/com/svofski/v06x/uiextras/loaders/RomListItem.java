package com.svofski.v06x.uiextras.loaders;

import com.svofski.v06x.cpp.LoadKind;

public abstract class RomListItem {
    public static final int SELECTOR_NONE = -1;
    public static final int SELECTOR_A = 0;
    public static final int SELECTOR_B = 1;
    public static final int SELECTOR_E = 2;

    public final String id;
    public final String content;
    public final String details;

    public RomListItem(String id, String content, String details) {
        this.id = id;
        this.content = content;
        this.details = details;
    }

    public int getKind() {
        String lower = id.toLowerCase();
        if (lower.endsWith(".fdd")) {
            return LoadKind.FDD;
        }
        else if (lower.endsWith(".rom") || lower.matches(".*\\.r[0-9]m$")) {
            return LoadKind.ROM;
        }
        else if (lower.endsWith(".com")) {
            return LoadKind.COM;
        }
        else if (lower.endsWith(".edd")) {
            return LoadKind.EDD;
        }
        return LoadKind.UNKIND;
    }

    public int getOrg() {
        switch (getKind()) {
            case LoadKind.EDD:  return 0;
            case LoadKind.FDD:  return 0;
            case LoadKind.COM:  return 256;
            case LoadKind.ROM:
                char c = id.toLowerCase().charAt(id.length() - 2);
                if (c == 'o')   return 256;
                return 256 * ((int)c - (int)'0');
        }
        return 256;
    }

    abstract public byte[] load();

    @Override
    public String toString() {
        return content;
    }
}
