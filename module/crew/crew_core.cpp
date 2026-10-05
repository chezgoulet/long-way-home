// crew_core.cpp -- see crew_core.h. No game headers in this file.

#include "crew_core.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <sstream>

namespace crew {

// ---- arbitration ---------------------------------------------------------------------------

Level Arbitrate(const Signals &s)
{
	if (s.scripted) return LEVEL_SCRIPT;
	if (s.hostile) return LEVEL_COMBAT;
	if (s.overridden) return LEVEL_DIRECTOR;
	if (s.hasPost) return LEVEL_DUTY;
	return LEVEL_IDLE;
}

Action Decide(const Member &m, const Signals &s, bool atPost)
{
	const Level level = Arbitrate(s);
	if (level <= LEVEL_COMBAT) return ACT_YIELD;
	if (m.flags & (MF_FAILED | MF_UNAVAILABLE)) return ACT_IDLE;
	if (m.EffectivePost() == NO_POST) return ACT_IDLE;
	return atPost ? ACT_HOLD : ACT_TRAVEL;
}

// ---- configuration -------------------------------------------------------------------------

static bool ParseInt(const std::string &tok, int &out)
{
	if (tok.empty()) return false;
	char *end = nullptr;
	const long v = std::strtol(tok.c_str(), &end, 10);
	if (*end != '\0' || v < -100000000 || v > 100000000) return false;
	out = static_cast<int>(v);
	return true;
}

static bool ParseFloat(const std::string &tok, float &out)
{
	if (tok.empty()) return false;
	char *end = nullptr;
	const double v = std::strtod(tok.c_str(), &end);
	if (*end != '\0' || !(v > -1.0e7 && v < 1.0e7)) return false; // also rejects NaN
	out = static_cast<float>(v);
	return true;
}

namespace {

// Reads `key value...` attribute pairs off the rest of a line.
struct Attrs {
	const std::vector<std::string> &tok;
	size_t i;
	std::string why;

