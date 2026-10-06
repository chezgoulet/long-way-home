// g_crew.cpp -- the reactive-crew direction layer, the half that touches entities.
//
// A direction layer above the existing behaviour states, not a replacement for them. It gives
// crew a post (a waypoint_navgoal already in the map), hands the existing navigator a goal, and
// then gets out of the way: it sets no behaviour state, plays no animation and owns no dialogue.
// The decisions are crew_core's; this file reads signals from the entity and applies the result.
//
// The rule it must never break: a script owns its NPC. See IsScripted() and docs/g3-reactive-crew.md.

#include "b_local.h"
#include "g_functions.h"
#include "g_nav.h"
#include "Q3_Interface.h"
#include "sequencer.h"

#include "g_navigator.h"

#include "crew_core.h"
#include "g_crew.h"
#include "g_ship.h"
#include "ship_core.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>

// The game's own effect spawn functions, used to make damage and fire visible in the world (S6). A
// spawn function, not a new class: the map's fx_spark and this are the same thing.
extern void SP_fx_spark( gentity_t *ent );
extern void SP_fx_smoke( gentity_t *ent );
extern void SP_fx_electricfire( gentity_t *ent );

extern void NPC_Respond( gentity_t *self, int userNum );
extern void NPC_SetLookTarget( gentity_t *self, int entNum, int clearTime );
extern void SP_NPC_starfleet( gentity_t *self );
extern void SP_NPC_klingon( gentity_t *self );
extern void SP_NPC_borg( gentity_t *self );
extern SavedGameJustLoaded_e g_eSavedGameJustLoaded;
extern CNavigator navigator;

