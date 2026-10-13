// The scenario declaration loader -- see scenario_core.h for what it is and what it deliberately is
// not. Pure C++ over ship_core: no game header, so tests/ship can build and drive it.

#include "scenario_core.h"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>

namespace scenario {

using ship::Ship;
using ship::CrewMember;

// ---- small helpers ------------------------------------------------------------------------------

static std::string Lower(const std::string &s)
{
	std::string out = s;
	for (char &c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	return out;
}

static bool StartsWith(const std::string &s, const char *prefix)
{
	const size_t n = std::strlen(prefix);
	return s.size() >= n && std::strncmp(s.c_str(), prefix, n) == 0;
}

// Split a line on whitespace, honouring "quoted strings" as one token. The builder quotes every text
// field, so a line's own words never have to be guessed at.
static std::vector<std::string> Tokenize(const std::string &line, std::string *error, int lineNo)
{
	std::vector<std::string> out;
	size_t i = 0;
	while (i < line.size()) {
		while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i]))) ++i;
		if (i >= line.size() || line[i] == '#') break;
		std::string tok;
		if (line[i] == '"') {
			++i;
			while (i < line.size() && line[i] != '"') tok.push_back(line[i++]);
			if (i >= line.size()) {
				*error = "line " + std::to_string(lineNo) + ": unterminated quoted string";
				return {};
			}
			++i; // closing quote
		} else {
			while (i < line.size() && !std::isspace(static_cast<unsigned char>(line[i])) && line[i] != '#') {
				if (line[i] == '"') {
					++i;
					while (i < line.size() && line[i] != '"') tok.push_back(line[i++]);
					if (i >= line.size()) {
						*error = "line " + std::to_string(lineNo) + ": unterminated quoted string";
						return {};
					}
					++i; // closing quote
				} else {
					tok.push_back(line[i++]);
				}
			}
		}
		if (!tok.empty()) out.push_back(tok);
	}
	return out;
}

std::string Effect::Arg(const std::string &key) const
{
	for (const auto &kv : args)
		if (kv.first == key) return kv.second;
	return std::string();
}

// ---- parsing ------------------------------------------------------------------------------------

namespace {

struct Parser {
	Decl *d = nullptr;
	std::string error;
	int lineNo = 0;

	bool fail(const std::string &why)
	{
		error = "line " + std::to_string(lineNo) + ": " + why;
		return false;
	}

	bool number(const std::string &tok, double *out)
	{
		char *end = nullptr;
		const double v = std::strtod(tok.c_str(), &end);
		if (end == tok.c_str() || (end && *end != '\0')) return false;
		*out = v;
		return true;
	}

	Position *position(const std::string &post)
	{
		for (Position &p : d->positions)
			if (p.post == post) return &p;
		return nullptr;
	}

