"""The order set check, demonstrated failing in both directions (docs/orders.md, Task B).

`tools/orders/orders.py check` compares module/ship/order_set.def (the register) to the order classes
the dispatch handles (module/ship/ship_core.cpp, ApplyOrder). It must fail when the surface drifts in
either direction, and pass on the tree. This test plants each failure in a temporary copy -- the tree
is never touched -- and proves the check catches it.
"""

import importlib.util
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def _load():
    spec = importlib.util.spec_from_file_location("orders", ROOT / "tools" / "orders" / "orders.py")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


class OrderSetCheck(unittest.TestCase):
    def setUp(self):
        self.orders = _load()
        self.register = (ROOT / "module" / "ship" / "order_set.def").read_text(encoding="utf-8")
        self.dispatch = (ROOT / "module" / "ship" / "ship_core.cpp").read_text(encoding="utf-8")

    def _run(self, register, dispatch, tmp):
        reg = Path(tmp) / "order_set.def"
        dis = Path(tmp) / "ship_core.cpp"
        reg.write_text(register, encoding="utf-8")
        dis.write_text(dispatch, encoding="utf-8")
        out = []
        try:
            rc = self.orders.check(str(reg), str(dis), out.append)
        except ValueError as e:   # the tool's own malformed-row failure, as main() reports it
            return 1, str(e)
        return rc, "\n".join(out)

    def test_passes_on_the_tree(self):
        import tempfile
        with tempfile.TemporaryDirectory() as tmp:
            rc, out = self._run(self.register, self.dispatch, tmp)
        self.assertEqual(rc, 0, out)
        self.assertIn("PASS", out)

    def test_fails_when_the_register_names_an_unhandled_order(self):
        import tempfile
        planted = self.register + (
            'ORDER_CLASS(ORDER_GHOST, "ghost", "ghost the {deck}", "nothing", ORD_NEED_SECURITY, JOB_SECURE, -1)\n')
        with tempfile.TemporaryDirectory() as tmp:
            rc, out = self._run(planted, self.dispatch, tmp)
        self.assertEqual(rc, 1)
        self.assertIn("ORDER_GHOST", out)
        self.assertIn("does not handle", out)

    def test_fails_when_the_dispatch_handles_a_foreign_case(self):
        import tempfile
        planted = self.dispatch.replace(
            "case ORDER_BUILD_PARTS: {",
            "case ORDER_GHOST_CASE: break;\n\t\tcase ORDER_BUILD_PARTS: {", 1)
        with tempfile.TemporaryDirectory() as tmp:
            rc, out = self._run(self.register, planted, tmp)
        self.assertEqual(rc, 1)
        self.assertIn("ORDER_GHOST_CASE", out)
        self.assertIn("register does not name", out)

    def test_rejects_a_malformed_register(self):
        import tempfile
        planted = self.register.replace("ORD_NEED_SECURITY", "ORD_NEED_TYPO")
        with tempfile.TemporaryDirectory() as tmp:
            rc, out = self._run(planted, self.dispatch, tmp)
        self.assertEqual(rc, 1)
        self.assertIn("unknown need", out)


if __name__ == "__main__":
    unittest.main()
