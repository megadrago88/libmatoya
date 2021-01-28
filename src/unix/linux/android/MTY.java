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
import android.view.PointerIcon;
import android.view.InputDevice;
import android.view.InputEvent;
import android.text.InputType;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.content.Context;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.pm.ActivityInfo;
import android.util.Log;
import android.util.DisplayMetrics;
import android.widget.Scroller;
import java.util.Base64;

class MTYSurface extends SurfaceView implements SurfaceHolder.Callback,
	GestureDetector.OnGestureListener, GestureDetector.OnDoubleTapListener,
	GestureDetector.OnContextClickListener, ScaleGestureDetector.OnScaleGestureListener
{
	String iCursorB64 =
		"iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAQAAADZc7J/AAAAH0lEQVR42mNk" +
		"oBAwjhowasCoAaMGjBowasCoAcPNAACOMAAhOO/A7wAAAABJRU5ErkJggg==";

	PointerIcon cursor;
	PointerIcon iCursor;
	GestureDetector detector;
	ScaleGestureDetector sdetector;
	Scroller scroller;
	boolean hiddenCursor;
	boolean defaultCursor;


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

		byte[] iCursorData = Base64.getDecoder().decode(this.iCursorB64);
		Bitmap bm = BitmapFactory.decodeByteArray(iCursorData, 0, iCursorData.length, null);
		this.iCursor = PointerIcon.create(bm, 0, 0);
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


	// Cursor

	public PointerIcon getCursor() {
		return this.defaultCursor ? null : this.hiddenCursor ? this.iCursor : this.cursor;
	}

	public void setCursor() {
		this.setPointerIcon(this.getCursor());
	}

	public void setCursor(byte[] data, float hotX, float hotY) {
		if (data != null) {
			Bitmap bm = BitmapFactory.decodeByteArray(data, 0, data.length, null);
			this.cursor = PointerIcon.create(bm, hotX, hotY);

		} else {
			this.cursor = null;
		}

		this.setPointerIcon(this.cursor);
	}

	@Override
	public PointerIcon onResolvePointerIcon(MotionEvent event, int pointerIndex) {
		return this.getCursor();
	}

	public void setRelativeMouse(boolean relative) {
		if (relative) {
			this.requestPointerCapture();

		} else {
			this.releasePointerCapture();
		}
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

	native boolean app_key(boolean pressed, int code, int text, int mods);
	native void app_single_tap_up(float x, float y);
	native void app_scroll(float initX, float initY, float x, float y);
	native void app_long_press(float x, float y);
	native void app_check_scroller(boolean check);
	native void app_mouse_motion(boolean relative, float x, float y);
	native void app_mouse_button(boolean pressed, int button, float x, float y);
	native void app_mouse_wheel(float x, float y);
	native void app_button(boolean pressed, int code);
	native void app_axis(float hatX, float hatY, float lX, float lY, float rX, float rY, float lT, float rT);

	private static boolean isMouseEvent(InputEvent event) {
		return
			(event.getSource() & InputDevice.SOURCE_MOUSE) == InputDevice.SOURCE_MOUSE ||
			(event.getSource() & InputDevice.SOURCE_MOUSE_RELATIVE) == InputDevice.SOURCE_MOUSE_RELATIVE;
	}

	private static boolean isGamepadEvent(InputEvent event) {
		return
			(event.getSource() & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD ||
			(event.getSource() & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK ||
			(event.getSource() & InputDevice.SOURCE_DPAD) == InputDevice.SOURCE_DPAD;
	}

	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		// Button events fire here (sometimes dpad)
		if (isGamepadEvent(event)) {
			app_button(true, keyCode);

		// Prevents back buttons etc. from being generated from mice
		} else if (!isMouseEvent(event)) {
			return app_key(true, keyCode, event.getUnicodeChar(), event.getMetaState());
		}

		return true;
	}

	@Override
	public boolean onKeyUp(int keyCode, KeyEvent event) {
		// Button events fire here (sometimes dpad)
		if (isGamepadEvent(event)) {
			app_button(false, keyCode);

		// Prevents back buttons etc. from being generated from mice
		} else if (!isMouseEvent(event)) {
			return app_key(false, keyCode, event.getUnicodeChar(), event.getMetaState());
		}

		return true;
	}

	@Override
	public boolean onTouchEvent(MotionEvent event) {
		// Mouse motion while buttons are held down fire here
		if (isMouseEvent(event)) {
			if (event.getActionMasked() == MotionEvent.ACTION_MOVE)
				app_mouse_motion(false, event.getX(0), event.getY(0));

		} else {
			this.detector.onTouchEvent(event);
			this.sdetector.onTouchEvent(event);
		}

		return true;
	}

	@Override
	public boolean onDown(MotionEvent event) {
		if (isMouseEvent(event))
			return false;

		this.scroller.forceFinished(true);
		return true;
	}

	@Override
	public boolean onFling(MotionEvent event1, MotionEvent event2, float velocityX, float velocityY) {
		if (isMouseEvent(event1) || isMouseEvent(event2))
			return false;

		this.scroller.forceFinished(true);
		this.scroller.fling(0, 0, Math.round(velocityX), Math.round(velocityY),
			Integer.MIN_VALUE, Integer.MAX_VALUE, Integer.MIN_VALUE, Integer.MAX_VALUE);
		app_check_scroller(true);

		return true;
	}

	@Override
	public void onLongPress(MotionEvent event) {
		if (isMouseEvent(event))
			return;

		app_long_press(event.getX(0), event.getY(0));
	}

	@Override
	public boolean onScroll(MotionEvent event1, MotionEvent event2, float distanceX, float distanceY) {
		if (isMouseEvent(event1) || isMouseEvent(event2))
			return false;

		this.scroller.forceFinished(true);
		app_scroll(event1.getX(0), event1.getY(0), distanceX, distanceY);
		return true;
	}

	@Override
	public void onShowPress(MotionEvent event) {
	}

	@Override
	public boolean onSingleTapUp(MotionEvent event) {
		if (isMouseEvent(event))
			return false;

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
		if (isMouseEvent(event)) {
			switch (event.getActionMasked()) {
				case MotionEvent.ACTION_HOVER_MOVE:
					app_mouse_motion(false, event.getX(0), event.getY(0));
					return true;
				case MotionEvent.ACTION_SCROLL:
					app_mouse_wheel(event.getAxisValue(MotionEvent.AXIS_HSCROLL), event.getAxisValue(MotionEvent.AXIS_VSCROLL));
					return true;
				case MotionEvent.ACTION_BUTTON_PRESS:
					app_mouse_button(true, event.getActionButton(), event.getX(0), event.getY(0));
					return true;
				case MotionEvent.ACTION_BUTTON_RELEASE:
					app_mouse_button(false, event.getActionButton(), event.getX(0), event.getY(0));
					return true;
				default:
					break;
			}

		// DPAD and axis events fire here
		} else if (isGamepadEvent(event)) {
			app_axis(
				event.getAxisValue(MotionEvent.AXIS_HAT_X),
				event.getAxisValue(MotionEvent.AXIS_HAT_Y),
				event.getAxisValue(MotionEvent.AXIS_X),
				event.getAxisValue(MotionEvent.AXIS_Y),
				event.getAxisValue(MotionEvent.AXIS_Z),
				event.getAxisValue(MotionEvent.AXIS_RZ),
				event.getAxisValue(MotionEvent.AXIS_LTRIGGER),
				event.getAxisValue(MotionEvent.AXIS_RTRIGGER));
		}

		return true;
	}

	@Override
	public boolean onCapturedPointerEvent(MotionEvent event) {
		app_mouse_motion(true, event.getX(0), event.getY(0));

		return this.onGenericMotionEvent(event);
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
	static float DISPLAY_DENSITY;

	int scrollY;


	// Called from Java

	public MTY(String name, Activity activity) {
		if (!runOnce) {
			System.loadLibrary(name);
			gfx_global_init();
		}

		ACTIVITY = activity;
		SURFACE = new MTYSurface(activity.getApplicationContext());

		ACTIVITY.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_USER);

		DisplayMetrics dm = new DisplayMetrics();
		ACTIVITY.getWindowManager().getDefaultDisplay().getMetrics(dm);
		DISPLAY_DENSITY = dm.xdpi;

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

	public float getDisplayDensity() {
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


	// Cursor

	public void setCursor(byte[] _data, float _hotX, float _hotY) {
		final byte[] data = _data;
		final float hotX = _hotX;
		final float hotY = _hotY;

		ACTIVITY.runOnUiThread(new Runnable() {
			@Override
			public void run() {
				SURFACE.setCursor(data, hotX, hotY);
			}
		});
	}

	public void showCursor(boolean _show) {
		final boolean show = _show;

		ACTIVITY.runOnUiThread(new Runnable() {
			@Override
			public void run() {
				SURFACE.hiddenCursor = !show;
				SURFACE.setCursor();
			}
		});
	}

	public void useDefaultCursor(boolean _useDefault) {
		final boolean useDefault = _useDefault;

		ACTIVITY.runOnUiThread(new Runnable() {
			@Override
			public void run() {
				SURFACE.defaultCursor = useDefault;
				SURFACE.setCursor();
			}
		});
	}

	public void setRelativeMouse(boolean _relative) {
		final boolean relative = _relative;

		ACTIVITY.runOnUiThread(new Runnable() {
			@Override
			public void run() {
				SURFACE.setRelativeMouse(relative);
			}
		});
	}

	public boolean getRelativeMouse() {
		return SURFACE.hasPointerCapture();
	}
}
