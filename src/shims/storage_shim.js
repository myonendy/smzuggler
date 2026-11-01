/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 * Copyright (C) 2026 The smzuggler contributors
 * Modified 2026-10-31: throw MODULE_NOT_FOUND; Chrome/NW.js argv, cwd, exit.
 */

/*
 * Storage shim: localStorage, localforage, Node-style require("fs"/"path"),
 * process and nw stubs, all backed by native file I/O under save/.
 */

(function() {
    "use strict";

    /* localStorage: persisted as a single JSON file. */

    var STORAGE_FILE = "save/localStorage.json";
    var _storageData = {};
    var _storageDirty = false;

    (function() {
        try {
            if (__native_io.existsSync(STORAGE_FILE)) {
                var raw = __native_io.readFileSync(STORAGE_FILE);
                if (raw) {
                    _storageData = JSON.parse(raw);
                }
            }
        } catch (e) {
            _storageData = {};
        }
    })();

    function _flushStorage() {
        if (!_storageDirty) return;
        try {
            if (!__native_io.existsSync("save")) {
                __native_io.mkdirSync("save");
            }
            __native_io.writeFileSync(STORAGE_FILE, JSON.stringify(_storageData));
            _storageDirty = false;
        } catch (e) {
            console.warn("localStorage: flush failed: " + e);
        }
    }

    var localStorageShim = {
        getItem: function(key) {
            if (key in _storageData) {
                return String(_storageData[key]);
            }
            return null;
        },

        setItem: function(key, value) {
            _storageData[String(key)] = String(value);
            _storageDirty = true;
            _flushStorage();
        },

        removeItem: function(key) {
            delete _storageData[String(key)];
            _storageDirty = true;
            _flushStorage();
        },

        clear: function() {
            _storageData = {};
            _storageDirty = true;
            _flushStorage();
        },

        key: function(index) {
            var keys = Object.keys(_storageData);
            return index < keys.length ? keys[index] : null;
        },

        get length() {
            return Object.keys(_storageData).length;
        }
    };

    globalThis.localStorage = localStorageShim;

    /* localforage: one file per key in save/, plus a key map for listing. */

    var KEYMAP_FILE = "save/_localforage_keymap.json";
    var _forageKeyMap = {};  /* sanitized filename suffix -> original key */

    (function() {
        try {
            if (__native_io.existsSync(KEYMAP_FILE)) {
                var raw = __native_io.readFileSync(KEYMAP_FILE);
                if (raw) {
                    _forageKeyMap = JSON.parse(raw);
                }
            }
        } catch (e) {
            _forageKeyMap = {};
        }
    })();

    function _flushKeyMap() {
        try {
            if (!__native_io.existsSync("save")) {
                __native_io.mkdirSync("save");
            }
            __native_io.writeFileSync(KEYMAP_FILE, JSON.stringify(_forageKeyMap));
        } catch (e) {
            console.warn("localforage: keymap flush failed: " + e);
        }
    }

    function _forageSanitize(key) {
        /* Injective encoding: unsafe chars (including "_") become _XXXX with
           fixed-width hex so encoded and literal characters cannot collide. */
        return String(key).replace(/[^a-zA-Z0-9.\-]/g, function(ch) {
            var code = ch.charCodeAt(0);
            return "_" + ("0000" + code.toString(16)).slice(-4);
        });
    }

    function _forageFilePath(key) {
        return "save/forage_" + _forageSanitize(key);
    }

    var localforageShim = {
        setItem: function(key, value) {
            return new Promise(function(resolve, reject) {
                try {
                    if (!__native_io.existsSync("save")) {
                        __native_io.mkdirSync("save");
                    }
                    var path = _forageFilePath(key);
                    __native_io.writeFileSync(path, String(value));
                    _forageKeyMap[_forageSanitize(key)] = String(key);
                    _flushKeyMap();
                    resolve(value);
                } catch (e) {
                    reject(e);
                }
            });
        },

        getItem: function(key) {
            return new Promise(function(resolve, reject) {
                try {
                    var path = _forageFilePath(key);
                    if (__native_io.existsSync(path)) {
                        var data = __native_io.readFileSync(path);
                        resolve(data);
                    } else {
                        resolve(null);
                    }
                } catch (e) {
                    reject(e);
                }
            });
        },

        removeItem: function(key) {
            return new Promise(function(resolve, reject) {
                try {
                    var path = _forageFilePath(key);
                    if (__native_io.existsSync(path)) {
                        __native_io.unlinkSync(path);
                    }
                    delete _forageKeyMap[_forageSanitize(key)];
                    _flushKeyMap();
                    resolve();
                } catch (e) {
                    reject(e);
                }
            });
        },

        keys: function() {
            return new Promise(function(resolve, reject) {
                try {
                    if (!__native_io.existsSync("save")) {
                        resolve([]);
                        return;
                    }
                    var entries = __native_io.readdirSync("save");
                    var prefix = "forage_";
                    var result = [];
                    for (var i = 0; i < entries.length; i++) {
                        if (entries[i].indexOf(prefix) === 0) {
                            var sanitized = entries[i].substring(prefix.length);
                            var original = _forageKeyMap[sanitized];
                            result.push(original !== undefined ? original : sanitized);
                        }
                    }
                    resolve(result);
                } catch (e) {
                    reject(e);
                }
            });
        },

        length: function() {
            return this.keys().then(function(keys) {
                return keys.length;
            });
        },

        clear: function() {
            return this.keys().then(function(keys) {
                var promises = [];
                for (var i = 0; i < keys.length; i++) {
                    promises.push(localforageShim.removeItem(keys[i]));
                }
                return Promise.all(promises);
            });
        }
    };

    globalThis.localforage = localforageShim;

    /* require("fs") */

    var fsModule = {
        existsSync: function(path) {
            return __native_io.existsSync(path);
        },

        mkdirSync: function(path, options) {
            __native_io.mkdirSync(path);
        },

        readFileSync: function(path, options) {
            /* The encoding option is accepted but files are always read as text. */
            var encoding = null;
            if (options && typeof options === "object") {
                encoding = options.encoding;
            } else if (typeof options === "string") {
                encoding = options;
            }
            return __native_io.readFileSync(path);
        },

        writeFileSync: function(path, data, options) {
            __native_io.writeFileSync(path, String(data));
        },

        unlinkSync: function(path) {
            __native_io.unlinkSync(path);
        },

        readdirSync: function(path) {
            return __native_io.readdirSync(path);
        },

        renameSync: function(oldPath, newPath) {
            __native_io.renameSync(oldPath, newPath);
        }
    };

    /* require("path") */

    var pathModule = {
        join: function() {
            var parts = [];
            for (var i = 0; i < arguments.length; i++) {
                var part = String(arguments[i]);
                if (part) parts.push(part);
            }
            var result = parts.join("/");
            result = result.replace(/\/+/g, "/");
            /* Like Node, a trailing separator survives: rmmz_core builds the
               save directory as path.join(base, "save/") and appends the
               file name to it. */
            return result;
        },

        dirname: function(p) {
            if (!p) return ".";
            p = String(p);
            var normalized = p.replace(/\\/g, "/");
            var lastSlash = normalized.lastIndexOf("/");
            if (lastSlash === -1) return ".";
            if (lastSlash === 0) return "/";
            return p.substring(0, lastSlash);
        },

        basename: function(p, ext) {
            if (!p) return "";
            p = String(p);
            var normalized = p.replace(/\\/g, "/");
            if (normalized.length > 1 && normalized[normalized.length - 1] === "/") {
                normalized = normalized.substring(0, normalized.length - 1);
            }
            var lastSlash = normalized.lastIndexOf("/");
            var name = lastSlash >= 0 ? normalized.substring(lastSlash + 1) : normalized;
            if (ext && name.length >= ext.length &&
                name.substring(name.length - ext.length) === ext) {
                name = name.substring(0, name.length - ext.length);
            }
            return name;
        },

        resolve: function() {
            var resolved = "";
            for (var i = arguments.length - 1; i >= 0; i--) {
                var part = String(arguments[i]);
                if (!part) continue;
                if (resolved) {
                    resolved = part + "/" + resolved;
                } else {
                    resolved = part;
                }
                if (part[0] === "/" || (part.length >= 2 && part[1] === ":")) {
                    break;
                }
            }
            resolved = resolved.replace(/\/+/g, "/");
            if (resolved.length > 1 && resolved[resolved.length - 1] === "/") {
                resolved = resolved.substring(0, resolved.length - 1);
            }
            return resolved;
        },

        sep: "/",
        delimiter: ":"
    };

    /* require() */

    var _modules = {
        "fs": fsModule,
        "path": pathModule
    };

    /* Node require: load fs, path, or nw.gui (the nw object). Unknown names
     * throw MODULE_NOT_FOUND so plugins feature-detect with try/catch.
     * moduleName is ToString'd like Node; returns the module object or throws.
     * Compat require; https://nodejs.org/api/modules.html#requireid */
    globalThis.require = function(moduleName) {
        var name = String(moduleName);
        if (_modules[name]) return _modules[name];
        if (name === "nw.gui" && typeof globalThis.nw === "object" && globalThis.nw) {
            return globalThis.nw;
        }
        var err = new Error("Cannot find module '" + name + "'");
        err.code = "MODULE_NOT_FOUND";
        throw err;
    };

    /* process: used by Utils.isNwjs(), StorageManager and save-location
     * plugins, which build paths from process.platform and process.env.
     * Host facts come from __native_process (bind_platform). */

    var _nativeProcess = typeof __native_process !== "undefined" ? __native_process : null;

    /* Reference NW.js build (0.80.0) and the Node and Chromium it ships. */
    function _processVersions() {
        var v = { node: "20.7.0", nw: "0.80.0", chromium: "117.0.5938.63" };
        if (_nativeProcess) v.quickjs = _nativeProcess.engine;
        return v;
    }

    /* Monotonic [seconds, nanoseconds]; without the native clock, falls
     * back to Date.now(), which has only millisecond resolution. */
    function _hrtimeNow() {
        if (_nativeProcess) return _nativeProcess.hrtime();
        var ms = Date.now();
        return [Math.floor(ms / 1000), (ms % 1000) * 1000000];
    }

    /* Node's process.hrtime([prev]): with prev, the time elapsed since it. */
    function _hrtime(prev) {
        var now = _hrtimeNow();
        if (prev === undefined) return now;
        if (!(prev instanceof Array) || prev.length !== 2) {
            throw new TypeError("process.hrtime: argument must be an array of 2 numbers");
        }
        var s = now[0] - prev[0];
        var ns = now[1] - prev[1];
        if (ns < 0) {
            s -= 1;
            ns += 1000000000;
        }
        return [s, ns];
    }

    if (typeof BigInt === "function") {
        _hrtime.bigint = function() {
            var t = _hrtimeNow();
            return BigInt(t[0]) * BigInt(1000000000) + BigInt(t[1]);
        };
    }

    /* process.nextTick: callbacks run as one microtask, in order, before
     * timers. Ticks queued while draining run in the same drain, ahead of
     * Promise jobs queued meanwhile, as in Node. A Promise job queued before
     * the first tick of a turn still runs first (Node would run the tick). */
    var _tickQueue = [];
    var _tickScheduled = false;

    function _drainTicks() {
        try {
            while (_tickQueue.length > 0) {
                var tick = _tickQueue.shift();
                tick.fn.apply(undefined, tick.args);
            }
        } catch (e) {
            /* Surface as an uncaught error, not a swallowed rejection. */
            setTimeout(function() { throw e; }, 0);
        } finally {
            _tickScheduled = false;
            if (_tickQueue.length > 0) _scheduleTicks();
        }
    }

    function _scheduleTicks() {
        if (_tickScheduled) return;
        _tickScheduled = true;
        Promise.resolve().then(_drainTicks);
    }

    function _nextTick(fn) {
        if (typeof fn !== "function") {
            throw new TypeError("process.nextTick: callback must be a function");
        }
        _tickQueue.push({ fn: fn, args: Array.prototype.slice.call(arguments, 1) });
        _scheduleTicks();
    }

    /* Join the game root and a file name for process.mainModule.filename.
     * Empty root yields the game-relative name, matching NW.js when cwd is
     * the game folder. name is a basename; returns a new string. */
    function _joinRoot(name) {
        var root = "";
        if (typeof __native_io !== "undefined" && __native_io.getGameRoot) {
            root = String(__native_io.getGameRoot() || "");
        }
        if (!root) return name;
        var last = root.charAt(root.length - 1);
        if (last === "/" || last === "\\") return root + name;
        return root + "/" + name;
    }

    /* Chrome/NW.js process.argv: argv[0] is the chromium/nw binary name,
     * argv[1] is the app. NW.js launched from the game root reports the
     * main HTML as a game-relative path. Returns a new array each call.
     * Compat process.argv; https://nodejs.org/api/process.html#processargv */
    function _processArgv() {
        return ["chrome", "index.html"];
    }

    /* Node process.exit(code): record exitCode and take the same clean quit
     * path as nw.App.quit() (SDL quit, no abort). code is ToInt32; omitted
     * code keeps a numeric exitCode or becomes 0. Main-thread only.
     * Compat process.exit; https://nodejs.org/api/process.html#processexitcode */
    function _processExit(code) {
        var p = globalThis.process;
        if (arguments.length > 0) {
            p.exitCode = (code === undefined || code === null) ? 0 : (code | 0);
        } else if (typeof p.exitCode !== "number") {
            p.exitCode = 0;
        }
        _nwQuit();
    }

    /* Node process.cwd(): the game root, or "." when none is set.
     * Live value from __native_io; main-thread only; never throws.
     * Compat process.cwd; https://nodejs.org/api/process.html#processcwd */
    function _processCwd() {
        if (typeof __native_io !== "undefined" && __native_io.getGameRoot) {
            return __native_io.getGameRoot() || ".";
        }
        return ".";
    }

    if (typeof globalThis.process === "undefined") {
        globalThis.process = {
            mainModule: {
                /* StorageManager.fileDirectoryPath() derives save/ from this.
                 * Spec §5.3: `<root>/index.html`; game-relative when root is empty. */
                filename: _joinRoot("index.html")
            },
            /* Real OS/CPU in Node's spelling; env is a start-up snapshot. */
            platform: _nativeProcess ? _nativeProcess.platform : "linux",
            arch: _nativeProcess ? _nativeProcess.arch : "x64",
            env: _nativeProcess ? _nativeProcess.environ() : {},
            versions: _processVersions(),
            hrtime: _hrtime,
            nextTick: _nextTick,
            argv: _processArgv(),
            exit: _processExit,
            cwd: _processCwd
        };
    }

    /* indexedDB stub for Utils.canUseIndexedDB(). */

    if (typeof globalThis.indexedDB === "undefined") {
        globalThis.indexedDB = {
            open: function() {
                return { onupgradeneeded: null, onsuccess: null, onerror: null };
            }
        };
    }

    /* nw (NW.js) window/app stubs. */

    /* Minimal Node-style EventEmitter for nw.Window.get(). Plugins such as
     * CleanExit call removeAllListeners("close") and then on("close", fn). */
    function _nwAddListener(win, name, fn, once) {
        if (typeof fn !== "function") {
            throw new TypeError("nw.Window listener for '" + name + "' must be a function");
        }
        if (!win._events.hasOwnProperty(name)) win._events[name] = [];
        win._events[name].push({ fn: fn, once: once });
        return win;
    }

    function _nwDropEntry(win, name, entry) {
        var list = win._events.hasOwnProperty(name) ? win._events[name] : [];
        var i = list.indexOf(entry);
        if (i >= 0) list.splice(i, 1);
        if (list.length === 0) delete win._events[name];
    }

    function _nwSetFullscreen(win, on) {
        win.isFullscreen = on;
        if (typeof __native_platform !== "undefined" && __native_platform.setFullscreen) {
            __native_platform.setFullscreen(on);
        }
    }

    /* Set once the app has decided to quit, so the SDL quit event that
     * __native_platform.quit() pushes is not turned into another "close". */
    var _nwQuitRequested = false;

    function _nwQuit() {
        _nwQuitRequested = true;
        if (typeof __native_platform !== "undefined") __native_platform.quit();
    }

    /* Called by the game loop on SDL quit or window close. Emits "close" on
     * the window like NW.js; returns true when the loop should stop, i.e. the
     * app already asked to quit or nobody listens for "close". */
    function _nwHostClose() {
        var win = globalThis.nw && globalThis.nw.Window.get();
        if (_nwQuitRequested || !win || typeof win.listeners !== "function") return true;
        if (win.listeners("close").length === 0) return true;
        win.emit("close");
        return _nwQuitRequested;
    }

    if (typeof globalThis.nw === "undefined") {
        var _nwWindowInstance = {
            _events: {},
            on: function(name, fn) { return _nwAddListener(this, name, fn, false); },
            addListener: function(name, fn) { return _nwAddListener(this, name, fn, false); },
            once: function(name, fn) { return _nwAddListener(this, name, fn, true); },
            removeListener: function(name, fn) {
                /* As in Node: drop only the most recently added match. */
                var list = this._events.hasOwnProperty(name) ? this._events[name] : null;
                if (!list) return this;
                for (var i = list.length - 1; i >= 0; i--) {
                    if (list[i].fn === fn) {
                        list.splice(i, 1);
                        break;
                    }
                }
                if (list.length === 0) delete this._events[name];
                return this;
            },
            off: function(name, fn) { return this.removeListener(name, fn); },
            removeAllListeners: function(name) {
                if (arguments.length === 0 || name === undefined) {
                    this._events = {};
                } else {
                    delete this._events[name];
                }
                return this;
            },
            emit: function(name) {
                var list = this._events.hasOwnProperty(name) ? this._events[name].slice() : [];
                var args = Array.prototype.slice.call(arguments, 1);
                for (var i = 0; i < list.length; i++) {
                    if (list[i].once) _nwDropEntry(this, name, list[i]);
                    list[i].fn.apply(this, args);
                }
                return list.length > 0;
            },
            listeners: function(name) {
                var list = this._events.hasOwnProperty(name) ? this._events[name] : [];
                var out = [];
                for (var i = 0; i < list.length; i++) out.push(list[i].fn);
                return out;
            },
            listenerCount: function(name) { return this.listeners(name).length; },
            x: 0,
            y: 0,
            width: 816,
            height: 624,
            title: "",
            zoomLevel: 0,
            moveBy: function() {},
            resizeTo: function() {},
            setAlwaysOnTop: function() {},
            setVisibleOnAllWorkspaces: function() {},
            isFullscreen: false,
            enterFullscreen: function() { _nwSetFullscreen(this, true); },
            leaveFullscreen: function() { _nwSetFullscreen(this, false); },
            toggleFullscreen: function() { _nwSetFullscreen(this, !this.isFullscreen); },
            /* NW.js: with a "close" listener, close() only emits; close(true) quits. */
            close: function(force) {
                if (force !== true && this.listeners("close").length > 0) {
                    this.emit("close");
                    return;
                }
                _nwQuit();
            },
            closeDevTools: function() {},
            focus: function() {},
            show: function() {},
            hide: function() {},
            maximize: function() {},
            restore: function() {}
        };
        globalThis.nw = {
            Window: {
                get: function() {
                    return _nwWindowInstance;
                }
            },
            App: {
                argv: [],
                quit: _nwQuit
            }
        };
        globalThis.__nw_hostClose = _nwHostClose;
    }

})();
