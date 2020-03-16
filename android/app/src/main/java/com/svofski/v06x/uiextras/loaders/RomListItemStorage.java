package com.svofski.v06x.uiextras.loaders;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileReader;

public class RomListItemStorage extends RomListItem {
    public RomListItemStorage(String id, String content, String details) {
        super(id, content, details);
    }

    @Override
    public byte[] load() {
        File f = new File(id);
        byte[] result;

        try {
            FileInputStream fis = new FileInputStream(f);
            result = new byte[fis.available()];
            fis.read(result);
        } catch (Exception e) {
            result = null;
        }
        return result;
    }
}