	bool Done() const { return i >= tok.size(); }
	const std::string &Key() { return tok[i++]; }
	bool Word(std::string &out)
	{
		if (Done()) return Fail("has no value");
		out = tok[i++];
		return true;
	}
	bool Int(int &out)
	{
		if (Done()) return Fail("has no value");
		if (!ParseInt(tok[i], out)) return Fail("has a bad number '" + tok[i] + "'");
		++i;
		return true;
	}
	bool Float(float &out)
	{
		if (Done()) return Fail("has no value");
		if (!ParseFloat(tok[i], out)) return Fail("has a bad number '" + tok[i] + "'");
		++i;
		return true;
	}
	bool Vec3(float *out) { return Float(out[0]) && Float(out[1]) && Float(out[2]); }
	bool Fail(const std::string &w)
	{
		why = w;
		return false;
	}
};

} // namespace

bool ParseConfig(const std::string &text, Config &cfg, std::string &error)
{
	std::istringstream in(text);
	std::string line;
	int lineNo = 0;
	auto fail = [&](const std::string &why) {
		error = "line " + std::to_string(lineNo) + ": " + why;
		return false;
	};

	while (std::getline(in, line)) {
		++lineNo;
		const size_t hash = line.find('#');
		if (hash != std::string::npos) line.erase(hash);
		std::istringstream ls(line);
		std::vector<std::string> tok;
		for (std::string t; ls >> t;) tok.push_back(t);
		if (tok.empty()) continue;

		if (tok[0] == "post") {
			if (tok.size() < 2) return fail("post needs a name");
			if (static_cast<int>(cfg.posts.size()) >= MAX_POSTS) return fail("too many posts");
			Post p;
			p.name = tok[1];
			for (const Post &q : cfg.posts)
				if (q.name == p.name) return fail("duplicate post '" + p.name + "'");
			Attrs a{tok, 2, ""};
			while (!a.Done()) {
				const std::string key = a.Key();
				bool ok;
				if (key == "at") ok = a.Vec3(p.origin), p.hasOrigin = true;
				else if (key == "navgoal") ok = a.Word(p.navgoal);
				else if (key == "yaw") ok = a.Float(p.yaw), p.hasYaw = true;
				else if (key == "radius") ok = a.Int(p.radius) && (p.radius > 0 || a.Fail("must be positive"));
				else if (key == "holder") ok = a.Word(p.holder);
				else if (key == "priority") ok = a.Int(p.priority);
				else return fail("unknown post attribute '" + key + "'");
				if (!ok) return fail("post attribute '" + key + "' " + a.why);
			}
			if (p.hasOrigin && !p.navgoal.empty()) return fail("post '" + p.name + "' gives both 'at' and 'navgoal'");
			if (!p.hasOrigin && p.navgoal.empty()) p.navgoal = p.name; // `post <targetname>`
			cfg.posts.push_back(p);
			continue;
		}

		if (tok[0] == "member") {
			if (tok.size() < 2) return fail("member needs a name");
			if (static_cast<int>(cfg.roster.size()) >= MAX_CREW) return fail("too many members");
			Roster r;
			r.name = tok[1];
			for (const Roster &q : cfg.roster)
				if (q.name == r.name) return fail("duplicate member '" + r.name + "'");
			bool at = false;
			Attrs a{tok, 2, ""};
			while (!a.Done()) {
				const std::string key = a.Key();
				bool ok;
				if (key == "type") ok = a.Word(r.type);
				else if (key == "at") ok = a.Vec3(r.origin), at = true;
				else if (key == "yaw") ok = a.Float(r.yaw);
				else return fail("unknown member attribute '" + key + "'");
				if (!ok) return fail("member attribute '" + key + "' " + a.why);
			}
			if (at != !r.type.empty())
				return fail("member '" + r.name + "' needs both 'type' and 'at' to be spawned, or neither to name a placed NPC");
			r.spawn = at;
			cfg.roster.push_back(r);
			continue;
		}

		if (tok.size() != 3) return fail("expected '<section> <key> <value>'");
		int v = 0;
		if (!ParseInt(tok[2], v) || v < 0) return fail("bad number '" + tok[2] + "'");
		const std::string key = tok[0] + " " + tok[1];
		if (key == "crew max") {
			if (v < 1 || v > MAX_CREW) return fail("crew max must be 1.." + std::to_string(MAX_CREW));
			cfg.maxCrew = v;
		} else if (key == "bound reach_ms") cfg.reachBoundMs = v;
		else if (key == "bound ack_ms") cfg.ackBoundMs = v;
		else if (key == "bound stuck_ms") cfg.stuckMs = v;
		else if (key == "bound stuck_strikes") cfg.stuckStrikes = v;
		else if (key == "bound sample_ms") {
			if (v < 100) return fail("sample_ms must be at least 100");
			cfg.sampleMs = v;
		} else if (key == "bound coverage_percent") {
			if (v > 100) return fail("coverage_percent must be 0..100");
			cfg.coveragePercent = v;
		} else if (key == "budget save_bytes_per_npc") cfg.saveBytesPerNpc = v;
		else return fail("unknown setting '" + key + "'");
	}
	if (static_cast<int>(cfg.roster.size()) > cfg.maxCrew)
		return fail(std::to_string(cfg.roster.size()) + " members declared but crew max is " + std::to_string(cfg.maxCrew));
	return true;
}

// ---- assignment ----------------------------------------------------------------------------

static float DistSq(const float *a, const float *b)
{
	const float dx = a[0] - b[0], dy = a[1] - b[1], dz = a[2] - b[2];
	return dx * dx + dy * dy + dz * dz;
}

static bool NameEq(const std::string &a, const std::string &b)
{
	if (a.size() != b.size()) return false;
	for (size_t i = 0; i < a.size(); ++i)
		if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i])))
			return false;
	return true;
}

