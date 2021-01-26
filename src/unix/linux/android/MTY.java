package group.matoya.lib;

import android.app.Activity;
import android.view.View;
import android.view.ViewGroup;
import android.view.Surface;
import android.view.SurfaceView;
import android.view.SurfaceHolder;
import android.view.MotionEvent;
import android.view.KeyEvent;
import android.view.GestureDetector;
import android.view.ScaleGestureDetector;
import android.view.WindowManager;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputMethodManager;
import android.text.InputType;
import android.content.Context;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.pm.ActivityInfo;
import android.util.Log;
import android.util.DisplayMetrics;
import android.widget.Scroller;

class MTYSurface extends SurfaceView implements SurfaceHolder.Callback,
	GestureDetector.OnGestureListener, GestureDetector.OnDoubleTapListener,
	GestureDetector.OnContextClickListener, ScaleGestureDetector.OnScaleGestureListener
{
	GestureDetector detector;
	ScaleGestureDetector sdetector;
	Scroller scroller;

	// Surface

	native void app_dims(int w, int h);
	native void gfx_dims(int w, int h);
	native void gfx_set_surface(Surface surface);
	native void gfx_unset_surface();

	public MTYSurface(Context context) {
		super(context);
		this.getHolder().addCallback(this);

		this.setFocusableInTouchMode(true);
		this.setFocusable(true);

		this.detector = new GestureDetector(context, this);
		this.detector.setOnDoubleTapListener(this);
		this.detector.setContextClickListener(this);

		this.sdetector = new ScaleGestureDetector(context, this);

		this.scroller = new Scroller(context);
	}

	@Override
	public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
		app_dims(w, h);
		gfx_dims(w, h);
	}

	@Override
	public void surfaceCreated(SurfaceHolder holder) {
		gfx_set_surface(holder.getSurface());
	}

	@Override
	public void surfaceDestroyed(SurfaceHolder holder) {
		gfx_unset_surface();
	}


	// InputConnection (for IME keyboard)

	@Override
	public boolean onCheckIsTextEditor() {
		return true;
	}

	@Override
	public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
		outAttrs.inputType = InputType.TYPE_NULL;
		outAttrs.imeOptions = EditorInfo.IME_FLAG_NO_FULLSCREEN;

		return new BaseInputConnection(this, false);
	}


	// Events

	native void app_key(boolean pressed, int code, int text, int mods);
	native void app_single_tap_up(float x, float y);
	native void app_scroll(float initX, float initY, float x, float y);
	native void app_long_press(float x, float y);
	native void app_check_scroller(boolean check);

	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		app_key(true, keyCode, event.getUnicodeChar(), event.getMetaState());
		return true;
	}

	@Override
	public boolean onKeyUp(int keyCode, KeyEvent event) {
		app_key(false, keyCode, event.getUnicodeChar(), event.getMetaState());
		return true;
	}

	@Override
	public boolean onTouchEvent(MotionEvent event) {
		this.detector.onTouchEvent(event);
		this.sdetector.onTouchEvent(event);

		return true;
	}

	@Override
	public boolean onDown(MotionEvent event) {
		this.scroller.forceFinished(true);
		return true;
	}

	@Override
	public boolean onFling(MotionEvent event1, MotionEvent event2, float velocityX, float velocityY) {
		this.scroller.forceFinished(true);
		this.scroller.fling(0, 0, Math.round(velocityX), Math.round(velocityY),
			Integer.MIN_VALUE, Integer.MAX_VALUE, Integer.MIN_VALUE, Integer.MAX_VALUE);
		app_check_scroller(true);
		return true;
	}

	@Override
	public void onLongPress(MotionEvent event) {
		app_long_press(event.getX(0), event.getY(0));
	}

	@Override
	public boolean onScroll(MotionEvent event1, MotionEvent event2, float distanceX, float distanceY) {
		this.scroller.forceFinished(true);
		app_scroll(event1.getX(0), event1.getY(0), distanceX, distanceY);
		return true;
	}

	@Override
	public void onShowPress(MotionEvent event) {
	}

	@Override
	public boolean onSingleTapUp(MotionEvent event) {
		app_single_tap_up(event.getX(0), event.getY(0));
		return true;
	}

	@Override
	public boolean onDoubleTap(MotionEvent event) {
		return true;
	}

	@Override
	public boolean onDoubleTapEvent(MotionEvent event) {
		return true;
	}

	@Override
	public boolean onSingleTapConfirmed(MotionEvent event) {
		return true;
	}

	@Override
	public boolean onContextClick(MotionEvent event) {
		return true;
	}

	@Override
	public boolean onGenericMotionEvent(MotionEvent event) {
		return true;
	}

	@Override
	public boolean onScale(ScaleGestureDetector detector) {
		return true;
	}

	@Override
	public boolean onScaleBegin(ScaleGestureDetector sdetector) {
		return true;
	}

	@Override
	public void onScaleEnd(ScaleGestureDetector sdetector) {
	}
}

public class MTY extends Thread implements ClipboardManager.OnPrimaryClipChangedListener {
	native void app_start(String name);
	native void gfx_global_init();

