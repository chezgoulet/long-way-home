// Long Way Home: the Borg asset-replacement shader (S8's first hard problem).
//
// A surface pointed at textures/lwh/borg renders Borg-green: the same image, a constant green tint.
// The module (module/ship/g_ship.cpp, BorgAssets) remaps a deck's shaders to this one when the deck
// is assimilated and remaps them back when it is stripped. No new art: the tint is the shader's.
//
// Deck-unique aliases: the five generated decks name their own wall/floor/ceiling shaders so a remap
// can address one deck and not the whole ship. The published decks cannot be addressed this way
// without a stitcher pass (see docs/evidence/s8-the-borg.md).

textures/lwh/borg
{
	qer_editorimage textures/hall/hallfloor1
	{
		map textures/hall/hallfloor1
		rgbGen const ( 0.20 0.55 0.28 )
	}
}

// Each generated deck is split into four sections (west wall, east wall, ends/ceiling, floor) so
// the Borg can take it part by part; BorgAssets turns them in order as assimilation rises.
textures/lwh/deck06wall0  { { map textures/hall/hallcomp2 } }
textures/lwh/deck06wall1  { { map textures/hall/hallcomp2 } }
textures/lwh/deck06wall2  { { map textures/hall/hallcomp2 } }
textures/lwh/deck06floor  { { map textures/hall/hallfloor1 } }
textures/lwh/deck07wall0  { { map textures/hall/hallcomp2 } }
textures/lwh/deck07wall1  { { map textures/hall/hallcomp2 } }
textures/lwh/deck07wall2  { { map textures/hall/hallcomp2 } }
textures/lwh/deck07floor  { { map textures/hall/hallfloor1 } }
textures/lwh/deck12wall0  { { map textures/hall/hallcomp2 } }
textures/lwh/deck12wall1  { { map textures/hall/hallcomp2 } }
textures/lwh/deck12wall2  { { map textures/hall/hallcomp2 } }
textures/lwh/deck12floor  { { map textures/hall/hallfloor1 } }
textures/lwh/deck13wall0  { { map textures/hall/hallcomp2 } }
textures/lwh/deck13wall1  { { map textures/hall/hallcomp2 } }
textures/lwh/deck13wall2  { { map textures/hall/hallcomp2 } }
textures/lwh/deck13floor  { { map textures/hall/hallfloor1 } }
textures/lwh/deck14wall0  { { map textures/hall/hallcomp2 } }
textures/lwh/deck14wall1  { { map textures/hall/hallcomp2 } }
textures/lwh/deck14wall2  { { map textures/hall/hallcomp2 } }
textures/lwh/deck14floor  { { map textures/hall/hallfloor1 } }