void AssignPosts(std::vector<Member> &members, const std::vector<Candidate> &cands,
                 const std::vector<Post> &posts)
{
	std::vector<bool> taken(posts.size(), false);
	for (const Member &m : members)
		if (m.dutyPost >= 0 && m.dutyPost < static_cast<int>(posts.size())) taken[m.dutyPost] = true;

	// Authored holders first: a targetname match outranks a type match.
	for (int pass = 0; pass < 2; ++pass) {
		for (size_t p = 0; p < posts.size(); ++p) {
			if (taken[p] || posts[p].holder.empty()) continue;
			for (size_t i = 0; i < members.size() && i < cands.size(); ++i) {
				if (members[i].dutyPost != NO_POST) continue;
				const std::string &name = pass == 0 ? cands[i].targetname : cands[i].type;
				if (name.empty() || !NameEq(name, posts[p].holder)) continue;
				members[i].dutyPost = static_cast<int16_t>(p);
				taken[p] = true;
				break;
			}
		}
	}

	// The rest: nearest free pair, repeatedly. A post reserved for a holder who is not present
	// stays empty rather than being handed to whoever is closest.
	struct Pair { float d; int m; int p; };
	std::vector<Pair> pairs;
	for (size_t i = 0; i < members.size() && i < cands.size(); ++i) {
		if (members[i].dutyPost != NO_POST) continue;
		for (size_t p = 0; p < posts.size(); ++p) {
			if (taken[p] || !posts[p].holder.empty()) continue;
			pairs.push_back({DistSq(cands[i].origin, posts[p].origin), static_cast<int>(i), static_cast<int>(p)});
		}
	}
	std::sort(pairs.begin(), pairs.end(), [](const Pair &a, const Pair &b) {
		if (a.d != b.d) return a.d < b.d;
		if (a.m != b.m) return a.m < b.m;
		return a.p < b.p;
	});
	for (const Pair &pr : pairs) {
		if (members[pr.m].dutyPost != NO_POST || taken[pr.p]) continue;
		members[pr.m].dutyPost = static_cast<int16_t>(pr.p);
		taken[pr.p] = true;
	}
}

// ---- the director --------------------------------------------------------------------------

// Importance order: priority, then table position. Lower rank = more important.
static bool MoreImportant(const std::vector<Post> &posts, int a, int b)
{
	if (posts[a].priority != posts[b].priority) return posts[a].priority < posts[b].priority;
	return a < b;
}

int Restaff(std::vector<Member> &members, const std::vector<Post> &posts,
            const std::vector<bool> &scripted)
{
	int changes = 0;
	const int n = static_cast<int>(members.size());
	auto isScripted = [&](int i) { return i < static_cast<int>(scripted.size()) && scripted[i]; };
	auto available = [&](int i) { return !(members[i].flags & (MF_UNAVAILABLE | MF_FAILED)); };

	// An override whose reason has gone (the post's own holder is back) is cleared, which is the
	// "returns to its routine when the interrupt clears" corollary. A scripted member keeps its
	// override until the script lets go: the director does not touch it either way.
	for (int i = 0; i < n; ++i) {
		if (members[i].overridePost == NO_POST || isScripted(i)) continue;
		bool ownerBack = false;
		for (int j = 0; j < n; ++j)
			if (j != i && members[j].dutyPost == members[i].overridePost && available(j)) ownerBack = true;
		if (ownerBack || !available(i)) {
			members[i].overridePost = NO_POST;
			++changes;
		}
	}

	std::vector<int> order;
	for (int p = 0; p < static_cast<int>(posts.size()); ++p) order.push_back(p);
	std::sort(order.begin(), order.end(), [&](int a, int b) { return MoreImportant(posts, a, b); });

	for (int p : order) {
		bool assigned = false, staffed = false;
		for (int i = 0; i < n; ++i) {
			if (members[i].dutyPost == p) assigned = true;
			if (available(i) && members[i].EffectivePost() == p) staffed = true;
		}
		if (!assigned || staffed) continue; // never assigned is not a vacancy; it is an unused post

		// Take whoever holds the least important staffed post, provided it matters less than p.
		int pick = -1;
		for (int i = 0; i < n; ++i) {
			if (!available(i) || isScripted(i) || members[i].overridePost != NO_POST) continue;
			const int own = members[i].dutyPost;
			if (own == NO_POST) { pick = i; break; } // a spare is always the first choice
			if (!MoreImportant(posts, p, own)) continue;
			if (pick == -1 || MoreImportant(posts, members[pick].dutyPost, own)) pick = i;
		}
		if (pick != -1) {
			members[pick].overridePost = static_cast<int16_t>(p);
			++changes;
		}
	}
	return changes;
}

