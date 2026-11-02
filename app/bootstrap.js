"use strict";
(function () {
    const statusElement = document.getElementById("status");
    const canvasElement = document.getElementById("canvas");

    canvasElement.addEventListener("contextmenu", function (e) {
        e.preventDefault();
    });

    canvasElement.addEventListener("webglcontextlost", function (e) {
        statusElement.textContent = "WebGL context lost. Please reload the page.";
        e.preventDefault();
    }, false);

    function toggleFullscreen() {
        if (document.fullscreenElement) {
            document.exitFullscreen();
        } else {
            document.documentElement.requestFullscreen().catch(function () {});
        }
    }

    window.addEventListener("keydown", function (e) {
        if ((e.code === "KeyF" || e.code === "F11") && !e.repeat) {
            e.preventDefault();
            toggleFullscreen();
        }
    }, { passive: false });

    window.addEventListener("keydown", function (e) {
        if (["Space", "ArrowUp", "ArrowDown", "ArrowLeft", "ArrowRight", "Tab"].includes(e.code)) {
            e.preventDefault();
        }
    }, { passive: false });

    window.Module = {
        canvas: canvasElement,
        setStatus: function (text) {
            statusElement.textContent = text ? String(text) : "";
        }
    };
})();
