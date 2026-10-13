// The scenario declaration loader: the manifest's field contract, in the game.
//
// `tools/validator/declarations.py` owns the contract and validates a `scenario.json`;
// `tools/scenario/scenario_build.py` compiles a validated manifest to the flat `maps/<map>.scenario`
// this parses. The loader is deliberately thin and pure -- no game header, so it is unit-tested on
// its own (`tests/ship`), in the same shape as `module/crew/crew_core.{h,cpp}`.
//
// What it does, and what it does not:
//   * it carries every one of the declaration fields the design names (`Fields()`), and refuses a
//     directive it does not know -- a field the loader ignores is a field a check can catch;
//   * `Apply` instantiates the fields that have an *immediate* instantiation against the ship: the
//     systems it stresses, the staff it needs, the warmth beat, and the positions and lines, which it
//     resolves **from the post to a person** (the post-not-name law, `docs/the-entry-point.md`, part
//     three);
//   * it *carries* the fields the evaluator will read -- breeding conditions, cost, residue and the
//     decision -- rather than half-building the evaluator. Reading a scenario's breeding conditions
//     and deciding it fires is the next pass (`docs/evidence/the-manifest-contract.md`).
//
// A malformed declaration is refused whole, with the line number, the way a malformed `.crew` file
// is: a half-read configuration is worse than none.

#ifndef LWH_SCENARIO_CORE_H
#define LWH_SCENARIO_CORE_H

#include "ship_core.h"

#include <string>
#include <vector>

namespace scenario {

// One breeding condition: a readable state fact, an operator, and the value it compares to.
struct Condition {
	std::string state;
	std::string op;
	double value = 0.0;
};

// One system the scenario stresses: a system id or `<system>.<arc>`, and how hard; or an `off` line.
struct SystemStress {
	std::string ref;
	double amount = 0.0;
	bool off = false;
	bool read_only = false;   // the scenario reads/writes it but the premise changes nothing yet
};

// One effect a cost or a residue writes. The kind is from the contract's vocabulary; the arguments
// are kept as strings and resolved by the evaluator, never here.
struct Effect {
	std::string branch;             // answer | ignore | residue | option:<n>
	std::string kind;               // write_off, damage_system, ...
	std::vector<std::pair<std::string, std::string>> args;
	std::string Arg(const std::string &key) const;
};

// One position the scenario can be experienced from (docs/scenario-atlas.md, "the positions").
struct Position {
	std::string post;
	std::vector<std::string> information;
	std::vector<std::string> authority;
	std::vector<std::string> responsible_for;
	std::string decision;
};

struct Option {
	std::string label;
	std::vector<Effect> cost;
};

// One line of content: addressed to a post, delivered a declared way, never naming a person.
struct Line {
	std::string to;
	std::string delivery;
	std::string text;
};

struct Decl {
	std::string name;
	std::string canon_root;
	std::vector<Condition> conditions;
	std::vector<SystemStress> systems;
	std::vector<std::string> staff;
	std::string answer_text, ignore_text;
	std::vector<Effect> answer_effects, ignore_effects, residue_effects;
	std::vector<std::string> residue;                  // the lasting thing, as words
	std::vector<std::string> visibility;               // "station:<name>", "system:<ref>", "deck:<n>"
	std::vector<std::string> warmth;                   // the beats, by name
	std::vector<Position> positions;
	std::string decision_question;
	std::vector<Option> options;
	std::vector<Line> lines;