	static boolean runOnce;
	static MTYSurface SURFACE;
	static Activity ACTIVITY;
	static boolean IS_FULLSCREEN;
	static int DISPLAY_DENSITY;

	int scrollY;


	// Called from Java

	public MTY(String name, Activity activity) {
		if (!runOnce) {
			System.loadLibrary(name);
			gfx_global_init();
		}

		ACTIVITY = activity;
		SURFACE = new MTYSurface(activity.getApplicationContext());

		DisplayMetrics dm = new DisplayMetrics();
		ACTIVITY.getWindowManager().getDefaultDisplay().getMetrics(dm);
		DISPLAY_DENSITY = dm.densityDpi;

		ViewGroup vg = activity.findViewById(android.R.id.content);
		activity.addContentView(SURFACE, vg.getLayoutParams());

		SURFACE.requestFocus();

		ClipboardManager clipboard = (ClipboardManager) ACTIVITY.getSystemService(Context.CLIPBOARD_SERVICE);
		clipboard.addPrimaryClipChangedListener(this);

		if (!runOnce)
			this.start();

		runOnce = true;
	}

	@Override
	public void run() {
		app_start(ACTIVITY.getApplicationContext().getPackageName());
		ACTIVITY.finishAndRemoveTask();
	}


	// Called from C

	private static int normalFlags() {
		return
			View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY |  // Prevents hidden stuff from coming back spontaneously
			View.SYSTEM_UI_FLAG_LAYOUT_STABLE;
	}

	private static int fullscreenFlags() {
		return
			View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN | // Removes space at the top for menu bar
			View.SYSTEM_UI_FLAG_FULLSCREEN |        // Removes menu bar
			View.SYSTEM_UI_FLAG_HIDE_NAVIGATION;    // Hides navigation buttons at the bottom
	}

	private static boolean flagsFullscreen() {
		return (ACTIVITY.getWindow().getDecorView().getSystemUiVisibility() & MTY.fullscreenFlags()) == MTY.fullscreenFlags();
	}

	private static void setUiFlags(int flags) {
		ACTIVITY.getWindow().getDecorView().setSystemUiVisibility(flags);
	}

	public void checkScroller() {
		SURFACE.scroller.computeScrollOffset();

		if (!SURFACE.scroller.isFinished()) {
			int currY = SURFACE.scroller.getCurrY();
			int diff = this.scrollY - currY;

			if (diff != 0)
				SURFACE.app_scroll(-1.0f, -1.0f, 0.0f, diff);

			this.scrollY = currY;

		} else {
			SURFACE.app_check_scroller(false);
			this.scrollY = 0;
		}
	}

	public void enableScreenSaver(boolean _enable) {
		final boolean enable = _enable;

		ACTIVITY.runOnUiThread(new Runnable() {
			@Override
			public void run() {
				if (enable) {
					ACTIVITY.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

				} else {
					ACTIVITY.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
				}
			}
		});
	}

	public void showKeyboard(boolean show) {
		Context context = SURFACE.getContext();
		InputMethodManager imm = (InputMethodManager) context.getSystemService(Context.INPUT_METHOD_SERVICE);

		if (show) {
			imm.showSoftInput(SURFACE, 0, null);

		} else {
			imm.hideSoftInputFromWindow(SURFACE.getWindowToken(), 0, null);
		}
	}

	public boolean isFullscreen() {
		return IS_FULLSCREEN;
	}

	public void setOrientation(int _orientation) {
		final int orienation = _orientation;

		ACTIVITY.runOnUiThread(new Runnable() {
			@Override
			public void run() {
				switch (orienation) {
					case 1:
						ACTIVITY.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);
						break;
					case 2:
						ACTIVITY.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT);
						break;
					default:
						ACTIVITY.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_USER);
						break;
				}
			}
		});
	}

	public int getDisplayDensity() {
		return DISPLAY_DENSITY;
	}

	public void enableFullscreen(boolean _enable) {
		final boolean enable = _enable;

		ACTIVITY.runOnUiThread(new Runnable() {
			@Override
			public void run() {
				// Static variable update only on main thread
				IS_FULLSCREEN = flagsFullscreen();

				if (enable && !IS_FULLSCREEN) {
					setUiFlags(normalFlags() | fullscreenFlags());

				} else if (!enable && IS_FULLSCREEN) {
					setUiFlags(normalFlags());
				}

				// Update static after set
				IS_FULLSCREEN = flagsFullscreen();
			}
		});
	}


	// Clipboard

	public void setClipboard(String str) {
		ClipboardManager clipboard = (ClipboardManager) ACTIVITY.getSystemService(Context.CLIPBOARD_SERVICE);
		clipboard.setPrimaryClip(ClipData.newPlainText("MTY", str));
	}

	public String getClipboard() {
		ClipboardManager clipboard = (ClipboardManager) ACTIVITY.getSystemService(Context.CLIPBOARD_SERVICE);
		ClipData primary = clipboard.getPrimaryClip();

		if (primary != null) {
			ClipData.Item item = primary.getItemAt(0);
			CharSequence chars = item.getText();

			if (chars != null)
				return chars.toString();
		}

		return null;
	}

	@Override
	public void onPrimaryClipChanged() {
		// Send native notification
	}
}