	bool line(const std::vector<std::string> &t)
	{
		const std::string &dir = t[0];

		if (dir == "scenario") {
			if (t.size() != 2) return fail("'scenario' takes one argument");
			d->name = t[1];
			return true;
		}
		if (dir == "canon-root") {
			// The rest of the line is free text; the builder quotes it, so it is one token.
			if (t.size() != 2) return fail("'canon-root' takes one quoted argument");
			d->canon_root = t[1];
			return true;
		}
		if (dir == "condition") {
			if (t.size() != 4) return fail("'condition' takes <state> <op> <value>");
			Condition c;
			c.state = t[1];
			c.op = t[2];
			if (!number(t[3], &c.value)) {
				if (t[3] == "true" || t[3] == "false") c.value = (t[3] == "true") ? 1.0 : 0.0;
				else return fail("condition value '" + t[3] + "' is not a number");
			}
			d->conditions.push_back(c);
			return true;
		}
		if (dir == "system") {
			if (t.size() == 3 && t[1] == "off") {
				SystemStress ss; ss.ref = t[2]; ss.off = true;
				d->systems.push_back(ss);
				return true;
			}
			if (t.size() == 3 && t[1] == "read") {
				SystemStress ss; ss.ref = t[2]; ss.read_only = true;
				d->systems.push_back(ss);
				return true;
			}
			if (t.size() == 4 && t[1] == "stress") {
				SystemStress ss; ss.ref = t[2];
				if (!number(t[3], &ss.amount)) return fail("system stress amount '" + t[3] + "' is not a number");
				d->systems.push_back(ss);
				return true;
			}
			return fail("'system' takes 'off <ref>', 'read <ref>' or 'stress <ref> <amount>'");
		}
		if (dir == "staff") {
			if (t.size() != 2) return fail("'staff' takes one post");
			d->staff.push_back(t[1]);
			return true;
		}
		if (dir == "cost-answer" || dir == "cost-ignore") {
			if (t.size() != 2) return fail("'" + dir + "' takes one quoted string");
			(dir == "cost-answer" ? d->answer_text : d->ignore_text) = t[1];
			return true;
		}
		if (dir == "effect") {
			if (t.size() < 3) return fail("'effect' takes <branch> <kind> [key=value ...]");
			Effect e;
			e.branch = t[1];
			e.kind = t[2];
			for (size_t i = 3; i < t.size(); ++i) {
				const size_t eq = t[i].find('=');
				if (eq == std::string::npos) return fail("effect argument '" + t[i] + "' is not key=value");
				e.args.emplace_back(t[i].substr(0, eq), t[i].substr(eq + 1));
			}
			if (e.branch == "answer") d->answer_effects.push_back(e);
			else if (e.branch == "ignore") d->ignore_effects.push_back(e);
			else if (e.branch == "residue") d->residue_effects.push_back(e);
			else if (!StartsWith(e.branch, "option:")) return fail("effect branch '" + e.branch + "' is not answer, ignore, residue or option:<n>");
			else if (d->options.empty()) return fail("an 'option:' effect precedes any 'option' line");
			else d->options.back().cost.push_back(e);
			return true;
		}
		if (dir == "residue") {
			if (t.size() != 2) return fail("'residue' takes one quoted string");
			d->residue.push_back(t[1]);
			return true;
		}
		if (dir == "visibility") {
			if (t.size() != 3) return fail("'visibility' takes <station|system|deck> <value>");
			if (t[1] != "station" && t[1] != "system" && t[1] != "deck")
				return fail("visibility kind '" + t[1] + "' is not station, system or deck");
			d->visibility.push_back(t[1] + ":" + t[2]);
			return true;
		}
		if (dir == "warmth") {
			if (t.size() != 2) return fail("'warmth' takes one beat");
			d->warmth.push_back(t[1]);
			return true;
		}
		if (dir == "position") {
			if (t.size() != 2) return fail("'position' takes one post");
			if (position(t[1])) return fail("position '" + t[1] + "' is declared twice");
			Position p; p.post = t[1];
			d->positions.push_back(p);
			return true;
		}
		if (StartsWith(dir, "position-")) {
			if (t.size() != 3) return fail("'" + dir + "' takes <post> <value>");
			Position *p = position(t[1]);
			if (!p) return fail("'" + dir + "' names a post with no 'position' line: " + t[1]);
			if (dir == "position-info") p->information.push_back(t[2]);
			else if (dir == "position-authority") p->authority.push_back(t[2]);
			else if (dir == "position-responsible") p->responsible_for.push_back(t[2]);
			else if (dir == "position-decision") p->decision = t[2];
			else return fail("unknown declaration '" + dir + "'");
			return true;
		}
		if (dir == "decision") {
			if (t.size() != 2) return fail("'decision' takes one quoted question");
			d->decision_question = t[1];
			return true;
		}
		if (dir == "option") {
			if (t.size() != 2) return fail("'option' takes one quoted label");
			Option o; o.label = t[1];
			d->options.push_back(o);
			return true;
		}
		if (dir == "line") {
			if (t.size() != 4) return fail("'line' takes <post> <delivery> <text>");
			Line ln; ln.to = t[1]; ln.delivery = t[2]; ln.text = t[3];
			d->lines.push_back(ln);
			return true;
		}
		return fail("unknown declaration '" + dir + "'");
	}
};

} // namespace

bool Parse(const std::string &text, Decl &out, std::string &error)
{
	Parser p;
	p.d = &out;
	std::istringstream in(text);
	std::string raw;
	int lineNo = 0;
	while (std::getline(in, raw)) {
		++lineNo;
		if (!raw.empty() && raw.back() == '\r') raw.pop_back();
		std::string tokErr;
		const std::vector<std::string> t = Tokenize(raw, &tokErr, lineNo);
		if (!tokErr.empty()) { error = tokErr; return false; }
		if (t.empty()) continue;
		p.lineNo = lineNo;
		if (!p.line(t)) { error = p.error; return false; }
	}
	if (out.name.empty()) { error = "no 'scenario' line"; return false; }
	error.clear();
	return true;
}

// ---- the field list -----------------------------------------------------------------------------

static const char *const FIELDS[] = {
	"canon_root", "breeding_conditions", "systems", "staff", "cost", "residue", "visibility",
	"warmth", "positions", "decision", "lines",
};

const char *const *Fields(int *count)
{
	if (count) *count = static_cast<int>(sizeof(FIELDS) / sizeof(FIELDS[0]));
	return FIELDS;
}

// ---- resolving a post to a person ---------------------------------------------------------------

int ResolvePost(const Ship &s, const std::string &post)
{
	const std::string low = Lower(post);

	// Exact first: a seat, a system id, or a department, each as its own vocabulary spells it.
	for (int i = 0; i < ship::SEAT_COUNT; ++i)
		if (Lower(ship::SeatName(static_cast<uint8_t>(i))) == low)
			return ship::SeatHolder(s, static_cast<uint8_t>(i));
	const int exactId = ship::SystemIdByName(low.c_str());
	if (exactId >= 0 && low == Lower(ship::Spec(static_cast<ship::SystemId>(exactId)).name)) {
		for (int i = 0; i < static_cast<int>(s.crew.size()); ++i)
			if (s.crew[i].post == exactId && s.crew[i].status == ship::CREW_FIT) return i;
	}
	const int dept = ship::DepartmentByName(low.c_str());
	if (dept >= 0) {
		const int head = ship::DepartmentHead(s, static_cast<ship::Department>(dept));
		if (head >= 0) return head;
	}
	// Then the engine's own leading-part lookups, so "sickbay" and "security" both land.
	if (exactId >= 0) {
		for (int i = 0; i < static_cast<int>(s.crew.size()); ++i)
			if (s.crew[i].post == exactId && s.crew[i].status == ship::CREW_FIT) return i;
	}
	return -1;
}

