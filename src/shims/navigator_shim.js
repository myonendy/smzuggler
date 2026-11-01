/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 */

/*
 * navigator shim: user agent, platform info and the Gamepad API, with
 * gamepad slots populated from native input.
 */

(function() {
    "use strict";

    var _gamepads = [null, null, null, null];

    /* RPG Maker MZ looks for "Chrome" in the user agent to enable WebGL;
       "nwjs" is deliberately absent so NW.js-only paths stay off. */
    var UA_PRODUCT = "Outsider/1.0 Chrome/120.0.0.0";

    /* Node's process.platform/arch, read at access time because
       storage_shim.js defines process after this shim runs. Without
       process, the shape of the game root path is the only hint the
       engine offers: a drive letter means Windows. */
    function hostOs() {
        var p = globalThis.process;
        if (p && typeof p.platform === "string") {
            return { platform: p.platform, arch: String(p.arch || "x64") };
        }
        var root = "";
        if (typeof __native_io !== "undefined" && __native_io.getGameRoot) {
            root = String(__native_io.getGameRoot() || "");
        }
        if (/^[A-Za-z]:/.test(root) || root.indexOf("\\") >= 0) {
            return { platform: "win32", arch: "x64" };
        }
        return { platform: "linux", arch: "x64" };
    }

    /* Machine names as Chrome's navigator.platform spells them on Linux. */
    function linuxMachine(arch) {
        if (arch === "x64") return "x86_64";
        if (arch === "arm64") return "aarch64";
        if (arch === "ia32") return "i686";
        if (arch === "arm") return "armv7l";
        return arch;
    }

    /* navigator.platform as Chrome reports it for the host OS. */
    function chromePlatform(os) {
        if (os.platform === "win32") return "Win32";
        if (os.platform === "darwin") return "MacIntel";
        return "Linux " + linuxMachine(os.arch);
    }

    /* The parenthesised OS part of Chrome's user agent. Chrome freezes
       the macOS and Windows versions it reports, so these are fixed. */
    function chromeOsToken(os) {
        if (os.platform === "win32") return "Windows NT 10.0; Win64; x64";
        if (os.platform === "darwin") return "Macintosh; Intel Mac OS X 10_15_7";
        if (os.platform === "android") return "Linux; Android 10; K";
        return "X11; Linux " + linuxMachine(os.arch);
    }

    function appVersion() {
        return "5.0 (" + chromeOsToken(hostOs()) + ") " + UA_PRODUCT;
    }

    var navigator = {
        /* RPG Maker MZ checks this for standalone app detection. */
        standalone: true,

        language: "en-US",
        languages: ["en-US", "en"],
        maxTouchPoints: 0,
        hardwareConcurrency: 4,
        onLine: true,
        cookieEnabled: false,
        vendor: "",
        vendorSub: "",
        product: "Gecko",
        productSub: "20030107",
        appName: "Netscape",
        appCodeName: "Mozilla",

        getGamepads: function() {
            return _gamepads.slice();
        },

        vibrate: function() { return false; },

        clipboard: {
            writeText: function(text) { return Promise.resolve(); },
            readText: function() { return Promise.resolve(""); }
        },

        mediaDevices: {
            getUserMedia: function() { return Promise.reject(new Error("Not supported")); },
            enumerateDevices: function() { return Promise.resolve([]); }
        },

        permissions: {
            query: function() { return Promise.resolve({ state: "denied" }); }
        },

        storage: {
            estimate: function() { return Promise.resolve({ quota: 0, usage: 0 }); },
            persist: function() { return Promise.resolve(false); },
            persisted: function() { return Promise.resolve(false); }
        },

        serviceWorker: null,
        geolocation: null,

        sendBeacon: function() { return false; }
    };

    /* Getters, so the values follow process.platform once it exists. */
    Object.defineProperty(navigator, "userAgent", {
        get: function() { return "Mozilla/" + appVersion(); },
        enumerable: true,
        configurable: true
    });
    Object.defineProperty(navigator, "appVersion", {
        get: appVersion,
        enumerable: true,
        configurable: true
    });
    Object.defineProperty(navigator, "platform", {
        get: function() { return chromePlatform(hostOs()); },
        enumerable: true,
        configurable: true
    });

    /* Gamepad slot updates, called from native input. */

    globalThis.__navigator_setGamepad = function(index, gamepad) {
        if (index >= 0 && index < 4) {
            _gamepads[index] = gamepad;
        }
    };

    globalThis.__navigator_clearGamepad = function(index) {
        if (index >= 0 && index < 4) {
            _gamepads[index] = null;
        }
    };

    globalThis.navigator = navigator;
    if (globalThis.window) {
        globalThis.window.navigator = navigator;
    }

})();
