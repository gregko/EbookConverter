package com.hyperionics.ebookconverter;

import android.Manifest;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;

import java.io.*;

public class MainActivity extends AppCompatActivity {
    protected static final int REQUEST_CODE_ASK_PERMISSIONS = 100;
    protected static final int CONVERT_EBOOK_REQUEST = 200;
    private String lastPath = "/sdcard";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        FloatingActionButton fab = (FloatingActionButton) findViewById(R.id.fab);
        fab.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Intent intent = new Intent(getBaseContext(), GenericFileDialog.class);
                intent.putExtra(GenericFileDialog.START_PATH, lastPath);
                intent.putExtra(GenericFileDialog.SELECTION_MODE, GenericFileDialog.MODE_OPEN);
                intent.putExtra(GenericFileDialog.SET_TITLE_TEXT, getString(R.string.sel_eb_file));
                intent.putExtra(GenericFileDialog.FORMAT_FILTER, new String[] {
                        "mobi","prc","azw","azw3","kf8","fb2","fb2.zip"
                });
                startActivityForResult(intent, CONVERT_EBOOK_REQUEST);

//                Snackbar.make(view, "Replace with your own action", Snackbar.LENGTH_LONG)
//                        .setAction("Action", null).show();
            }
        });

        checkPermissions();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_about) {
            Intent intent = new Intent(this, InfoActivity.class);
            startActivity(intent);
            return true;
        }

        return super.onOptionsItemSelected(item);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (resultCode == RESULT_CANCELED || data == null)
            return;

        if (requestCode == CONVERT_EBOOK_REQUEST) {
            String fileName = data.getStringExtra(GenericFileDialog.RESULT_PATH);
            if (fileName != null) {
                Lt.d("Selected: " + fileName);
                lastPath = new File(fileName).getParent();
                String outName = fileName + ".epub";
                int ret;
                if (isMobi(fileName)) {
                    ret = ConvLib.mobiToEpubNative(fileName, outName);
                }
                else if (isFb2(fileName)) {
                    String cssDir = getFilesDir().getAbsolutePath() + "/css";
                    copyAssetDir("css", cssDir);
                    ret = ConvLib.fb2ToEpubNative(fileName, outName, cssDir, "");
                }
                else {
                    ret = -9999;
                }
                String msg;
                if (ret == 0) {
                    msg = getString(R.string.conv_success) + outName;
                } else if (ret > -9999) {
                    // TODO: need meaningful error messages from the conversion libraries...
                    msg = getString(R.string.conv_error);
                } else {
                    msg = getString(R.string.unknown_file);
                }
                Snackbar.make(findViewById(R.id.fab), msg, Snackbar.LENGTH_LONG)
                        .setAction("Action", null).show();
            }
        }
    }

    private boolean isMobi(String fileName) {
        int n = fileName.lastIndexOf('.');
        if (n < 0)
            return false;
        String ext = fileName.substring(n).toLowerCase();
        return ".mobi".equals(ext) || ".prc".equals(ext) ||
                ".azw".equals(ext) || ".azw3".equals(ext) || ".kf8".equals(ext);
    }

    private boolean isFb2(String fileName) {
        int n = fileName.lastIndexOf('.');
        if (n < 0)
            return false;
        String ext = fileName.substring(n).toLowerCase();
        if (".fb2".equals(ext))
            return true;
        if (".zip".equals(ext)) {
            String name2 = fileName.substring(0, n);
            n = name2.lastIndexOf('.');
            if (n < 0)
                return false;
            ext = name2.substring(n).toLowerCase();
            return ".fb2".equals(ext);
        }
        return false;
    }

    private int copyAssetDir(String assetDirName, String targetDir) {
        AssetManager assetManager = getAssets();
        String[] list;
        try {
            list = assetManager.list(assetDirName);
            if (list.length == 0)
                return 0;
        } catch(IOException e1) {
            return 0;
        }
        int numFilesCopied = 0;
        for (String assetName : list) {
            String nameWithPath = assetDirName + "/" + assetName;
            InputStream in;
            OutputStream out;
            try {
                in = assetManager.open(nameWithPath);
                // ... proceed to copy it
                String targetName = targetDir + "/" + assetName;
                new File(targetName).getParentFile().mkdirs(); // just in case, create the directory.
                out = new FileOutputStream(targetName);
                copyFile(in, out);
                in.close();
                out.flush();
                out.close();
                numFilesCopied++;
            } catch (IOException e2) {
                // Maybe assetName a directory?
                numFilesCopied += copyAssetDir(nameWithPath, targetDir + "/" + assetName);
            }
        }
        return numFilesCopied;
    }

    private static void copyFile(InputStream in, OutputStream out) throws IOException {
        byte[] buffer = new byte[1024];
        int read;
        while ((read = in.read(buffer)) != -1)
        {
            out.write(buffer, 0, read);
        }
        out.flush();
    }

    private boolean checkPermissions() {
        int prm = ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE);
        if (prm == PackageManager.PERMISSION_GRANTED)
            return true;
//        if (!ActivityCompat.shouldShowRequestPermissionRationale(this,
//                Manifest.permission.WRITE_EXTERNAL_STORAGE))
//        {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setMessage(R.string.storage_perm_prompt);
        builder.setNeutralButton(R.string.next_arrow, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(final DialogInterface dialog, int which) {
                ActivityCompat.requestPermissions(MainActivity.this,
                        new String[] {Manifest.permission.WRITE_EXTERNAL_STORAGE},
                        REQUEST_CODE_ASK_PERMISSIONS);
                dialog.dismiss();
            }
        });
        final Dialog dlg = builder.create();
        dlg.setCancelable(false);
        dlg.show();
//        } else {
//            onRequestPermissionsResult(REQUEST_CODE_ASK_PERMISSIONS, new String[] {Manifest.permission.WRITE_EXTERNAL_STORAGE},
//                new int[] {PackageManager.PERMISSION_DENIED});
//        }
        return false;
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions,
                                           @NonNull int[] grantResults)
    {
        if (requestCode == REQUEST_CODE_ASK_PERMISSIONS) {
            if (grantResults[0] != PackageManager.PERMISSION_GRANTED) {
                AlertDialog.Builder builder = new AlertDialog.Builder(this);
                builder.setMessage(R.string.no_perm_exit);
                builder.setNeutralButton(R.string.close_app, null);
                Dialog dlg = builder.create();
                dlg.setOnDismissListener(new DialogInterface.OnDismissListener() {
                    @Override
                    public void onDismiss(DialogInterface dialog) {
                        finish();
                    }
                });
                dlg.show();
            }
        }
    }

}
