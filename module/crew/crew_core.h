// crew_core.h -- the reactive-crew direction layer, engine-independent half.
//
// Everything here is plain data and pure functions: arbitration, post assignment, the director's
// restaffing rule, the run statistics the G3 gate is judged by, and the save record. It includes
// no game header, so it is compiled and tested on its own (tests/crew) as well as inside the game
// module. The half that touches entities is g_crew.cpp.
//
// Specification: docs/g3-reactive-crew.md. The arbitration order in Arbitrate() is the charter's
// rule; deviations are bugs.

#ifndef LWH_CREW_CORE_H
#define LWH_CREW_CORE_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace crew {

const int MAX_POSTS = 64;
const int MAX_CREW = 32;        // G3 uses 5-10; the ceiling leaves room for G4's 20-30
const int NO_POST = -1;

// ---- arbitration ---------------------------------------------------------------------------

// Precedence, highest first. The numeric values are the charter's level numbers.
enum Level : uint8_t {
	LEVEL_SCRIPT = 1,    // an ICARUS sequence owns the NPC
	LEVEL_COMBAT = 2,    // an enemy, or being hurt
	LEVEL_DIRECTOR = 3,  // the director has moved the NPC to another post
	LEVEL_DUTY = 4,      // the NPC's own post
	LEVEL_IDLE = 5,      // nothing above applies
};

struct Signals {
	bool scripted = false;   // any script task pending, or a state/goal the layer did not set
	bool hostile = false;    // an enemy is held or perceived
	bool overridden = false; // the director has assigned an override post
	bool hasPost = false;    // a duty post is assigned
};

Level Arbitrate(const Signals &s);

enum Action : uint8_t {
	ACT_YIELD = 0,  // levels 1-2: the layer issues nothing and releases any goal it set
	ACT_TRAVEL,     // walk to the effective post
	ACT_HOLD,       // stand at the effective post
	ACT_IDLE,       // no post to hold
};

// ---- posts and crew -------------------------------------------------------------------------

// A post exists whether or not anyone holds it. It is a place, a facing and a radius: either
// authored outright (`at`), or borrowed from a waypoint_navgoal already in the map (`navgoal`).
struct Post {
	std::string name;
	std::string navgoal;   // targetname to take the position from; empty when `at` was given
	std::string holder;    // authored holder (NPC targetname or type); empty = assign nearest
	int priority = 0;      // lower is more important; ties broken by table order
	bool hasOrigin = false;
	bool hasYaw = false;
	float origin[3] = {0, 0, 0};
	float yaw = 0;
	int radius = 24;
};

// A crew member the scenario declares. With a type and a position the layer spawns it (an existing
// character type -- no new art); with neither, it names an NPC the map already places.
struct Roster {
	std::string name;
	std::string type;
	bool spawn = false;
	float origin[3] = {0, 0, 0};
	float yaw = 0;
};

enum MemberFlags : uint8_t {
	MF_OWNS_GOAL = 1 << 0,  // the goal entity currently on the NPC was set by this layer
	MF_ARRIVED = 1 << 1,    // has reached its post at least once
	MF_FAILED = 1 << 2,     // gave up reaching its post; reported, and no longer driven
	MF_UNAVAILABLE = 1 << 3 // dead or removed: its post is empty until restaffed
};

struct Member {
	// persisted (see Pack/Unpack)
	int16_t ent = -1;
	int16_t dutyPost = NO_POST;      // the routine: what the NPC should be doing
	int16_t overridePost = NO_POST;  // the director's reassignment, if any
	int16_t goalPost = NO_POST;      // where it is currently headed, if anywhere
	uint8_t action = ACT_IDLE;
	uint8_t flags = 0;
	int32_t scheduleCursor = 0;      // unused in G3, present for G4
	int32_t firstArrivalMs = -1;     // from level start; -1 = not yet

	// runtime only
	uint8_t level = LEVEL_IDLE;
	int32_t progressMs = 0;          // when it last made progress toward its goal
	float progressPos[3] = {0, 0, 0};
	uint8_t stuckStrikes = 0;
	int32_t addressedMs = -1;        // pending address awaiting acknowledgement
	int32_t nextBumpMs = 0;

	int EffectivePost() const { return overridePost != NO_POST ? overridePost : dutyPost; }
};

// What the layer should do with a member this think. `atPost` is the glue's distance test.
Action Decide(const Member &m, const Signals &s, bool atPost);

// ---- configuration (maps/<map>.crew) --------------------------------------------------------

