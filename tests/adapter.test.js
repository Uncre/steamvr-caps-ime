const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const {execFileSync} = require('node:child_process');
const adapters = JSON.parse(execFileSync('python',['-c',
  'import sys,json;sys.path.insert(0,"tools");import patch_steam as p;print(json.dumps([p.CAPS_ADAPTER,p.MANAGER_ADAPTER]))'],{encoding:'utf8'}));
let sent=[];
const status={bIsOpen:true,sOverlayKey:'system.desktop.2'};
const manager=vm.runInNewContext('({' + adapters[1] + '})', {
  s:{qL:{VRKeyboardStatus:status}},
  SteamClient:{Input:{ControllerKeyboardSetKeyState:(...args)=>sent.push(args)}}
});
assert.equal(manager.SteamVrCapsImeToggle(),true);
assert.deepEqual(sent,[[72,true],[72,false]]);
for(const key of ['steam.overlay.250820','system.desktop.evil','system.keyboard','']) {
  sent=[];status.sOverlayKey=key;
  assert.equal(manager.SteamVrCapsImeToggle(),false);assert.equal(sent.length,0);
}
status.sOverlayKey='system.desktop.1';status.bIsOpen=false;
assert.equal(manager.SteamVrCapsImeToggle(),false);
status.bIsOpen=true;
const caps=vm.runInNewContext('(function(t){'+adapters[0]+'return "original"})');
const component={state:{toggleStates:{CapsLock:2,Shift:0}},BIsVR:()=>true,
  props:{VirtualKeyboardManager:manager},setState(fn){this.state=fn(this.state)}};
sent=[];assert.equal(caps.call(component,'CapsLock'),undefined);
assert.equal(component.state.toggleStates.CapsLock,0);assert.equal(sent.length,2);
component.BIsVR=()=>false;sent=[];
assert.equal(caps.call(component,'CapsLock'),'original');assert.equal(sent.length,0);
component.BIsVR=()=>true;
assert.equal(caps.call(component,'Shift'),'original');
component.props.VirtualKeyboardManager={};
assert.equal(caps.call(component,'CapsLock'),'original');
console.log('PASS: VR desktop Caps sends marker; Steam/game/desktop UI retain normal Caps; Shift unchanged');
