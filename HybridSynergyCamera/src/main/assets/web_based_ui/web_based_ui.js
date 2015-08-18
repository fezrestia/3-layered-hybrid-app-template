// Web based UI JavaScript

//// CAPABILITY CHECK ////////////////////////////////////////////////////////////////////////////

var isTouchEventSupported = window.TouchEvent;
if (isTouchEventSupported) {
    // Touch event supported.
    console.log("Touch event is supported.");
} else {
    // Touch event is NOT supported.
    console.log("Touch event is NOT supported.");
}

//// EVENT LISTENER //////////////////////////////////////////////////////////////////////////////

// Left-Top icon.
window.document.getElementById("left-top-icon").addEventListener(
        "touchstart",
        onLeftTopIconTouched);
window.document.getElementById("left-top-icon").addEventListener(
        "touchmove",
        onLeftTopIconTouched);
window.document.getElementById("left-top-icon").addEventListener(
        "touchend",
        onLeftTopIconTouched);
window.document.getElementById("left-top-icon").addEventListener(
        "touchcancel",
        onLeftTopIconTouched);

// Left-Bottom icon.
window.document.getElementById("left-bottom-icon").addEventListener(
        "touchstart",
        onLeftBottomIconTouched);
window.document.getElementById("left-bottom-icon").addEventListener(
        "touchmove",
        onLeftBottomIconTouched);
window.document.getElementById("left-bottom-icon").addEventListener(
        "touchend",
        onLeftBottomIconTouched);
window.document.getElementById("left-bottom-icon").addEventListener(
        "touchcancel",
        onLeftBottomIconTouched);

// Right-Top icon.
window.document.getElementById("right-top-icon").addEventListener(
        "touchstart",
        onRightTopIconTouched);
window.document.getElementById("right-top-icon").addEventListener(
        "touchmove",
        onRightTopIconTouched);
window.document.getElementById("right-top-icon").addEventListener(
        "touchend",
        onRightTopIconTouched);
window.document.getElementById("right-top-icon").addEventListener(
        "touchcancel",
        onRightTopIconTouched);

// Right-Bottom icon.
window.document.getElementById("right-bottom-icon").addEventListener(
        "touchstart",
        onRightBottomIconTouched);
window.document.getElementById("right-bottom-icon").addEventListener(
        "touchmove",
        onRightBottomIconTouched);
window.document.getElementById("right-bottom-icon").addEventListener(
        "touchend",
        onRightBottomIconTouched);
window.document.getElementById("right-bottom-icon").addEventListener(
        "touchcancel",
        onRightBottomIconTouched);

//// FUNCTION ////////////////////////////////////////////////////////////////////////////////////

function onLeftTopIconTouched(event) {
    onFourCornerIconTouched("left-top-icon", event);
}

function onLeftBottomIconTouched(event) {
    onFourCornerIconTouched("left-bottom-icon", event);
}

function onRightTopIconTouched(event) {
    onFourCornerIconTouched("right-top-icon", event);
}

function onRightBottomIconTouched(event) {
    onFourCornerIconTouched("right-bottom-icon", event);
}

function onFourCornerIconTouched(viewId, event) {
    console.log("onFourCornerIconTouched() : E");
    console.log("onFourCornerIconTouched() : View=" + viewId + "Type=" + event.type);

    event.preventDefault();

    switch (event.type) {
        case "touchstart":
            JSNI.onFourCornerIconTouched(viewId, "down");
            break;

        case "touchmove":
            JSNI.onFourCornerIconTouched(viewId, "move");
            break;

        case "touchend":
            // Fall througn.
        case "touchcancel":
            JSNI.onFourCornerIconTouched(viewId, "up");
            break;

        default:
            console.log("Unexpected TouchEvent.");
    }

    console.log("onFourCornerIconTouched() : X");
    return;
}