// ---- applying what a declaration can instantiate now --------------------------------------------

static std::string NameOf(const Ship &s, int crew)
{
	if (crew >= 0 && crew < static_cast<int>(s.crew.size())) return s.crew[crew].name;
	return std::string("(no one holds it)");
}

void Apply(Ship &s, const Decl &d, std::vector<std::string> *out)
{
	std::vector<std::string> sink;
	std::vector<std::string> &log = out ? *out : sink;
	char buf[512];

	// The systems it stresses: the premise of the scenario, applied to the ship's own systems.
	for (const SystemStress &ss : d.systems) {
		const size_t dot = ss.ref.find('.');
		if (dot != std::string::npos) {
			const std::string base = ss.ref.substr(0, dot);
			const std::string arc = ss.ref.substr(dot + 1);
			const int id = ship::SystemIdByName(base.c_str());
			const ship::ShieldSection sec = ship::ShieldSectionByName(arc.c_str());
			if (id >= 0 && sec < ship::SHIELD_SECTION_COUNT) {
				const float before = s.shields[sec];
				ship::ShieldHit(s, sec, static_cast<float>(ss.amount));
				std::snprintf(buf, sizeof(buf), "system %s: %s arc %.0f%% -> %.0f%%",
					ship::Spec(static_cast<ship::SystemId>(id)).name, ship::ShieldSectionName(sec),
					before * 100.0f, s.shields[sec] * 100.0f);
				log.push_back(buf);
				continue;
			}
		}
		const int id = ship::SystemIdByName(ss.ref.c_str());
		if (id < 0) {
			log.push_back("system " + ss.ref + ": NOT A SYSTEM (the loader could not resolve it)");
			continue;
		}
		const ship::SystemId sid = static_cast<ship::SystemId>(id);
		if (ss.read_only) {
			std::snprintf(buf, sizeof(buf), "system %s: read and written, no initial change", ship::Spec(sid).name);
			log.push_back(buf);
		} else if (ss.off) {
			ship::SetEnabled(s, sid, false);
			std::snprintf(buf, sizeof(buf), "system %s: off", ship::Spec(sid).name);
			log.push_back(buf);
		} else {
			const float before = ship::SystemCondition(s.systems[id]);
			ship::DamageSystem(s, sid, static_cast<float>(ss.amount));
			std::snprintf(buf, sizeof(buf), "system %s: condition %.2f -> %.2f (%s)",
				ship::Spec(sid).name, before, ship::SystemCondition(s.systems[id]),
				ship::SystemStateName(ship::FailureStateOf(s.systems[id].health, s.systems[id].enabled)));
			log.push_back(buf);
		}
	}

	// The staff it needs: the crew model is the cost, and these are the two primitives the derivation
	// found reached by nothing but the tests (`docs/the-derivation.md`, "Staff needed").
	if (!d.staff.empty()) {
		const int need = ship::PostsNeeded();
		const ship::WatchCoverage cov = ship::CoverWithCrew(static_cast<int>(s.crew.size()));
		int filled = 0;
		for (const std::string &p : d.staff) {
			const int who = ResolvePost(s, p);
			if (who >= 0) ++filled;
			std::snprintf(buf, sizeof(buf), "staff %s %s", p.c_str(), who >= 0 ? "held" : "UNFILLED");
			log.push_back(buf);
		}
		std::snprintf(buf, sizeof(buf),
			"staff: %zu post(s) required; the ship needs %d posts fully manned, its %d crew cover %d per watch (%s); %d/%zu held",
			d.staff.size(), need, static_cast<int>(s.crew.size()), cov.postsCovered,
			cov.critical ? "critical set holds" : "below the critical set", filled, d.staff.size());
		log.push_back(buf);
	}

	// The warmth it leaves: a beat, run through the primitive the design names (docs/scenario-atlas.md).
	for (const std::string &beat : d.warmth) {
		if (beat == "holodeck") {
			int who = d.positions.empty() ? -1 : ResolvePost(s, d.positions.front().post);
			if (who < 0) who = ship::SeniorFitOfficer(s);
			const bool ran = who >= 0 && ship::RunHolodeck(s, ship::HOLO_RECREATION, who);
			log.push_back("warmth holodeck: " + std::string(ran ? "a program runs for " : "no program (no hand) for ") + NameOf(s, who));
		} else {
			log.push_back("warmth " + beat + ": declared (the beat the scenario leaves behind)");
		}
	}

	// The positions: each is a post, and the simulation resolves it to the person who holds it -- the
	// post-not-name law, demonstrated rather than asserted.
	for (const Position &p : d.positions) {
		const int who = ResolvePost(s, p.post);
		std::snprintf(buf, sizeof(buf), "position %s -> %s (%zu information, %zu authority, %zu responsible, decision: %s)",
			p.post.c_str(), NameOf(s, who).c_str(), p.information.size(), p.authority.size(),
			p.responsible_for.size(), p.decision.empty() ? "-" : p.decision.c_str());
		log.push_back(buf);
	}

	// The lines: each addresses a post and declares its delivery; the person who holds it may be
	// fictitious, and the line is unchanged. The line also reaches the record -- `LogEvent`, an entry
	// point content can call (docs/the-derivation.md's 53), signed by the resolved holder.
	for (const Line &ln : d.lines) {
		const int who = ResolvePost(s, ln.to);
		ship::LogEvent(s, NameOf(s, who), "scenario", ln.delivery + ": " + ln.text);
		log.push_back(ln.delivery + " to " + ln.to + " (" + NameOf(s, who) + "): \"" + ln.text + "\"");
	}

	// The visibility: what the player sees where they are.
	for (const std::string &v : d.visibility) {
		if (StartsWith(v, "deck:")) {
			const int deck = std::atoi(v.c_str() + 5);
			const std::string reading = ship::ScanCompartment(s, deck);
			std::string first = reading.substr(0, reading.find('\n'));
			std::snprintf(buf, sizeof(buf), "visibility deck:%d: %s", deck, first.c_str());
			log.push_back(buf);
		} else {
			log.push_back("visibility " + v + ": a surface at the post");
		}
	}

	// Carried, not applied: the evaluator reads these, and building it here would half-build it.
	std::snprintf(buf, sizeof(buf), "carried: %zu breeding condition(s), cost-to-answer \"%s\", cost-to-ignore \"%s\", %zu residue, %zu decision option(s)",
		d.conditions.size(), d.answer_text.c_str(), d.ignore_text.c_str(),
		d.residue.size() + d.residue_effects.size(), d.options.size());
	log.push_back(buf);
}

