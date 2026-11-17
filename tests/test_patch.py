import sys
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import patch_steam as p

class PatchTests(unittest.TestCase):
    def test_reversible_and_idempotent(self):
        for name, (anchor, _) in p.ADAPTERS.items():
            original = 'before;' + anchor + ';after;'
            patched = p.transform(original, name)
            self.assertEqual(p.transform(patched, name), patched)
            self.assertEqual(p.transform(patched, name, remove=True), original)

    def test_changed_steam_version_rejected_before_writing(self):
        with TemporaryDirectory() as root:
            ui = Path(root)
            originals = {}
            for name, (anchor, _) in p.ADAPTERS.items():
                data = anchor if name == 'virtualkeyboard.js' else 'unknown new version'
                (ui/name).write_text(data, encoding='utf-8')
                originals[name] = data
            with self.assertRaises(ValueError):
                p.apply(ui, ui/'backup')
            for name, data in originals.items():
                self.assertEqual((ui/name).read_text(encoding='utf-8'), data)

    def test_remove_preserves_unrelated_edits(self):
        for name, (anchor, _) in p.ADAPTERS.items():
            patched = p.transform(anchor, name) + 'other modification'
            self.assertEqual(p.transform(patched, name, remove=True), anchor + 'other modification')

if __name__ == '__main__':
    unittest.main()
