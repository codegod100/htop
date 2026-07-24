/*
htop - ColorsPanel.c
(C) 2004-2011 Hisham H. Muhammad
(C) 2026 htop contributors
Released under the GNU GPLv2+, see the COPYING file
in the source distribution for its full text.
*/

#include "config.h" // IWYU pragma: keep

#include "ColorsPanel.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "CRT.h"
#include "FunctionBar.h"
#include "Object.h"
#include "OptionItem.h"
#include "ProvideCurses.h"
#include "Theme.h"
#include "XUtils.h"


// Built-in schemes (must match ColorScheme enum order in CRT.h).
static const char* const ColorSchemeNames[] = {
   "Default",
   "Monochromatic",
   "Black on White",
   "Light Terminal",
   "MC",
   "Black Night",
   "Broken Gray",
   "Nord",
   NULL
};

static const char* const ColorsFunctions[] = {"      ", "      ", "      ", "      ", "      ", "      ", "      ", "      ", "      ", "Done  ", NULL};

static void ColorsPanel_delete(Object* object) {
   ColorsPanel* this = (ColorsPanel*) object;
   Theme_listDelete(this->themes);
   free(this->itemKinds);
   Panel_done(&this->super);
   free(this);
}

static void ColorsPanel_clearChecks(Panel* super) {
   for (int i = 0; i < Panel_size(super); i++) {
      Object* obj = Panel_get(super, i);
      if (OptionItem_kind((OptionItem*) obj) == OPTION_ITEM_CHECK)
         CheckItem_set((CheckItem*) obj, false);
   }
}

static HandlerResult ColorsPanel_eventHandler(Panel* super, int ch) {
   ColorsPanel* this = (ColorsPanel*) super;

   HandlerResult result = IGNORED;

   switch (ch) {
      case 0x0a:
      case 0x0d:
      case KEY_ENTER:
      case KEY_MOUSE:
      case KEY_RECLICK:
      case ' ': {
         int mark = Panel_getSelectedIndex(super);
         if (mark < 0 || mark >= this->nItems)
            break;

         int kind = this->itemKinds[mark];
         if (kind < 0)
            break; /* section header */

         ColorsPanel_clearChecks(super);
         CheckItem_set((CheckItem*)Panel_get(super, mark), true);

         if (kind < LAST_COLORSCHEME) {
            free(this->settings->themeName);
            this->settings->themeName = NULL;
            this->settings->colorScheme = kind;
            CRT_setColors(kind);
         } else {
            size_t themeIndex = (size_t)(kind - LAST_COLORSCHEME);
            assert(this->themes);
            assert(themeIndex < this->themes->count);
            const ThemeInfo* info = &this->themes->items[themeIndex];
            free_and_xStrdup(&this->settings->themeName, info->id);
            if (!CRT_setThemeById(info->id)) {
               /* Fall back to default if the file vanished. */
               free(this->settings->themeName);
               this->settings->themeName = NULL;
               this->settings->colorScheme = COLORSCHEME_DEFAULT;
               CRT_setColors(COLORSCHEME_DEFAULT);
            }
         }

         this->settings->changed = true;
         this->settings->lastUpdate++;
         clear();
         result = HANDLED | REDRAW;
      }
   }

   return result;
}

const PanelClass ColorsPanel_class = {
   .super = {
      .extends = Class(Panel),
      .delete = ColorsPanel_delete
   },
   .eventHandler = ColorsPanel_eventHandler
};

ColorsPanel* ColorsPanel_new(Settings* settings) {
   ColorsPanel* this = AllocThis(ColorsPanel);
   Panel* super = &this->super;

   FunctionBar* fuBar = FunctionBar_new(ColorsFunctions, NULL, NULL);
   Panel_init(super, 1, 1, 1, 1, Class(OptionItem), true, fuBar);

   this->settings = settings;
   this->themes = Theme_scan();

   assert(ARRAYSIZE(ColorSchemeNames) == LAST_COLORSCHEME + 1);

   size_t nBuiltins = LAST_COLORSCHEME;
   size_t nThemes = this->themes ? this->themes->count : 0;
   /* builtins + optional "File themes" header + themes */
   size_t nItems = nBuiltins + (nThemes ? 1 + nThemes : 0);
   this->itemKinds = xMallocArray(nItems, sizeof(int));
   this->nItems = 0;

   Panel_setHeader(super, "Colors");

   int selectedIndex = 0;
   bool haveSelection = false;

   for (size_t i = 0; i < nBuiltins; i++) {
      bool selected = !settings->themeName && ((int)i == (int)CRT_colorScheme);
      Panel_add(super, (Object*) CheckItem_newByVal(ColorSchemeNames[i], selected));
      this->itemKinds[this->nItems] = (int) i;
      if (selected) {
         selectedIndex = this->nItems;
         haveSelection = true;
      }
      this->nItems++;
   }

   if (nThemes > 0) {
      Panel_add(super, (Object*) TextItem_new("--- File themes ---"));
      this->itemKinds[this->nItems] = -1;
      this->nItems++;

      for (size_t i = 0; i < nThemes; i++) {
         const ThemeInfo* info = &this->themes->items[i];
         bool selected = settings->themeName && String_eq(settings->themeName, info->id);
         char label[256];
         xSnprintf(label, sizeof(label), "%s", info->name);
         Panel_add(super, (Object*) CheckItem_newByVal(label, selected));
         this->itemKinds[this->nItems] = LAST_COLORSCHEME + (int) i;
         if (selected) {
            selectedIndex = this->nItems;
            haveSelection = true;
         }
         this->nItems++;
      }
   }

   if (!haveSelection && Panel_size(super) > 0)
      CheckItem_set((CheckItem*)Panel_get(super, 0), true);

   Panel_setSelected(super, selectedIndex);
   return this;
}
