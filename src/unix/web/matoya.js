// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Helpers

let MODULE;
let MTY_ALLOC;
let MTY_FREE;

function mem() {
	return MODULE.instance.exports.memory.buffer;
}

function MTY_CFunc(ptr) {
	return MODULE.instance.exports.__indirect_function_table.get(ptr);
}

function MTY_Alloc(size, el) {
	return MTY_CFunc(MTY_ALLOC)(size, el ? el : 1);
}

function MTY_Free(ptr) {
	MTY_CFunc(MTY_FREE)(ptr);
}

function mem_view() {
	return new DataView(mem());
}

function setUint32(ptr, value) {
	mem_view().setUint32(ptr, value, true);
}

function setInt32(ptr, value) {
	mem_view().setInt32(ptr, value, true);
}

function setInt8(ptr, value) {
	mem_view().setInt8(ptr, value);
}

function setFloat(ptr, value) {
	mem_view().setFloat32(ptr, value, true);
}

function setUint64(ptr, value) {
	mem_view().setBigUint64(ptr, BigInt(value), true);
}

function getUint32(ptr) {
	return mem_view().getUint32(ptr, true);
}

function copy(cptr, abuffer) {
	let heap = new Uint8Array(mem(), cptr);
	heap.set(abuffer);
}

function char_to_js(buf) {
	let str = '';

	for (let x = 0; x < 0x7FFFFFFF && x < buf.length; x++) {
		if (buf[x] == 0)
			break;

		str += String.fromCharCode(buf[x]);
	}

	return str;
}

function c_to_js(ptr) {
	return char_to_js(new Uint8Array(mem(), ptr));
}

function js_to_c(js_str, ptr) {
	const view = new Uint8Array(mem(), ptr);

	// XXX No bounds checking!
	let len = 0;
	for (; len < js_str.length; len++)
		view[len] = js_str.charCodeAt(len);

	// '\0' character
	view[len] = 0;

	return ptr;
}

function b64_to_buf(str) {
	return Uint8Array.from(atob(str), c => c.charCodeAt(0))
}

function buf_to_b64(buf) {
	let str = '';
	for (let x = 0; x < buf.length; x++)
		str += String.fromCharCode(buf[x]);

	return btoa(str);
}


// GL

let GL;
let GL_OBJ_INDEX = 0;
let GL_OBJ = {};

function gl_new(obj) {
	GL_OBJ[GL_OBJ_INDEX] = obj;

	return GL_OBJ_INDEX++;
}

function gl_del(index) {
	let obj = GL_OBJ[index];

	GL_OBJ[index] = undefined;
	delete GL_OBJ[index];

	return obj;
}

function gl_obj(index) {
	return GL_OBJ[index];
}

