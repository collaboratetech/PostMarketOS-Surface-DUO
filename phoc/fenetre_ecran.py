#!/usr/bin/env python3
# Patch phoc 0.47 (Surface Duo) : écran d'ouverture des fenêtres.
# phoc place une nouvelle fenêtre (ou boîte de dialogue) sur l'écran du pointeur de souris, qui reste sur
# l'écran de gauche quand on n'utilise que le tactile : « Nouveau message… » de Messages, ouvert depuis l'écran
# de droite, apparaissait à gauche. Maintenant :
#   - une fenêtre qui a un parent (dialogue) s'ouvre sur l'écran de son parent ;
#   - sinon, sur l'écran touché en dernier.
# Usage : python3 fenetre_ecran.py <src de phoc>   (idempotent)
import sys, os

p = os.path.join(sys.argv[1], "view.c")
s = open(p).read()
MARK = "DUO-ECRAN-OUVERTURE"
if MARK in s:
    print(p, ": déjà patché"); sys.exit(0)
old = """  cursor = phoc_seat_get_cursor (seat);

  if (!output) {"""
new = """  cursor = phoc_seat_get_cursor (seat);

  /* DUO-ECRAN-OUVERTURE : dialogue -> écran du parent ; sinon écran touché en dernier */
  if (!output && view->parent)
    output = phoc_view_get_output (view->parent);
  if (!output && (seat->touch_x != 0 || seat->touch_y != 0)) {
    struct wlr_output *touche = wlr_output_layout_output_at (desktop->layout, seat->touch_x, seat->touch_y);
    if (touche)
      output = PHOC_OUTPUT (touche->data);
  }

  if (!output) {"""
assert s.count(old) == 1, s.count(old)
s = s.replace(old, new)
# fonction publique : écran d'ouverture d'une nouvelle fenêtre (utilisée à la 1re apparition, xdg-toplevel.c)
s = s.replace("""bool
phoc_view_move_to_next_output (PhocView *view, enum wlr_direction direction)
{""", """PhocOutput *phoc_view_duo_sortie_ouverture (PhocView *view);

PhocOutput *
phoc_view_duo_sortie_ouverture (PhocView *view)
{
  PhocDesktop *desktop = phoc_server_get_desktop (phoc_server_get_default ());
  PhocInput *input = phoc_server_get_input (phoc_server_get_default ());
  PhocSeat *seat = phoc_input_get_last_active_seat (input);

  if (view->parent)
    return phoc_view_get_output (view->parent);
  if (seat && (seat->touch_x != 0 || seat->touch_y != 0)) {
    struct wlr_output *touche = wlr_output_layout_output_at (desktop->layout, seat->touch_x, seat->touch_y);
    if (touche)
      return PHOC_OUTPUT (touche->data);
  }
  return NULL;
}

bool
phoc_view_move_to_next_output (PhocView *view, enum wlr_direction direction)
{""", 1)
open(p, "w").write(s)
print(p, ": patché")

t = os.path.join(sys.argv[1], "xdg-toplevel.c")
x = open(t).read()
if MARK not in x:
    old = """    if (self->xdg_toplevel->requested.maximized)
      phoc_view_maximize (PHOC_VIEW (self), NULL);
"""
    new = """    /* DUO-ECRAN-OUVERTURE : agrandie sur l'écran du parent ou l'écran touché en dernier */
    PhocOutput *duo_sortie = phoc_view_duo_sortie_ouverture (PHOC_VIEW (self));
    if (self->xdg_toplevel->requested.maximized)
      phoc_view_maximize (PHOC_VIEW (self), duo_sortie);
"""
    assert x.count(old) == 1
    x = x.replace(old, new)
    old2 = """    phoc_view_auto_maximize (PHOC_VIEW (self));
    view_set_title (PHOC_VIEW (self), self->xdg_toplevel->title);"""
    new2 = """    if (phoc_view_want_auto_maximize (PHOC_VIEW (self)))
      phoc_view_maximize (PHOC_VIEW (self), duo_sortie);
    view_set_title (PHOC_VIEW (self), self->xdg_toplevel->title);"""
    assert x.count(old2) == 1
    x = x.replace(old2, new2)
    x = x.replace('#include "xdg-toplevel-decoration.h"', '#include "xdg-toplevel-decoration.h"\nPhocOutput *phoc_view_duo_sortie_ouverture (PhocView *view);', 1)
    open(t, "w").write(x)
    print(t, ": patché")