namespace {

const int HARNESS_MS = 100;
const int LOOK_HOLD_MS = 300;
const int ROSTER_SCAN_MS = 1000;
const int DIRECTOR_MS = 1000;
const int BUMP_DEBOUNCE_MS = 8000;
const int ADDRESS_STAGGER_MS = 3000;
const int SCRIPT_YIELD_MS = 500;        // the layer must have yielded to the test script by now
const int SCRIPT_RESUME_MS = 20000;     // ... and be back on duty by now
const char *const SCRIPT_TEST = "lwh/interrupt";
const float PROGRESS_UNITS = 16.0f;
const float LEAVE_SLACK = 24.0f;        // hysteresis: further than this beyond the radius = off post
const float POST_HEIGHT_SLACK = 48.0f;
const float NOTICE_RANGE = 192.0f;      // the player this close is looked at
const float FACE_RANGE = 112.0f;        // ... and this close is turned towards
const float OUT_OF_WORLD_DROP = 2048.0f;
const unsigned long SAVE_CHUNK = 0x43524557; // 'CREW', as the game's own multi-character chunk ids are encoded
const size_t CHUNK_OVERHEAD = 16;       // id, length, checksum, trailer: 4 bytes each

cvar_t *g_crew;         // 1 = the layer runs on this map
cvar_t *g_crewRun;      // seconds of sampled run, then report (0 = no automatic report)
cvar_t *g_crewQuit;     // 1 = save and quit when the run completes (the measurement harness)
cvar_t *g_crewDebug;    // 1 = print every decision change
cvar_t *g_crewFromShip; // 1 = the crew on this deck are whoever the ship simulation says is on it (S5)
cvar_t *g_crewDeck;     // which deck this map is, for a map of one deck; 0 = work it out from g_shipDeckPitch
cvar_t *g_env;          // 1 = the ship's environment is shown in the world (gravity, a breach, a field)
cvar_t *g_player;       // 1 = the player's body in the world (Stage B): the causes reach the person

struct CrewState {
	bool active = false;
	bool baseline = false;      // layer off, harness on: the same run measured without the crew
	std::chrono::steady_clock::time_point frameBegan;
	int64_t gameNs = 0;         // whole game frames, for the with/without comparison
	int64_t maxGameNs = 0;
	int gameFrames = 0;
	crew::Config cfg;
	bool ready = false;         // posts resolved and declared crew spawned (done on the first frame)
	std::vector<crew::Post> posts;
	std::vector<uint8_t> pendingSave;
	std::vector<crew::Member> members;
	std::vector<bool> lostToWorld;
	crew::Stats st;
	uint32_t postHash = 0;
	float floorZ = 0;
	int nextThinkMs = 0;
	int nextScanMs = 0;
	int nextDirectorMs = 0;
	int nextSampleMs = 0;
	int samplingFromMs = -1;    // -1 until every member has arrived, failed, or run out of time
	int nextAddress = -1;       // self-test: index of the next member to address
	int nextAddressMs = 0;
	bool addressTestDone = false;
	int scriptPhase = 0;        // self-test: 0 not started, 1 awaiting the yield, 2 awaiting the return, 3 done
	int scriptMember = -1;
	int scriptStartMs = 0;
	bool runReported = false;
	size_t lastSaveBytes = 0;
};

CrewState cs;

// ---- small helpers --------------------------------------------------------------------------

bool Eligible( const gentity_t *e )
{
	return e->inuse && e->NPC && e->client && e->health > 0 && e->s.number != 0
		&& e->client->playerTeam == TEAM_STARFLEET;
}

const char *NameOf( const gentity_t *e )
{
	if ( e->script_targetname && e->script_targetname[0] ) return e->script_targetname;
	if ( e->targetname && e->targetname[0] ) return e->targetname;
	if ( e->NPC_type && e->NPC_type[0] ) return e->NPC_type;
	return "crew";
}

int MemberIndex( const gentity_t *e )
{
	if ( !cs.active || !e ) return -1;
	for ( size_t i = 0; i < cs.members.size(); ++i )
		if ( cs.members[i].ent == e->s.number ) return static_cast<int>( i );
	return -1;
}

std::string MapKey( void )
{
	std::string s = level.mapname;
	for ( char &c : s )
		if ( c == '/' || c == '\\' ) c = '_';
	return s;
}

std::string Fmt( const char *fmt, ... ) __attribute__(( format( printf, 1, 2 ) ));
std::string Fmt( const char *fmt, ... )
{
	char buf[1024];
	va_list ap;
	va_start( ap, fmt );
	vsnprintf( buf, sizeof( buf ), fmt, ap );
	va_end( ap );
	return buf;
}

float Dist2D( const float *a, const float *b )
{
	const float dx = a[0] - b[0], dy = a[1] - b[1];
	return std::sqrt( dx * dx + dy * dy );
}

// ---- reading signals ------------------------------------------------------------------------

// Level 1. True whenever anything other than this layer is directing the NPC:
//   * its ICARUS sequencer has commands outstanding, or a task is pending
//   * it is playing back a recorded path (ROFF)
//   * a temporary behaviour is in force, or the behaviour state is one that does not simply
//     follow a goal -- a state the layer did not set and therefore must not disturb
//   * it carries a goal the layer did not give it (the layer's own goals are the NPC's tempGoal,
//     parked on a post's position -- the engine's mechanism for locational goals)
// Each is one test, and the layer issues nothing while any of them holds.
bool GoalIsOurs( const gentity_t *e, const crew::Member &m )
{
	return ( m.flags & crew::MF_OWNS_GOAL ) && m.goalPost != crew::NO_POST
		&& e->NPC->goalEntity && e->NPC->goalEntity == e->NPC->tempGoal
		&& DistanceSquared( e->NPC->tempGoal->currentOrigin, cs.posts[m.goalPost].origin ) < 1.0f;
}

// The script half of IsScripted(), without its side effects: usable from the event hooks, which
// run between the layer's turns and cannot rely on the level it last computed.
bool ScriptOwns( gentity_t *e )
{
	if ( e->sequencer && e->sequencer->IsRunning() ) return true;
	for ( int tid = 0; tid < NUM_TIDS; ++tid )
		if ( Q3_TaskIDPending( e, static_cast<taskID_t>( tid ) ) ) return true;
	if ( e->next_roff_time && e->next_roff_time >= level.time ) return true;
	return e->NPC->tempBehavior != BS_DEFAULT;
}

bool IsScripted( gentity_t *e, crew::Member &m )
{
	if ( ScriptOwns( e ) )
	{//a script began while the NPC was walking for the layer: the goal is released this same turn
		if ( GoalIsOurs( e, m ) ) ++cs.st.handovers;
		return true;
	}

	const bState_t bs = e->NPC->behaviorState ? e->NPC->behaviorState : e->NPC->defaultBehavior;
	if ( bs != BS_DEFAULT && bs != BS_IDLE && bs != BS_WALK && bs != BS_ROAM ) return true;

	if ( e->NPC->goalEntity )
	{
		if ( !( m.flags & crew::MF_OWNS_GOAL ) ) return true;
		if ( !GoalIsOurs( e, m ) )
		{//something replaced the goal we set: it is theirs now
			m.flags &= ~crew::MF_OWNS_GOAL;
			m.goalPost = crew::NO_POST;
			++cs.st.preemptions;
			return true;
		}
	}
	return false;
}

// Arrival is the engine's own test (NAV_HitNavGoal, the one UpdateGoal uses to decide the NPC has
// reached its goal), so the layer and the navigator never disagree about whether the walk is over.
// Once holding, a little slack keeps a nudge from the player from counting as leaving the post.
bool AtPost( gentity_t *e, crew::Post &p, bool wasHolding )
{
	if ( std::fabs( e->currentOrigin[2] - p.origin[2] ) > POST_HEIGHT_SLACK ) return false;
	if ( NAV_HitNavGoal( e->currentOrigin, e->mins, e->maxs, p.origin, p.radius ) ) return true;
	return wasHolding && Dist2D( e->currentOrigin, p.origin ) <= p.radius + e->maxs[0] + LEAVE_SLACK;
}

// ---- applying decisions ---------------------------------------------------------------------

// Every place the layer writes to an NPC passes through here first. The decision above it has
// already excluded a scripted NPC, so this should never refuse; it is the rule asserted at the
// point of action rather than trusted from the point of decision, and a refusal is counted.
bool WriteRefused( gentity_t *e )
{
	if ( !ScriptOwns( e ) ) return false;
	++cs.st.violations;
	return true;
}

void ReleaseGoal( gentity_t *e, crew::Member &m )
{
	if ( e->NPC && GoalIsOurs( e, m ) )
	{
		e->NPC->goalEntity = NULL;
	}
	m.flags &= ~crew::MF_OWNS_GOAL;
	m.goalPost = crew::NO_POST;
}

void IssueGoal( gentity_t *e, crew::Member &m, int post )
{
	if ( m.goalPost == post && GoalIsOurs( e, m ) )
	{
		return;
	}
	if ( !e->NPC->tempGoal )
	{
		return;
	}
	NPC_SetMoveGoal( e, cs.posts[post].origin, cs.posts[post].radius, qtrue );
	e->NPC->tempGoal->lastWaypoint = WAYPOINT_NONE;
	e->NPC->aiFlags &= ~NPCAI_TOUCHED_GOAL;
	m.flags |= crew::MF_OWNS_GOAL;
	m.goalPost = static_cast<int16_t>( post );
	m.progressMs = level.time;
	VectorCopy( e->currentOrigin, m.progressPos );
}

void TrackProgress( gentity_t *e, crew::Member &m )
{
	if ( Distance( e->currentOrigin, m.progressPos ) >= PROGRESS_UNITS )
	{
		m.progressMs = level.time;
		VectorCopy( e->currentOrigin, m.progressPos );
		return;
	}
	if ( level.time - m.progressMs < cs.cfg.stuckMs ) return;

	++cs.st.stuckEvents;
	++m.stuckStrikes;
	m.progressMs = level.time;
	if ( m.stuckStrikes >= cs.cfg.stuckStrikes )
	{//reports failure rather than pushing against a wall for the rest of the run
		m.flags |= crew::MF_FAILED;
		ReleaseGoal( e, m );
		gi.Printf( S_COLOR_YELLOW"CREW: %s failed to reach post '%s'\n", NameOf( e ), cs.posts[m.EffectivePost()].name.c_str() );
		return;
	}
	//let the navigator pick its route again from where the NPC actually is
	e->lastWaypoint = WAYPOINT_NONE;
	e->NPC->aiFlags &= ~NPCAI_TOUCHED_GOAL;
}

void HoldPost( gentity_t *e, crew::Member &m, const crew::Post &p )
{
	if ( !( m.flags & crew::MF_ARRIVED ) )
	{
		m.flags |= crew::MF_ARRIVED;
		m.firstArrivalMs = level.time;
		if ( g_crewDebug->integer )
			gi.Printf( "CREW: %s at post '%s' after %d ms\n", NameOf( e ), p.name.c_str(), level.time );
	}
	m.stuckStrikes = 0;
	ReleaseGoal( e, m );

	//stand the way the post faces; notice the player, and turn to them when they come close
	float yaw = p.yaw;
	gentity_t *player = &g_entities[0];
	if ( player->inuse && player->client && player->health > 0 )
	{
		const float d = Distance( player->currentOrigin, e->currentOrigin );
		if ( d <= NOTICE_RANGE && gi.inPVS( player->currentOrigin, e->currentOrigin ) )
		{
			NPC_SetLookTarget( e, 0, level.time + LOOK_HOLD_MS );
			if ( d <= FACE_RANGE )
			{
				vec3_t dir, angles;
				VectorSubtract( player->currentOrigin, e->currentOrigin, dir );
				vectoangles( dir, angles );
				yaw = angles[YAW];
			}
		}
	}
	e->NPC->desiredYaw = AngleNormalize360( yaw );
}

void ThinkMember( size_t i )
{
	crew::Member &m = cs.members[i];
	gentity_t *e = &g_entities[m.ent];

	if ( !Eligible( e ) )
	{
		if ( !( m.flags & crew::MF_UNAVAILABLE ) )
		{
			m.flags |= crew::MF_UNAVAILABLE;
			m.flags &= ~crew::MF_OWNS_GOAL;
			m.goalPost = crew::NO_POST;
			gi.Printf( "CREW: member %d is no longer available\n", m.ent );
		}
		return;
	}

	if ( !cs.lostToWorld[i] && e->currentOrigin[2] < cs.floorZ - OUT_OF_WORLD_DROP )
	{
		cs.lostToWorld[i] = true;
		++cs.st.outOfWorld;
		gi.Printf( S_COLOR_RED"CREW: %s fell out of the world at %s\n", NameOf( e ), vtos( e->currentOrigin ) );
	}

	if ( m.addressedMs >= 0 && level.time - m.addressedMs > cs.cfg.ackBoundMs )
	{
		++cs.st.missedAcks;
		m.addressedMs = -1;
	}

	const int post = m.EffectivePost();

	crew::Signals s;
	s.scripted = IsScripted( e, m );
	s.hostile = e->enemy != NULL;
	s.overridden = m.overridePost != crew::NO_POST;
	s.hasPost = m.dutyPost != crew::NO_POST;

	const bool at = post != crew::NO_POST && AtPost( e, cs.posts[post], m.action == crew::ACT_HOLD );
	const crew::Action act = crew::Decide( m, s, at );
	const uint8_t level_ = crew::Arbitrate( s );

	if ( act != m.action || level_ != m.level )
	{
		if ( act == crew::ACT_YIELD && m.action != crew::ACT_YIELD ) ++cs.st.yields[level_];
		if ( g_crewDebug->integer )
			gi.Printf( "CREW: %s level %d action %d -> level %d action %d\n", NameOf( e ), m.level, m.action, level_, act );
	}
	m.level = level_;
	m.action = act;

	switch ( act )
	{
	case crew::ACT_YIELD:
	case crew::ACT_IDLE:
		ReleaseGoal( e, m );
		break;

	case crew::ACT_TRAVEL:
		if ( g_crewDebug->integer > 1 && level.time / 1000 != level.previousTime / 1000 )
			gi.Printf( "CREW: %s at %s, %.0f from '%s', waypoint %d (goal %d), ground %d, bstate %d\n", NameOf( e ),
				vtos( e->currentOrigin ), Dist2D( e->currentOrigin, cs.posts[post].origin ), cs.posts[post].name.c_str(),
				e->lastWaypoint, e->NPC->tempGoal ? e->NPC->tempGoal->lastWaypoint : -1, e->client->ps.groundEntityNum, e->NPC->behaviorState );
		if ( WriteRefused( e ) ) break;
		IssueGoal( e, m, post );
		TrackProgress( e, m );
		break;

	case crew::ACT_HOLD:
		if ( WriteRefused( e ) ) break;
		HoldPost( e, m, cs.posts[post] );
		break;
	}
}

// ---- roster ---------------------------------------------------------------------------------

// With a declared roster the crew are exactly those names; otherwise every Starfleet NPC present.
bool OnRoster( const gentity_t *e )
{
	if ( cs.cfg.roster.empty() ) return true;
	const char *name = e->script_targetname && e->script_targetname[0] ? e->script_targetname : e->targetname;
	if ( !name ) return false;
	for ( const crew::Roster &r : cs.cfg.roster )
		if ( !Q_stricmp( r.name.c_str(), name ) ) return true;
	return false;
}

void ScanRoster( void )
{
	if ( static_cast<int>( cs.members.size() ) >= cs.cfg.maxCrew ) return;

	bool added = false;
	for ( int n = 1; n < globals.num_entities && static_cast<int>( cs.members.size() ) < cs.cfg.maxCrew; ++n )
	{
		gentity_t *e = &g_entities[n];
		if ( !Eligible( e ) || !OnRoster( e ) || MemberIndex( e ) != -1 ) continue;
		crew::Member m;
		m.ent = static_cast<int16_t>( n );
		m.progressMs = level.time;
		cs.members.push_back( m );
		cs.lostToWorld.push_back( false );
		added = true;
	}
	if ( !added ) return;

	std::vector<crew::Candidate> cands( cs.members.size() );
	for ( size_t i = 0; i < cs.members.size(); ++i )
	{
		const gentity_t *e = &g_entities[cs.members[i].ent];
		if ( !e->inuse ) continue;
		if ( e->script_targetname ) cands[i].targetname = e->script_targetname;
		else if ( e->targetname ) cands[i].targetname = e->targetname;
		if ( e->NPC_type ) cands[i].type = e->NPC_type;
		VectorCopy( e->currentOrigin, cands[i].origin );
	}
	crew::AssignPosts( cs.members, cands, cs.posts );

	for ( const crew::Member &m : cs.members )
	{
		const gentity_t *e = &g_entities[m.ent];
		if ( m.level == crew::LEVEL_IDLE && m.firstArrivalMs < 0 && g_crewDebug->integer )
			gi.Printf( "CREW: %s (ent %d) -> %s\n", NameOf( e ), m.ent,
				m.dutyPost == crew::NO_POST ? "(spare)" : cs.posts[m.dutyPost].name.c_str() );
	}
}

// ---- the ship's roster (S5) -------------------------------------------------------------------
//
// With g_crewFromShip the crew on the player's deck are not declared by a scenario: they are
// whoever the ship simulation has on that deck right now. People arrive when their routine brings
// them here and leave when it takes them away, so a watch change is visible as people changing.
// Each stands at a place taken from the deck's own navigation, the same place every time they are
// here. (Real stations and quarters replace these places when the decks are furnished.)

const float DECK_BASE = 1636.0f;    // as in ship/g_scope.cpp: maps a height on the merged ship to a deck
const float SPAWN_CLEAR = 256.0f;   // nobody appears this close to the player

std::vector<int> shipEmbodied;      // roster indices currently embodied, parallel to nothing: looked up by name
struct Leaving { int idx; int sinceMs; };
std::vector<Leaving> shipLeaving;   // those due off the deck who are still in the player's sight
const int LEAVE_PATIENCE_MS = 45000; // how long someone may take to walk out of sight before they simply go

std::string RosterName( int rosterIndex ) { return Fmt( "lwh_crew_%03d", rosterIndex ); }

int PlayersDeck( void )
{
	if ( g_crewDeck->integer > 0 ) return g_crewDeck->integer;
	const float pitch = gi.cvar( "g_shipDeckPitch", "0", 0 )->value;
	if ( pitch <= 0.0f ) return 0;
	return static_cast<int>( std::floor( ( -g_entities[0].currentOrigin[2] - DECK_BASE ) / pitch ) ) + 1;
}

// The navigation nodes of one deck. On a map of one deck that is all of them.
std::vector<int> DeckNodes( int deck )
{
	std::vector<int> nodes;
	const float pitch = gi.cvar( "g_shipDeckPitch", "0", 0 )->value;
	for ( int i = 0; i < navigator.GetNumNodes(); ++i )
	{
		vec3_t at;
		navigator.GetNodePosition( i, at );
		if ( g_crewDeck->integer > 0 || pitch <= 0.0f
			|| static_cast<int>( std::floor( ( -at[2] - DECK_BASE ) / pitch ) ) + 1 == deck )
		{
			nodes.push_back( i );
		}
	}
	return nodes;
}

gentity_t *FindByName( const std::string &name )
{
	for ( int n = 1; n < globals.num_entities; ++n )
	{
		gentity_t *e = &g_entities[n];
		if ( e->inuse && e->client && e->script_targetname && !Q_stricmp( e->script_targetname, name.c_str() ) ) return e;
	}
	return NULL;
}

// A station on the ship's map: a marker named "lwh_station_<system>", placed where that system is
// worked (the bridge, Main Engineering, the mess hall, ...). A crew member on duty stands there
// rather than at an arbitrary navigation node. No marker, no station: the deck's navigation is used.
bool StationFor( int system, vec3_t out )
{
	char name[32];
	Com_sprintf( name, sizeof( name ), "lwh_station_%d", system );
	for ( int n = 1; n < globals.num_entities; ++n )
	{
		gentity_t *e = &g_entities[n];
		if ( e->inuse && e->targetname && !Q_stricmp( e->targetname, name ) )
		{
			VectorCopy( e->currentOrigin, out );
			return true;
		}
	}
	return false;
}

void SyncShipRoster( void )
{
	ship::Ship *vessel = Ship_Get();
	if ( !vessel ) return;
	const int deck = PlayersDeck();
	if ( deck < 1 ) return;
	const std::vector<int> nodes = DeckNodes( deck );
	if ( nodes.empty() ) return;

	std::vector<int> wanted = ship::CrewOnDeck( *vessel, deck );
	if ( static_cast<int>( wanted.size() ) > cs.cfg.maxCrew ) wanted.resize( cs.cfg.maxCrew );
	const gentity_t *player = &g_entities[0];

	// Those whose routine has taken them off this deck leave -- once the player is not looking.
	for ( size_t k = 0; k < shipEmbodied.size(); )
	{
		const int idx = shipEmbodied[k];
		if ( std::find( wanted.begin(), wanted.end(), idx ) != wanted.end() ) { ++k; continue; }
		gentity_t *e = FindByName( RosterName( idx ) );
		const bool inSight = e && gi.inPVS( player->currentOrigin, e->currentOrigin )
			&& Distance( player->currentOrigin, e->currentOrigin ) < 1024.0f;
		if ( inSight )
		{//they do not vanish in front of the player: they walk off, to the far end of the deck
			size_t l = 0;
			while ( l < shipLeaving.size() && shipLeaving[l].idx != idx ) ++l;
			if ( l == shipLeaving.size() )
			{
				shipLeaving.push_back( { idx, level.time } );
				const std::string place = "place_" + RosterName( idx );
				for ( crew::Post &q : cs.posts )
				{
					if ( q.name != place ) continue;
					float best = -1.0f;
					for ( int node : nodes )
					{
						vec3_t at;
						navigator.GetNodePosition( node, at );
						const float d = Distance( at, player->currentOrigin );
						if ( d > best ) { best = d; VectorCopy( at, q.origin ); }
					}
				}
			}
			if ( level.time - shipLeaving[l].sinceMs < LEAVE_PATIENCE_MS ) { ++k; continue; }
		}
		for ( size_t l = 0; l < shipLeaving.size(); ++l )
			if ( shipLeaving[l].idx == idx ) { shipLeaving.erase( shipLeaving.begin() + l ); break; }
		if ( e ) G_FreeEntity( e );
		shipEmbodied.erase( shipEmbodied.begin() + k );
	}

	// Those whose routine has brought them here arrive, somewhere the player is not standing.
	for ( int idx : wanted )
	{
		if ( std::find( shipEmbodied.begin(), shipEmbodied.end(), idx ) != shipEmbodied.end() ) continue;
		// someone who should have left but is still in the player's sight holds their place in the
		// count: the deck never shows more than its cap, even across a change of company
		if ( static_cast<int>( shipEmbodied.size() ) >= cs.cfg.maxCrew ) break;
		const std::string name = RosterName( idx );
		vec3_t at;
		bool placed = false;

		// On duty at a system the ship's map gives a station to: stand at that station.
		const ship::CrewMember &who = vessel->crew[idx];
		if ( who.activity == ship::ACT_ON_DUTY && who.post < ship::SYS_COUNT
			&& StationFor( who.post, at ) && Distance( at, player->currentOrigin ) >= SPAWN_CLEAR )
		{
			placed = true;
		}
		else for ( size_t tries = 0; tries < nodes.size() && !placed; ++tries )
		{
			navigator.GetNodePosition( nodes[( idx * 7919u + tries * 31u ) % nodes.size()], at );
			placed = Distance( at, player->currentOrigin ) >= SPAWN_CLEAR;
		}
		if ( !placed ) continue;

		// their place on this deck: a station if there is one, else the same node whenever they are here
		crew::Post p;
		p.name = "place_" + name;
		p.holder = name;
		p.hasOrigin = true;
		VectorCopy( at, p.origin );
		bool known = false;
		for ( crew::Post &q : cs.posts )
		{
			if ( q.name != p.name ) continue;
			known = true;
			VectorCopy( p.origin, q.origin ); //back from wherever they last walked off to
		}
		if ( !known && static_cast<int>( cs.posts.size() ) < crew::MAX_POSTS )
		{
			cs.posts.push_back( p );
			cs.postHash = crew::PostTableHash( cs.posts );
		}
		bool listed = false;
		for ( const crew::Roster &r : cs.cfg.roster ) listed = listed || r.name == name;
		if ( !listed ) { crew::Roster r; r.name = name; cs.cfg.roster.push_back( r ); }

		gentity_t *sp = G_Spawn();
		if ( !sp ) return;
		vec3_t angles = { 0, 0, 0 };
		G_SetOrigin( sp, at );
		VectorCopy( at, sp->s.origin );
		G_SetAngles( sp, angles );
		sp->NPC_type = G_NewString( vessel->crew[idx].type.c_str() );
		sp->NPC_targetname = G_NewString( name.c_str() );
		sp->fullName = G_NewString( vessel->crew[idx].name.c_str() );
		sp->spawnflags = SFB_SILENTSPAWN;
		SP_NPC_starfleet( sp );
		shipEmbodied.push_back( idx );
	}

	// Members whose bodies are gone are dropped, so the table does not fill with the departed.
	for ( size_t i = 0; i < cs.members.size(); )
	{
		if ( cs.members[i].flags & crew::MF_UNAVAILABLE )
		{
			cs.members.erase( cs.members.begin() + i );
			cs.lostToWorld.erase( cs.lostToWorld.begin() + i );
		}
		else ++i;
	}

	static int lastWanted = -1, lastEmbodied = -1, lastDeck = -1;
	if ( static_cast<int>( wanted.size() ) != lastWanted || static_cast<int>( shipEmbodied.size() ) != lastEmbodied || deck != lastDeck )
	{
		lastWanted = static_cast<int>( wanted.size() ); lastEmbodied = static_cast<int>( shipEmbodied.size() ); lastDeck = deck;
		const int sod = vessel->SecondOfDay();
		gi.Printf( "CREW: ship time %02d:%02d, deck %d: the ship has %d here (showing up to %d), %d embodied\n", sod / 3600, sod % 3600 / 60,
			deck, static_cast<int>( ship::CrewOnDeck( *vessel, deck ).size() ), cs.cfg.maxCrew, lastEmbodied );
	}
}

// ---- boarders, embodied (S7, S8) --------------------------------------------------------------
//
// The ship simulation counts the boarders on each deck. On the deck the player is on they are
// bodies: hostile NPCs, as many as the count says (to a cap), drones if the party is Borg. The tie
// runs both ways -- when the count falls, bodies out of sight are withdrawn; when a body is killed,
// by the player or by anyone, the ship has one boarder fewer.

const int INTRUDER_CAP = 6;
struct IntruderBody { std::string name; int deck; };
std::vector<IntruderBody> intruderBodies;
int intruderSerial = 0;

void SyncIntruders( void )
{
	ship::Ship *vessel = Ship_Get();
	if ( !vessel ) return;
	const int deck = PlayersDeck();
	if ( deck < 1 || deck > ship::DECKS ) return;
	const gentity_t *player = &g_entities[0];
	ship::Deck &where = vessel->decks[deck - 1];

	for ( size_t k = 0; k < intruderBodies.size(); )
	{
		gentity_t *e = FindByName( intruderBodies[k].name );
		if ( intruderBodies[k].deck != deck )
		{//the player has left that deck: its boarders go back to being a number
			if ( e ) G_FreeEntity( e );
			intruderBodies.erase( intruderBodies.begin() + k );
			continue;
		}
		if ( !e ) { intruderBodies.erase( intruderBodies.begin() + k ); continue; }
		if ( e->health <= 0 )
		{//killed: the ship has one fewer aboard
			where.intruders = std::max( 0.0f, where.intruders - 1.0f );
			gi.Printf( "CREW: a boarder is down on deck %d; the ship counts %.0f left there\n", deck, std::ceil( where.intruders - 1e-3f ) );
			intruderBodies.erase( intruderBodies.begin() + k );
			continue;
		}
		++k;
	}

	const int want = std::min( INTRUDER_CAP, static_cast<int>( std::ceil( where.intruders - 1e-3f ) ) );
	// fewer than there are bodies (the ship's own defenders have accounted for some): withdraw the unseen
	for ( size_t k = 0; k < intruderBodies.size() && static_cast<int>( intruderBodies.size() ) > want; )
	{
		gentity_t *e = FindByName( intruderBodies[k].name );
		if ( e && gi.inPVS( player->currentOrigin, e->currentOrigin ) && Distance( player->currentOrigin, e->currentOrigin ) < 1024.0f ) { ++k; continue; }
		if ( e ) G_FreeEntity( e );
		intruderBodies.erase( intruderBodies.begin() + k );
	}
	if ( static_cast<int>( intruderBodies.size() ) >= want ) return;

	const std::vector<int> nodes = DeckNodes( deck );
	if ( nodes.empty() ) return;
	while ( static_cast<int>( intruderBodies.size() ) < want )
	{
		vec3_t at;
		bool placed = false;
		for ( size_t tries = 0; tries < nodes.size() && !placed; ++tries )
		{
			navigator.GetNodePosition( nodes[( intruderSerial * 2654435761u + tries * 97u ) % nodes.size()], at );
			placed = Distance( at, player->currentOrigin ) >= 2 * SPAWN_CLEAR;
		}
		if ( !placed ) return;
		gentity_t *sp = G_Spawn();
		if ( !sp ) return;
		const std::string name = Fmt( "lwh_boarder_%03d", intruderSerial++ );
		vec3_t angles = { 0, 0, 0 };
		G_SetOrigin( sp, at );
		VectorCopy( at, sp->s.origin );
		G_SetAngles( sp, angles );
		sp->NPC_targetname = G_NewString( name.c_str() );
		sp->spawnflags = SFB_SILENTSPAWN;
		if ( where.borg ) SP_NPC_borg( sp );
		else SP_NPC_klingon( sp );
		intruderBodies.push_back( { name, deck } );
		gi.Printf( "CREW: a %s is on deck %d (%d embodied of %.0f the ship counts)\n", where.borg ? "drone" : "boarder", deck,
			static_cast<int>( intruderBodies.size() ), std::ceil( where.intruders ) );
	}
}

// ---- damage, visible (S6) ---------------------------------------------------------------------
//
// Damage must be visible in the world, not only a health number: a system the ship has damaged
// sparks where it is worked, on the deck the player is on. The tie is one-way and honest -- the
// ship's own health decides, and repairing the system or leaving the deck puts the sparks out. The
// spot is the game's own fx_spark, spawned at the station marker (the same marker the crew stand
// at), so no new art and no new class.

const float DAMAGE_VISIBLE = 0.85f; // below this share of health, a system visibly sparks

struct DamageSpot { int system; int deck; int ent; };
std::vector<DamageSpot> damageSpots;
int damageSerial = 0;

void SyncDamage( void )
{
	ship::Ship *vessel = Ship_Get();
	if ( !vessel ) return;
	const int deck = PlayersDeck();
	if ( deck < 1 || deck > ship::DECKS ) return;

	// Drop a spot whose system is repaired, or which the player has left, or whose entity is gone.
	for ( size_t k = 0; k < damageSpots.size(); )
	{
		const int ent = damageSpots[k].ent;
		const bool live = ent > 0 && ent < globals.num_entities && g_entities[ent].inuse;
		const bool hurt = vessel->systems[damageSpots[k].system].health < DAMAGE_VISIBLE;
		if ( damageSpots[k].deck != deck || !hurt )
		{
			if ( live ) G_FreeEntity( &g_entities[ent] );
			damageSpots.erase( damageSpots.begin() + k );
			continue;
		}
		if ( !live ) { damageSpots.erase( damageSpots.begin() + k ); continue; }
		++k;
	}

	for ( int sys = 0; sys < ship::SYS_COUNT; ++sys )
	{
		if ( ship::Spec( static_cast<ship::SystemId>( sys ) ).deck != deck ) continue;
		if ( vessel->systems[sys].health >= DAMAGE_VISIBLE ) continue;
		bool have = false;
		for ( const DamageSpot &d : damageSpots ) if ( d.system == sys ) { have = true; break; }
		if ( have ) continue;
		vec3_t at;
		if ( !StationFor( sys, at ) ) continue; // no marker: nowhere to show it
		gentity_t *sp = G_Spawn();
		if ( !sp ) return;
		G_SetOrigin( sp, at );
		VectorCopy( at, sp->s.origin );
		vec3_t angles = { 0, 0, 0 };
		G_SetAngles( sp, angles );
		SP_fx_spark( sp );
		damageSpots.push_back( { sys, deck, static_cast<int>( sp - g_entities ) } );
		gi.Printf( "CREW: %s is damaged; sparks at %s on deck %d\n",
			ship::Spec( static_cast<ship::SystemId>( sys ) ).name, vtos( at ), deck );
	}
}

// ---- fire, visible (S6) -----------------------------------------------------------------------
//
// A burning deck is a deck the player can see burn: smoke (and a little electrical fire) at points
// across the deck, as many as the fire is strong, put out when the fire is, or when the player
// leaves. The ship counts the fire; the world shows it. Uses the game's own fx_smoke/fx_electricfire.

const float FIRE_VISIBLE = 0.05f; // below this the fire is not worth drawing
const int FIRE_SPOT_CAP = 8;

struct FireSpot { int deck; int ent; bool flame; };
std::vector<FireSpot> fireSpots;

void SyncFire( void )
{
	ship::Ship *vessel = Ship_Get();
	if ( !vessel ) return;
	const int deck = PlayersDeck();
	if ( deck < 1 || deck > ship::DECKS ) return;
	const float fire = vessel->decks[deck - 1].fire;

	for ( size_t k = 0; k < fireSpots.size(); )
	{
		const int ent = fireSpots[k].ent;
		const bool live = ent > 0 && ent < globals.num_entities && g_entities[ent].inuse;
		if ( fireSpots[k].deck != deck || fire < FIRE_VISIBLE )
		{
			if ( live ) G_FreeEntity( &g_entities[ent] );
			fireSpots.erase( fireSpots.begin() + k );
			continue;
		}
		if ( !live ) { fireSpots.erase( fireSpots.begin() + k ); continue; }
		++k;
	}
	if ( fire < FIRE_VISIBLE ) return;

	const std::vector<int> nodes = DeckNodes( deck );
	if ( nodes.empty() ) return;
	const int want = std::min( FIRE_SPOT_CAP, static_cast<int>( std::ceil( fire * FIRE_SPOT_CAP - 1e-3f ) ) );
	const size_t before = fireSpots.size();
	while ( static_cast<int>( fireSpots.size() ) < want )
	{
		vec3_t at;
		navigator.GetNodePosition( nodes[fireSpots.size() % nodes.size()], at );
		gentity_t *sp = G_Spawn();
		if ( !sp ) return;
		G_SetOrigin( sp, at );
		VectorCopy( at, sp->s.origin );
		vec3_t angles = { 0, 0, 0 };
		G_SetAngles( sp, angles );
		const bool flame = ( fireSpots.size() % 3 == 0 ); // one in three is flame, the rest smoke
		if ( flame ) SP_fx_electricfire( sp );
		else SP_fx_smoke( sp );
		fireSpots.push_back( { deck, static_cast<int>( sp - g_entities ), flame } );
	}
	if ( fireSpots.size() > before )
		gi.Printf( "CREW: fire on deck %d (%d%%); %d effects in the world\n", deck,
			static_cast<int>( fire * 100 + 0.5f ), static_cast<int>( fireSpots.size() ) );
}

// ---- the environment, in the world --------------------------------------------------------------
//
// The model carries per-deck atmosphere, gravity and a breach; the world showed none of it. This is
// the same pattern as SyncDamage and SyncFire: the ship decides, the module switches an authored
// effect on and off. Gravity is per person (the engine's own mechanism: ps.gravity plus
// SVF_CUSTOM_GRAVITY, as target_gravity_change_use sets it), so the player feels the deck's plating
// and the crew float on BS_FLY. A breach is an authored trigger_push aimed at the hole and a
// trigger_hurt; the force field over it is an authored func_usable brush. With `g_env 0` none of it
// runs, and the ship behaves and saves exactly as before.

const float BREACH_WHOLE = 0.99f;                 // below this the hull is open and the air is going
const char *const BREACH_PUSH = "lwh_breach_push";
const char *const BREACH_HURT = "lwh_breach_hurt";
const char *const BREACH_FIELD = "lwh_breach_field";

gentity_t *FindTargetname( const char *name )
{
	for ( int n = 1; n < globals.num_entities; ++n )
	{
		gentity_t *e = &g_entities[n];
		if ( e->inuse && e->targetname && !Q_stricmp( e->targetname, name ) ) return e;
	}
	return NULL;
}

// A trigger is live when it does NOT carry SVF_INACTIVE -- exactly what the engine's own
// target_activate / target_deactivate write (g_utils.cpp, G_SetActiveState).
void SetTriggerLive( gentity_t *e, bool live )
{
	if ( !e ) return;
	const bool isLive = !( e->svFlags & SVF_INACTIVE );
	if ( isLive == live ) return;
	if ( live ) e->svFlags &= ~SVF_INACTIVE;
	else e->svFlags |= SVF_INACTIVE;
}

// The field is the game's own func_usable, toggled exactly as a button would: its `count` is the
// on/off state the game already keeps, so we call its own use function rather than re-implement it.
void SetFieldBrush( gentity_t *e, bool on )
{
	if ( !e ) return;
	const bool isOn = e->count != 0;
	if ( isOn == on ) return;
	GEntity_UseFunc( e, &g_entities[0], &g_entities[0] );
}

// The crew on this deck float when the plating is weak: BS_FLY is the behaviour state the code
// itself documents as "Moves around without gravity" (bstate.h). Only the layer's own embodied crew
// are touched, and a script's NPC is left alone.
void SyncGravityCrew( const std::vector<int> &embodied, bool floatNow )
{
	for ( int idx : embodied )
	{
		gentity_t *e = FindByName( RosterName( idx ) );
		if ( !e || !e->NPC || e->health <= 0 ) continue;
		if ( floatNow )
		{
			if ( e->NPC->behaviorState == BS_FLY ) continue;
			if ( ScriptOwns( e ) ) continue;
			e->NPC->behaviorState = BS_FLY;
			e->NPC->stats.moveType = MT_FLYSWIM;
		}
		else if ( e->NPC->behaviorState == BS_FLY )
		{
			e->NPC->behaviorState = BS_DEFAULT;
		}
	}
}

bool envGravityOn = false;   // the module, not the map, set the player's custom gravity
bool playerBoots = false;    // the way back: standard gravity for the player while they get out
bool envCrewFloat = false;

// One person's gravity, driven from the deck's plating. The engine has the mechanism and no way
// back (its own FIXME); the way back is the point of this function: when the plating holds again,
// clear SVF_CUSTOM_GRAVITY and the world's value returns.
void SyncGravity( void )
{
	ship::Ship *vessel = Ship_Get();
	if ( !vessel ) return;
	gentity_t *p = &g_entities[0];
	if ( !p->client ) return;
	const int deck = PlayersDeck();
	const bool known = deck >= 1 && deck <= ship::DECKS;
	const float scale = known ? ship::GravityScale( *vessel, deck ) : 1.0f;
	const float world = g_gravity ? g_gravity->value : gi.cvar( "g_gravity", "800", 0 )->value;

	// Hands back to the engine: the plate is at full, the deck is not the ship's, or the player has
	// chosen the boots. A deck that recovers must not stay wrong.
	if ( !known || scale >= 1.0f || playerBoots )
	{
		if ( p->svFlags & SVF_CUSTOM_GRAVITY )
		{
			p->svFlags &= ~SVF_CUSTOM_GRAVITY;
			p->client->ps.gravity = world;
			if ( envGravityOn )
				gi.Printf( playerBoots ? "ENV: magnetic boots on; the player has standard gravity\n"
					: "ENV: deck %d has hold again; gravity is standard\n", deck );
		}
		if ( !known || scale >= 1.0f ) playerBoots = false;
		envGravityOn = false;
		if ( envCrewFloat ) { SyncGravityCrew( shipEmbodied, false ); envCrewFloat = false; }
		return;
	}

	const int want = ship::ScaleGravity( static_cast<int>( world + 0.5f ), scale );
	if ( !( p->svFlags & SVF_CUSTOM_GRAVITY ) || p->client->ps.gravity != want )
	{
		p->client->ps.gravity = want;
		p->svFlags |= SVF_CUSTOM_GRAVITY;
		if ( !envGravityOn )
			gi.Printf( "ENV: deck %d plating at %d%%; the player floats on %d gravity\n",
				deck, static_cast<int>( scale * 100 + 0.5f ), want );
		envGravityOn = true;
	}
	envCrewFloat = true;
	SyncGravityCrew( shipEmbodied, true );
}

// A breach the player can feel, and a field they can see. The ship already tracks the hull and the
// field; this switches the authored effects to match, and says so when the state changes.
void SyncBreach( void )
{
	gentity_t *push = FindTargetname( BREACH_PUSH );
	gentity_t *hurt = FindTargetname( BREACH_HURT );
	gentity_t *field = FindTargetname( BREACH_FIELD );
	if ( !push && !hurt && !field ) return; // this map has no authored breach
	ship::Ship *vessel = Ship_Get();
	if ( !vessel ) return;
	const int deck = PlayersDeck();
	if ( deck < 1 || deck > ship::DECKS ) return;
	const ship::Deck &d = vessel->decks[deck - 1];

	const bool breached = d.hull < BREACH_WHOLE;
	const bool fieldUp = breached && d.forceField;
	const bool open = breached && !fieldUp; // the hole is pulling air and bodies out

	SetTriggerLive( push, open );
	SetTriggerLive( hurt, open );
	SetFieldBrush( field, fieldUp );

	static int last = -1;
	const int now = open ? 0 : fieldUp ? 1 : 2;
	if ( now != last )
	{
		last = now;
		if ( open )
			gi.Printf( "ENV: deck %d is breached; the air is going (push and hurt on, field off)%s\n", deck,
				push ? Fmt( "; the push throws %s", vtos( push->s.origin2 ) ).c_str() : "; no push trigger" );
		else if ( fieldUp )
			gi.Printf( "ENV: a force field is up on deck %d; the air holds\n", deck );
		else
			gi.Printf( "ENV: deck %d is whole; no breach effect\n", deck );
	}
}

void Crew_EnvFrame( void )
{
	if ( !g_env || !g_env->integer )
	{//turning the extension off puts the world back the way the engine expects it
		if ( envGravityOn )
		{
			gentity_t *p = &g_entities[0];
			if ( p->client && ( p->svFlags & SVF_CUSTOM_GRAVITY ) )
			{
				p->svFlags &= ~SVF_CUSTOM_GRAVITY;
				p->client->ps.gravity = g_gravity ? g_gravity->value : 800;
			}
			envGravityOn = false;
			if ( envCrewFloat ) { SyncGravityCrew( shipEmbodied, false ); envCrewFloat = false; }
		}
		return;
	}
	SyncGravity();
	SyncBreach();
}

// ---- the player in the world (Stage B) ----------------------------------------------------------
//
// The player is a crew record (vessel.player), so the causes the model already tracks -- a console
// that lets go, a deck without air, fire -- reach them by the same tick as anyone else. What was
// missing is the body in the world: this reads the record and drives the player's own health, and
// reads the world's damage back into the record, so a boarder or a weapon lands on the same person
// the ship treats in the ward. The same pattern as SyncDamage/SyncFire. Gated by g_player, off by
// default, and it adds no save field (everything here is recomputed from world and record).

bool playerFelled = false;      // the body has already been dropped for a closed record
uint8_t trackedStatus = 255;    // the record's status last frame (for the announcements)
float trackedSeverity = 0.0f;   // ... and its injury, so healing is told from taking a fresh hit

// What the world did to the player's body, named from the state of the deck they are on, so the
// record's line traces to the room and not to a bare number.
std::string PlayerCause( const ship::Ship &s, int deck )
{
	if ( deck >= 1 && deck <= ship::DECKS )
	{
		const ship::Deck &d = s.decks[deck - 1];
		if ( d.intruders > 0.0f ) return "boarders";
		if ( d.fire > 0.05f ) return "fire";
		if ( d.atmosphere < ship::AIRLESS ) return "no air";
	}
	return "a weapon";
}

void SyncPlayerBody( ship::Ship *s, gentity_t *p, int deck )
{
	const int me = s->player;
	if ( me < 0 || me >= static_cast<int>( s->crew.size() ) ) return;
	const int maxh = p->max_health > 0 ? p->max_health : 100;
	const uint8_t st = s->crew[me].status;
	const float sev = s->crew[me].severity;

	// The record is closed. The body falls, once, through the game's own damage path; there is no
	// reload (LWH_BlockRespawn), and the closed record is never restored.
	if ( st == ship::CREW_DEAD || st == ship::CREW_ASSIMILATED )
	{
		if ( !playerFelled && p->health > 0 )
		{
			playerFelled = true;
			gi.Printf( "PLAYER: %s is %s; the body falls where it stood\n", s->crew[me].name.c_str(),
				st == ship::CREW_ASSIMILATED ? "lost to the Collective" : "dead" );
			G_Damage( p, p, p, NULL, p->currentOrigin, 100000, 0, MOD_UNKNOWN );
		}
		trackedStatus = st; trackedSeverity = sev;
		return;
	}

	// Recovered: the body is whole again.
	if ( st == ship::CREW_FIT )
	{
		playerFelled = false;
		if ( p->health < maxh ) p->health = maxh;
		trackedStatus = st; trackedSeverity = sev;
		return;
	}

	// Injured. The record and the body agree: where the model hurt the player (the console, the air,
	// the fire) the body follows the record down; where the world hurt the body (a boarder, a weapon)
	// the record follows the body down, into the same ward any casualty reaches.
	if ( trackedStatus != st && trackedStatus != 255 )
		gi.Printf( "PLAYER: %s is hurt; the ship treats them as any casualty\n", s->crew[me].name.c_str() );
	const int want = ship::PlayerBodyHealth( *s, maxh );
	if ( sev > trackedSeverity + 1e-4f )
	{
		if ( p->health > want ) p->health = want;         // the model worsened: the body follows
	}
	else if ( sev < trackedSeverity - 1e-4f )
	{
		if ( p->health < want ) p->health = want;         // treatment: the body recovers with the record
	}
	else if ( p->health < want )
	{
		// The body is below the record: something in the world hurt the player. Write it in.
		ship::WoundPlayer( *s, ( want - p->health ) / static_cast<float>( maxh ), PlayerCause( *s, deck ) );
		const int now = ship::PlayerBodyHealth( *s, maxh );
		if ( p->health > now ) p->health = now;
	}
	else if ( p->health > want )
	{
		p->health = want;
	}
	trackedStatus = st; trackedSeverity = s->crew[me].severity;
}

void Crew_PlayerFrameImpl( void )
{
	if ( !g_player || !g_player->integer ) { trackedStatus = 255; playerFelled = false; return; }
	ship::Ship *s = Ship_Get();
	if ( !s ) return;
	gentity_t *p = &g_entities[0];
	if ( !p->inuse || !p->client ) return;

	// Where the player's body is: the record must know, or the deck's air, fire and hazards never
	// reach the person holding the controls.
	const int deck = PlayersDeck();
	ship::SetPlayerDeck( *s, deck );
	SyncPlayerBody( s, p, deck );
}

// Declared crew with a type and a position are spawned through the map's own spawner, exactly as
// an NPC_starfleet entity in the map would be: an existing character, its own model and voice.
void SpawnDeclaredCrew( void )
{
	for ( const crew::Roster &r : cs.cfg.roster )
	{
		if ( !r.spawn ) continue;
		gentity_t *sp = G_Spawn();
		if ( !sp ) return;
		vec3_t origin, angles;
		VectorCopy( r.origin, origin );
		VectorSet( angles, 0, r.yaw, 0 );
		G_SetOrigin( sp, origin );
		VectorCopy( origin, sp->s.origin );
		G_SetAngles( sp, angles );
		sp->NPC_type = G_NewString( r.type.c_str() );
		sp->NPC_targetname = G_NewString( r.name.c_str() );
		sp->spawnflags = SFB_SILENTSPAWN;
		SP_NPC_starfleet( sp );
	}
}

// ---- posts ----------------------------------------------------------------------------------

const gentity_t *FindNavgoal( const char *name )
{
	const gentity_t *fallback = NULL;
	for ( int n = 1; n < globals.num_entities; ++n )
	{
		const gentity_t *e = &g_entities[n];
		if ( !e->inuse || !e->targetname || Q_stricmp( e->targetname, name ) ) continue;
		if ( e->classname && !Q_stricmp( e->classname, "navgoal" ) ) return e;
		if ( !fallback && !e->client ) fallback = e;
	}
	return fallback;
}

void FillFromNavgoal( crew::Post &p, const gentity_t *e )
{
	VectorCopy( e->currentOrigin, p.origin );
	p.hasOrigin = true;
	if ( !p.hasYaw ) p.yaw = e->s.angles[YAW];
}

void LoadPosts( void )
{
	std::vector<crew::Post> authored;
	authored.swap( cs.cfg.posts );

	if ( !authored.empty() )
	{
		for ( crew::Post &p : authored )
		{
			if ( !p.hasOrigin )
			{
				const gentity_t *e = FindNavgoal( p.navgoal.c_str() );
				if ( !e )
				{
					gi.Printf( S_COLOR_YELLOW"CREW: post '%s': no navgoal '%s' in this map; dropped\n", p.name.c_str(), p.navgoal.c_str() );
					continue;
				}
				FillFromNavgoal( p, e );
			}
			cs.posts.push_back( p );
		}
	}
	else if ( !g_crewFromShip->integer ) //the ship's roster brings its own places; the map's navgoals belong to its scripts
	{//no authored list: every script navgoal in the map is a post, in a stable order
		std::vector<const gentity_t *> ents;
		for ( int n = 1; n < globals.num_entities; ++n )
		{
			const gentity_t *e = &g_entities[n];
			if ( e->inuse && e->classname && !Q_stricmp( e->classname, "navgoal" ) && e->targetname && e->targetname[0] )
				ents.push_back( e );
		}
		std::sort( ents.begin(), ents.end(), []( const gentity_t *a, const gentity_t *b ) {
			const int c = Q_stricmp( a->targetname, b->targetname );
			return c ? c < 0 : a->s.number < b->s.number;
		} );
		for ( const gentity_t *e : ents )
		{
			if ( static_cast<int>( cs.posts.size() ) >= crew::MAX_POSTS ) break;
			if ( !cs.posts.empty() && !Q_stricmp( cs.posts.back().name.c_str(), e->targetname ) ) continue;
			crew::Post p;
			p.name = p.navgoal = e->targetname;
			FillFromNavgoal( p, e );
			cs.posts.push_back( p );
		}
	}

	cs.postHash = crew::PostTableHash( cs.posts );
	cs.floorZ = 0;
	for ( size_t i = 0; i < cs.posts.size(); ++i )
		cs.floorZ = i ? std::min( cs.floorZ, cs.posts[i].origin[2] ) : cs.posts[i].origin[2];
}

void LoadConfig( void )
{
	cs.cfg = crew::Config();
	const std::string path = std::string( "maps/" ) + level.mapname + ".crew";
	void *buf = NULL;
	const int len = gi.FS_ReadFile( path.c_str(), &buf );
	if ( len <= 0 || !buf )
	{
		if ( buf ) gi.FS_FreeFile( buf );
		return;
	}
	const std::string text( static_cast<const char *>( buf ), len );
	gi.FS_FreeFile( buf );

	std::string error;
	crew::Config parsed;
	if ( !crew::ParseConfig( text, parsed, error ) )
	{//a half-read configuration is worse than none: fall back to the defaults, loudly
		gi.Printf( S_COLOR_RED"CREW: %s: %s -- using defaults\n", path.c_str(), error.c_str() );
		return;
	}
	cs.cfg = parsed;
	gi.Printf( "CREW: read %s (%d post(s), %d declared crew)\n", path.c_str(),
		static_cast<int>( cs.cfg.posts.size() ), static_cast<int>( cs.cfg.roster.size() ) );
}

void ApplySave( void );

// Deferred to the first frame: by then the map's entities exist on every path into a level --
// a fresh start, an autosave restart, or a full load -- which is not true inside InitGame.
void Ready( void )
{
	cs.ready = true;
	LoadPosts();
	gi.Printf( "CREW: %d post(s), up to %d crew\n", static_cast<int>( cs.posts.size() ), cs.cfg.maxCrew );
	if ( cs.posts.empty() )
		gi.Printf( S_COLOR_YELLOW"CREW: no posts: the map has no waypoint_navgoal and the scenario authored none\n" );
	ApplySave();
}

// ---- reporting ------------------------------------------------------------------------------

std::vector<bool> AtPostNow( void )
{
	std::vector<bool> at( cs.members.size(), false );
	for ( size_t i = 0; i < cs.members.size(); ++i )
	{
		const crew::Member &m = cs.members[i];
		const int post = m.EffectivePost();
		gentity_t *e = &g_entities[m.ent];
		if ( post == crew::NO_POST || !Eligible( e ) ) continue;
		at[i] = AtPost( e, cs.posts[post], m.action == crew::ACT_HOLD );
	}
	return at;
}

std::string MembersJson( const std::vector<crew::Member> &members )
{
	std::string j = "[";
	for ( size_t i = 0; i < members.size(); ++i )
	{
		const crew::Member &m = members[i];
		const gentity_t *e = &g_entities[m.ent];
		j += Fmt( "%s\n    {\"ent\": %d, \"name\": \"%s\", \"duty_post\": %d, \"override_post\": %d, \"goal_post\": %d, "
			"\"action\": %d, \"flags\": %d, \"schedule_cursor\": %d, \"first_arrival_ms\": %d}",
			i ? "," : "", m.ent, e->inuse ? NameOf( e ) : "", m.dutyPost, m.overridePost, m.goalPost,
			m.action, m.flags, m.scheduleCursor, m.firstArrivalMs );
	}
	return j + "\n  ]";
}

void WriteFile( const char *suffix, const std::string &text )
{
	const std::string path = "crew/" + MapKey() + suffix;
	fileHandle_t f = 0;
	gi.FS_FOpenFile( path.c_str(), &f, FS_WRITE );
	if ( !f )
	{
		gi.Printf( S_COLOR_YELLOW"CREW: could not write %s\n", path.c_str() );
		return;
	}
	gi.FS_Write( text.c_str(), static_cast<int>( text.size() ), f );
	gi.FS_FCloseFile( f );
	gi.Printf( "CREW: wrote %s\n", path.c_str() );
}

// Which entities have a script in flight. Taken with the layer on and with it off, the two lists
// are the evidence that the deck's own scripted sequences were not displaced.
std::string ScriptCensusJson( void )
{
	std::vector<std::string> running;
	int withSequencer = 0;
	for ( int n = 0; n < globals.num_entities; ++n )
	{
		gentity_t *e = &g_entities[n];
		if ( !e->inuse || !e->sequencer || MemberIndex( e ) != -1 ) continue;
		++withSequencer;
		if ( !e->sequencer->IsRunning() ) continue;
		const char *name = e->script_targetname && e->script_targetname[0] ? e->script_targetname
			: e->targetname && e->targetname[0] ? e->targetname : e->classname ? e->classname : "?";
		running.push_back( name );
	}
	std::sort( running.begin(), running.end() );
	std::string j = Fmt( "{\"with_sequencer\": %d, \"running\": [", withSequencer );
	for ( size_t i = 0; i < running.size(); ++i ) j += std::string( i ? ", " : "" ) + "\"" + running[i] + "\"";
	return j + "]}";
}

std::string GameFrameJson( void )
{
	return Fmt( "  \"game_frames\": %d,\n  \"game_frame_avg_us\": %d,\n  \"game_frame_max_us\": %d,\n", cs.gameFrames,
		cs.gameFrames ? static_cast<int>( cs.gameNs / cs.gameFrames / 1000 ) : 0, static_cast<int>( cs.maxGameNs / 1000 ) );
}

void Report( bool toFile )
{
	const crew::Stats &st = cs.st;
	const size_t saveBytes = crew::Pack( cs.members, cs.postHash ).size() + CHUNK_OVERHEAD;
	const crew::Verdict v = crew::Judge( cs.cfg, st, cs.members, saveBytes, level.time );
	const int avgUs = st.frames ? static_cast<int>( st.frameNs / st.frames / 1000 ) : 0;
	const int maxUs = static_cast<int>( st.maxFrameNs / 1000 );
	const int n = static_cast<int>( cs.members.size() );

	gi.Printf( "CREW: map %s  crew %d  posts %d  t=%d ms\n", level.mapname, n, static_cast<int>( cs.posts.size() ), level.time );
	gi.Printf( "CREW: coverage min %d%% last %d%% over %d sample(s)\n", st.minCoveragePercent, st.lastCoveragePercent, st.samples );
	gi.Printf( "CREW: yields script %d combat %d  handovers %d  preemptions %d  violations %d  restaffs %d\n",
		st.yields[crew::LEVEL_SCRIPT], st.yields[crew::LEVEL_COMBAT], st.handovers, st.preemptions, st.violations, st.restaffs );
	gi.Printf( "CREW: script test: %d run, %d yielded, %d resumed\n", st.scriptRuns, st.scriptYields, st.scriptResumes );
	gi.Printf( "CREW: stuck %d  out-of-world %d  addresses %d  acks %d  missed %d  slowest ack %d ms  bumps %d\n",
		st.stuckEvents, st.outOfWorld, st.addresses, st.acks, st.missedAcks, st.maxAckMs, st.bumps );
	gi.Printf( "CREW: save %d bytes (%d per NPC, budget %d)  layer cost avg %d us max %d us per frame\n",
		static_cast<int>( saveBytes ), n ? static_cast<int>( saveBytes ) / n : 0, cs.cfg.saveBytesPerNpc, avgUs, maxUs );
	for ( const crew::Member &m : cs.members )
	{
		const gentity_t *e = &g_entities[m.ent];
		gi.Printf( "CREW:   %-16s post %-16s level %d action %d arrived %d ms%s%s\n", e->inuse ? NameOf( e ) : "(gone)",
			m.dutyPost == crew::NO_POST ? "(spare)" : cs.posts[m.dutyPost].name.c_str(), m.level, m.action, m.firstArrivalMs,
			( m.flags & crew::MF_FAILED ) ? " FAILED" : "", ( m.flags & crew::MF_UNAVAILABLE ) ? " UNAVAILABLE" : "" );
	}
	for ( const std::string &f : v.failures ) gi.Printf( S_COLOR_YELLOW"CREW: FAIL %s\n", f.c_str() );
	gi.Printf( "CREW: verdict %s\n", v.pass ? "PASS" : "FAIL" );

	if ( !toFile ) return;

	std::string j = "{\n";
	j += Fmt( "  \"map\": \"%s\",\n  \"time_ms\": %d,\n  \"crew\": %d,\n  \"posts\": %d,\n", level.mapname, level.time, n, static_cast<int>( cs.posts.size() ) );
	j += Fmt( "  \"samples\": %d,\n  \"min_coverage_percent\": %d,\n  \"last_coverage_percent\": %d,\n", st.samples, st.minCoveragePercent, st.lastCoveragePercent );
	j += Fmt( "  \"yields_script\": %d,\n  \"yields_combat\": %d,\n  \"preemptions\": %d,\n  \"violations\": %d,\n  \"restaffs\": %d,\n",
		st.yields[crew::LEVEL_SCRIPT], st.yields[crew::LEVEL_COMBAT], st.preemptions, st.violations, st.restaffs );
	j += Fmt( "  \"stuck_events\": %d,\n  \"out_of_world\": %d,\n  \"addresses\": %d,\n  \"acks\": %d,\n  \"missed_acks\": %d,\n  \"max_ack_ms\": %d,\n  \"bumps\": %d,\n",
		st.stuckEvents, st.outOfWorld, st.addresses, st.acks, st.missedAcks, st.maxAckMs, st.bumps );
	j += Fmt( "  \"save_bytes\": %d,\n  \"save_budget_per_npc\": %d,\n  \"layer_avg_us\": %d,\n  \"layer_max_us\": %d,\n",
		static_cast<int>( saveBytes ), cs.cfg.saveBytesPerNpc, avgUs, maxUs );
	j += Fmt( "  \"sample_ms\": %d,\n", cs.cfg.sampleMs );
	j += Fmt( "  \"reach_bound_ms\": %d,\n  \"ack_bound_ms\": %d,\n  \"coverage_bound_percent\": %d,\n", cs.cfg.reachBoundMs, cs.cfg.ackBoundMs, cs.cfg.coveragePercent );
	j += Fmt( "  \"handovers\": %d,\n  \"script_runs\": %d,\n  \"script_yields\": %d,\n  \"script_resumes\": %d,\n",
		st.handovers, st.scriptRuns, st.scriptYields, st.scriptResumes );
	j += GameFrameJson();
	j += "  \"scripts\": " + ScriptCensusJson() + ",\n";
	j += "  \"failures\": [";
	for ( size_t i = 0; i < v.failures.size(); ++i ) j += std::string( i ? ", " : "" ) + "\"" + v.failures[i] + "\"";
	j += "],\n";
	j += std::string( "  \"verdict\": \"" ) + ( v.pass ? "PASS" : "FAIL" ) + "\",\n";
	j += "  \"members\": " + MembersJson( cs.members ) + "\n}\n";
	WriteFile( ".report.json", j );
}

// ---- the measured run -----------------------------------------------------------------------

void Sample( void )
{
	if ( cs.samplingFromMs < 0 )
	{//coverage is sampled once the crew have had their chance to arrive, not while they walk
		bool settled = !cs.members.empty();
		for ( const crew::Member &m : cs.members )
			if ( m.dutyPost != crew::NO_POST && !( m.flags & ( crew::MF_ARRIVED | crew::MF_FAILED | crew::MF_UNAVAILABLE ) ) )
				settled = false;
		if ( !settled && level.time < cs.cfg.reachBoundMs ) return;
		cs.samplingFromMs = level.time;
		cs.nextSampleMs = level.time;
		gi.Printf( "CREW: sampling post coverage from t=%d ms\n", level.time );
	}
	if ( level.time < cs.nextSampleMs ) return;
	cs.nextSampleMs = level.time + cs.cfg.sampleMs;
	crew::RecordSample( cs.st, crew::CoveragePercent( cs.members, AtPostNow() ) );
}

void AddressTest( void )
{//address each member in turn, as the player would, a few seconds apart
	if ( cs.nextAddress < 0 || level.time < cs.nextAddressMs ) return;
	gentity_t *player = &g_entities[0];
	while ( cs.nextAddress < static_cast<int>( cs.members.size() ) )
	{
		gentity_t *e = &g_entities[cs.members[cs.nextAddress++].ent];
		if ( !Eligible( e ) ) continue;
		GEntity_UseFunc( e, player, player );
		cs.nextAddressMs = level.time + ADDRESS_STAGGER_MS;
		return;
	}
	cs.nextAddress = -1;
	cs.addressTestDone = true;
}

// Precedence, exercised for real: run an ICARUS script on a crew member who is holding a post.
// The layer must yield on its next turn and, when the script ends, put the member back on duty --
// "an interruption is a pause, not a reassignment".
void ScriptTest( void )
{
	if ( cs.scriptPhase == 0 )
	{
		cs.scriptPhase = 3;
		for ( size_t i = 0; i < cs.members.size(); ++i )
		{
			gentity_t *e = &g_entities[cs.members[i].ent];
			if ( !Eligible( e ) || cs.members[i].action != crew::ACT_HOLD || !e->sequencer ) continue;
			if ( !ICARUS_RunScript( e, Fmt( "%s/%s", Q3_SCRIPT_DIR, SCRIPT_TEST ).c_str() ) )
			{
				gi.Printf( S_COLOR_YELLOW"CREW: script test skipped: %s/%s is not installed\n", Q3_SCRIPT_DIR, SCRIPT_TEST );
				return;
			}
			++cs.st.scriptRuns;
			cs.scriptMember = static_cast<int>( i );
			cs.scriptStartMs = level.time;
			cs.scriptPhase = 1;
			gi.Printf( "CREW: script test: running %s on %s\n", SCRIPT_TEST, NameOf( e ) );
			return;
		}
		gi.Printf( S_COLOR_YELLOW"CREW: script test skipped: no crew member is holding a post\n" );
		return;
	}

	const crew::Member &m = cs.members[cs.scriptMember];
	const int elapsed = level.time - cs.scriptStartMs;
	if ( cs.scriptPhase == 1 )
	{
		if ( m.level == crew::LEVEL_SCRIPT && m.action == crew::ACT_YIELD )
		{
			++cs.st.scriptYields;
			cs.scriptPhase = 2;
		}
		else if ( elapsed > SCRIPT_YIELD_MS ) cs.scriptPhase = 3;
	}
	else if ( cs.scriptPhase == 2 )
	{
		if ( m.level >= crew::LEVEL_DIRECTOR && m.action == crew::ACT_HOLD )
		{
			++cs.st.scriptResumes;
			cs.scriptPhase = 3;
			gi.Printf( "CREW: script test: yielded, and back on duty %d ms after the script began\n", elapsed );
		}
		else if ( elapsed > SCRIPT_RESUME_MS ) cs.scriptPhase = 3;
	}
}

void RunHarness( void )
{
	if ( g_crewRun->integer <= 0 || cs.runReported || cs.samplingFromMs < 0 ) return;
	const int runMs = g_crewRun->integer * 1000;
	const int elapsed = level.time - cs.samplingFromMs;

	if ( !cs.addressTestDone && cs.nextAddress < 0 && elapsed >= runMs / 2 )
	{
		cs.nextAddress = 0;
		cs.nextAddressMs = level.time;
	}
	if ( cs.addressTestDone && cs.scriptPhase < 3 ) ScriptTest();
	if ( elapsed < runMs || cs.nextAddress >= 0 || cs.scriptPhase < 3 ) return;
	for ( const crew::Member &m : cs.members )
		if ( m.addressedMs >= 0 ) return; //an acknowledgement is still in flight

	cs.runReported = true;
	Report( true );
	gi.Printf( "CREW: run complete\n" );
	if ( g_crewQuit->integer )
	{
		gi.SendConsoleCommand( "save crewrun\n" );
		gi.SendConsoleCommand( "wait 20\n" );
		gi.SendConsoleCommand( "quit\n" );
	}
}

void ApplySave( void )
{
	if ( cs.pendingSave.empty() ) return;
	std::vector<crew::Member> restored;
	const bool ok = crew::Unpack( cs.pendingSave.data(), cs.pendingSave.size(), cs.postHash, restored );
	cs.pendingSave.clear();
	if ( !ok )
	{
		gi.Printf( S_COLOR_YELLOW"CREW: saved crew state does not match this map's posts; reassigning\n" );
		return;
	}

	const int posts = static_cast<int>( cs.posts.size() );
	for ( crew::Member &m : restored )
	{
		if ( m.ent <= 0 || m.ent >= globals.num_entities ) { m.flags |= crew::MF_UNAVAILABLE; m.ent = 0; }
		if ( m.dutyPost >= posts ) m.dutyPost = crew::NO_POST;
		if ( m.overridePost >= posts ) m.overridePost = crew::NO_POST;
		if ( m.goalPost >= posts ) { m.goalPost = crew::NO_POST; m.flags &= ~crew::MF_OWNS_GOAL; }
		m.progressMs = level.time;
	}
	cs.members.swap( restored );
	cs.lostToWorld.assign( cs.members.size(), false );
	gi.Printf( "CREW: restored %d crew from the save\n", static_cast<int>( cs.members.size() ) );
	if ( g_crewRun->integer > 0 )
		WriteFile( ".restored.json", "{\n  \"members\": " + MembersJson( cs.members ) + "\n}\n" );
}

} // namespace

