package com.svofski.v06x.uiextras.loaders;

public class RomListItemDummy extends RomListItem {
    public RomListItemDummy(String id, String contents, String details) {
        super(id, contents, details);
    }

    @Override
    public byte[] load() {
        return new byte[0];
    }
}

