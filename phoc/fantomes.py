#!/usr/bin/env python3
# Patch phoc (Surface Duo) : oubli des « doigts fantômes ». Si des contacts restent enregistrés sans aucun événement
# tactile depuis 1,5 s (fin de contact perdue, ex. collision d'identifiants du 12/11), un nouveau toucher les efface ;
# sinon le filtre « 3 doigts = geste système » (DUO-GESTES) pouvait bloquer tout le tactile des applis.
import sys, os
p = os.path.join(sys.argv[1], "cursor.c"); s = open(p).read()
MARK = "DUO-FANTOMES"
if MARK in s:
    print(p, ": deja patche"); sys.exit(0)
old = """  touch_point = phoc_cursor_add_touch_point (self, event);
  lx = touch_point->lx;"""
new = """  /* DUO-FANTOMES : contacts restés sans événement depuis 1,5 s -> oubliés */
  {
    gint64 maintenant = g_get_monotonic_time ();
    if (g_hash_table_size (priv->touch_points) > 0 && maintenant - duo_dernier_toucher > 1500000) {
      g_hash_table_remove_all (priv->touch_points);
      duo_touch_avale = FALSE;
      seat->touch_id = -1;
    }
    duo_dernier_toucher = maintenant;
  }
  touch_point = phoc_cursor_add_touch_point (self, event);
  lx = touch_point->lx;"""
assert s.count(old) == 1
s = s.replace(old, new)
s = s.replace("static gboolean duo_touch_avale = FALSE;", "static gboolean duo_touch_avale = FALSE;\nstatic gint64 duo_dernier_toucher = 0;   /* DUO-FANTOMES */", 1)
old2 = """phoc_cursor_handle_touch_motion (PhocCursor                    *self,
                                 struct wlr_touch_motion_event *event)
{"""
assert s.count(old2) == 1
s = s.replace(old2, old2 + "\n  duo_dernier_toucher = g_get_monotonic_time ();   /* DUO-FANTOMES */", 1)
old3 = """  phoc_cursor_remove_touch_point (self, event->touch_id);
  if (g_hash_table_size (priv->touch_points) == 0)"""
assert s.count(old3) == 1
s = s.replace(old3, "  duo_dernier_toucher = g_get_monotonic_time ();   /* DUO-FANTOMES */\n" + old3, 1)
open(p, "w").write(s); print(p, ": patche")
