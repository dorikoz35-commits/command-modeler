package com.modeler.app;

import android.content.Context;
import android.os.Build;
import android.os.Environment;
import android.util.Log;

import java.io.File;
import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.nio.charset.StandardCharsets;

/**
 * Сохраняет модели в общее хранилище:
 * /storage/emulated/0/3Dobj/
 *
 * Требует разрешения MANAGE_EXTERNAL_STORAGE (запрашивается при запуске).
 */
public class Saver {

    private static final String TAG = "3DCommands.Saver";
    private static final String FOLDER_NAME = "3Dobj";

    /**
     * Сохраняет текстовый файл (скрипт / OBJ / MTL) в папку 3Dobj.
     * Возвращает "ok" или "err:..."
     */
    public static String save(Context c, String name, String data) {
        try {
            // Убеждаемся, что папка существует
            File dir = new File(Environment.getExternalStorageDirectory(), FOLDER_NAME);
            if (!dir.exists()) {
                if (!dir.mkdirs()) {
                    return "err:cannot_create_3Dobj";
                }
            }

            // Защита от path traversal
            String safeName = new File(name).getName();
            if (safeName.isEmpty()) safeName = "model.txt";

            File outFile = new File(dir, safeName);

            try (FileOutputStream fos = new FileOutputStream(outFile);
                 OutputStreamWriter writer = new OutputStreamWriter(fos, StandardCharsets.UTF_8)) {
                writer.write(data);
                writer.flush();
            }

            Log.i(TAG, "Сохранено: " + outFile.getAbsolutePath());
            return "ok";
        } catch (SecurityException se) {
            Log.e(TAG, "Нет доступа к файлам", se);
            return "err:no_permission — дайте доступ «Управление всеми файлами»";
        } catch (Exception e) {
            Log.e(TAG, "Ошибка сохранения", e);
            return "err:" + e.getMessage();
        }
    }
}