// ---- statistics ----------------------------------------------------------------------------

int CoveragePercent(const std::vector<Member> &members, const std::vector<bool> &atPost)
{
	// A post counts once however many members point at it, and a vacated duty post still counts as
	// assigned: restaffing moves the hole, it does not hide it.
	std::vector<int> assigned, occupied;
	auto add = [](std::vector<int> &v, int p) {
		if (p != NO_POST && std::find(v.begin(), v.end(), p) == v.end()) v.push_back(p);
	};
	for (size_t i = 0; i < members.size(); ++i) {
		add(assigned, members[i].dutyPost);
		if (i < atPost.size() && atPost[i] && !(members[i].flags & MF_UNAVAILABLE))
			add(occupied, members[i].EffectivePost());
	}
	if (assigned.empty()) return 100;
	int n = 0;
	for (int p : occupied)
		if (std::find(assigned.begin(), assigned.end(), p) != assigned.end()) ++n;
	return n * 100 / static_cast<int>(assigned.size());
}

void RecordSample(Stats &st, int coveragePercent)
{
	++st.samples;
	st.lastCoveragePercent = coveragePercent;
	st.minCoveragePercent = std::min(st.minCoveragePercent, coveragePercent);
}

Verdict Judge(const Config &cfg, const Stats &st, const std::vector<Member> &members,
              size_t saveBytes, int nowMs)
{
	Verdict v;
	auto fail = [&](const std::string &s) { v.failures.push_back(s); };

	int present = 0, assigned = 0;
	for (const Member &m : members) {
		if (m.flags & MF_UNAVAILABLE) continue;
		++present;
		if (m.dutyPost == NO_POST) continue;
		++assigned;
		if (m.flags & MF_FAILED)
			fail("crew " + std::to_string(m.ent) + " failed to reach its post");
		else if (m.firstArrivalMs < 0 && nowMs > cfg.reachBoundMs)
			fail("crew " + std::to_string(m.ent) + " has not reached its post within the bound");
		else if (m.firstArrivalMs > cfg.reachBoundMs)
			fail("crew " + std::to_string(m.ent) + " took " + std::to_string(m.firstArrivalMs) + " ms to reach its post");
	}
	if (present < 5 || present > cfg.maxCrew)
		fail("crew present: " + std::to_string(present) + " (need 5.." + std::to_string(cfg.maxCrew) + ")");
	if (assigned < present)
		fail(std::to_string(present - assigned) + " crew have no post");
	if (st.samples == 0)
		fail("no occupancy samples were taken");
	else if (st.minCoveragePercent < cfg.coveragePercent)
		fail("minimum post coverage " + std::to_string(st.minCoveragePercent) + "% is below " +
		     std::to_string(cfg.coveragePercent) + "%");
	if (st.outOfWorld) fail(std::to_string(st.outOfWorld) + " out-of-world event(s)");
	if (st.violations) fail(std::to_string(st.violations) + " script-precedence violation(s)");
	if (st.missedAcks) fail(std::to_string(st.missedAcks) + " address(es) went unacknowledged");
	if (st.maxAckMs > cfg.ackBoundMs)
		fail("slowest acknowledgement " + std::to_string(st.maxAckMs) + " ms exceeds the bound");
	if (!members.empty() && saveBytes > static_cast<size_t>(cfg.saveBytesPerNpc) * members.size())
		fail("save record " + std::to_string(saveBytes) + " bytes exceeds the per-NPC budget");

	v.pass = v.failures.empty();
	return v;
}