// The environment in the world's public entry points (g_crew.h): the way back out of freefall, and
// where the player is in it. `playerBoots` is the module's own, above.
void Crew_ToggleBoots( void ) { playerBoots = !playerBoots; }
bool Crew_BootsOn( void ) { return playerBoots; }

// How many of the layer's embodied crew are floating on BS_FLY right now: the world's answer to the
// deck's plating, for the report.
int Crew_Floating( void )
{
	int n = 0;
	for ( int idx : shipEmbodied )
	{
		gentity_t *e = FindByName( RosterName( idx ) );
		if ( e && e->NPC && e->NPC->behaviorState == BS_FLY ) ++n;
	}
	return n;
}

// The player's body in the world (Stage B). The frame hook runs it; and the console's `ship operate`
// reaches a system with the player as the operator, so a degraded console lets go at the person
// holding the controls. False means the layer is off or no character is the player, so the caller can
// fall back.
void Crew_PlayerFrame( void ) { Crew_PlayerFrameImpl(); }

bool Crew_PlayerUseSystem( int system )
{
	if ( !g_player || !g_player->integer ) return false;
	ship::Ship *s = Ship_Get();
	if ( !s || system < 0 || system >= ship::SYS_COUNT ) return false;
	if ( s->player < 0 || s->player >= static_cast<int>( s->crew.size() ) ) return false;
	ship::UseSystemBy( *s, static_cast<ship::SystemId>( system ), ship::StressNow( *s ), s->player );
	return true;
}

