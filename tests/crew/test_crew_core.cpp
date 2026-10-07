// Tests for module/crew/crew_core. One function per rule in docs/g3-reactive-crew.md; a failure
// names the rule, not just the line.

#include "crew_core.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace crew;

static int g_failures = 0;
static const char *g_test = "";

#define CHECK(cond) \
	do { \
		if (!(cond)) { \
			std::printf("FAIL  %s: %s (line %d)\n", g_test, #cond, __LINE__); \
			++g_failures; \
		} \
	} while (0)

static Signals Sig(bool scripted, bool hostile, bool overridden, bool hasPost)
{
	Signals s;
	s.scripted = scripted;
	s.hostile = hostile;
	s.overridden = overridden;
	s.hasPost = hasPost;
	return s;
}

static std::vector<Post> Posts(int n)
{
	std::vector<Post> posts(n);
	for (int i = 0; i < n; ++i) {
		posts[i].name = "post" + std::to_string(i);
		posts[i].origin[0] = 100.0f * i;
	}
	return posts;
}

static std::vector<Member> Crew(int n)
{
	std::vector<Member> m(n);
	for (int i = 0; i < n; ++i) m[i].ent = static_cast<int16_t>(10 + i);
	return m;
}

// Precedence: scripted > combat > director > duty > idle, for every combination of signals.
static void TestArbitrationOrder()
{
	g_test = "arbitration order";
	for (int bits = 0; bits < 16; ++bits) {
		const bool sc = bits & 1, ho = bits & 2, ov = bits & 4, po = bits & 8;
		const Level got = Arbitrate(Sig(sc, ho, ov, po));
		const Level want = sc ? LEVEL_SCRIPT : ho ? LEVEL_COMBAT : ov ? LEVEL_DIRECTOR : po ? LEVEL_DUTY : LEVEL_IDLE;
		CHECK(got == want);
	}
}

// Levels 1-2 always yield, whatever post or override the member holds.
static void TestScriptAndCombatYield()
{
	g_test = "script and combat yield";
	Member m;
	m.dutyPost = 0;
	m.overridePost = 1;
	for (int at = 0; at < 2; ++at) {
		CHECK(Decide(m, Sig(true, false, true, true), at) == ACT_YIELD);
		CHECK(Decide(m, Sig(true, true, true, true), at) == ACT_YIELD);
		CHECK(Decide(m, Sig(false, true, true, true), at) == ACT_YIELD);
	}
}

// Corollary 1: an interruption is a pause. The duty post survives it and is resumed.
static void TestInterruptionIsAPause()
{
	g_test = "interruption is a pause";
	Member m;
	m.dutyPost = 3;
	CHECK(Decide(m, Sig(false, false, false, true), false) == ACT_TRAVEL);
	CHECK(Decide(m, Sig(false, true, false, true), false) == ACT_YIELD);
	CHECK(m.dutyPost == 3);
	CHECK(Decide(m, Sig(false, false, false, true), false) == ACT_TRAVEL);
	CHECK(Decide(m, Sig(false, false, false, true), true) == ACT_HOLD);
}

static void TestDecideEdges()
{
	g_test = "decide edges";
	Member m;
	CHECK(Decide(m, Sig(false, false, false, false), false) == ACT_IDLE);
	m.dutyPost = 1;
	m.overridePost = 0;
	CHECK(m.EffectivePost() == 0);
	m.flags = MF_FAILED;
	CHECK(Decide(m, Sig(false, false, true, true), false) == ACT_IDLE);
	m.flags = MF_UNAVAILABLE;
	CHECK(Decide(m, Sig(false, false, false, true), false) == ACT_IDLE);
}

static void TestAssignNearest()
{
	g_test = "assignment: nearest, deterministic, one each";
	std::vector<Post> posts = Posts(3);
	std::vector<Member> crew = Crew(3);
	std::vector<Candidate> c(3);
	c[0].origin[0] = 190; // nearest post2
	c[1].origin[0] = 10;  // nearest post0
	c[2].origin[0] = 120; // nearest post1
	AssignPosts(crew, c, posts);
	CHECK(crew[0].dutyPost == 2);
	CHECK(crew[1].dutyPost == 0);
	CHECK(crew[2].dutyPost == 1);

	// More crew than posts: the surplus are spares, and nobody shares.
	std::vector<Member> five = Crew(5);
	std::vector<Candidate> c5(5);
	AssignPosts(five, c5, posts);
	int assigned = 0;
	std::vector<int> seen(3, 0);
	for (const Member &m : five)
		if (m.dutyPost != NO_POST) { ++assigned; ++seen[m.dutyPost]; }
	CHECK(assigned == 3);
	CHECK(seen[0] == 1 && seen[1] == 1 && seen[2] == 1);

	// Equidistant candidates resolve by index, so two runs agree.
	std::vector<Member> again = Crew(5);
	AssignPosts(again, c5, posts);
	for (int i = 0; i < 5; ++i) CHECK(again[i].dutyPost == five[i].dutyPost);
}

static void TestAssignAuthoredHolders()
{
	g_test = "assignment: authored holders";
	std::vector<Post> posts = Posts(3);
	posts[0].holder = "NPC_Tuvok";
	posts[2].holder = "nobody_here";
	std::vector<Member> crew = Crew(3);
	std::vector<Candidate> c(3);
	c[0].type = "paris";
	c[1].type = "tuvok";
	c[1].targetname = "someone";
	c[2].targetname = "npc_tuvok"; // a targetname match outranks a type match, case-insensitively
	c[2].origin[0] = 1000;
	AssignPosts(crew, c, posts);
	CHECK(crew[2].dutyPost == 0);
	// post2 is reserved for an absent holder: it stays empty, it is not handed to the nearest.
	CHECK(crew[0].dutyPost == 1 || crew[1].dutyPost == 1);
	CHECK(crew[0].dutyPost != 2 && crew[1].dutyPost != 2);

	// Existing assignments are kept (a late arrival does not reshuffle the watch).
	std::vector<Member> late = Crew(2);
	late[0].dutyPost = 1;
	std::vector<Candidate> c2(2);
	std::vector<Post> open = Posts(3);
	AssignPosts(late, c2, open);
	CHECK(late[0].dutyPost == 1);
	CHECK(late[1].dutyPost == 0);
}

static void TestRestaff()
{
	g_test = "director: restaffing";
	std::vector<Post> posts = Posts(3); // post0 most important
	std::vector<Member> crew = Crew(3);
	for (int i = 0; i < 3; ++i) crew[i].dutyPost = static_cast<int16_t>(i);
	std::vector<bool> scripted(3, false);

	CHECK(Restaff(crew, posts, scripted) == 0); // fully staffed: the director does nothing

	crew[0].flags |= MF_UNAVAILABLE; // the most important post empties
	CHECK(Restaff(crew, posts, scripted) > 0);
	// The hole moves to the least important post: 2 covers 0, and 1 stays put.
	CHECK(crew[2].overridePost == 0);
	CHECK(crew[1].overridePost == NO_POST);
	CHECK(crew[2].dutyPost == 2); // the routine is remembered

	// The least important post emptying pulls nobody: no one holds anything that matters less.
	std::vector<Member> b = Crew(3);
	for (int i = 0; i < 3; ++i) b[i].dutyPost = static_cast<int16_t>(i);
	b[2].flags |= MF_UNAVAILABLE;
	CHECK(Restaff(b, posts, scripted) == 0);

	// The holder returns: the override clears and the member goes back to its routine.
	crew[0].flags &= ~MF_UNAVAILABLE;
	Restaff(crew, posts, scripted);
	CHECK(crew[2].overridePost == NO_POST);
	CHECK(crew[2].EffectivePost() == 2);

	// A spare is taken before anyone is pulled off a post.
	std::vector<Member> s = Crew(4);
	for (int i = 0; i < 3; ++i) s[i].dutyPost = static_cast<int16_t>(i);
	s[0].flags |= MF_UNAVAILABLE;
	std::vector<bool> sc4(4, false);
	Restaff(s, posts, sc4);
	CHECK(s[3].overridePost == 0);
	CHECK(s[1].overridePost == NO_POST && s[2].overridePost == NO_POST);

	// Explicit priority outranks table order.
	std::vector<Post> pri = Posts(3);
	pri[2].priority = -1; // post2 is now the most important
	std::vector<Member> p = Crew(3);
	for (int i = 0; i < 3; ++i) p[i].dutyPost = static_cast<int16_t>(i);
	p[2].flags |= MF_UNAVAILABLE;
	Restaff(p, pri, scripted);
	CHECK(p[1].overridePost == 2);
}

// Corollary 2: nothing in levels 3-5 interrupts a script. The director waits.
static void TestDirectorWaitsForScripts()
{
	g_test = "director waits for scripts";
	std::vector<Post> posts = Posts(2);
	std::vector<Member> crew = Crew(2);
	crew[0].dutyPost = 0;
	crew[1].dutyPost = 1;
	crew[0].flags |= MF_UNAVAILABLE;
	std::vector<bool> scripted = {false, true};

	CHECK(Restaff(crew, posts, scripted) == 0);
	CHECK(crew[1].overridePost == NO_POST);

	scripted[1] = false; // the script ends; now the director may act
	CHECK(Restaff(crew, posts, scripted) == 1);
	CHECK(crew[1].overridePost == 0);

	// Nor does it clear an override out from under a script.
	crew[0].flags &= ~MF_UNAVAILABLE;
	scripted[1] = true;
	Restaff(crew, posts, scripted);
	CHECK(crew[1].overridePost == 0);
}

static void TestCoverage()
{
	g_test = "coverage";
	std::vector<Member> crew = Crew(10);
	for (int i = 0; i < 10; ++i) crew[i].dutyPost = static_cast<int16_t>(i);
	std::vector<bool> at(10, true);
	CHECK(CoveragePercent(crew, at) == 100);
	at[3] = false;
	CHECK(CoveragePercent(crew, at) == 90);
	at[4] = false;
	CHECK(CoveragePercent(crew, at) == 80);

	// A dead holder standing on its post does not occupy it.
	std::vector<bool> all(10, true);
	crew[0].flags |= MF_UNAVAILABLE;
	CHECK(CoveragePercent(crew, all) == 90);
	// Restaffing moves the hole rather than hiding it: 9 covers 0, post 9 is now the empty one.
	crew[9].overridePost = 0;
	CHECK(CoveragePercent(crew, all) == 90);

	std::vector<Member> none = Crew(3);
	CHECK(CoveragePercent(none, std::vector<bool>(3, false)) == 100);

	Stats st;
	RecordSample(st, 100);
	RecordSample(st, 70);
	RecordSample(st, 95);
	CHECK(st.samples == 3 && st.minCoveragePercent == 70 && st.lastCoveragePercent == 95);
}

static void TestSaveRoundTrip()
{
	g_test = "persistence: round trip";
	std::vector<Post> posts = Posts(4);
	const uint32_t hash = PostTableHash(posts);
	std::vector<Member> crew = Crew(5);
	for (int i = 0; i < 5; ++i) {
		crew[i].dutyPost = static_cast<int16_t>(i % 4);
		crew[i].overridePost = i == 2 ? 0 : NO_POST;
		crew[i].goalPost = i == 1 ? 1 : NO_POST;
		crew[i].action = static_cast<uint8_t>(i % 4);
		crew[i].flags = static_cast<uint8_t>(i);
		crew[i].scheduleCursor = 1000 * i - 7;
		crew[i].firstArrivalMs = i == 4 ? -1 : 2500 * i;
	}
	const std::vector<uint8_t> blob = Pack(crew, hash);
	CHECK(blob.size() == SAVE_HEADER_BYTES + 5 * SAVE_MEMBER_BYTES);

	std::vector<Member> back;
	CHECK(Unpack(blob.data(), blob.size(), hash, back));
	CHECK(back.size() == crew.size());
	for (size_t i = 0; i < back.size() && i < crew.size(); ++i) {
		CHECK(back[i].ent == crew[i].ent);
		CHECK(back[i].dutyPost == crew[i].dutyPost);
		CHECK(back[i].overridePost == crew[i].overridePost);
		CHECK(back[i].goalPost == crew[i].goalPost);
		CHECK(back[i].action == crew[i].action);
		CHECK(back[i].flags == crew[i].flags);
		CHECK(back[i].scheduleCursor == crew[i].scheduleCursor);
		CHECK(back[i].firstArrivalMs == crew[i].firstArrivalMs);
	}
	CHECK(Pack(back, hash) == blob);
}

static void TestSaveRejectsBadInput()
{
	g_test = "persistence: rejects bad input";
	std::vector<Post> posts = Posts(4);
	const uint32_t hash = PostTableHash(posts);
	std::vector<Member> crew = Crew(5);
	const std::vector<uint8_t> blob = Pack(crew, hash);
	std::vector<Member> out = Crew(1);

	CHECK(!Unpack(nullptr, 0, hash, out));
	CHECK(!Unpack(blob.data(), blob.size() - 1, hash, out)); // truncated
	CHECK(!Unpack(blob.data(), SAVE_HEADER_BYTES - 1, hash, out));
	CHECK(!Unpack(blob.data(), blob.size(), hash + 1, out)); // a different post table

	std::vector<uint8_t> bad = blob;
	bad[0] ^= 0xff; // magic
	CHECK(!Unpack(bad.data(), bad.size(), hash, out));
	bad = blob;
	bad[4] = 99; // version
	CHECK(!Unpack(bad.data(), bad.size(), hash, out));
	bad = blob;
	bad[6] = 200; // count beyond the ceiling
	CHECK(!Unpack(bad.data(), bad.size(), hash, out));
	bad = blob;
	bad[SAVE_HEADER_BYTES + 2] = 0x40; // a post index outside the table
	bad[SAVE_HEADER_BYTES + 3] = 0x40;
	CHECK(!Unpack(bad.data(), bad.size(), hash, out));
	bad = blob;
	bad[SAVE_HEADER_BYTES + 8] = 77; // an action that does not exist
	CHECK(!Unpack(bad.data(), bad.size(), hash, out));

	CHECK(out.size() == 1); // a rejected record leaves the caller's state alone

	// The fingerprint follows names, in order, case-insensitively.
	std::vector<Post> renamed = posts;
	renamed[1].name = "elsewhere";
	CHECK(PostTableHash(renamed) != hash);
	std::vector<Post> upper = posts;
	upper[0].name = "POST0";
	CHECK(PostTableHash(upper) == hash);
	std::vector<Post> ab(2), ba(2);
	ab[0].name = "a"; ab[1].name = "bc";
	ba[0].name = "ab"; ba[1].name = "c";
	CHECK(PostTableHash(ab) != PostTableHash(ba));
}

// The budget exists and is tested: the record must fit it at G3's five crew and at G4's thirty.
static void TestSaveBudget()
{
	g_test = "persistence: budget";
	const Config cfg;
	for (int n : {5, 10, 30}) {
		const std::vector<uint8_t> blob = Pack(Crew(n), 0);
		CHECK(blob.size() <= static_cast<size_t>(cfg.saveBytesPerNpc) * n);
	}
	CHECK(SAVE_HEADER_BYTES + SAVE_MEMBER_BYTES <= 256);
}

static void TestParseConfig()
{
	g_test = "config parsing";
	Config cfg;
	std::string err;
	const char *good =
	    "# deck 4\n"
	    "crew max 8\n"
	    "bound reach_ms 60000   # one minute\n"
	    "bound ack_ms 4000\n"
	    "budget save_bytes_per_npc 128\n"
	    "\n"
	    "post bridge_conn holder NPC_Paris priority 1\n"
	    "post ops navgoal opsnav1\n"
	    "post corridor at -3161 -3295.5 -3912 yaw 90 radius 32 priority 2 holder tuvok\n"
	    "member Laird\n"
	    "member watch1 type Nelson at -2168 -3078 -3936 yaw 270\n";
	CHECK(ParseConfig(good, cfg, err));
	CHECK(err.empty());
	CHECK(cfg.maxCrew == 8 && cfg.reachBoundMs == 60000 && cfg.ackBoundMs == 4000 && cfg.saveBytesPerNpc == 128);
	CHECK(cfg.posts.size() == 3);
	if (cfg.posts.size() == 3) {
		const Post &a = cfg.posts[0], &b = cfg.posts[1], &c = cfg.posts[2];
		CHECK(a.name == "bridge_conn" && a.holder == "NPC_Paris" && a.priority == 1);
		CHECK(a.navgoal == "bridge_conn" && !a.hasOrigin); // the bare form borrows the navgoal of that name
		CHECK(b.name == "ops" && b.navgoal == "opsnav1" && b.holder.empty());
		CHECK(c.hasOrigin && c.navgoal.empty() && c.origin[0] == -3161 && c.origin[1] == -3295.5f && c.origin[2] == -3912);
		CHECK(c.hasYaw && c.yaw == 90 && c.radius == 32 && c.priority == 2 && c.holder == "tuvok");
	}
	CHECK(cfg.roster.size() == 2);
	if (cfg.roster.size() == 2) {
		CHECK(cfg.roster[0].name == "Laird" && !cfg.roster[0].spawn);
		CHECK(cfg.roster[1].spawn && cfg.roster[1].type == "Nelson" && cfg.roster[1].origin[2] == -3936 && cfg.roster[1].yaw == 270);
	}

	const char *bad[] = {
	    "post\n",
	    "post a\npost a\n",
	    "post a holder\n",
	    "post a colour blue\n",
	    "post a priority high\n",
	    "post a at 1 2\n",
	    "post a at 1 2 north\n",
	    "post a at 1 2 nan\n",
	    "post a at 1 2 3 navgoal b\n",
	    "post a radius 0\n",
	    "member\n",
	    "member a\nmember a\n",
	    "member a type Nelson\n",
	    "member a at 1 2 3\n",
	    "member a rank ensign\n",
	    "crew max 1\nmember a\nmember b\n",
	    "crew max 0\n",
	    "crew max 99\n",
	    "bound reach_ms soon\n",
	    "bound reach_ms -5\n",
	    "bound nonsense 5\n",
	    "bound sample_ms 1\n",
	    "bound coverage_percent 101\n",
	    "loose words\n",
	};
	for (const char *text : bad) {
		Config c;
		std::string e;
		CHECK(!ParseConfig(text, c, e));
		CHECK(e.rfind("line ", 0) == 0);
	}
	Config c;
	std::string e;
	CHECK(!ParseConfig("crew max 5\n\nbound oops 1\n", c, e));
	CHECK(e.rfind("line 3:", 0) == 0); // the line number is the file's, blank lines included
}

static void TestJudge()
{
	g_test = "verdict";
	Config cfg;
	Stats st;
	std::vector<Member> crew = Crew(6);
	for (int i = 0; i < 6; ++i) {
		crew[i].dutyPost = static_cast<int16_t>(i);
		crew[i].firstArrivalMs = 4000 + i;
		crew[i].flags = MF_ARRIVED;
	}
	RecordSample(st, 100);
	st.addresses = st.acks = 1;
	st.maxAckMs = 1200;
	const size_t bytes = Pack(crew, 0).size();
	CHECK(Judge(cfg, st, crew, bytes, 700000).pass);

	auto failsWith = [&](const Stats &s, const std::vector<Member> &m, size_t b, const char *needle) {
		const Verdict v = Judge(cfg, s, m, b, 700000);
		bool found = false;
		for (const std::string &f : v.failures)
			if (f.find(needle) != std::string::npos) found = true;
		return !v.pass && found;
	};

	Stats s = st;
	RecordSample(s, 80);
	CHECK(failsWith(s, crew, bytes, "coverage"));
	s = st; s.violations = 1;
	CHECK(failsWith(s, crew, bytes, "violation"));
	s = st; s.outOfWorld = 1;
	CHECK(failsWith(s, crew, bytes, "out-of-world"));
	s = st; s.missedAcks = 1;
	CHECK(failsWith(s, crew, bytes, "unacknowledged"));
	s = st; s.maxAckMs = cfg.ackBoundMs + 1;
	CHECK(failsWith(s, crew, bytes, "acknowledgement"));
	s = Stats();
	CHECK(failsWith(s, crew, bytes, "no occupancy samples"));
	CHECK(failsWith(st, crew, 256 * 6 + 1, "budget"));

	std::vector<Member> m = crew;
	m[2].flags |= MF_FAILED;
	CHECK(failsWith(st, m, bytes, "failed to reach"));
	m = crew; m[2].firstArrivalMs = -1;
	CHECK(failsWith(st, m, bytes, "has not reached"));
	CHECK(Judge(cfg, st, m, bytes, 1000).pass); // still inside the bound: not yet a failure
	m = crew; m[2].firstArrivalMs = cfg.reachBoundMs + 1;
	CHECK(failsWith(st, m, bytes, "took"));
	m = crew; m[2].dutyPost = NO_POST;
	CHECK(failsWith(st, m, bytes, "no post"));
	m = Crew(4);
	CHECK(failsWith(st, m, bytes, "crew present"));
	// The dead are not counted as present.
	m = crew; m[0].flags |= MF_UNAVAILABLE; m[1].flags |= MF_UNAVAILABLE;
	CHECK(failsWith(st, m, bytes, "crew present: 4"));
}

int main()
{
	TestArbitrationOrder();
	TestScriptAndCombatYield();
	TestInterruptionIsAPause();
	TestDecideEdges();
	TestAssignNearest();
	TestAssignAuthoredHolders();
	TestRestaff();
	TestDirectorWaitsForScripts();
	TestCoverage();
	TestSaveRoundTrip();
	TestSaveRejectsBadInput();
	TestSaveBudget();
	TestParseConfig();
	TestJudge();

	if (g_failures) {
		std::printf("%d check(s) failed\n", g_failures);
		return 1;
	}
	std::printf("crew_core: all checks passed\n");
	return 0;
}
