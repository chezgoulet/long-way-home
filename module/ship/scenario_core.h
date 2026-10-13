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

} // namespace scenario

#endif // LWH_SCENARIO_CORE_H