// The dead are not reloaded: the engine's respawn hook calls this. True only when the extension is
// on and the player is down -- then no reload, and the ship carries on under its own command.
bool LWH_BlockRespawn( gentity_t *ent )
{
	if ( !ent || ent->s.number != 0 ) return false;
	if ( !g_player || !g_player->integer ) return false;
	ship::Ship *s = Ship_Get();
	if ( !s ) return false;
	if ( !ship::PlayerIncapacitated( *s ) ) return false;
	gi.Printf( "PLAYER: no reload; %s's record is closed and the ship carries on\n", s->crew[s->player].name.c_str() );
	return true;
}

// ---- entry points ---------------------------------------------------------------------------

void Crew_RegisterCvars( void )
{
	g_crew = gi.cvar( "g_crew", "0", 0 );
	g_crewRun = gi.cvar( "g_crewRun", "0", 0 );
	g_crewQuit = gi.cvar( "g_crewQuit", "0", 0 );
	g_crewDebug = gi.cvar( "g_crewDebug", "0", 0 );
	g_crewFromShip = gi.cvar( "g_crewFromShip", "0", 0 );
	g_crewDeck = gi.cvar( "g_crewDeck", "0", 0 );
	g_env = gi.cvar( "g_env", "0", 0 );
	g_player = gi.cvar( "g_player", "0", 0 );
}