// ==================================================================================================
// The evaluator -- what reads the breeding conditions and decides the scenario fires.
// ==================================================================================================

namespace {

// The 18 states the contract's `breeding_conditions.state` may name (`docs/scenario-manifest.md`,
// *The readable state*). The first 15 are facts the ship carries as a single number; the last three
// are **scoped** -- each names a system or a deck, and the `{state, op, value}` tuple carries no
// subject, so they are answered by a decided worst-case default (see `ReadState`).
const char *const READABLE[] = {
	"deuterium", "antimatter", "batteries", "torpedoes", "spare_parts", "materials", "rations",
	"probes", "patients", "refugees", "resentment", "borg_awareness", "power_shortfall", "coreless",
	"intruders",
};
const char *const SCOPED[] = {"system_condition", "deck_life_support", "minutes_of_air"};
// The design's own breeding facts that no manifest key and no state reaches at all
// (`docs/scenario-atlas.md`: "a contact, a mass reading, no transponder"; "a wounded crew list plus
// a battle"; "one source available"; "a hull breach"; structural integrity; nine days at warp).
const char *const DESIGN_IMPLIED[] = {"contact", "in_battle", "beacon", "breach", "structural_integrity", "clock"};

const double HUGE = 1.0e18; // a finite stand-in for +infinity, beyond any count the ship holds

int ConditionByName(const std::string &low)
{
	if (low == "exhausted") return ship::COND_EXHAUSTED;
	if (low == "hungry") return ship::COND_HUNGRY;
	if (low == "sick" || low == "infected") return ship::COND_INFECTED;
	if (low == "grieving") return ship::COND_GRIEVING;
	if (low == "frightened" || low == "afraid") return ship::COND_AFRAID;
	if (low == "wounded" || low == "concussed") return ship::COND_CONCUSSED;
	if (low == "steadied" || low == "rested") return ship::COND_RESTED;
	return ship::COND_NONE;
}

int MagnitudeByName(const std::string &low)
{
	if (low == "sharp") return ship::CMAG_SHARP;
	if (low == "clear") return ship::CMAG_CLEAR;
	return ship::CMAG_SLIGHT;
}

int HolodeckUseByName(const std::string &low)
{
	if (low == "training") return ship::HOLO_TRAINING;
	if (low == "therapy") return ship::HOLO_THERAPY;
	if (low == "forensic") return ship::HOLO_FORENSIC;
	return ship::HOLO_RECREATION;
}

int AirSourceByName(const std::string &low)
{
	if (low == "grid" || low == "main grid") return ship::AIR_GRID;
	if (low == "impulse" || low == "impulse reactors") return ship::SRC_IMPULSE_REACTORS;
	if (low == "auxiliary" || low == "auxiliary fusion") return ship::SRC_AUXILIARY;
	if (low == "batteries" || low == "emergency batteries") return ship::SRC_BATTERIES;
	if (low == "holodeck" || low == "holodeck reactor") return ship::AIR_HOLODECK;
	return -1;
}

// Does the condition (`op`, `value`) hold for every / for no value in the state's own range? These
// are the degenerate cases: a tautology recruits nothing, a contradiction can never fire.
bool AllTrue(const std::string &op, double value, double lo, double hi)
{
	if (op == ">") return value < lo;
	if (op == ">=") return value <= lo;
	if (op == "<") return value > hi;
	if (op == "<=") return value >= hi;
	if (op == "==") return lo == hi && value == lo;
	if (op == "!=") return value < lo || value > hi;
	return false;
}

bool AllFalse(const std::string &op, double value, double lo, double hi)
{
	if (op == ">") return value >= hi;
	if (op == ">=") return value > hi;
	if (op == "<") return value <= lo;
	if (op == "<=") return value < lo;
	if (op == "==") return value < lo || value > hi;
	if (op == "!=") return lo == hi && value == lo;
	return false;
}

bool Cmp(const std::string &op, double v, double value)
{
	if (op == "<") return v < value;
	if (op == "<=") return v <= value;
	if (op == ">") return v > value;
	if (op == ">=") return v >= value;
	if (op == "==") return v == value;
	if (op == "!=") return v != value;
	return false;
}

// The signed slack: how far past the threshold the value is met (>= 0), or short (< 0). The nearest
// miss is the unmet condition with the smallest |shortfall| relative to its threshold; the condition
// that fired is the met one with the smallest such slack, the premise the scenario leans on hardest.
double Slack(const std::string &op, double now, double value)
{
	if (op == "<" || op == "<=") return value - now;
	if (op == ">" || op == ">=") return now - value;
	return -std::fabs(now - value); // == is met only at zero; != never counts as a near miss
}

double Score(const ConditionReading &r)
{
	const double denom = std::fabs(r.cond.value) > 1.0 ? std::fabs(r.cond.value) : 1.0;
	if (r.cond.op == "!=") return 1.0e9; // an inequality is never "nearly" missed
	return std::fabs(r.margin) / denom;
}

} // namespace

