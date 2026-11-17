const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const adapter = fs.readFileSync('adapter/caps-adapter.js','utf8');
let sent=[];
const status={bIsOpen:true,sOverlayKey:'system.desktop.2'};
const caps=vm.runInNewContext('(function(t){'+adapter+'return "original"})', {
  Rt:{qL:{VRKeyboardStatus:status}},
  fetch:(url,options)=>{sent.push([url,options.method,options.headers['X-SteamVR-Caps-IME']]);return Promise.resolve({ok:true});}
});
const component={state:{toggleStates:{CapsLock:2,Shift:0}},BIsVR:()=>true,
  setState(fn){this.state=fn(this.state)}};
assert.equal(caps.call(component,'CapsLock'),undefined);
assert.deepEqual(sent,[['http://127.0.0.1:27391/toggle','POST','1']]);
assert.equal(component.state.toggleStates.CapsLock,0);
for(const key of ['steam.overlay.250820','system.desktop.evil','system.keyboard','']) {
  sent=[];status.sOverlayKey=key;
  assert.equal(caps.call(component,'CapsLock'),'original');assert.equal(sent.length,0);
}
status.sOverlayKey='system.desktop.1';status.bIsOpen=false;
assert.equal(caps.call(component,'CapsLock'),'original');
status.bIsOpen=true;
component.BIsVR=()=>false;sent=[];
assert.equal(caps.call(component,'CapsLock'),'original');assert.equal(sent.length,0);
component.BIsVR=()=>true;
assert.equal(caps.call(component,'Shift'),'original');
console.log('PASS: VR desktop Caps sends local request; Steam/game/desktop UI retain normal Caps; Shift unchanged');