void Crew_Init( void )
{
	cs = CrewState();
	shipEmbodied.clear();
	shipLeaving.clear();
	intruderBodies.clear();
	trackedStatus = 255;
	playerFelled = false;
	if ( !g_crew || !g_crew->integer )
	{
		cs.baseline = g_crewRun && g_crewRun->integer > 0;
		return;
	}

	// A scenario's crew file declares its own people; the ship's roster replaces it entirely.
	if ( g_crewFromShip->integer ) cs.cfg = crew::Config();
	else LoadConfig();
	cs.active = true;
	gi.Printf( "CREW: direction layer active on %s\n", level.mapname );
	//a full load restores every entity from the save, declared crew included; anything else
	//starts the level from the map, so they are spawned here, with the rest of its entities
	if ( g_eSavedGameJustLoaded != eFULL ) SpawnDeclaredCrew();
}

void Crew_FrameBegin( void )
{
	if ( cs.active || cs.baseline ) cs.frameBegan = std::chrono::steady_clock::now();
}

// The same run with the layer off: no crew, no decisions -- only the clock and the script census,
// written at the moment the crewed run would have reported.
static void BaselineFrame( void )
{
	if ( cs.runReported || level.time < g_crewRun->integer * 1000 ) return;
	cs.runReported = true;
	WriteFile( ".baseline.json", "{\n" + Fmt( "  \"map\": \"%s\",\n  \"time_ms\": %d,\n", level.mapname, level.time )
		+ GameFrameJson() + "  \"scripts\": " + ScriptCensusJson() + "\n}\n" );
	gi.Printf( "CREW: baseline run complete\n" );
	if ( g_crewQuit->integer )
	{
		gi.SendConsoleCommand( "save crewbase\n" );
		gi.SendConsoleCommand( "wait 20\n" );
		gi.SendConsoleCommand( "quit\n" );
	}
}

