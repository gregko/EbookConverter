package com.hyperionics.ebookconverter;

import android.annotation.TargetApi;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ListActivity;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.res.ColorStateList;
import android.content.res.Resources;
import android.content.res.XmlResourceParser;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewConfiguration;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;

import java.io.File;
import java.io.IOException;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;

public class GenericFileDialog extends ListActivity implements AdapterView.OnItemLongClickListener {
    public static final int MODE_CREATE = 0;
    public static final int MODE_OPEN = 1;

	private static final String ROOT = "/";
	public static final String START_PATH = "START_PATH";
	public static final String FORMAT_FILTER = "FORMAT_FILTER";
	public static final String RESULT_PATH = "RESULT_PATH";
	public static final String SELECTION_MODE = "SELECTION_MODE";

    public static final String MUST_SELECT_DIR = "MUST_SELECT_DIR";
    public static final String MUST_CREATE_NEW = "MUST_CREATE_NEW";
    public static final String SUGGESTED_NAME = "SUGGESTED_NAME";
    public static final String SET_TITLE_TEXT = "SET_TITLE_TEXT";

    protected static final int REQUEST_MOVE_FILES = 1;

    public static final String INVALID_FNAME_CHARS =
            "\\/\"\0\n\r\t\f`'?*<>|:\u2018\u2019";
    public static final String INVALID_FNAME_CHARS_REGEX =
            "['\\\\'|'/'|'\"'|\\n'|'\\r'|'\\t'|'\\000'|'\\f'|'`'|'\\''|'\\?'|'\\*'|'<'|'>'|'\\|'|':'|'\\u2018'|'\\u2019']";
    public static final String INVALID_FNAME_REGEX = ".*" + INVALID_FNAME_CHARS_REGEX + ".*";

	private TextView myPath;
	private EditText mFileName;
    private ArrayList<FileRowData> fileList;
    FileRowAdapter adapter = null;

	private Button selectButton;

	private LinearLayout layoutSelect;
	private LinearLayout layoutCreate;
	private InputMethodManager inputManager;
	private String currentPath = ROOT;
    private String startPath;
    private ArrayList<String> pathStack = new ArrayList<String>();

	private int selectionMode = MODE_CREATE;

	private String[] formatFilter = null;

	private boolean mustSelectDir = false;
    private boolean mustCreateNew = false;
    private int mySelectedItem = -1;

	private File selectedFile;
    private File selectedDir = null;

