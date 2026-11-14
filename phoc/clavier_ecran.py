#!/usr/bin/env python3
"""Patch phoc : écran du clavier visuel imposé par /run/duo/clavier-ecran (ex. « DSI-2 »).
Sans ce fichier, comportement d'origine (écran touché en dernier). Écrit par duo-posture en posture « portable »
(Duo ouvert en L, écran B posé à plat) pour mettre le clavier sur l'écran du bas."""
import sys

f = sys.argv[1]
s = open(f).read()
if "clavier-ecran" in s:
    print("déjà patché")
    sys.exit(0)
ancre = "  if (!wlr_layer_surface->output) {\n    PhocSeat *seat = phoc_server_get_last_active_seat"
assert ancre in s, "ancre introuvable"
ajout = """  /* Surface Duo : écran du clavier visuel imposé (posture « portable ») */
  if (g_strcmp0 (wlr_layer_surface->namespace, "osk") == 0) {
    g_autofree char *voulu = NULL;
    if (g_file_get_contents ("/run/duo/clavier-ecran", &voulu, NULL, NULL)) {
      struct wlr_output_layout_output *lo;
      g_strstrip (voulu);
      wl_list_for_each (lo, &desktop->layout->outputs, link) {
        if (g_strcmp0 (lo->output->name, voulu) == 0 && lo->output->enabled) {
          g_debug ("osk imposé sur %s", voulu);
          wlr_layer_surface->output = lo->output;
          break;
        }
      }
    }
  }

"""
s = s.replace(ancre, ajout + ancre, 1)
open(f, "w").write(s)
print("PATCH_CLAVIER_OK")