struct Config {
	int maxCrew = 10;
	int reachBoundMs = 90000;     // every NPC reaches its post within this, or reports failure
	int ackBoundMs = 5000;        // address -> acknowledgement
	int stuckMs = 10000;          // no progress for this long while travelling = one strike
	int stuckStrikes = 3;         // strikes before the member is marked failed
	int sampleMs = 5000;          // occupancy sampling interval
	int saveBytesPerNpc = 256;    // persistence budget
	int coveragePercent = 90;     // minimum sampled occupancy
	std::vector<Post> posts;      // authored posts; empty = every navgoal in the map
	std::vector<Roster> roster;   // declared crew; empty = every Starfleet NPC present, up to maxCrew
};

// Parses the text of a .crew file. Returns false and sets `error` (with a line number) on the
// first malformed line; `cfg` is left with whatever preceded it.
bool ParseConfig(const std::string &text, Config &cfg, std::string &error);

// ---- assignment and the director --------------------------------------------------------------

struct Candidate {
	std::string targetname;
	std::string type;
	float origin[3] = {0, 0, 0};
};

// Assigns duty posts to members that have none. Authored holders are honoured first; the rest
// pair off greedily by distance, ties broken by member then post index, so the result is
// deterministic for a given map. `cands[i]` describes `members[i]`.
void AssignPosts(std::vector<Member> &members, const std::vector<Candidate> &cands,
                 const std::vector<Post> &posts);

// The director's one G3 duty: keep the most important posts staffed. For each post whose holder is
// unavailable, pull the available member holding the least important staffed post below it.
// Members for which `scripted[i]` is set are never taken -- the director waits. Returns the number
// of overrides issued or cleared.
int Restaff(std::vector<Member> &members, const std::vector<Post> &posts,
            const std::vector<bool> &scripted);

// ---- run statistics: the numbers the gate is judged by ---------------------------------------

struct Stats {
	int32_t samples = 0;
	int32_t minCoveragePercent = 100;
	int32_t lastCoveragePercent = 0;
	int32_t yields[6] = {0, 0, 0, 0, 0, 0};  // indexed by Level
	int32_t preemptions = 0;      // a script took a goal the layer had set (expected, counted)
	int32_t handovers = 0;        // a script began while the NPC walked for the layer; released that turn
	int32_t violations = 0;       // the layer reached a write to an NPC a script owned (must be 0)
	int32_t scriptRuns = 0;       // the harness's precedence test: scripts run on a post-holder,
	int32_t scriptYields = 0;     //   ... the layer yielding on its next turn,
	int32_t scriptResumes = 0;    //   ... and the member back on duty when the script ended
	int32_t stuckEvents = 0;
	int32_t outOfWorld = 0;
	int32_t addresses = 0;
	int32_t acks = 0;
	int32_t missedAcks = 0;
	int32_t maxAckMs = 0;
	int32_t bumps = 0;
	int32_t restaffs = 0;
	int64_t frameNs = 0;
	int64_t maxFrameNs = 0;
	int32_t frames = 0;
};

// Occupied posts as a percentage of assigned posts (100 when none are assigned).
int CoveragePercent(const std::vector<Member> &members, const std::vector<bool> &atPost);
void RecordSample(Stats &st, int coveragePercent);

struct Verdict {
	bool pass = false;
	std::vector<std::string> failures;
};

// Applies the G3 criteria that are decidable from the run itself. `saveBytes` is the size of the
// packed save record; `nowMs` is time since level start.
Verdict Judge(const Config &cfg, const Stats &st, const std::vector<Member> &members,
              size_t saveBytes, int nowMs);

// ---- persistence ------------------------------------------------------------------------------

const uint32_t SAVE_MAGIC = 0x57455243; // 'CREW', little-endian
const uint16_t SAVE_VERSION = 1;
const size_t SAVE_HEADER_BYTES = 16;
const size_t SAVE_MEMBER_BYTES = 20;

// A stable fingerprint of the post table, so a save is never applied to a different set of posts.
uint32_t PostTableHash(const std::vector<Post> &posts);

// Fixed-width little-endian, independent of the host's struct layout.
std::vector<uint8_t> Pack(const std::vector<Member> &members, uint32_t postHash);

// Returns false (and leaves `members` untouched) if the record is truncated, of another version,
// or was written against a different post table.
bool Unpack(const uint8_t *data, size_t len, uint32_t postHash, std::vector<Member> &members);

} // namespace crew

#endif