	/**
	 * Called when the activity is first created. Configura todos os parametros
	 * de entrada e das VIEWS..
	 */
	@SuppressWarnings("AndroidLintNewApi")
    @Override
	public void onCreate(Bundle savedInstanceState) {
        setTheme(android.R.style.Theme_Holo_Light_DarkActionBar);
		super.onCreate(savedInstanceState);
		setResult(RESULT_CANCELED, getIntent());
        if (Build.VERSION.SDK_INT > 10) {
            // Always show vertical ellipsis overflow menu button.
            try {
                ViewConfiguration config = ViewConfiguration.get(this);
                Field menuKeyField = ViewConfiguration.class.getDeclaredField("sHasPermanentMenuKey");
                if(menuKeyField != null) {
                    menuKeyField.setAccessible(true);
                    menuKeyField.setBoolean(config, false);
                }
            } catch (Exception ex) {}
        }
		setContentView(R.layout.generic_file_dialog);

        String newTitle = getIntent().getStringExtra(SET_TITLE_TEXT);
        if (newTitle != null) {
            setTitle(newTitle);
            try {
                if (getActionBar() != null) {
                    final int titleId = Resources.getSystem().getIdentifier("action_bar_title", "id", "android");
                    TextView title = (TextView) getWindow().findViewById(titleId);
                    title.setTextSize(14);
                    title.setSingleLine(false);
                    getActionBar().setDisplayShowHomeEnabled(false);
                }
            } catch (NoSuchMethodError e) {
                Lt.d("No such method exception: " + e);
            } catch (Exception e) {
                Lt.d("Failed to obtain action bar title reference");
            }
        }

		myPath = (TextView) findViewById(R.id.path);
		mFileName = (EditText) findViewById(R.id.fdEditTextFile);

		inputManager = (InputMethodManager) getSystemService(INPUT_METHOD_SERVICE);
        selectionMode = getIntent().getIntExtra(SELECTION_MODE, MODE_CREATE);
        formatFilter = getIntent().getStringArrayExtra(FORMAT_FILTER);
        mustSelectDir = getIntent().getBooleanExtra(MUST_SELECT_DIR, false);
        mustCreateNew = getIntent().getBooleanExtra(MUST_CREATE_NEW, false);
        String suggestedName = getIntent().getStringExtra(SUGGESTED_NAME);


        selectButton = (Button) findViewById(R.id.fdButtonSelect);
        selectButton.setEnabled(mustSelectDir);
		selectButton.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
                if (selectedFile != null || mustSelectDir) {
                    if (selectedFile == null)
                        selectedFile = new File(currentPath);
                    if (mustSelectDir && selectionMode == MODE_CREATE && !selectedFile.canWrite()) {
                        Lt.alert(GenericFileDialog.this, R.string.hts_not_writeable);
                        return;
                    }
					getIntent().putExtra(RESULT_PATH, selectedFile.getPath());
					setResult(RESULT_OK, getIntent());
					finish();
				}
			}
		});

		final Button newButton = (Button) findViewById(R.id.fdButtonNew);
		newButton.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
                if (selectionMode >= MODE_OPEN || mustSelectDir) {
                    finish();
                } else {
                    setCreateVisible(v);
                    mFileName.setText("");
                    mFileName.requestFocus();
                }
			}
		});

		if (selectionMode >= MODE_OPEN || mustSelectDir) {
			newButton.setText(R.string.hts_cancel);
		}

		layoutSelect = (LinearLayout) findViewById(R.id.fdSelectButtons);
        layoutCreate = (LinearLayout) findViewById(R.id.fdLinearLayoutCreate);
        if (mustCreateNew && suggestedName != null) {
            mFileName.setText(suggestedName);
        }
        layoutSelect.setVisibility(mustCreateNew ? View.GONE : View.VISIBLE);
		layoutCreate.setVisibility(mustCreateNew ? View.VISIBLE : View.GONE);

		final Button cancelButton = (Button) findViewById(R.id.fdButtonCancel);
		cancelButton.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
                if (mustCreateNew)
                    finish();
                else
				    setSelectVisible();
			}

		});
		final Button createButton = (Button) findViewById(R.id.fdButtonCreate);
		createButton.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				if (mFileName.getText().length() > 0) {
					getIntent().putExtra(RESULT_PATH, currentPath + "/" + mFileName.getText());
					setResult(RESULT_OK, getIntent());
					finish();
				}
			}
		});
        if (mustCreateNew) {
            mFileName.addTextChangedListener(new TextWatcher() {
                public void afterTextChanged(Editable s) {}
                public void beforeTextChanged(CharSequence s, int start, int count, int after) {}
                public void onTextChanged(CharSequence s, int start, int before, int count) {
                    String dirName = mFileName.getText().toString().trim();
                    File f = new File(currentPath + "/" + dirName);
                    boolean b = dirName.length() > 0;
                    if (b && dirName.matches(INVALID_FNAME_REGEX))
                        b = false;
                    if (b && (f.isDirectory() || f.exists()))
                        b = false;
                    createButton.setEnabled(b);
                }
            });
        }

        getListView().setLongClickable(true);
        getListView().setOnItemLongClickListener(this);

        startPath = getIntent().getStringExtra(START_PATH);
		startPath = startPath != null ? startPath : ROOT;
		if (mustSelectDir) {
			File file = new File(startPath);
			selectedFile = file;
			selectButton.setEnabled(true);
            selectButton.setText(R.string.hts_select);
            mySelectedItem = -1;
            if (Build.VERSION.SDK_INT > 10)
                invalidateOptionsMenu();
		}
		getDir(startPath);
        if (mustCreateNew)
            layoutCreate.findViewById(R.id.fdEditTextFile).requestFocus();
	}

    @Override protected void onStart() {
        super.onStart();
        //EasyTracker.getInstance(this).activityStart(this);
    }

    @Override protected void onStop() {
        super.onStop();
        //EasyTracker.getInstance(this).activityStop(this);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.fd_menu, menu);
        if (Build.VERSION.SDK_INT > 10) {
            onPrepareOptionsMenu(menu);
        }
        return true;
    }

    @Override
    public boolean onPrepareOptionsMenu (Menu menu) {
        if (menu.size() > 2) {
            if (selectedDir != null && selectedDir.equals(currentPath))
                selectedDir = null;
            boolean b = selectedFile != null || selectedDir != null;
            if (b && mySelectedItem == 0)
                b = false; // do not delete/rename ../ directory
            menu.findItem(R.id.rename).setEnabled(b);
            menu.findItem(R.id.delete).setEnabled(b);
            super.onPrepareOptionsMenu(menu);
        }
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle item selection
        int id = item.getItemId();
        if (id == R.id.create_dir) {
            createRenamePrompt(true);
            return true;
        }
        else if (id == R.id.rename) {
            createRenamePrompt(false);
            return true;
        }
        else if (id == R.id.delete) {
            deleteFileWithPrompt(selectedDir != null ? selectedDir : selectedFile);
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        switch (requestCode) {
            case REQUEST_MOVE_FILES:
                if (data != null && resultCode == RESULT_OK) {
                    File targetDir = new File(data.getStringExtra(GenericFileDialog.RESULT_PATH));
                    if (targetDir.canRead() && targetDir.canWrite()) {
                        String dir = targetDir.getAbsolutePath() + "/";
                        for (FileRowData frd : fileList) {
                            if (frd.selected) {
                                File f = new File(dir + frd.file.getName());
                                frd.file.renameTo(f);
                                frd.file = f;
                            }
                        }
                        getDir(targetDir.getAbsolutePath());
                    }
                } else {
                    getDir(currentPath);
                }
                break;
        }
    }

    @TargetApi(Build.VERSION_CODES.FROYO)
    void createRenamePrompt(final boolean createDir) {
        if (!dirWritable(currentPath)) {
            Lt.alert(GenericFileDialog.this, R.string.hts_no_write_perm);
            return;
        }
        final View textEntryView = LayoutInflater.from(this).inflate(R.layout.alert_dialog_text_entry, null);
        final File file = selectedDir != null ? selectedDir : selectedFile;
        if (!createDir) {
            ((TextView)textEntryView.findViewById(R.id.prompt)).setText(R.string.hts_rename_name);
            String fName = file.getName();
            if (fName.endsWith(".avar"))
                fName = fName.substring(0, fName.lastIndexOf(".avar"));
            ((EditText)textEntryView.findViewById(R.id.edit)).setText(fName);
        }
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle(createDir ? R.string.hts_create_dir : R.string.hts_rename);
        builder.setView(textEntryView);
        final EditText edit = (EditText)textEntryView.findViewById(R.id.edit);
        builder.setPositiveButton(createDir ? R.string.hts_create : R.string.hts_rename, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int whichButton) {
                String fileName = edit.getText().toString().trim();
                if (createDir) {
                    File file = new File(currentPath + "/" + fileName);
                    if (!file.mkdirs()) {
                        Lt.alert(GenericFileDialog.this, "Could not create directory: " + file + "\nWe have no write permission here.");
                    }
                } else { // rename selected file or dir
                    File oldFile = new File(file.getAbsolutePath());
                    String oldName = oldFile.getName(); // gets name with extension, e.g. abc.html
                    String oldExt = "", newExt = "";
                    int n = oldName.lastIndexOf('.');
                    if (n > -1)
                        oldExt = oldName.substring(n);
                    n = fileName.lastIndexOf('.');
                    if (n > -1)
                        newExt = fileName.substring(n);
                    if (newExt.equals(""))
                        fileName += oldExt;
                    String newName = oldFile.getParent() + "/" + fileName;
                    oldFile.renameTo(new File(newName));

                }
                getDir(currentPath);
            }
        });
        builder.setNegativeButton(android.R.string.cancel, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int whichButton) {
                selectedDir = null;
                if (Build.VERSION.SDK_INT > 10)
                    invalidateOptionsMenu();
            }
        });
        final AlertDialog dlg = builder.create();
        edit.addTextChangedListener(new TextWatcher() {
            public void afterTextChanged(Editable s) {}
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {}
            public void onTextChanged(CharSequence s, int start, int before, int count) {
                String fileName = edit.getText().toString().trim();
                File f = new File(currentPath + "/" + fileName);
                boolean b = fileName.length() > 0;
                if (b && fileName.matches(INVALID_FNAME_REGEX))
                    b = false;
                if (b && (f.isDirectory() || f.exists()))
                    b = false;
                dlg.getButton(AlertDialog.BUTTON_POSITIVE).setEnabled(b);
            }
        });
        dlg.setOnShowListener(new Dialog.OnShowListener() {
            @Override
            public void onShow(DialogInterface dialog) {
                ((AlertDialog)dialog).getButton(AlertDialog.BUTTON_POSITIVE).setEnabled(false);
            }
        });
        dlg.show();
    }

    private void getDir(final String dirPath) {
        int popTo = -1;
        for (int i = 0; i < pathStack.size(); i++)
            if (pathStack.get(i).equals(dirPath)) {
                popTo = i;
                break;
            }
        if (popTo < 0)
            pathStack.add(0, dirPath);
        else for (int i = 0; i < popTo; i++)
            pathStack.remove(0);


        myPath.setText(getText(R.string.hts_folder) + ": " + currentPath);
        bgTask(this, true, null, null, new OpCallback() {
            @Override
            public boolean runInBg() {
                getDirImpl(dirPath);
                return true;
            }

            @Override
            public void onFinished(boolean result) {
                // set it again, currentPath could be changed
                if (adapter == null) {
                    adapter = new FileRowAdapter(fileList);
                    setListAdapter(adapter);
                } else {
                    adapter.clear(); // adapter.addAll(fileList); - no such method on GB
                    for (FileRowData frd : fileList)
                        adapter.add(frd);
                }
                myPath.setText(getText(R.string.hts_folder) + ": " + currentPath);
                selectedFile = null;
                selectedDir = null;
                mySelectedItem = -1;
                if (Build.VERSION.SDK_INT > 10)
                    invalidateOptionsMenu();
            }
        });
	}

	private void getDirImpl(final String dirPath) {

		currentPath = dirPath;

		File f = new File(currentPath);
		File[] files = f.listFiles();
		if (files == null) {
			currentPath = ROOT;
			f = new File(currentPath);
			files = f.listFiles();
		}
		//myPath.setText(getText(R.string.folder) + ": " + currentPath);

        fileList=new ArrayList<FileRowData>();

        for (File file : files) {
            if (file.getName().startsWith("."))
                continue;
            if (file.isDirectory()) {
                String dirName = file.getName();
                fileList.add(new FileRowData(file, dirName, R.drawable.folder));
            } else {
                String fileName = file.getName();
                String fileNameLwr = fileName.toLowerCase();
                if (formatFilter != null) {
                    boolean contains = false;
                    for (int i = 0; i < formatFilter.length; i++) {
                        final String formatLwr = formatFilter[i].toLowerCase();
                        if (fileNameLwr.endsWith(formatLwr)) {
                            contains = true;
                            break;
                        }
                    }
                    if (contains) {
                        fileList.add(new FileRowData(file, fileName, R.drawable.file));
                    }
                } else {
                    fileList.add(new FileRowData(file, fileName, R.drawable.file));
                }
            }
        }
        Collections.sort(fileList, new FrdComparator());
        if (!currentPath.equals(ROOT)) {
            fileList.add(0, new FileRowData(f.getParentFile(), "../", R.drawable.folder));
        }
	}

	@Override
	protected void onListItemClick(ListView l, View v, int position, long id) {

		File file = fileList.get(position).file;

		setSelectVisible();
//        l.requestFocusFromTouch();
//        l.setSelection(position);
        selectedDir = null;
		if (file.isDirectory()) {
			selectButton.setEnabled(mustSelectDir);
			if (file.canRead()) {
				getDir(fileList.get(position).file.getAbsolutePath());
                if (!file.equals(currentPath))
                    selectedDir = file;
				if (mustSelectDir) {
					selectedFile = file;
					v.setSelected(true);
					//selectButton.setEnabled(!file.getAbsolutePath().equals(startPath));
                    mySelectedItem = position;
                    if (Build.VERSION.SDK_INT > 10)
                        invalidateOptionsMenu();
                }
			} else {
				new AlertDialog.Builder(this)
						.setTitle("[" + file.getName() + "] " + getText(R.string.hts_cant_read_folder))
						.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {

							@Override
							public void onClick(DialogInterface dialog, int which) {

							}
						}).show();
			}
		} else if (!mustCreateNew && !mustSelectDir) {
			selectedFile = file;
			selectButton.setEnabled(true);
            v.setSelected(true);
            mySelectedItem = position;
            if (Build.VERSION.SDK_INT > 10)
                invalidateOptionsMenu();
		}
	}

    private void deleteFileWithPrompt(final File file) {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setMessage(file == null ? R.string.hts_delete_files_prompt : R.string.hts_delete_prompt);
        builder.setPositiveButton(android.R.string.yes, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialogInterface, int i) {
                if (file != null) {
                    file.delete();
                    selectButton.setEnabled(mustSelectDir);
                } else for (FileRowData frd : fileList) {
                    if (frd.selected) {
                        frd.file.delete();
                    }
                }
                getDir(currentPath);
            }
        });
        builder.setNegativeButton(android.R.string.no, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialogInterface, int i) {
                if (Build.VERSION.SDK_INT > 10)
                    invalidateOptionsMenu();
            }
        });
        builder.show();
    }

    @Override
    public boolean onItemLongClick(AdapterView<?> parent, View view, int position, long id) {
        if (position < 1)
            return true;
        view.setSelected(true);
        final File file = fileList.get(position).file;
        if (file.isDirectory()) {
            if (file.equals(currentPath))
                selectedDir = null;
            else
                selectedDir = file;
        } else {
            selectedDir = null;
            mySelectedItem = position;
            selectedFile = file;
        }
        deleteFileWithPrompt(file);
        return true;
    }

    @Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		if ((keyCode == KeyEvent.KEYCODE_BACK)) {
			selectButton.setEnabled(mustSelectDir);

			if (layoutCreate.getVisibility() == View.VISIBLE) {
                layoutCreate.setVisibility(mustCreateNew ? View.VISIBLE : View.GONE);
                layoutSelect.setVisibility(mustCreateNew ? View.GONE : View.VISIBLE);
			} else {
                if (pathStack.size() < 2)
                    return super.onKeyDown(keyCode, event);
                pathStack.remove(0);
                String pth = pathStack.get(0);
                getDir(pth);
			}

			return true;
		} else {
			return super.onKeyDown(keyCode, event);
		}
	}

	private void setCreateVisible(View v) {
		layoutCreate.setVisibility(View.VISIBLE);
		layoutSelect.setVisibility(View.GONE);
		inputManager.hideSoftInputFromWindow(v.getWindowToken(), 0);
		selectButton.setEnabled(mustSelectDir);
	}

	private void setSelectVisible() {
        layoutCreate.setVisibility(mustCreateNew ? View.VISIBLE : View.GONE);
        layoutSelect.setVisibility(mustCreateNew ? View.GONE : View.VISIBLE);
        //selectButton.setEnabled(!mustSelectDir || (currentPath.equals(startPath) ? false : true));
        selectButton.setEnabled(true);
	}

    private FileRowData getFrd(int position) {
        return(((FileRowAdapter)getListAdapter()).getItem(position));
    }

    private static boolean dirWritable(String dir) {
        return dirWritable(new File(dir));
    }

    private static boolean dirWritable(File dirFile) {
        try {
            if (!dirFile.isDirectory())
                return false;
            File testFile = File.createTempFile("test", "tmp", dirFile);
            testFile.delete();
            return true;
        } catch (IOException e) {
            return false;
        }
    }


    class FileRowAdapter extends ArrayAdapter<FileRowData> {
        FileRowAdapter(ArrayList<FileRowData> list) {
            super(GenericFileDialog.this, R.layout.generic_file_row, R.id.fdrowtext, list);
        }

        public View getView(int position, View convertView, ViewGroup parent) {
            View row=super.getView(position, convertView, parent);
            ViewHolder holder=(ViewHolder)row.getTag();

            if (holder==null) {
                holder=new ViewHolder(row);
                row.setTag(holder);
            }

            FileRowData frd=getFrd(position);
            holder.select_cb.setTag(Integer.valueOf(position));
            ImageView iv = (ImageView)row.findViewById(R.id.fdrowimage);
            CheckBox cb = (CheckBox)row.findViewById(R.id.fdrowcb);
            TextView tv = (TextView)row.findViewById(R.id.fdrowtext);
            try {
                XmlResourceParser xpp;
                if (frd.imageId == R.drawable.folder)
                    xpp = getResources().getXml(R.xml.file_selector_fdtext_light);
                else
                    xpp = getResources().getXml(R.xml.file_selector_text_light);
                tv.setTextColor(ColorStateList.createFromXml(getResources(), xpp));
            } catch (Exception e) {}
            cb.setVisibility(View.GONE);
            iv.setImageResource(frd.imageId);
            tv.setText(frd.name);

            return(row);
        }
    }

    public class FrdComparator implements Comparator<FileRowData> {
        @Override
        public int compare(FileRowData o1, FileRowData o2) {
            if (o1.imageId == o2.imageId) {
                return o1.file.getName().compareToIgnoreCase(o2.file.getName());
            }
            if (o1.imageId > o2.imageId)
                return -1;
            return 1;
        }
    }

    class FileRowData {
        File file;
        String name;
        int imageId;
        boolean selected;

        FileRowData(File fil, String fName, int iId) {
            file = fil;
            name = fName;
            imageId = iId;
            selected = false;
        }
    }

    class ViewHolder {
        CheckBox select_cb=null;

        ViewHolder(View base) {
            this.select_cb=(CheckBox)base.findViewById(R.id.fdrowcb);
        }
    }

    static abstract class OpCallback {
        BgTask myTask = null;
        public BgTask getTask() { return myTask; }
        public abstract boolean runInBg();
        public abstract void onFinished(boolean result); // result is: -2 not started, -1 in progress, 0 failed, 1 succeeded
    }

    static BgTask bgTask(Context context, boolean wantProgress, String progTitle, String progText, OpCallback cb) {
        return bgTask(context, wantProgress, progTitle, progText, cb, false, null);
    }

    static BgTask bgTask(Context context, boolean wantProgress, String progTitle, String progText, OpCallback cb,
                                boolean cancelable, DialogInterface.OnCancelListener cancelListener) {
        BgTask task = new BgTask(context);
        if (cb != null) {
            task.myOpCallback = cb;
            cb.myTask = task;
        }
        if (wantProgress)
            task.progress = ProgressDialog.show(context,
                    progTitle == null ? "" : progTitle,
                    progText == null ? "" : progText,
                    true, cancelable, cancelListener);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
            task.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
        } else {
            task.execute();
        }
        return task;
    }

    static class BgTask extends AsyncTask<Void, Boolean, Boolean> {
        ProgressDialog progress = null;
        OpCallback myOpCallback = null;
        Context context;

        private BgTask() {} // prevent direct object creation

        protected BgTask(Context ctx) {
            context = ctx;
        }

        protected void onPreExecute() {
        }

        public void setProgressMsg(final String text) {
            if (progress != null && context != null && context instanceof Activity) {
                ((Activity)context).runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        progress.setMessage(text);
                    }
                });
            }
        }

        @Override
        protected Boolean doInBackground(Void... params) {
            return myOpCallback.runInBg();
        }

        @Override
        protected void onPostExecute(Boolean result) {
            if(progress != null)
                try { progress.dismiss(); } catch (Exception dont_care) {}
            if (myOpCallback != null) {
                myOpCallback.onFinished(result);
                myOpCallback = null;
            }
        }

        @Override
        protected void onCancelled(Boolean result) {
            super.onCancelled();
            if(progress != null)
                try { progress.dismiss(); } catch (Exception dont_care) {}
            if (myOpCallback != null) {
                myOpCallback.onFinished(result);
                myOpCallback = null;
            }
        }
    }

}