void Crew_Frame( void )
{
	Crew_EnvFrame();   // the ship's environment in the world: gravity per person, a breach, a field
	Crew_PlayerFrame(); // the player's body in the world: the causes reach the person (Stage B)
	if ( cs.active || cs.baseline )
	{//everything the game did this frame before the layer's own turn
		const int64_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>( std::chrono::steady_clock::now() - cs.frameBegan ).count();
		cs.gameNs += ns;
		cs.maxGameNs = std::max( cs.maxGameNs, ns );
		++cs.gameFrames;
	}
	if ( cs.baseline ) BaselineFrame();
	if ( !cs.active ) return;
	const auto t0 = std::chrono::steady_clock::now();

	bool harness = false;
	if ( !cs.ready ) Ready();
	if ( level.time >= cs.nextScanMs )
	{
		cs.nextScanMs = level.time + ROSTER_SCAN_MS;
		if ( g_crewFromShip->integer ) { SyncShipRoster(); SyncIntruders(); SyncDamage(); SyncFire(); }
		ScanRoster();
	}
	//Decisions are taken every frame, after every entity has thought. A script that takes an NPC
	//during a frame therefore finds the layer's goal gone before that NPC next thinks: the
	//hand-over costs no think in which the NPC walks for the layer while a script owns it.
	for ( size_t i = 0; i < cs.members.size(); ++i ) ThinkMember( i );
	if ( level.time >= cs.nextThinkMs )
	{
		cs.nextThinkMs = level.time + HARNESS_MS;
		Sample();
		harness = true;
	}
	if ( level.time >= cs.nextDirectorMs )
	{
		cs.nextDirectorMs = level.time + DIRECTOR_MS;
		std::vector<bool> scripted( cs.members.size() );
		for ( size_t i = 0; i < cs.members.size(); ++i ) scripted[i] = cs.members[i].level == crew::LEVEL_SCRIPT;
		// A place given to one person by the ship's roster is theirs, not a post someone else should
		// be sent to cover; the director's restaffing applies to declared posts only.
		if ( !g_crewFromShip->integer ) cs.st.restaffs += crew::Restaff( cs.members, cs.posts, scripted );
	}

	const int64_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>( std::chrono::steady_clock::now() - t0 ).count();
	cs.st.frameNs += ns;
	cs.st.maxFrameNs = std::max( cs.st.maxFrameNs, ns );
	++cs.st.frames;

	//the measurement harness is not the layer: its cost stays out of the layer's own figure
	if ( harness )
	{
		AddressTest();
		RunHarness();
	}
}