const char *ConditionOutcomeName(uint8_t o)
{
	static const char *const NAMES[] = {"met", "unmet", "unreadable", "always", "never"};
	return o < 5 ? NAMES[o] : "?";
}

const char *const *ReadableStates(int *count)
{
	if (count) *count = static_cast<int>(sizeof(READABLE) / sizeof(READABLE[0]));
	return READABLE;
}

const char *const *UnreadableStates(int *count)
{
	if (count) *count = static_cast<int>(sizeof(SCOPED) / sizeof(SCOPED[0]));
	return SCOPED;
}

const char *const *DesignImpliedStates(int *count)
{
	if (count) *count = static_cast<int>(sizeof(DESIGN_IMPLIED) / sizeof(DESIGN_IMPLIED[0]));
	return DESIGN_IMPLIED;
}

bool ReadState(const Ship &s, const std::string &state, double *out, std::string *note)
{
	const std::string k = Lower(state);
	auto set = [&](double v, const char *what) {
		if (out) *out = v;
		if (note) *note = what;
		return true;
	};
	if (k == "deuterium") return set(s.stores.deuterium, "stores");
	if (k == "antimatter") return set(s.stores.antimatter, "stores");
	if (k == "batteries") return set(s.stores.batteries, "stores");
	if (k == "torpedoes") return set(s.stores.torpedoes, "stores");
	if (k == "spare_parts") return set(s.stores.spareParts, "stores");
	if (k == "materials") return set(s.stores.materials, "stores");
	if (k == "rations") return set(s.stores.rations, "stores");
	if (k == "probes") return set(s.stores.probes, "stores");
	if (k == "patients") return set(static_cast<double>(ship::Patients(s).size()), "sickbay's ward");
	if (k == "refugees") return set(ship::Refugees(s), "aboard, and not crew");
	if (k == "resentment") return set(ship::Resentment(s), "the Maquis split");
	if (k == "borg_awareness") return set(ship::BorgAwareness(s), "what the Collective knows");
	if (k == "power_shortfall") return set(ship::PowerShortfall(s), "committed over available");
	if (k == "coreless") return set(ship::Coreless(s) ? 1.0 : 0.0, "the warp core gone");
	if (k == "intruders") return set(ship::Intruders(s), "aboard");
	// The scoped three: read as the ship's worst, the default the shape forces (no subject).
	if (k == "system_condition") {
		// The damage axis, not SystemCondition: `SystemCondition` is health capped by *power*, and
		// the standby weapons (shields, phasers, the launchers) draw no power until used, so a
		// ship-wide worst condition is 0 on a completely healthy ship. The worst **health** is the
		// reading that means "a system is ailing".
		double worst = 1.0;
		for (int i = 0; i < ship::SYS_COUNT; ++i) {
			const double h = s.systems[i].health;
			if (h < worst) worst = h;
		}
		if (worst < 0.0) worst = 0.0;
		if (worst > 1.0) worst = 1.0;
		return set(worst, "the worst-damaged system (no system named: the shape carries no subject)");
	}
	if (k == "deck_life_support") {
		double worst = 1.0;
		for (int d = 1; d <= ship::DECKS; ++d) {
			const double v = ship::DeckLifeSupport(s, d);
			if (v >= 0.0 && v < worst) worst = v;
		}
		return set(worst, "the least-supported deck (no deck named: the shape carries no subject)");
	}
	if (k == "minutes_of_air") {
		double best = HUGE;
		for (int d = 1; d <= ship::DECKS; ++d) {
			const double m = ship::MinutesOfAir(s, d);
			if (m >= 0.0 && m < best) best = m;
		}
		return set(best, "the deck soonest out of air (no deck named: the shape carries no subject)");
	}
	return false;
}

