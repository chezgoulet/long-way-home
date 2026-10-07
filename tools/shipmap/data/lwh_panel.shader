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
