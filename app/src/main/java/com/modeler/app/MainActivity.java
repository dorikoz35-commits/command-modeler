package com.modeler.app;

import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.provider.Settings;
import android.util.Log;

import org.libsdl.app.SDLActivity;

import java.io.File;

/**
 * Кастомная Activity поверх SDL.
 * При запуске сразу просит доступ «Управление всеми файлами»
 * и создаёт папку 3Dobj в общем хранилище.
 */
public class MainActivity extends SDLActivity {

    private static final String TAG = "3DCommands";
    private static final String FOLDER_NAME = "3Dobj";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestAllFilesAccessIfNeeded();
        ensureModelFolder();
    }

    @Override
    protected void onResume() {
        super.onResume();
        // После возврата из настроек снова проверяем и создаём папку
        if (hasAllFilesAccess()) {
            ensureModelFolder();
        }
    }

    private boolean hasAllFilesAccess() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            return Environment.isExternalStorageManager();
        }
        return true; // на старых Android достаточно обычных permissions
    }

    private void requestAllFilesAccessIfNeeded() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            if (!Environment.isExternalStorageManager()) {
                try {
                    Intent intent = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION);
                    intent.setData(Uri.parse("package:" + getPackageName()));
                    startActivity(intent);
                    Log.i(TAG, "Запрос доступа ко всем файлам");
                } catch (Exception e) {
                    // На некоторых прошивках ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION может не работать
                    try {
                        Intent intent = new Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION);
                        startActivity(intent);
                    } catch (Exception e2) {
                        Log.e(TAG, "Не удалось открыть настройки доступа к файлам", e2);
                    }
                }
            }
        }
    }

    /** Создаёт папку /storage/emulated/0/3Dobj если её ещё нет */
    private void ensureModelFolder() {
        try {
            File dir = new File(Environment.getExternalStorageDirectory(), FOLDER_NAME);
            if (!dir.exists()) {
                boolean ok = dir.mkdirs();
                Log.i(TAG, "Создание папки 3Dobj: " + (ok ? "успешно" : "ошибка") + " → " + dir.getAbsolutePath());
            }
        } catch (Exception e) {
            Log.e(TAG, "Не удалось создать папку 3Dobj", e);
        }
    }

    /** Путь к папке 3Dobj (для использования из Java/Saver) */
    public static File getModelsDir() {
        return new File(Environment.getExternalStorageDirectory(), FOLDER_NAME);
    }
}