const GL_API = {
	glGenFramebuffers: function (n, ids) {
		for (let x = 0; x < n; x++)
			setUint32(ids + x * 4, gl_new(GL.createFramebuffer()));
	},
	glDeleteFramebuffers: function (n, ids) {
		for (let x = 0; x < n; x++)
			GL.deleteFramebuffer(gl_del(getUint32(ids + x * 4)));
	},
	glBindFramebuffer: function (target, fb) {
		GL.bindFramebuffer(target, fb ? gl_obj(fb) : null);
	},
	glBlitFramebuffer: function (srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter) {
		GL.blitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
	},
	glFramebufferTexture2D: function (target, attachment, textarget, texture, level) {
		GL.framebufferTexture2D(target, attachment, textarget, gl_obj(texture), level);
	},
	glEnable: function (cap) {
		GL.enable(cap);
	},
	glIsEnabled: function (cap) {
		return GL.isEnabled(cap);
	},
	glDisable: function (cap) {
		GL.disable(cap);
	},
	glViewport: function (x, y, width, height) {
		GL.viewport(x, y, width, height);
	},
	glGetIntegerv: function (name, data) {
		const p = GL.getParameter(name);

		switch (name) {
			// object
			case GL.READ_FRAMEBUFFER_BINDING:
			case GL.DRAW_FRAMEBUFFER_BINDING:
			case GL.ARRAY_BUFFER_BINDING:
			case GL.TEXTURE_BINDING_2D:
			case GL.CURRENT_PROGRAM:
				setUint32(data, gl_new(p));
				break;

			// int32[4]
			case GL.VIEWPORT:
			case GL.SCISSOR_BOX:
				for (let x = 0; x < 4; x++)
					setUint32(data + x * 4, p[x]);
				break;

			// int
			case GL.ACTIVE_TEXTURE:
			case GL.BLEND_SRC_RGB:
			case GL.BLEND_DST_RGB:
			case GL.BLEND_SRC_ALPHA:
			case GL.BLEND_DST_ALPHA:
			case GL.BLEND_EQUATION_RGB:
			case GL.BLEND_EQUATION_ALPHA:
				setUint32(data, p);
				break;
		}

		setUint32(data, p);
	},
	glGetFloatv: function (name, data) {
		switch (name) {
			case GL.COLOR_CLEAR_VALUE:
				const p = GL.getParameter(name);

				for (let x = 0; x < 4; x++)
					setFloat(data + x * 4, p[x]);
				break;
		}
	},
	glBindTexture: function (target, texture) {
		GL.bindTexture(target, texture ? gl_obj(texture) : null);
	},
	glDeleteTextures: function (n, ids) {
		for (let x = 0; x < n; x++)
			GL.deleteTexture(gl_del(getUint32(ids + x * 4)));
	},
	glTexParameteri: function (target, pname, param) {
		GL.texParameteri(target, pname, param);
	},
	glGenTextures: function (n, ids) {
		for (let x = 0; x < n; x++)
			setUint32(ids + x * 4, gl_new(GL.createTexture()));
	},
	glTexImage2D: function (target, level, internalformat, width, height, border, format, type, data) {
		GL.texImage2D(target, level, internalformat, width, height, border, format, type,
			new Uint8Array(mem(), data));
	},
	glTexSubImage2D: function (target, level, xoffset, yoffset, width, height, format, type, pixels) {
		GL.texSubImage2D(target, level, xoffset, yoffset, width, height, format, type,
			new Uint8Array(mem(), pixels));
	},
	glDrawElements: function (mode, count, type, indices) {
		GL.drawElements(mode, count, type, indices);
	},
	glGetAttribLocation: function (program, c_name) {
		return GL.getAttribLocation(gl_obj(program), c_to_js(c_name));
	},
	glShaderSource: function (shader, count, c_strings, c_len) {
		let source = '';
		for (let x = 0; x < count; x++)
			source += c_to_js(getUint32(c_strings + x * 4));

		GL.shaderSource(gl_obj(shader), source);
	},
	glBindBuffer: function (target, buffer) {
		GL.bindBuffer(target, buffer ? gl_obj(buffer) : null);
	},
	glVertexAttribPointer: function (index, size, type, normalized, stride, pointer) {
		GL.vertexAttribPointer(index, size, type, normalized, stride, pointer);
	},
	glCreateProgram: function () {
		return gl_new(GL.createProgram());
	},
	glUniform1i: function (loc, v0) {
		GL.uniform1i(gl_obj(loc), v0);
	},
	glUniform1f: function (loc, v0) {
		GL.uniform1f(gl_obj(loc), v0);
	},
	glUniform3i: function (loc, v0, v1, v2) {
		GL.uniform3i(gl_obj(loc), v0, v1, v2);
	},
	glUniform4f: function (loc, v0, v1, v2, v3) {
		GL.uniform4f(gl_obj(loc), v0, v1, v2, v3);
	},
	glActiveTexture: function (texture) {
		GL.activeTexture(texture);
	},
	glDeleteBuffers: function (n, ids) {
		for (let x = 0; x < n; x++)
			GL.deleteBuffer(gl_del(getUint32(ids + x * 4)));
	},
	glEnableVertexAttribArray: function (index) {
		GL.enableVertexAttribArray(index);
	},
	glBufferData: function (target, size, data, usage) {
		GL.bufferData(target, new Uint8Array(mem(), data, size), usage);
	},
	glDeleteShader: function (shader) {
		GL.deleteShader(gl_del(shader));
	},
	glGenBuffers: function (n, ids) {
		for (let x = 0; x < n; x++)
			setUint32(ids + x * 4, gl_new(GL.createBuffer()));
	},
	glCompileShader: function (shader) {
		GL.compileShader(gl_obj(shader));
	},
	glLinkProgram: function (program) {
		GL.linkProgram(gl_obj(program));
	},
	glGetUniformLocation: function (program, name) {
		return gl_new(GL.getUniformLocation(gl_obj(program), c_to_js(name)));
	},
	glCreateShader: function (type) {
		return gl_new(GL.createShader(type));
	},
	glAttachShader: function (program, shader) {
		GL.attachShader(gl_obj(program), gl_obj(shader));
	},
	glUseProgram: function (program) {
		GL.useProgram(program ? gl_obj(program) : null);
	},
	glGetShaderiv: function (shader, pname, params) {
		if (pname == 0x8B81) {
			let ok = GL.getShaderParameter(gl_obj(shader), GL.COMPILE_STATUS);
			setUint32(params, ok);

			if (!ok)
				console.warn(GL.getShaderInfoLog(gl_obj(shader)));

		} else {
			setUint32(params, 0);
		}
	},
	glDetachShader: function (program, shader) {
		GL.detachShader(gl_obj(program), gl_obj(shader));
	},
	glDeleteProgram: function (program) {
		GL.deleteProgram(gl_del(program));
	},
	glClear: function (mask) {
		GL.clear(mask);
	},
	glClearColor: function (red, green, blue, alpha) {
		GL.clearColor(red, green, blue, alpha);
	},
	glGetError: function () {
		return GL.getError();
	},
	glGetShaderInfoLog: function () {
		// FIXME Logged automatically as part of glGetShaderiv
	},
	glFinish: function () {
		GL.finish();
	},
	glScissor: function (x, y, width, height) {
		GL.scissor(x, y, width, height);
	},
	glBlendFunc: function (sfactor, dfactor) {
		GL.blendFunc(sfactor, dfactor);
	},
	glBlendEquation: function (mode) {
		GL.blendEquation(mode);
	},
	glUniformMatrix4fv: function (loc, count, transpose, value) {
		GL.uniformMatrix4fv(gl_obj(loc), transpose, new Float32Array(mem(), value, 4 * 4 * count));
	},
	glBlendEquationSeparate: function (modeRGB, modeAlpha) {
		GL.blendEquationSeparate(modeRGB, modeAlpha);
	},
	glBlendFuncSeparate: function (srcRGB, dstRGB, srcAlpha, dstAlpha) {
		GL.blendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
	},
	glGetProgramiv: function (program, pname, params) {
		setUint32(params, GL.getProgramParameter(gl_obj(program), pname));
	},
};