void SpeakFromMemory( gentity_t *self ); // defined with the response code, below

// The player used an NPC. Counted as an address only when the game's own rules say a generic
// response is due -- the same tests NPC_Use and NPC_UseResponse apply -- so an NPC that is
// rightly silent (mid-script, mid-sentence, hostile) is not scored as having ignored the player.
void Crew_OnUsed( gentity_t *self, gentity_t *user )
{
	if ( !user || user->s.number != 0 || !self->NPC || !self->client || !user->client ) return;
	if ( self->enemy || gi.S_Override[self->s.number] ) return;
	if ( ( self->NPC->scriptFlags & SCF_NO_RESPONSE ) || self->NPC->blockedSpeechDebounceTime > level.time ) return;
	if ( self->client->playerTeam != user->client->playerTeam ) return;

	// The player has addressed them: they answer from what they remember, whichever way the game's
	// own use behaviour then goes. (Ship-embodied crew are not the layer's own members, so this runs
	// before the member lookup; the acknowledgement bookkeeping needs a member and follows.)
	SpeakFromMemory( self );

	const int i = MemberIndex( self );
	if ( i < 0 ) return;
	if ( self->behaviorSet[BSET_USE] ) return;
	if ( cs.members[i].addressedMs >= 0 ) return;
	cs.members[i].addressedMs = level.time;
	++cs.st.addresses;
}