// ---- persistence ---------------------------------------------------------------------------

uint32_t PostTableHash(const std::vector<Post> &posts)
{
	uint32_t h = 2166136261u; // FNV-1a
	for (const Post &p : posts) {
		for (char c : p.name) {
			h ^= static_cast<uint8_t>(std::tolower(static_cast<unsigned char>(c)));
			h *= 16777619u;
		}
		h ^= 0xffu;
		h *= 16777619u;
	}
	return h;
}

static void Put16(std::vector<uint8_t> &b, uint16_t v)
{
	b.push_back(static_cast<uint8_t>(v));
	b.push_back(static_cast<uint8_t>(v >> 8));
}

static void Put32(std::vector<uint8_t> &b, uint32_t v)
{
	Put16(b, static_cast<uint16_t>(v));
	Put16(b, static_cast<uint16_t>(v >> 16));
}

static uint16_t Get16(const uint8_t *p) { return static_cast<uint16_t>(p[0] | (p[1] << 8)); }
static uint32_t Get32(const uint8_t *p) { return Get16(p) | (static_cast<uint32_t>(Get16(p + 2)) << 16); }

std::vector<uint8_t> Pack(const std::vector<Member> &members, uint32_t postHash)
{
	std::vector<uint8_t> b;
	b.reserve(SAVE_HEADER_BYTES + SAVE_MEMBER_BYTES * members.size());
	Put32(b, SAVE_MAGIC);
	Put16(b, SAVE_VERSION);
	Put16(b, static_cast<uint16_t>(members.size()));
	Put32(b, postHash);
	Put32(b, 0); // reserved
	for (const Member &m : members) {
		Put16(b, static_cast<uint16_t>(m.ent));
		Put16(b, static_cast<uint16_t>(m.dutyPost));
		Put16(b, static_cast<uint16_t>(m.overridePost));
		Put16(b, static_cast<uint16_t>(m.goalPost));
		b.push_back(m.action);
		b.push_back(m.flags);
		Put16(b, 0); // reserved
		Put32(b, static_cast<uint32_t>(m.scheduleCursor));
		Put32(b, static_cast<uint32_t>(m.firstArrivalMs));
	}
	return b;
}

bool Unpack(const uint8_t *data, size_t len, uint32_t postHash, std::vector<Member> &members)
{
	if (!data || len < SAVE_HEADER_BYTES) return false;
	if (Get32(data) != SAVE_MAGIC || Get16(data + 4) != SAVE_VERSION) return false;
	const size_t count = Get16(data + 6);
	if (Get32(data + 8) != postHash) return false;
	if (count > static_cast<size_t>(MAX_CREW) || len != SAVE_HEADER_BYTES + count * SAVE_MEMBER_BYTES) return false;

	std::vector<Member> out(count);
	for (size_t i = 0; i < count; ++i) {
		const uint8_t *p = data + SAVE_HEADER_BYTES + i * SAVE_MEMBER_BYTES;
		Member &m = out[i];
		m.ent = static_cast<int16_t>(Get16(p));
		m.dutyPost = static_cast<int16_t>(Get16(p + 2));
		m.overridePost = static_cast<int16_t>(Get16(p + 4));
		m.goalPost = static_cast<int16_t>(Get16(p + 6));
		m.action = p[8];
		m.flags = p[9];
		m.scheduleCursor = static_cast<int32_t>(Get32(p + 12));
		m.firstArrivalMs = static_cast<int32_t>(Get32(p + 16));
		if (m.dutyPost < NO_POST || m.dutyPost >= MAX_POSTS || m.overridePost < NO_POST ||
		    m.overridePost >= MAX_POSTS || m.goalPost < NO_POST || m.goalPost >= MAX_POSTS ||
		    m.action > ACT_IDLE)
			return false;
	}
	members.swap(out);
	return true;
}

} // namespace crew
