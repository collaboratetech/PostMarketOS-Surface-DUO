#!/usr/bin/env python3
# Patch phoc 0.47 : placement des menus (xdg_popup). phoc trouve l'écran de la fenêtre (coin haut-gauche) puis
# l'écrase sans condition par « le premier écran touché par la surface » -> sur le Surface Duo, le menu « + » de
# Messages (fenêtre à gauche) était contraint dans l'écran de droite. On ne prend ce repli que s'il n'y a rien.
import sys, os

p = os.path.join(sys.argv[1], "view.c")
s = open(p).read()
MARK = "DUO-MENU-ECRAN"
if MARK in s:
    print(p, ": déjà patché"); sys.exit(0)
old = """  /* Otherwise just take the first output */
  surface_output = wl_container_of (self->wlr_surface->current_outputs.next, surface_output, link);
  output = PHOC_OUTPUT (surface_output->output->data);
  g_assert (PHOC_IS_OUTPUT (output));
"""
new = """  /* Otherwise just take the first output (DUO-MENU-ECRAN : seulement si le coin n'a rien donné) */
  if (!output) {
    surface_output = wl_container_of (self->wlr_surface->current_outputs.next, surface_output, link);
    output = PHOC_OUTPUT (surface_output->output->data);
    g_assert (PHOC_IS_OUTPUT (output));
  }
"""
assert s.count(old) == 1, s.count(old)
s = s.replace(old, new)
open(p, "w").write(s)
print(p, ": patché")
