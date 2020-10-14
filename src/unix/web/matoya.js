// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Helpers

let MODULE;

function mem() {
	return MODULE.instance.exports.memory.buffer;
}

function mem_view() {
	return new DataView(mem());
}

function setUint32(ptr, value) {
	mem_view().setUint32(ptr, value, true);
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

function func_ptr(ptr) {
	return MODULE.instance.exports.__indirect_function_table.get(ptr);
}

function copy(cptr, abuffer) {
	let heap = new Uint8Array(mem(), cptr);
	heap.set(abuffer);
}

function c_to_js(ptr) {
	const view = new Uint8Array(mem(), ptr);
	let str = '';

	for (let x = 0; x < 0x7FFFFFFF; x++) {
		if (view[x] == 0)
			break;

		str += String.fromCharCode(view[x]);
	}

	return str;
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


// GL

let GL;
let GL_OBJ_INDEX = 0;
let GL_BOUND_TEX = 0;
let GL_OBJ = {};
let GL_TEX = {};

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
		GL_BOUND_TEX = texture;
	},
	glDeleteTextures: function (n, ids) {
		for (let x = 0; x < n; x++)
			GL.deleteTexture(gl_del(getUint32(ids + x * 4)));
	},
	glGetTexLevelParameteriv: function (target, level, pname, params) {
		switch (pname) {
			case 0x1000: // Width
				setUint32(params, GL_TEX[GL_BOUND_TEX].x);
				break;
			case 0x1001: // Height
				setUint32(params, GL_TEX[GL_BOUND_TEX].y);
				break;
		}
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

		if (!GL_TEX[GL_BOUND_TEX])
			GL_TEX[GL_BOUND_TEX] = {};

		GL_TEX[GL_BOUND_TEX].x = width;
		GL_TEX[GL_BOUND_TEX].y = height;
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
	MTY_AudioGetQueuedFrames: function () {
		if (AC.ctx) {
			const queued_ms = Math.round((AC.next_time - AC.ctx.currentTime) * 1000.0);
		}
		return 0;
	},
};


// Matoya web API
let FRAME_CTR = 0;

const MTY_WEB_API = {
	web_get_size: function (c_width, c_height) {
		setUint32(c_width, GL.drawingBufferWidth);
		setUint32(c_height, GL.drawingBufferHeight);
	},
	web_resize_canvas: function () {
		GL.canvas.width = window.innerWidth;
		GL.canvas.height = window.innerHeight;
	},
	web_set_title: function (title) {
		document.title = c_to_js(title);
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

		GL = canvas.getContext('webgl2', {depth: 0, antialias: 0, premultipliedAlpha: false});
	},
	web_attach_events: function (w, malloc, free, mouse_motion, mouse_button, mouse_wheel, keyboard, drop) {
		// A static buffer for copying javascript strings to C
		const cbuf = func_ptr(malloc)(1024);

		GL.canvas.addEventListener('mousemove', (ev) => {
			func_ptr(mouse_motion)(w, ev.clientX, ev.clientY);
		});

		GL.canvas.addEventListener('mousedown', (ev) => {
			ev.preventDefault();
			func_ptr(mouse_button)(w, true, ev.which);
		});

		GL.canvas.addEventListener('mouseup', (ev) => {
			ev.preventDefault();
			func_ptr(mouse_button)(w, false, ev.which);
		});

		GL.canvas.addEventListener('contextmenu', (ev) => {
			ev.preventDefault();
		});

		GL.canvas.addEventListener('wheel', (ev) => {
			func_ptr(mouse_wheel)(w, false, ev.deltaX, ev.deltaY);
		}, {passive: true});

		window.addEventListener('keydown', (ev) => {
			func_ptr(keyboard)(w, true, js_to_c(ev.code, cbuf));
		});

		window.addEventListener('keyup', (ev) => {
			func_ptr(keyboard)(w, false, js_to_c(ev.code, cbuf));
		});

		GL.canvas.addEventListener('dragover', (ev) => {
			ev.preventDefault();
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
							let cmem = func_ptr(malloc)(buf.length);
							copy(cmem, buf);
							func_ptr(drop)(w, js_to_c(file.name, cbuf), cmem, buf.length);
							func_ptr(free)(cmem);
						}
					});
					reader.readAsArrayBuffer(file);
					break;
				}
			}
		});
	},
	web_raf: function (func, opaque) {
		const step = () => {
			// TODO This number will affect the "swap interval"
			if (++FRAME_CTR % 1 == 0)
				func_ptr(func)(opaque);

			window.requestAnimationFrame(step);
		};

		window.requestAnimationFrame(step);
		throw 'MTY_AppRun halted execution';
	},
};


// WASI API

const WASI_API = {
	args_get: function () {
		return 0;
	},
	args_sizes_get: function (argc, argv_buf_size) {
		setUint32(argc, 0);
		setUint32(argv_buf_size, 0);
		return 0;
	},
	clock_time_get: function (id, precision, time_out) {
		setUint64(time_out, Math.trunc(performance.now() * 1000.0 * 1000.0));
		return 0;
	},
	fd_close: function () {
	},
	fd_fdstat_get: function () {
	},
	fd_fdstat_set_flags: function () {
	},
	fd_prestat_dir_name: function (fd, path, path_len) {
		return 28;
	},
	fd_prestat_get: function (fd, path) {
		return 8;
	},
	fd_read: function () {
	},
	fd_readdir: function () {
	},
	fd_seek: function () {
	},
	fd_write: function (fd, iovs, iovs_len, nwritten) {
		let buffers = Array.from({length: iovs_len}, function (_, i) {
		   let ptr = iovs + i * 8;
		   let buf = getUint32(ptr);
		   let bufLen = getUint32(ptr + 4);

		   return new Uint8Array(mem(), buf, bufLen);
		});

		let written = 0;
		let bufferBytes = [];
		for (let x = 0; x < buffers.length; x++) {
			for (var b = 0; b < buffers[x].byteLength; b++)
			   bufferBytes.push(buffers[x][b]);

			written += buffers[x].byteLength;
		}

		if (fd == 1)
			console.log(String.fromCharCode.apply(null, bufferBytes));

		if (fd == 2)
			console.error(String.fromCharCode.apply(null, bufferBytes));

		setUint32(nwritten, written);

		return 0;
	},
	path_create_directory: function () {
	},
	path_filestat_get: function () {
	},
	path_open: function () {
	},
	path_readlink: function () {
	},
	poll_oneoff: function (sin, sout, nsubscriptions, nevents) {
		return 0;
	},
	proc_exit: function () {
	},
};


// Entry

export async function MTY_Start(bin) {
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
		},

		// Current version of WASI we're compiling against, 'wasi_snapshot_preview1'
		wasi_snapshot_preview1: {
			...WASI_API,
		},
	});

	// Execute the '_start' entry point, this will fetch args and execute the 'main' function
	MODULE.instance.exports._start();
}