	bool Empty() const { return name.empty(); }
};

// Parse the flat declaration. Returns false and fills `error` with `line N: ...` on the first fault.
bool Parse(const std::string &text, Decl &out, std::string &error);

// The declaration field names this loader carries. The coverage check (`scripts/scenario-check.sh`)
// compares this to the schema's own list, so a field on one side and not the other fails a check.
const char *const *Fields(int *count);

// The person a post resolves to, or -1. A post is a command seat ("the chair"), a department
// ("engineering"), or a system id ("sickbay"): the simulation resolves it, content never names it.
int ResolvePost(const ship::Ship &s, const std::string &post);

// Instantiate what a declaration can instantiate now, writing one line per action to `out`. The
// fields the evaluator owns are carried and named, not applied.
void Apply(ship::Ship &s, const Decl &d, std::vector<std::string> *out);

// ---- the evaluator: what reads the breeding conditions and decides the scenario fires ------------
//
// `docs/scenario-manifest.md` states it: the loader *carries* the breeding conditions, and reading
// them and deciding a scenario fires is the evaluator. This is it. It reads each condition against
// the ship's *actual* state -- the stores, the crew, the clock, the record -- and returns a verdict
// a caller can act on and an author can read:
//
//   * `fires` names the condition that fired it (the one met by the narrowest margin, the load the
//     premise leans on hardest);
//   * a scenario that does not fire names its **nearest miss**, quoted with the ship's own number;
//   * a condition that names state no reader carries is **UNREADABLE** and is named -- a field that
//     cannot recruit anything;
//   * a condition that is true for *every* value the state can take (`ALWAYS`) or for *no* value
//     (`NEVER`) is named as **degenerate**: a scenario recruited by a tautology recruits itself, and
//     one recruited by a contradiction can never fire. Neither is ordinary.
//
// The evaluator consumes the conditions and the decision; `Recruit` instantiates the scenario
// through the loader (`Apply`) and records it in the ship's ledger; `Resolve` applies the chosen
// branch's cost and writes the residue. So the seam is named: the evaluator *reads* the conditions
// and the decision, the simulation *applies* the systems, the staff, the warmth, the cost and the
// residue.

enum ConditionOutcome : uint8_t {
	COND_MET = 0,      // the state satisfies it now
	COND_UNMET,        // the state does not satisfy it now
	COND_UNREADABLE,   // it names state no reader carries
	COND_ALWAYS,       // true for every value the state can take (a tautology: it recruits nothing)
	COND_NEVER,        // true for no value the state can take (a contradiction: it can never fire)
};
const char *ConditionOutcomeName(uint8_t o);

struct ConditionReading {
	Condition cond;
	uint8_t outcome = COND_UNMET;
	bool has_now = false;   // the ship's current value was readable
	double now = 0.0;       // ... and is this
	double margin = 0.0;    // signed slack: how far past the threshold it is met (>= 0), or short (< 0)
	std::string note;       // why, in words, for the report
};

struct Evaluation {
	bool fires = false;
	bool already_fired = false;         // `Recruit` only: the ledger holds it
	bool degenerate = false;            // an ALWAYS or a NEVER condition was found
	int fired_by = -1;                  // the decisive condition's index, when it fires
	int nearest_miss = -1;              // the closest unmet condition's index, or -1
	std::vector<ConditionReading> readings;
	std::vector<std::string> unreadable; // the states named with no reader, deduplicated
	std::vector<std::string> degenerate_conditions; // the tautologies and contradictions, named
	std::string reason;                 // one line, in words: why it fired, or did not
};

// Read one breeding-condition state against the ship. Returns false (and leaves `out`) when the
// state has no reader -- the condition cannot recruit. Scoped states (`system_condition`,
// `deck_life_support`, `minutes_of_air`) name no subject in the condition tuple, so they are read as
// the ship's worst: the most ailing system, the least-supported deck, the soonest-running-out deck.
// That default is a decision recorded in `docs/scenario-manifest.md`; it is why those states are
// also named by `UnreadableStates()` -- the reader answers, but the shape cannot say *which*.
bool ReadState(const ship::Ship &s, const std::string &state, double *out, std::string *note);

// The range a readable state can take, for the tautology/contradiction test. `hi` may be +inf. False
// for a state with no reader.
bool StateRange(const std::string &state, double *lo, double *hi);

// Evaluate every condition against the ship. The ship is not changed.
Evaluation Evaluate(const ship::Ship &s, const Decl &d);

// Recruit: evaluate; if it fires and the ledger does not already hold it, instantiate it (`Apply`)
// and record it in flight. Returns true if it fired on this call. `out` receives the transcript.
bool Recruit(ship::Ship &s, const Decl &d, std::vector<std::string> *out);

// The player's decision on a fired scenario: apply the chosen branch's effects and write the
// residue. `branch` is SCENARIO_ANSWER, SCENARIO_IGNORE, or SCENARIO_OPTION_BASE + an option's
// index. Returns false, changing nothing, if the scenario has not fired or the branch is not one the
// declaration offers. Records the branch in the ledger.
bool Resolve(ship::Ship &s, const Decl &d, int branch, std::vector<std::string> *out);

// Apply one effect of the contract's vocabulary, through the primitive the design names. `post`,
// where an effect acts on a person, is resolved to whoever holds it -- the law holds inside an
// effect too. Returns false, changing nothing, for an unknown kind.
bool ApplyEffect(ship::Ship &s, const Effect &e, std::vector<std::string> *out);

// The breeding-condition states the contract names, split by whether this build can read them.
// `UnreadableStates` is the list the report carries: the states the design implies but no state
// carries *as declared*, so the next pass must give the condition shape a subject.
const char *const *ReadableStates(int *count);
const char *const *UnreadableStates(int *count);
// The design's own breeding facts that no manifest key and no state reaches at all
// (`docs/scenario-atlas.md`: a contact, a battle, a beacon, a breach, structural integrity, the
// clock). Named for the next pass -- a condition cannot even be declared for them.
const char *const *DesignImpliedStates(int *count);

} // namespace scenario

#endif // LWH_SCENARIO_CORE_H