// The crew member's account, drawn from their strongest mark -- what they say when the player
// addresses them. This is the memory model reaching dialogue, not only the log: the line is theirs.
std::string AccountLine( const ship::Ship &s, const ship::CrewMember &c )
{
	const ship::Memory *best = NULL;
	for ( const ship::Memory &mk : c.memories )
		if ( mk.event != 0 && ( !best || mk.salience > best->salience ) ) best = &mk;
	if ( !best ) return "All quiet, Captain.";
	const bool about = best->person >= 0 && best->person < static_cast<int>( s.crew.size() );
	const char *who = about ? s.crew[best->person].name.c_str() : "them";
	switch ( best->event )
	{
	case ship::MEM_DEATH:     return Fmt( "We lost %s. I still think about it.", who );
	case ship::MEM_ORDER:     return "Your order still stands.";
	case ship::MEM_PROMISE:   return "You gave me your word. I am holding you to it.";
	case ship::MEM_LIE:       return "You told me one thing and did another.";
	case ship::MEM_RESCUE:    return "You came back for us. I have not forgotten.";
	case ship::MEM_VIOLATION: return "We crossed a line out there. People will remember.";
	}
	return "All quiet, Captain.";
}

void SpeakFromMemory( gentity_t *self )
{
	ship::Ship *s = Ship_Get();
	if ( !s || !self->fullName ) return;
	for ( size_t k = 0; k < s->crew.size(); ++k )
		if ( s->crew[k].name == self->fullName )
		{
			gi.Printf( "CREW: %s says: \"%s\"\n", s->crew[k].name.c_str(), AccountLine( *s, s->crew[k] ).c_str() );
			return;
		}
}

void Crew_OnResponded( gentity_t *self )
{
	const int i = MemberIndex( self );
	if ( i < 0 || cs.members[i].addressedMs < 0 ) return;
	const int ms = level.time - cs.members[i].addressedMs;
	cs.members[i].addressedMs = -1;
	++cs.st.acks;
	cs.st.maxAckMs = std::max( cs.st.maxAckMs, ms );
}

// Walked into by the player: say something, and look at them. Only when the NPC is the layer's to
// direct, and not more often than a person would.
void Crew_OnTouched( gentity_t *self, gentity_t *other )
{
	const int i = MemberIndex( self );
	if ( i < 0 || !other || other->s.number != 0 || other->health <= 0 || !self->NPC ) return;
	crew::Member &m = cs.members[i];
	if ( level.time < m.nextBumpMs || ScriptOwns( self ) ) return;
	if ( self->enemy || gi.S_Override[self->s.number] || ( self->NPC->scriptFlags & SCF_NO_RESPONSE ) ) return;

	m.nextBumpMs = level.time + BUMP_DEBOUNCE_MS;
	++cs.st.bumps;
	NPC_Respond( self, 0 );
	NPC_SetLookTarget( self, 0, level.time + 2000 );
}

// One chunk, written only while the layer is active: with g_crew 0 a save is byte-for-byte what
// it was before this file existed.
void Crew_WriteSave( void )
{
	if ( !cs.active || !cs.ready ) return; //nothing to record before the first frame
	std::vector<uint8_t> blob = crew::Pack( cs.members, cs.postHash );
	gi.AppendToSaveGame( SAVE_CHUNK, blob.data(), static_cast<int>( blob.size() ) );
	cs.lastSaveBytes = blob.size() + CHUNK_OVERHEAD;
	if ( g_crewRun->integer > 0 )
		WriteFile( ".saved.json", "{\n  \"members\": " + MembersJson( cs.members ) + "\n}\n" );
}

// Always attempted, active or not: a save written with the layer on must still load with it off,
// and an unread chunk would desynchronise everything after it. The record is applied on the
// first frame, once the posts it refers to have been resolved.
void Crew_ReadSave( void )
{
	void *data = NULL;
	const int len = gi.ReadFromSaveGameOptional( SAVE_CHUNK, NULL, 0, &data );
	if ( len > 0 && data && cs.active )
	{
		const uint8_t *bytes = static_cast<const uint8_t *>( data );
		cs.pendingSave.assign( bytes, bytes + len );
	}
	if ( data ) gi.Free( data );
}

void Svcmd_Crew_f( void )
{
	if ( !cs.active )
	{
		gi.Printf( "crew: the direction layer is off (set g_crew 1 and load a map)\n" );
		return;
	}
	const char *cmd = gi.argc() > 1 ? gi.argv( 1 ) : "report";
	if ( !Q_stricmp( cmd, "report" ) ) Report( false );
	else if ( !Q_stricmp( cmd, "write" ) ) Report( true );
	else if ( !Q_stricmp( cmd, "address" ) ) { cs.nextAddress = 0; cs.nextAddressMs = level.time; }
	else if ( !Q_stricmp( cmd, "posts" ) )
	{
		for ( size_t i = 0; i < cs.posts.size(); ++i )
			gi.Printf( "CREW: post %2d %-20s at %s yaw %.0f radius %d\n", static_cast<int>( i ), cs.posts[i].name.c_str(),
				vtos( cs.posts[i].origin ), cs.posts[i].yaw, cs.posts[i].radius );
	}
	else gi.Printf( "usage: crew [report|write|address|posts]\n" );
}
