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

#endif
