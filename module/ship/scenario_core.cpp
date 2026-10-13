// The scenario declaration loader -- see scenario_core.h for what it is and what it deliberately is
// not. Pure C++ over ship_core: no game header, so tests/ship can build and drive it.

#include "scenario_core.h"

#include <cctype>
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

} // namespace scenario
