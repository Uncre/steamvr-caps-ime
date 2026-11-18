/* SteamVR Caps IME v3 */
if (this.BIsVR()) {
    const keyboard = Rt.qL.VRKeyboardStatus;
    if (keyboard.bIsOpen && /^system\.desktop(?:\.\d+)?$/.test(keyboard.sOverlayKey)) {
        const toggles = this.state.toggleStates;
        const shift = V(toggles.Shift);
        const keyNames = {
            " ": "space", Space: "space", Tab: "tab", Enter: "enter",
            Backspace: "backspace", Delete: "delete", Escape: "escape",
            ArrowLeft: "left", ArrowRight: "right", ArrowUp: "up", ArrowDown: "down"
        };
        let route;
        if (t === "CapsLock") {
            route = "toggle";
        } else if (Object.hasOwn(keyNames, t)) {
            route = "key/" + keyNames[t] + (shift ? "-shift" : "");
        } else if (!s && !f && !a && this.state.layoutState < 2 && /^[!-~]$/.test(t)) {
            const character = (shift || V(toggles.CapsLock)) && !c ? Se(t) : t;
            route = "key/ascii-" + character.charCodeAt(0).toString(16);
        }
        if (route) {
            // Keep conversion and confirmation ordered with the preceding letters.
            this.capsImeQueue = (this.capsImeQueue ?? Promise.resolve()).then(async () => {
                const response = await fetch("http://127.0.0.1:27391/" + route, {
                    method: "POST",
                    headers: {"X-SteamVR-Caps-IME": "3"},
                    signal: AbortSignal.timeout(1500)
                });
                if (!response.ok) throw Error("IME bridge status " + response.status);
            }).catch(error => console.warn("[SteamVR Caps IME]", error));
            this.setState(state => {
                const keys = state.toggleStates;
                return {toggleStates: route === "toggle" ? {...keys, CapsLock: 0} : {
                    ...keys, Shift: ve(keys.Shift), Control: ve(keys.Control),
                    Alt: ve(keys.Alt), AltGr: ve(keys.AltGr)
                }};
            });
            return;
        }
    }
}
/* End SteamVR Caps IME v3 */
