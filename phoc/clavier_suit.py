#!/usr/bin/env python3
# Patch phoc 0.47 (Surface Duo) : le clavier visuel suit l'écran de l'appli utilisée.
#  1. à chaque changement de fenêtre active, phoc écrit le nom de son écran dans /run/duo/ecran-actif
#     (duo-posture relance squeekboard quand cet écran change : squeekboard garde sa surface là où elle est née) ;
#  2. à la création de la surface du clavier (« osk »), sans écran imposé (mode portable), elle va sur l'écran
#     de la fenêtre active au lieu de celui du pointeur (toujours l'écran de gauche quand on n'a que le tactile).
import sys, os

src = sys.argv[1]
MARK = "DUO-CLAVIER-SUIT"


def patch(path, old, new):
    s = open(path).read()
    if MARK in s:
        print(path, ": deja patche"); return
    assert s.count(old) == 1, (path, s.count(old))
    open(path, "w").write(s.replace(old, new))
    print(path, ": patche")


patch(os.path.join(src, "seat.c"),
      """  g_debug ("Trying to focus view %p", view);
  if (view && !phoc_seat_allow_input (seat, view->wlr_surface->resource))
    return;
""",
      """  g_debug ("Trying to focus view %p", view);
  if (view && !phoc_seat_allow_input (seat, view->wlr_surface->resource))
    return;

  /* DUO-CLAVIER-SUIT : écran de la fenêtre active -> /run/duo/ecran-actif (lu par duo-posture) */
  if (view) {
    static char dernier[32];
    PhocOutput *o = phoc_view_get_output (view);
    if (o && o->wlr_output && g_strcmp0 (dernier, o->wlr_output->name) != 0) {
      g_strlcpy (dernier, o->wlr_output->name, sizeof dernier);
      g_file_set_contents ("/run/duo/ecran-actif", dernier, -1, NULL);
    }
  }
""")

patch(os.path.join(src, "layer-shell.c"),
      """  if (!wlr_layer_surface->output) {
    PhocSeat *seat = phoc_server_get_last_active_seat (phoc_server_get_default ());
    g_assert (PHOC_IS_SEAT (seat)); /* Technically speaking we should handle this case */""",
      """  /* DUO-CLAVIER-SUIT : sinon le clavier va sur l'écran de la fenêtre active */
  if (g_strcmp0 (wlr_layer_surface->namespace, "osk") == 0 && !g_file_test ("/run/duo/clavier-ecran", G_FILE_TEST_EXISTS)) {
    PhocSeat *s = phoc_server_get_last_active_seat (phoc_server_get_default ());
    PhocView *v = s ? phoc_seat_get_focus_view (s) : NULL;
    PhocOutput *o = v ? phoc_view_get_output (v) : NULL;
    if (o && o->wlr_output && o->wlr_output->enabled)
      wlr_layer_surface->output = o->wlr_output;
  }

  if (!wlr_layer_surface->output) {
    PhocSeat *seat = phoc_server_get_last_active_seat (phoc_server_get_default ());
    g_assert (PHOC_IS_SEAT (seat)); /* Technically speaking we should handle this case */""")
