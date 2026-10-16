// lwh_panel.h -- the status panel's live surface (S4's "glance", second half).
//
// The ship's state is drawn into a small RGBA image and handed to the renderer, which overwrites
// the image a panel shader uses (gi.UpdatePanelImage). Inert unless the ship simulation is running
// and g_shipPanel is on.

#ifndef LWH_PANEL_H
#define LWH_PANEL_H

namespace ship { struct Ship; }

// Called each game frame while the ship simulation runs. Draws the ship's state onto the panel.
void LWH_Panel_Frame( const ship::Ship *s );

// The panel list (O10): the shaders whose surfaces carry the ship's own readout. One list, read by
// the remap above, by the deck tooling that marks the panel brushes
// (tools/shipmap/markpanels.py), and by the acceptance check
// (scripts/panel-control-check.sh). See docs/evidence/the-panel-control-built.md.
int LWH_PanelShaderCount( void );
const char *LWH_PanelShader( int i );          // the i-th shader in the list, or ""
const char *LWH_PanelReadoutShader( void );     // the shader the readout is drawn into
const char *LWH_PanelTag( void );               // the targetname prefix the tooling marks with

#endif
