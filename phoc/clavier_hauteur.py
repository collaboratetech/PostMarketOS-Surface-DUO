#!/usr/bin/env python3
"""Patch phoc n°2 : en mode portable, hauteur du clavier visuel imposée.
/run/duo/clavier-ecran contient « ECRAN [HAUTEUR] » (ex. « DSI-2 615 ») : le clavier (namespace osk) affiché sur
ECRAN reçoit HAUTEUR points logiques, collé en bas (squeekboard plafonne sinon à ~2/3 de l'écran).
Corrige aussi le patch n°1 pour ne lire que le premier mot (le nom de l'écran)."""
import sys

f = sys.argv[1]
s = open(f).read()
if "clavier-hauteur" in s:
    print("déjà patché")
    sys.exit(0)

# n°1 : ne garder que le premier mot
vieux = "      g_strstrip (voulu);\n"
assert vieux in s, "patch n°1 absent"
s = s.replace(vieux, "      g_strstrip (voulu);\n      voulu[strcspn (voulu, \" \\t\")] = '\\0';\n", 1)

# n°2 : dans arrange_layer, juste avant le calcul des marges
ancre = "    /* Margin */\n    if ((state->anchor & both_horiz) == both_horiz) {"
assert ancre in s, "ancre arrange_layer introuvable"
ajout = """    /* Surface Duo (clavier-hauteur) : hauteur du clavier imposée en mode portable */
    if (g_strcmp0 (wlr_layer_surface->namespace, "osk") == 0) {
      g_autofree char *spec = NULL;
      if (g_file_get_contents ("/run/duo/clavier-ecran", &spec, NULL, NULL)) {
        g_auto (GStrv) mots = g_strsplit_set (g_strstrip (spec), " \\t", 2);
        if (mots[0] && mots[1] && g_strcmp0 (mots[0], output->wlr_output->name) == 0) {
          int h = atoi (mots[1]);
          if (h > 0 && h <= full_area.height) {
            box.height = h;
            box.y = full_area.y + full_area.height - h;
          }
        }
      }
    }
"""
s = s.replace(ancre, ajout + ancre, 1)
if "#include <string.h>" not in s:
    s = "#include <string.h>\n#include <stdlib.h>\n" + s
open(f, "w").write(s)
print("PATCH_HAUTEUR_OK")
