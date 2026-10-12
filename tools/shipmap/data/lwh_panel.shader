// Long Way Home: the shader a status panel names. Its base image is replaced at runtime with live
// RGBA by the ship module (gi.UpdatePanelImage); this placeholder keeps it a real, loadable shader
// so the map's surfaces bind to it. Fullbright: a screen is not lit by the room.
//
// World surfaces are looked up as "textures/<name>", so the name carries that prefix.
textures/lwh/panel
{
	nomipmaps
	nopicmip
	{
		map gfx/lwh/panel.tga
		rgbGen identity
	}
}

// The deck-12 environmental readout (G18, docs/evidence/the-deck12-readout.md). A second live
// surface, and a separate one on purpose: it shows *this deck's* air, drawn where the problem is,
// not the ship-wide status above. Its own image, so the two panels are independent. Placed on the
// watch console in environmental control; same 128-wide image, half the height (128x64).
textures/lwh/env
{
	nomipmaps
	nopicmip
	{
		map gfx/lwh/env.tga
		rgbGen identity
	}
}
