"""Exact, reversible adapters for the installed Steam UI (no Valve code shipped)."""
import argparse
import hashlib
import json
from pathlib import Path

MARKER = "/* SteamVR Caps IME v2 */"
CAPS_ANCHOR = 'const g=u.s_keyToggleData[t];if(g)'
asset = Path(__file__).with_name('caps-adapter.js')
if not asset.exists():
    asset = Path(__file__).resolve().parents[1] / 'adapter' / 'caps-adapter.js'
CAPS_ADAPTER = asset.read_bytes().decode('utf-8')
ADAPTERS = {'virtualkeyboard.js': (CAPS_ANCHOR, CAPS_ADAPTER)}

def transform(text, name, remove=False):
    anchor, adapter = ADAPTERS[name]
    patched = adapter + anchor
    if remove:
        if MARKER not in text:
            return text
        if text.count(patched) != 1:
            raise ValueError(f'{name}: installed adapter missing or changed; refusing to overwrite')
        return text.replace(patched, anchor, 1)
    if patched in text:
        return text
    if 'SteamVR Caps IME' in text or text.count(anchor) != 1:
        raise ValueError(f'{name}: unsupported Steam UI version; no files changed')
    return text.replace(anchor, patched, 1)

def apply(steamui, backup, remove=False, check=False):
    # Validate both files before writing either one.
    changes = []
    for name in ADAPTERS:
        path = steamui / name
        original = path.read_bytes()
        result = transform(original.decode('utf-8'), name, remove).encode('utf-8')
        changes.append((path, original, result))
    if check:
        print('Compatible: Steam keyboard adapter validated')
        return
    backup.mkdir(parents=True, exist_ok=True)
    for path, original, result in changes:
        if original == result:
            continue
        digest = hashlib.sha256(original).hexdigest()
        (backup / (path.name + '.' + digest + '.bak')).write_bytes(original)
    written = []
    try:
        for path, original, result in changes:
            if original != result:
                path.write_bytes(result)
                written.append((path, original))
    except Exception:
        for path, original in written:
            path.write_bytes(original)
        raise
    print('Removed adapters' if remove else 'Installed adapters')
    print(json.dumps({p.name:hashlib.sha256(r).hexdigest() for p,_,r in changes}, indent=2))

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--steamui', required=True, type=Path)
    parser.add_argument('--backup', required=True, type=Path)
    parser.add_argument('--remove', action='store_true')
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    apply(args.steamui, args.backup, args.remove, args.check)