bool StateRange(const std::string &state, double *lo, double *hi)
{
	const std::string k = Lower(state);
	// Fractions and reads the ship keeps in 0..1.
	if (k == "deuterium" || k == "antimatter" || k == "batteries" || k == "resentment" ||
	    k == "borg_awareness" || k == "system_condition" || k == "deck_life_support" || k == "coreless") {
		*lo = 0.0; *hi = 1.0; return true;
	}
	// Counts and durations: zero or more, without an upper bound.
	if (k == "torpedoes" || k == "spare_parts" || k == "materials" || k == "rations" || k == "probes" ||
	    k == "patients" || k == "refugees" || k == "intruders" || k == "power_shortfall" || k == "minutes_of_air") {
		*lo = 0.0; *hi = HUGE; return true;
	}
	return false;
}

Evaluation Evaluate(const Ship &s, const Decl &d)
{
	Evaluation e;
	for (const Condition &c : d.conditions) {
		ConditionReading r;
		r.cond = c;
		double now = 0.0;
		std::string note;
		if (ReadState(s, c.state, &now, &note)) {
			r.has_now = true;
			r.now = now;
			r.note = note;
			double lo = 0.0, hi = 0.0;
			const bool range = StateRange(c.state, &lo, &hi);
			r.margin = Slack(c.op, now, c.value);
			if (range && AllTrue(c.op, c.value, lo, hi)) {
				r.outcome = COND_ALWAYS;
				e.degenerate = true;
			} else if (range && AllFalse(c.op, c.value, lo, hi)) {
				r.outcome = COND_NEVER;
				e.degenerate = true;
			} else {
				r.outcome = Cmp(c.op, now, c.value) ? COND_MET : COND_UNMET;
			}
		} else {
			r.outcome = COND_UNREADABLE;
			r.note = "no reader carries this state";
			bool seen = false;
			for (const std::string &u : e.unreadable) if (u == c.state) seen = true;
			if (!seen) e.unreadable.push_back(c.state);
		}
		if (r.outcome == COND_ALWAYS || r.outcome == COND_NEVER) {
			char db[128];
			std::snprintf(db, sizeof(db), "%s %s %g", c.state.c_str(), c.op.c_str(), c.value);
			e.degenerate_conditions.push_back(db);
		}
		e.readings.push_back(r);
	}

	if (d.conditions.empty()) {
		e.reason = "does not fire: no breeding conditions declared, so nothing recruits it";
		return e;
	}

	// Does every condition hold? UNREADABLE and NEVER block; ALWAYS is satisfied but named.
	bool all = true;
	for (const ConditionReading &r : e.readings)
		if (r.outcome != COND_MET && r.outcome != COND_ALWAYS) all = false;

	// The nearest miss: the unmet condition closest to being met, by relative shortfall.
	int miss = -1;
	double missScore = 0.0;
	for (size_t i = 0; i < e.readings.size(); ++i) {
		if (e.readings[i].outcome != COND_UNMET) continue;
		const double sc = Score(e.readings[i]);
		if (miss < 0 || sc < missScore) { miss = static_cast<int>(i); missScore = sc; }
	}
	e.nearest_miss = miss;

	if (!all) {
		e.fires = false;
		char buf[320];
		if (!e.unreadable.empty()) {
			std::string names;
			for (const std::string &u : e.unreadable) { if (!names.empty()) names += ", "; names += u; }
			e.reason = "does not fire: " + names + " is not a state the ship carries (no reader); an unreadable condition cannot recruit anything";
			return e;
		}
		bool hasNever = false;
		std::string never;
		for (const ConditionReading &r : e.readings)
			if (r.outcome == COND_NEVER) { hasNever = true; if (never.empty()) never = r.cond.state; }
		if (hasNever) {
			e.reason = "does not fire: " + never + " can never be true -- no value the state can take satisfies it";
			return e;
		}
		if (miss >= 0) {
			const ConditionReading &r = e.readings[miss];
			std::snprintf(buf, sizeof(buf), "does not fire: nearest miss %s %s %g -- now %g (short by %g)",
				r.cond.state.c_str(), r.cond.op.c_str(), r.cond.value, r.now, -r.margin);
			e.reason = buf;
		} else {
			e.reason = "does not fire";
		}
		return e;
	}

	// It fires. The decisive condition is the met one the premise leans on hardest: the smallest
	// slack. A scenario with only tautologies is flagged: the state is not what recruited it.
	e.fires = true;
	int decisive = -1;
	double best = 0.0;
	bool anyReal = false;
	for (size_t i = 0; i < e.readings.size(); ++i) {
		if (e.readings[i].outcome != COND_MET) continue;
		anyReal = true;
		const double sc = Score(e.readings[i]);
		if (decisive < 0 || sc < best) { decisive = static_cast<int>(i); best = sc; }
	}
	if (decisive < 0 && !e.readings.empty()) decisive = 0; // every condition a tautology
	e.fired_by = decisive;
	char buf[320];
	if (decisive >= 0) {
		const ConditionReading &r = e.readings[decisive];
		std::snprintf(buf, sizeof(buf), "fires: %s %s %g -- now %g%s", r.cond.state.c_str(), r.cond.op.c_str(),
			r.cond.value, r.now, anyReal ? "" : " (every condition is a tautology: recruited by nothing)");
		e.reason = buf;
	} else {
		e.reason = "fires";
	}
	if (!e.degenerate_conditions.empty()) {
		e.reason += " [degenerate: ";
		for (size_t i = 0; i < e.degenerate_conditions.size(); ++i) {
			if (i) e.reason += "; ";
			e.reason += e.degenerate_conditions[i];
		}
		e.reason += "]";
	}
	return e;
}