// Matoya audio API
let AC = {};

const MTY_AUDIO_API = {
	MTY_AudioCreate: function (sampleRate) {
		AC.sample_rate = sampleRate;
		AC.playing = false;

		return 1; // In case the app checks for NULL
	},
	MTY_AudioDestroy: function (_) {
		setUint32(_, 0);
		AC = {};
	},
	MTY_AudioPlay: function () {
		AC.playing = true;
	},
	MTY_AudioStop: function () {
		AC.playing = false;
	},
	MTY_AudioQueue: function (_, frames, count) {
		if (!AC.ctx) {
			AC.ctx = new AudioContext();
			AC.next_time = AC.ctx.currentTime;
		}

		if (AC.next_time - AC.ctx.currentTime > 0.200)
			return;

		if (AC.next_time < AC.ctx.currentTime)
			AC.next_time = AC.ctx.currentTime + 0.070;

		const buf = AC.ctx.createBuffer(2, count, AC.sample_rate);
		const left = buf.getChannelData(0);
		const right = buf.getChannelData(1);

		const src = new Int16Array(mem(), frames);

		let offset = 0;
		for (let x = 0; x < count * 2; x += 2) {
			left[offset] = src[x] / 32768;
			right[offset] = src[x + 1] / 32768;
			offset++;
		}

		const buf_source = AC.ctx.createBufferSource();
		buf_source.buffer = buf;
		buf_source.connect(AC.ctx.destination);
		buf_source.start(AC.next_time);
		AC.next_time += buf.duration;
	},
	MTY_AudioIsPlaying: function () {
		return AC.playing;
	},
	MTY_AudioGetQueuedMs: function () {
		if (AC.ctx) {
			const queued_ms = Math.round((AC.next_time - AC.ctx.currentTime) * 1000.0);
		}
		return 0;
	},
};


