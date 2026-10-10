"""Tests for tools/meetings/rooms.py: where a meeting is held (docs/evidence/the-meeting-place.md).

The check must fail when a kind of meeting has no room -- the bug the meeting-place phase closes --
and pass on the real tree. No engine is needed.

    python3 -m unittest discover -s tests/tools
"""

import importlib.util
import os
import tempfile
import unittest

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
ROOMS = os.path.join(ROOT, "tools", "meetings", "rooms.py")
HEADER = os.path.join(ROOT, "module", "ship", "ship_core.h")
TABLE = os.path.join(ROOT, "module", "ship", "meeting_rooms.def")

MINI_HEADER = """
enum MeetingKind : uint8_t {
\tMEET_WATCH_CHANGE = 0,
\tMEET_BORG,
\tMEET_KIND_COUNT
};
enum Department : uint8_t { DEPT_COMMAND = 0, DEPT_SECURITY, DEPT_COUNT };
"""

MINI_TABLE = """
MEETING_ROOM(MEET_WATCH_CHANGE, DEPT_COMMAND, "the briefing room", 1)
MEETING_ROOM(MEET_BORG, DEPT_SECURITY, "the security office", 6)
"""


def load():
    spec = importlib.util.spec_from_file_location("lwh_meeting_rooms", ROOMS)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


class Rooms(unittest.TestCase):
    def setUp(self):
        self.r = load()
        self.tmp = tempfile.TemporaryDirectory()
        self.header = os.path.join(self.tmp.name, "ship_core.h")
        self.table = os.path.join(self.tmp.name, "meeting_rooms.def")
        with open(self.header, "w") as f:
            f.write(MINI_HEADER)
        with open(self.table, "w") as f:
            f.write(MINI_TABLE)

    def tearDown(self):
        self.tmp.cleanup()

    def out(self):
        lines = []
        return lines, lambda s: lines.append(s)

    def test_reads_the_enum_names(self):
        self.assertEqual(self.r.enum_names(MINI_HEADER, "enum MeetingKind"),
                         ["MEET_WATCH_CHANGE", "MEET_BORG"])

    def test_reads_the_rows(self):
        rows = self.r.read_rows(MINI_TABLE)
        self.assertEqual(rows[0], ("MEET_WATCH_CHANGE", "DEPT_COMMAND", "the briefing room", 1))

    def test_check_passes_when_every_kind_has_a_room(self):
        lines, out = self.out()
        self.assertEqual(self.r.check(self.header, self.table, out), 0)
        self.assertTrue(any("PASS" in l for l in lines))

    def test_fails_when_a_kind_has_no_room(self):
        with open(self.table, "w") as f:  # drop the Borg row: a security matter with nowhere to happen
            f.write('MEETING_ROOM(MEET_WATCH_CHANGE, DEPT_COMMAND, "the briefing room", 1)\n')
        lines, out = self.out()
        self.assertEqual(self.r.check(self.header, self.table, out), 1)
        self.assertTrue(any("MEET_BORG" in l and "no room" in l for l in lines))

    def test_a_bad_deck_is_reported(self):
        with open(self.table, "a") as f:
            f.write('MEETING_ROOM(MEET_BORG, DEPT_SECURITY, "the moon", 16)\n')
        lines, out = self.out()
        # a duplicate (kind, subject) and a deck outside 1..15: either is a failure
        self.assertEqual(self.r.check(self.header, self.table, out), 1)

    def test_the_real_tree_passes(self):
        lines, out = self.out()
        self.assertEqual(self.r.check(HEADER, TABLE, out), 0, "\n".join(lines))


if __name__ == "__main__":
    unittest.main()
