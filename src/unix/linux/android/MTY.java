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
import android.hardware.input.InputManager;
import android.util.Log;
import android.util.DisplayMetrics;
import android.util.Base64;
import android.widget.Scroller;
import android.os.Vibrator;
import android.os.Build;

class MTYSurface extends SurfaceView implements SurfaceHolder.Callback,
	GestureDetector.OnGestureListener, GestureDetector.OnDoubleTapListener,
	GestureDetector.OnContextClickListener, ScaleGestureDetector.OnScaleGestureListener,
	InputManager.InputDeviceListener
{
	String iCursorB64 =
		"iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAQAAADZc7J/AAAAH0lEQVR42mNk" +
		"oBAwjhowasCoAaMGjBowasCoAcPNAACOMAAhOO/A7wAAAABJRU5ErkJggg==";

	PointerIcon cursor;
	PointerIcon iCursor;
	GestureDetector detector;
	ScaleGestureDetector sdetector;
	Scroller scroller;
	Vibrator vibrator;
	boolean hiddenCursor;
	boolean defaultCursor;


	// Surface

	native void gfx_dims(int w, int h);
	native void gfx_set_surface(Surface surface);
	native void gfx_unset_surface();

	public MTYSurface(Context context) {
		super(context);
		this.getHolder().addCallback(this);

		this.setFocusableInTouchMode(true);
		this.setFocusable(true);

		this.vibrator = (Vibrator) context.getSystemService(Context.VIBRATOR_SERVICE);

		this.detector = new GestureDetector(context, this);
		this.detector.setOnDoubleTapListener(this);
		this.detector.setContextClickListener(this);

		InputManager manager = (InputManager) context.getSystemService(Context.INPUT_SERVICE);
		manager.registerInputDeviceListener(this, null);

		this.sdetector = new ScaleGestureDetector(context, this);

		this.scroller = new Scroller(context);

		byte[] iCursorData = Base64.decode(this.iCursorB64, Base64.DEFAULT);
		Bitmap bm = BitmapFactory.decodeByteArray(iCursorData, 0, iCursorData.length, null);
		this.iCursor = PointerIcon.create(bm, 0, 0);
	}

	@Override
	public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
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
		if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O)
			return;

		if (relative) {
			this.requestPointerCapture();

		} else {
			this.releasePointerCapture();
		}
	}


	// InputDevice listener

	native void app_unplug(int deviceId);

	@Override
	public void onInputDeviceRemoved(int deviceId) {
		app_unplug(deviceId);
	}

	@Override
	public void onInputDeviceChanged(int deviceId) {
	}

	@Override
	public void onInputDeviceAdded(int deviceId) {
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
	native void app_scroll(float absX, float absY, float x, float y, int fingers);
	native boolean app_long_press(float x, float y);
	native void app_check_scroller(boolean check);
	native void app_mouse_motion(boolean relative, float x, float y);
	native void app_mouse_button(boolean pressed, int button, float x, float y);
	native void app_mouse_wheel(float x, float y);
	native void app_button(int deviceId, boolean pressed, int code);
	native void app_axis(int deviceId, float hatX, float hatY, float lX, float lY, float rX, float rY, float lT, float rT);
	native void app_unhandled_touch(int action, float x, float y, int fingers);

	private static boolean isMouseEvent(InputEvent event) {
		return
			(event.getSource() & InputDevice.SOURCE_MOUSE) == InputDevice.SOURCE_MOUSE;
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
			app_button(event.getDeviceId(), true, keyCode);

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
			app_button(event.getDeviceId(), false, keyCode);

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

			app_unhandled_touch(event.getActionMasked(), event.getX(0), event.getY(0), event.getPointerCount());
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

		if (app_long_press(event.getX(0), event.getY(0)))
			this.vibrator.vibrate(10);
	}

	@Override
	public boolean onScroll(MotionEvent event1, MotionEvent event2, float distanceX, float distanceY) {
		if (isMouseEvent(event1) || isMouseEvent(event2))
			return false;

		this.scroller.forceFinished(true);
		app_scroll(event2.getX(0), event2.getY(0), distanceX, distanceY, event2.getPointerCount());
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
		if (isMouseEvent(event))
			return false;

		app_single_tap_up(event.getX(0), event.getY(0));
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
				event.getDeviceId(),
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

	int scrollY;
	boolean isFullscreen;
	float displayDensity;
	MTYSurface surface;
	Activity activity;


	// Called from Java

	public MTY(String name, Activity activity) {
		if (!runOnce) {
			System.loadLibrary(name);
			gfx_global_init();
		}

		this.activity = activity;
		this.surface = new MTYSurface(activity.getApplicationContext());

		DisplayMetrics dm = new DisplayMetrics();
		this.activity.getWindowManager().getDefaultDisplay().getMetrics(dm);
		this.displayDensity = dm.xdpi;

		ViewGroup vg = activity.findViewById(android.R.id.content);
		activity.addContentView(this.surface, vg.getLayoutParams());

		this.surface.requestFocus();

		ClipboardManager clipboard = (ClipboardManager) this.activity.getSystemService(Context.CLIPBOARD_SERVICE);
		clipboard.addPrimaryClipChangedListener(this);

		if (!runOnce)
			this.start();

		runOnce = true;
	}

	@Override
	public void run() {
		app_start(this.activity.getApplicationContext().getPackageName());
		this.activity.finishAndRemoveTask();
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

	private boolean flagsFullscreen() {
		return (this.activity.getWindow().getDecorView().getSystemUiVisibility() & MTY.fullscreenFlags()) == MTY.fullscreenFlags();
	}

	private void setUiFlags(int flags) {
		this.activity.getWindow().getDecorView().setSystemUiVisibility(flags);
	}

	public void checkScroller() {
		this.surface.scroller.computeScrollOffset();

		if (!this.surface.scroller.isFinished()) {
			int currY = this.surface.scroller.getCurrY();
			int diff = this.scrollY - currY;

			if (diff != 0)
				this.surface.app_scroll(-1.0f, -1.0f, 0.0f, diff, 1);

			this.scrollY = currY;

		} else {
			this.surface.app_check_scroller(false);
			this.scrollY = 0;
		}
	}

	public void enableScreenSaver(boolean _enable) {
		final MTY self = this;
		final boolean enable = _enable;

		this.activity.runOnUiThread(new Runnable() {
			@Override
			public void run() {
				if (enable) {
					self.activity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

				} else {
					self.activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
				}
			}
		});
	}

	public void showKeyboard(boolean show) {
		Context context = this.surface.getContext();
		InputMethodManager imm = (InputMethodManager) context.getSystemService(Context.INPUT_METHOD_SERVICE);

		if (show) {
			imm.showSoftInput(this.surface, 0, null);

		} else {
			imm.hideSoftInputFromWindow(this.surface.getWindowToken(), 0, null);
		}
	}

	public boolean isFullscreen() {
		return this.isFullscreen;
	}

	public void setOrientation(int _orientation) {
		final MTY self = this;
		final int orienation = _orientation;

		this.activity.runOnUiThread(new Runnable() {
			@Override
			public void run() {
				switch (orienation) {
					case 1:
						self.activity.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);
						break;
					case 2:
						self.activity.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT);
						break;
					default:
						self.activity.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED);
						break;
				}
			}
		});
	}

	public float getDisplayDensity() {
		return this.displayDensity;
	}

	public void handleFullscreen(boolean enable) {
		// Static variable update only on main thread
		this.isFullscreen = this.flagsFullscreen();

		if (enable && !this.isFullscreen) {
			this.setUiFlags(this.normalFlags() | this.fullscreenFlags());

		} else if (!enable && this.isFullscreen) {
			this.setUiFlags(this.normalFlags());
		}

		// Update static after set
		this.isFullscreen = this.flagsFullscreen();
	}

	public void enableFullscreen(boolean _enable) {
		final MTY self = this;
		final boolean enable = _enable;

		this.activity.runOnUiThread(new Runnable() {
			@Override
			public void run() {
				self.handleFullscreen(enable);
			}
		});
	}


	// Clipboard

	public void setClipboard(String str) {
		ClipboardManager clipboard = (ClipboardManager) this.activity.getSystemService(Context.CLIPBOARD_SERVICE);
		clipboard.setPrimaryClip(ClipData.newPlainText("MTY", str));
	}

	public String getClipboard() {
		ClipboardManager clipboard = (ClipboardManager) this.activity.getSystemService(Context.CLIPBOARD_SERVICE);
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
		final MTY self = this;
		final byte[] data = _data;
		final float hotX = _hotX;
		final float hotY = _hotY;

		this.activity.runOnUiThread(new Runnable() {
			@Override
			public void run() {
				self.surface.setCursor(data, hotX, hotY);
			}
		});
	}

	public void showCursor(boolean _show) {
		final MTY self = this;
		final boolean show = _show;

		this.activity.runOnUiThread(new Runnable() {
			@Override
			public void run() {
				self.surface.hiddenCursor = !show;
				self.surface.setCursor();
			}
		});
	}

	public void useDefaultCursor(boolean _useDefault) {
		final MTY self = this;
		final boolean useDefault = _useDefault;

		this.activity.runOnUiThread(new Runnable() {
			@Override
			public void run() {
				self.surface.defaultCursor = useDefault;
				self.surface.setCursor();
			}
		});
	}

	public void setRelativeMouse(boolean _relative) {
		final MTY self = this;
		final boolean relative = _relative;

		this.activity.runOnUiThread(new Runnable() {
			@Override
			public void run() {
				self.surface.setRelativeMouse(relative);
			}
		});
	}

	public boolean getRelativeMouse() {
		if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O)
			return false;

		return this.surface.hasPointerCapture();
	}
}