bool Recruit(Ship &s, const Decl &d, std::vector<std::string> *out)
{
	std::vector<std::string> sink;
	std::vector<std::string> &log = out ? *out : sink;
	const Evaluation e = Evaluate(s, d);
	if (!e.fires) {
		log.push_back("scenario \"" + d.name + "\": " + e.reason);
		return false;
	}
	if (ship::ScenarioFired(s, d.name)) {
		log.push_back("scenario \"" + d.name + "\": " + e.reason + "; already in the ledger, not fired twice");
		return false;
	}
	log.push_back("scenario \"" + d.name + "\": " + e.reason);
	Apply(s, d, &log);
	ship::RecordScenario(s, d.name, ship::SCENARIO_UNDECIDED);
	log.push_back("scenario \"" + d.name + "\": instantiated, and recorded in flight");
	return true;
}

bool ApplyEffect(Ship &s, const Effect &e, std::vector<std::string> *out)
{
	std::vector<std::string> sink;
	std::vector<std::string> &log = out ? *out : sink;
	const std::string k = Lower(e.kind);
	char buf[256];

	if (k == "damage_system") {
		const int id = ship::SystemIdByName(e.Arg("system").c_str());
		if (id < 0) { log.push_back("effect damage_system: not a system: " + e.Arg("system")); return false; }
		const float amount = static_cast<float>(std::atof(e.Arg("amount").c_str()));
		ship::DamageSystem(s, static_cast<ship::SystemId>(id), amount);
		std::snprintf(buf, sizeof(buf), "effect damage_system %s %.2f -> condition %.2f",
			ship::Spec(static_cast<ship::SystemId>(id)).name, amount,
			ship::SystemCondition(s.systems[id]));
		log.push_back(buf);
		return true;
	}
	if (k == "shield_hit") {
		const ship::ShieldSection sec = ship::ShieldSectionByName(e.Arg("section").c_str());
		if (sec >= ship::SHIELD_SECTION_COUNT) { log.push_back("effect shield_hit: not an arc: " + e.Arg("section")); return false; }
		const float amount = static_cast<float>(std::atof(e.Arg("amount").c_str()));
		const float before = s.shields[sec];
		ship::ShieldHit(s, sec, amount);
		std::snprintf(buf, sizeof(buf), "effect shield_hit %s %.2f: %.0f%% -> %.0f%%",
			ship::ShieldSectionName(sec), amount, before * 100.0f, s.shields[sec] * 100.0f);
		log.push_back(buf);
		return true;
	}
	if (k == "ignite" || k == "breach") {
		const int deck = std::atoi(e.Arg("deck").c_str());
		const float amount = static_cast<float>(std::atof(e.Arg("amount").c_str()));
		if (deck < 1 || deck > ship::DECKS) { log.push_back("effect " + k + ": not a deck: " + e.Arg("deck")); return false; }
		if (k == "ignite") ship::IgniteDeck(s, deck, amount);
		else ship::BreachDeck(s, deck, amount);
		std::snprintf(buf, sizeof(buf), "effect %s deck %d %.2f", k.c_str(), deck, amount);
		log.push_back(buf);
		return true;
	}
	if (k == "kill" || k == "sicken" || k == "promote" || k == "run_holodeck") {
		const int who = ResolvePost(s, e.Arg("post"));
		if (who < 0) { log.push_back("effect " + k + ": post unresolved or vacant: " + e.Arg("post")); return false; }
		if (k == "kill") {
			const std::string cause = e.Arg("cause").empty() ? "a scenario's cost" : e.Arg("cause");
			if (!ship::KillCrew(s, who, cause)) return false;
			std::snprintf(buf, sizeof(buf), "effect kill %s (%s)", e.Arg("post").c_str(), s.crew[who].name.c_str());
			log.push_back(buf);
			return true;
		}
		if (k == "promote") {
			if (!ship::Promote(s, who)) return false;
			std::snprintf(buf, sizeof(buf), "effect promote %s -> %s", e.Arg("post").c_str(), s.crew[who].name.c_str());
			log.push_back(buf);
			return true;
		}
		if (k == "sicken") {
			const int cid = ConditionByName(Lower(e.Arg("condition")));
			if (cid == ship::COND_NONE) { log.push_back("effect sicken: not a condition: " + e.Arg("condition")); return false; }
			const uint8_t mag = static_cast<uint8_t>(MagnitudeByName(Lower(e.Arg("magnitude"))));
			const int8_t val = cid == ship::COND_RESTED ? static_cast<int8_t>(ship::CVAL_BUFF) : static_cast<int8_t>(ship::CVAL_DEBUFF);
			const uint8_t visible = ship::CVIS_PLAYER | ship::CVIS_CREW | ship::CVIS_LOG;
			if (!ship::Sicken(s, who, static_cast<uint8_t>(cid), "a scenario", val, mag, ship::CLEAR_SICKBAY, visible)) return false;
			std::snprintf(buf, sizeof(buf), "effect sicken %s (%s): %s", e.Arg("post").c_str(), s.crew[who].name.c_str(), e.Arg("condition").c_str());
			log.push_back(buf);
			return true;
		}
		// run_holodeck
		const int use = HolodeckUseByName(Lower(e.Arg("use")));
		if (!ship::RunHolodeck(s, static_cast<ship::HolodeckUse>(use), who)) return false;
		std::snprintf(buf, sizeof(buf), "effect run_holodeck %s for %s (%s)", e.Arg("use").c_str(), e.Arg("post").c_str(), s.crew[who].name.c_str());
		log.push_back(buf);
		return true;
	}
	if (k == "take_survivors") {
		const int n = std::atoi(e.Arg("count").c_str());
		if (!ship::TakeSurvivors(s, n)) return false;
		std::snprintf(buf, sizeof(buf), "effect take_survivors %d: refugees %d", n, ship::Refugees(s));
		log.push_back(buf);
		return true;
	}
	if (k == "fabricate_parts") {
		const int n = std::atoi(e.Arg("count").c_str());
		if (!ship::FabricateParts(s, n)) return false;
		std::snprintf(buf, sizeof(buf), "effect fabricate_parts %d: spare parts %.0f", n, s.stores.spareParts);
		log.push_back(buf);
		return true;
	}
	if (k == "fabricate_rations") {
		const int n = std::atoi(e.Arg("days").c_str());
		if (!ship::FabricateRations(s, n)) return false;
		std::snprintf(buf, sizeof(buf), "effect fabricate_rations %d: rations %.0f", n, s.stores.rations);
		log.push_back(buf);
		return true;
	}
	if (k == "air_source") {
		const int deck = std::atoi(e.Arg("deck").c_str());
		const int src = AirSourceByName(Lower(e.Arg("source")));
		if (src < 0) { log.push_back("effect air_source: not a source: " + e.Arg("source")); return false; }
		if (!ship::SetAirSource(s, deck, src)) return false;
		std::snprintf(buf, sizeof(buf), "effect air_source deck %d on %s", deck, ship::AirSourceName(static_cast<uint8_t>(src)));
		log.push_back(buf);
		return true;
	}
	if (k == "write_off") {
		const std::string target = e.Arg("target");
		const int sid = ship::SystemIdByName(Lower(target).c_str());
		if (sid >= 0) {
			const int kind = e.Arg("kind").empty() ? static_cast<int>(ship::LOSS_WRITTEN_OFF) : std::atoi(e.Arg("kind").c_str());
			if (!ship::WriteOff(s, true, sid, static_cast<uint8_t>(kind))) return false;
			std::snprintf(buf, sizeof(buf), "effect write_off system %s", ship::Spec(static_cast<ship::SystemId>(sid)).name);
			log.push_back(buf);
			return true;
		}
		const int deck = std::atoi(target.c_str());
		if (deck >= 1 && deck <= ship::DECKS) {
			const int kind = e.Arg("kind").empty() ? static_cast<int>(ship::LOSS_WRITTEN_OFF) : std::atoi(e.Arg("kind").c_str());
			if (!ship::WriteOff(s, false, deck, static_cast<uint8_t>(kind))) return false;
			std::snprintf(buf, sizeof(buf), "effect write_off deck %d", deck);
			log.push_back(buf);
			return true;
		}
		log.push_back("effect write_off: not a system or deck: " + target);
		return false;
	}
	log.push_back("effect " + e.kind + ": unknown kind");
	return false;
}