// Matoya web API
let FRAME_CTR = 0;
let CURSOR_ID = 0;
let CURSOR_CACHE = {};
let CURSOR_STYLES = [];
let CURSOR_CLASS = '';
let USE_DEFAULT_CURSOR = false;
let GPS = [false, false, false, false];

function get_mods(ev) {
	let mods = 0;

	if (ev.shiftKey) mods |= 0x01;
	if (ev.ctrlKey)  mods |= 0x02;
	if (ev.altKey)   mods |= 0x04;
	if (ev.metaKey)  mods |= 0x08;

	if (ev.getModifierState("CapsLock")) mods |= 0x10;
	if (ev.getModifierState("NumLock") ) mods |= 0x20;

	return mods;
}

function poll_gamepads(app, controller) {
	const gps = navigator.getGamepads();

	for (let x = 0; x < 4; x++) {
		const gp = gps[x];

		if (gp) {
			let state = 0;

			// Connected
			if (!GPS[x]) {
				GPS[x] = true;
				state = 1;
			}

			let lx = 0;
			let ly = 0;
			let rx = 0;
			let ry = 0;
			let lt = 0;
			let rt = 0;
			let buttons = 0;

			if (gp.buttons) {
				lt = gp.buttons[6].value;
				rt = gp.buttons[7].value;

				for (let i = 0; i < gp.buttons.length && i < 32; i++)
					if (gp.buttons[i].pressed)
						buttons |= 1 << i;
			}

			if (gp.axes) {
				if (gp.axes[0]) lx = gp.axes[0];
				if (gp.axes[1]) ly = gp.axes[1];
				if (gp.axes[2]) rx = gp.axes[2];
				if (gp.axes[3]) ry = gp.axes[3];
			}

			MTY_CFunc(controller)(app, x, state, buttons, lx, ly, rx, ry, lt, rt);

		// Disconnected
		} else if (GPS[x]) {
			MTY_CFunc(controller)(app, x, 2, 0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

			GPS[x] = false;
		}
	}
}

const MTY_WEB_API = {
	// crypto
	MTY_RandomBytes: function (cbuf, size) {
		const buf = new Uint8Array(mem(), size);
		Crypto.getRandomValues(buf);
	},

	// unistd
	gethostname: function(cbuf, size) {
		js_to_c(cbuf, location.hostname);
	},
	flock: function(fd, flags) {
		return 0;
	},

	// browser
	web_set_mem_funcs: function (alloc, free) {
		MTY_ALLOC = alloc;
		MTY_FREE = free;
	},
	web_rumble_gamepad: function (id, low, high) {
		const gps = navigator.getGamepads();
		const gp = gps[id];

		if (gp && gp.vibrationActuator)
			gp.vibrationActuator.playEffect('dual-rumble', {
				startDelay: 0,
				duration: 2000,
				weakMagnitude: low,
				strongMagnitude: high,
			});
	},
	web_show_cursor: function (show) {
		GL.canvas.style.cursor = show ? '': 'none';
	},
	web_set_clipboard_text: function (text_c) {
		navigator.clipboard.writeText(c_to_js(text_c));
	},
	web_get_clipboard_text: function () {
		navigator.clipboard.readText().then(text => {
			const text_c = MTY_Alloc(text.length * 4);

			js_to_c(text, text_c);
			console.log('web_get_clipboard_text:', text);

			MTY_Free(text_c);
		});

		return 0;
	},
	web_set_pointer_lock: function (enable) {
		if (enable && !document.pointerLockElement) {
			GL.canvas.requestPointerLock();

		} else if (!enable && document.pointerLockElement) {
			document.exitPointerLock();
		}
	},
	web_get_pointer_lock: function () {
		return document.pointerLockElement ? true : false;
	},
	web_has_focus: function () {
		return document.hasFocus();
	},
	web_is_visible: function () {
		if (document.hidden != undefined) {
			return !document.hidden;

		} else if (document.webkitHidden != undefined) {
			return !document.webkitHidden;
		}

		return true;
	},
	web_get_size: function (c_width, c_height) {
		setUint32(c_width, GL.drawingBufferWidth);
		setUint32(c_height, GL.drawingBufferHeight);
	},
	web_get_screen_size: function (c_width, c_height) {
		setUint32(c_width, screen.width);
		setUint32(c_height, screen.height);
	},
	web_set_title: function (title) {
		document.title = c_to_js(title);
	},
	web_use_default_cursor: function (use_default) {
		if (CURSOR_CLASS.length > 0) {
			if (use_default) {
				GL.canvas.classList.remove(CURSOR_CLASS);

			} else {
				GL.canvas.classList.add(CURSOR_CLASS);
			}
		}

		USE_DEFAULT_CURSOR = use_default;
	},
	web_set_png_cursor: function (buffer, size, hot_x, hot_y) {
		if (buffer) {
			const buf = new Uint8Array(mem(), buffer, size);
			const b64_png = buf_to_b64(buf);

			if (!CURSOR_CACHE[b64_png]) {
				CURSOR_CACHE[b64_png] = `cursor-x-${CURSOR_ID}`;

				const style = document.createElement('style');
				style.type = 'text/css';
				style.innerHTML = `.cursor-x-${CURSOR_ID++} ` +
					`{cursor: url(data:image/png;base64,${b64_png}) ${hot_x} ${hot_y}, auto;}`;
				document.querySelector('head').appendChild(style);

				CURSOR_STYLES.push(style);
			}

			if (CURSOR_CLASS.length > 0)
				GL.canvas.classList.remove(CURSOR_CLASS);

			CURSOR_CLASS = CURSOR_CACHE[b64_png];

			if (!USE_DEFAULT_CURSOR)
				GL.canvas.classList.add(CURSOR_CLASS);

		} else {
			if (!USE_DEFAULT_CURSOR && CURSOR_CLASS.length > 0)
				GL.canvas.classList.remove(CURSOR_CLASS);

			CURSOR_CLASS = '';
		}
	},
	web_get_pixel_ratio: function () {
		// FIXME Currently the browser scales the canvas to handle DPI, need
		// to figure out window.devicePixelRatio
		return 1.0;
	},
	web_create_canvas: function () {
		const html = document.querySelector('html');
		html.style.width = '100%';
		html.style.height = '100%';
		html.style.margin = 0;

		const body = document.querySelector('body');
		body.style.width = '100%';
		body.style.height = '100%';
		body.style.background = 'black';
		body.style.overflow = 'hidden';
		body.style.margin = 0;

		const canvas = document.createElement('canvas');
		document.body.appendChild(canvas);

		GL = canvas.getContext('webgl', {depth: 0, antialias: 0, premultipliedAlpha: true});
	},
	web_attach_events: function (app, mouse_motion, mouse_button, mouse_wheel, keyboard, focus, drop) {
		// A static buffer for copying javascript strings to C
		const cbuf0 = MTY_Alloc(1024);
		const cbuf1 = MTY_Alloc(16);

		GL.canvas.addEventListener('mousemove', (ev) => {
			let relative = document.pointerLockElement ? true : false;
			let x = ev.clientX;
			let y = ev.clientY;

			if (relative) {
				x = ev.movementX;
				y = ev.movementY;
			}

			MTY_CFunc(mouse_motion)(app, relative, x, y);
		});

		GL.canvas.addEventListener('mousedown', (ev) => {
			ev.preventDefault();
			MTY_CFunc(mouse_button)(app, true, ev.button, ev.clientX, ev.clientY);
		});

		GL.canvas.addEventListener('mouseup', (ev) => {
			ev.preventDefault();
			MTY_CFunc(mouse_button)(app, false, ev.button, ev.clientX, ev.clientY);
		});

		GL.canvas.addEventListener('contextmenu', (ev) => {
			ev.preventDefault();
		});

		GL.canvas.addEventListener('wheel', (ev) => {
			let x = ev.deltaX > 0 ? 120 : ev.deltaX < 0 ? -120 : 0;
			let y = ev.deltaY > 0 ? 120 : ev.deltaY < 0 ? -120 : 0;
			MTY_CFunc(mouse_wheel)(app, x, y);
		}, {passive: true});

		window.addEventListener('keydown', (ev) => {
			const text = ev.key.length == 1 ? js_to_c(ev.key, cbuf1) : 0;
			if (MTY_CFunc(keyboard)(app, true, ev.keyCode, js_to_c(ev.code, cbuf0), text, get_mods(ev)))
				ev.preventDefault();
		});

		window.addEventListener('keyup', (ev) => {
			if (MTY_CFunc(keyboard)(app, false, ev.keyCode, js_to_c(ev.code, cbuf0), 0, get_mods(ev)))
				ev.preventDefault();
		});

		GL.canvas.addEventListener('dragover', (ev) => {
			ev.preventDefault();
		});

		window.addEventListener('blur', (ev) => {
			MTY_CFunc(focus)(app, false);
		});

		window.addEventListener('focus', (ev) => {
			MTY_CFunc(focus)(app, true);
		});

		GL.canvas.addEventListener('drop', (ev) => {
			ev.preventDefault();

			if (!ev.dataTransfer.items)
				return;

			for (let x = 0; x < ev.dataTransfer.items.length; x++) {
				if (ev.dataTransfer.items[x].kind == 'file') {
					let file = ev.dataTransfer.items[x].getAsFile();

					const reader = new FileReader();
					reader.addEventListener('loadend', (fev) => {
						if (reader.readyState == 2) {
							let buf = new Uint8Array(reader.result);
							let cmem = MTY_Alloc(buf.length);
							copy(cmem, buf);
							MTY_CFunc(drop)(app, js_to_c(file.name, cbuf0), cmem, buf.length);
							MTY_Free(cmem);
						}
					});
					reader.readAsArrayBuffer(file);
					break;
				}
			}
		});
	},
	web_raf: function (app, func, controller, opaque) {
		const step = () => {
			// TODO This number will affect the "swap interval"
			if (++FRAME_CTR % 2 == 0) {
				poll_gamepads(app, controller);

				GL.canvas.width = window.innerWidth;
				GL.canvas.height = window.innerHeight;

				// TODO check return value, call cleanup routine on false (web_close)
				MTY_CFunc(func)(opaque);
			}

			window.requestAnimationFrame(step);
		};

		window.requestAnimationFrame(step);
		throw 'MTY_AppRun halted execution';
	},
};


// WASI API

// https://github.com/WebAssembly/WASI/blob/master/phases/snapshot/docs.md

const FDS = {};
let FD_NUM = 64;
let FD_PREOPEN = false;

function append_buf_to_b64(b64, buf)
{
	// FIXME This is a crude way to handle appending to an open file,
	// complex seek operations will break this

	const cur_buf = b64_to_buf(b64);
	const new_buf = new Uint8Array(cur_buf.length + buf.length);

	new_buf.set(cur_buf);
	new_buf.set(buf, cur_buf.length);

	return buf_to_b64(new_buf);
}

const WASI_API = {
	// Command line arguments
	args_get: function () {
		console.log('args_get', arguments);
		return 0;
	},
	args_sizes_get: function (argc, argv_buf_size) {
		console.log('args_sizes_get', arguments);
		setUint32(argc, 0);
		setUint32(argv_buf_size, 0);
		return 0;
	},


	// WASI preopened directory (/)
	fd_prestat_get: function (fd, path) {
		return !FD_PREOPEN ? 0 : 8;
	},
	fd_prestat_dir_name: function (fd, path, path_len) {
		if (!FD_PREOPEN) {
			js_to_c('/', path);
			FD_PREOPEN = true;

			return 0;
		}

		return 28;
	},

	// Paths
	path_filestat_get: function (fd, flags, cpath, _0, filestat_out) {
		const path = c_to_js(cpath);
		if (localStorage[path]) {
			// We only need to return the size
			const buf = b64_to_buf(localStorage[path]);
			setUint64(filestat_out + 32, buf.byteLength);
		}

		return 0;
	},
	path_open: function (fd, dir_flags, path, o_flags, _0, _1, _2, mode, fd_out) {
		const new_fd = FD_NUM++;
		setUint32(fd_out, new_fd);

		FDS[new_fd] = {
			path: c_to_js(path),
			append: mode == 1,
			offset: 0,
		};

		return 0;
	},
	path_create_directory: function () {
		return 0;
	},
	path_remove_directory: function () {
		console.log('path_remove_directory', arguments);
		return 0;
	},
	path_unlink_file: function () {
		console.log('path_unlink_file', arguments);
		return 0;
	},
	path_readlink: function () {
		console.log('path_readlink', arguments);
	},

	// File descriptors
	fd_close: function (fd) {
		delete FDS[fd];
	},
	fd_fdstat_get: function () {
		return 0;
	},
	fd_fdstat_set_flags: function () {
		console.log('fd_fdstat_set_flags', arguments);
	},
	fd_readdir: function () {
		console.log('fd_readdir', arguments);
		return 8;
	},
	fd_seek: function (fd, offset, whence, offset_out) {
		return 0;
	},
	fd_read: function (fd, iovs, iovs_len, nread) {
		const finfo = FDS[fd];

		if (finfo && localStorage[finfo.path]) {
			const full_buf = b64_to_buf(localStorage[finfo.path]);

			let ptr = iovs;
			let cbuf = getUint32(ptr);
			let cbuf_len = getUint32(ptr + 4);
			let len = cbuf_len < full_buf.length ? cbuf_len : full_buf.length;

			let view = new Uint8Array(mem(), cbuf, cbuf_len);
			let slice = new Uint8Array(full_buf.buffer, 0, len);
			view.set(slice);

			setUint32(nread, len);
		}

		return 0;
	},
	fd_write: function (fd, iovs, iovs_len, nwritten) {
		// Calculate full write size
		let len = 0;
		for (let x = 0; x < iovs_len; x++)
			len += getUint32(iovs + x * 8 + 4);

		setUint32(nwritten, len);

		// Create a contiguous buffer
		let offset = 0;
		let full_buf = new Uint8Array(len);
		for (let x = 0; x < iovs_len; x++) {
			let ptr = iovs + x * 8;
			let cbuf = getUint32(ptr);
			let cbuf_len = getUint32(ptr + 4);

			full_buf.set(new Uint8Array(mem(), cbuf, cbuf_len), offset);
			offset += cbuf_len;
		}

		// stdout
		if (fd == 1) {
			console.log(char_to_js(full_buf));

		// stderr
		} else if (fd == 2) {
			console.error(char_to_js(full_buf));

		// Filesystem
		} else if (FDS[fd]) {
			const finfo = FDS[fd];
			const cur_b64 = localStorage[finfo.path];

			if (cur_b64 && finfo.append) {
				localStorage[finfo.path] = append_buf_to_b64(cur_b64, full_buf);

			} else {
				localStorage[finfo.path] = buf_to_b64(full_buf, len);
			}

			finfo.offet += len;
		}

		return 0;
	},

	// Misc
	clock_time_get: function (id, precision, time_out) {
		setUint64(time_out, Math.trunc(performance.now() * 1000.0 * 1000.0));
		return 0;
	},
	poll_oneoff: function (sin, sout, nsubscriptions, nevents) {
		console.log('poll_oneoff', arguments);
		return 0;
	},
	proc_exit: function () {
		console.log('proc_exit', arguments);
	},
};


// Entry

async function MTY_Start(bin, userEnv) {
	if (!userEnv)
		userEnv = {};

	// Fetch the wasm file as an ArrayBuffer
	const res = await fetch(bin);
	const buf = await res.arrayBuffer();

	// Create wasm instance (module) from the ArrayBuffer
	MODULE = await WebAssembly.instantiate(buf, {
		// Custom imports
		env: {
			...GL_API,
			...MTY_AUDIO_API,
			...MTY_WEB_API,
			...userEnv,
		},

		// Current version of WASI we're compiling against, 'wasi_snapshot_preview1'
		wasi_snapshot_preview1: {
			...WASI_API,
		},
	});

	// Execute the '_start' entry point, this will fetch args and execute the 'main' function
	MODULE.instance.exports._start();
}
