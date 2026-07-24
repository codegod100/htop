#ifndef HEADER_ColorsPanel
#define HEADER_ColorsPanel
/*
htop - ColorsPanel.h
(C) 2004-2011 Hisham H. Muhammad
(C) 2026 htop contributors
Released under the GNU GPLv2+, see the COPYING file
in the source distribution for its full text.
*/

#include "Panel.h"
#include "Settings.h"
#include "Theme.h"


typedef struct ColorsPanel_ {
   Panel super;

   Settings* settings;
   ThemeList* themes;
   int* itemKinds; /* ColorScheme index, LAST_COLORSCHEME+themeIndex, or -1 for headers */
   int nItems;
} ColorsPanel;

extern const PanelClass ColorsPanel_class;

ColorsPanel* ColorsPanel_new(Settings* settings);

#endif
