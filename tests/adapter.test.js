const assert = require("node:assert/strict");
const fs = require("node:fs");
const vm = require("node:vm");

async function test() {
    const adapter = fs.readFileSync("adapter/caps-adapter.js", "utf8");
    const sent = [];
    const status = { bIsOpen: true, sOverlayKey: "system.desktop.2" };
    const press = vm.runInNewContext(
        "(function(t, c, a, s, f) {" + adapter + 'return "original";})',
        {
            Rt: { qL: { VRKeyboardStatus: status } },
            V: Boolean,
            Se: key => key.toUpperCase(),
            ve: key => key === 1 ? 0 : key,
            AbortSignal,
            fetch: async (url, options) => {
                sent.push([url.replace("http://127.0.0.1:27391/", ""), options.method, options.headers["X-SteamVR-Caps-IME"]]);
                return { ok: true };
            }
        }
    );
    const component = {
        state: { layoutState: 0, toggleStates: { CapsLock: 2, Shift: 0 } },
        BIsVR: () => true,
        setState(update) { Object.assign(this.state, update(this.state)); }
    };
    assert.equal(press.call(component, "CapsLock"), undefined);
    assert.equal(component.state.toggleStates.CapsLock, 0);
    for (const key of ["k", "y", "o", "u", " ", "Tab", "ArrowDown", "Enter"]) {
        assert.equal(press.call(component, key), undefined);
    }
    component.state.toggleStates.Shift = 1;
    press.call(component, "Tab");
    assert.equal(component.state.toggleStates.Shift, 0);
    component.state.toggleStates.Shift = 1;
    press.call(component, "a");
    await component.capsImeQueue;
    assert.deepEqual(sent, [
        "toggle", "key/ascii-6b", "key/ascii-79", "key/ascii-6f", "key/ascii-75",
        "key/space", "key/tab", "key/down", "key/enter", "key/tab-shift", "key/ascii-41"
    ].map(route => [route, "POST", "1"]));
    for (const key of ["Shift", "VKDone", "💬", "あ"]) {
        assert.equal(press.call(component, key), "original");
    }
    assert.equal(press.call(component, "a", false, true), "original");
    for (const overlay of ["steam.overlay.250820", "system.desktop.evil", "system.keyboard", ""]) {
        status.sOverlayKey = overlay;
        assert.equal(press.call(component, "CapsLock"), "original");
        assert.equal(press.call(component, "Enter"), "original");
    }
    status.sOverlayKey = "system.desktop.1";
    status.bIsOpen = false;
    assert.equal(press.call(component, "CapsLock"), "original");
    status.bIsOpen = true;
    component.BIsVR = () => false;
    assert.equal(press.call(component, "CapsLock"), "original");
    console.log("PASS: native text and IME commands stay in order and are limited to the VR desktop");
}
test().catch(error => { console.error(error); process.exitCode = 1; });