bool Resolve(Ship &s, const Decl &d, int branch, std::vector<std::string> *out)
{
	std::vector<std::string> sink;
	std::vector<std::string> &log = out ? *out : sink;
	if (!ship::ScenarioFired(s, d.name)) {
		log.push_back("resolve \"" + d.name + "\": it has not fired, so there is no decision to take");
		return false;
	}
	std::vector<Effect> effects;
	std::string label;
	if (branch == ship::SCENARIO_ANSWER) { effects = d.answer_effects; label = d.answer_text; }
	else if (branch == ship::SCENARIO_IGNORE) { effects = d.ignore_effects; label = d.ignore_text; }
	else {
		const int idx = branch - ship::SCENARIO_OPTION_BASE;
		if (idx < 0 || idx >= static_cast<int>(d.options.size())) {
			log.push_back("resolve \"" + d.name + "\": no such branch");
			return false;
		}
		effects = d.options[idx].cost;
		label = d.options[idx].label;
	}
	if (branch == ship::SCENARIO_ANSWER && d.answer_text.empty() && effects.empty()) {
		log.push_back("resolve \"" + d.name + "\": the declaration offers no answer");
		return false;
	}
	log.push_back("resolve \"" + d.name + "\": " + label);
	for (const Effect &e : effects) ApplyEffect(s, e, &log);
	// The residue: what is permanently different afterwards, the last thing the scenario leaves.
	for (const Effect &e : d.residue_effects) ApplyEffect(s, e, &log);
	for (const std::string &r : d.residue) log.push_back("residue: " + r);
	ship::RecordScenario(s, d.name, branch);
	log.push_back("scenario \"" + d.name + "\": resolved and recorded");
	return true;
}

} // namespace scenario
