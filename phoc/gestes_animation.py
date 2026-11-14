#!/usr/bin/env python3
# Patch phoc 0.47 (Surface Duo) :
#  1. 3 doigts ou plus posés = geste système (lisgd : fenêtre sur l'autre écran) -> le toucher est annulé chez
#     les applis (wl_touch.cancel) et plus rien ne leur est envoyé jusqu'au lever de tous les doigts
#     (sinon Firefox prenait les 3 doigts pour un zoom).
#  2. « Déplacer sur l'écran voisin » : la fenêtre glisse d'un écran à l'autre (animation ~280 ms) au lieu de sauter.
# Usage : python3 gestes_animation.py <src de phoc>   (idempotent)
import sys, os

src = sys.argv[1]
MARK = "DUO-GESTES"


def patch(path, edits):
    s = open(path).read()
    if MARK in s:
        print(path, ": déjà patché"); return
    for old, new in edits:
        assert s.count(old) == 1, (path, old[:60], s.count(old))
        s = s.replace(old, new)
    open(path, "w").write(s)
    print(path, ": patché")


cursor = os.path.join(src, "cursor.c")
patch(cursor, [
    ("""static PhocTouchPoint *
phoc_cursor_add_touch_point (PhocCursor *self, struct wlr_touch_down_event *event)""",
     """/* DUO-GESTES : 3 doigts ou plus = geste système, les applis ne reçoivent plus rien */
static gboolean duo_touch_avale = FALSE;

static void send_touch_cancel (PhocSeat *seat, struct wlr_surface *surface);

static void
duo_annuler_toucher_clients (PhocSeat *seat)
{
  for (int i = 0; i < 10 && !wl_list_empty (&seat->seat->touch_state.touch_points); i++) {
    struct wlr_touch_point *tp = wl_container_of (seat->seat->touch_state.touch_points.next, tp, link);
    if (!tp->surface)
      break;
    send_touch_cancel (seat, tp->surface);
  }
}

static PhocTouchPoint *
phoc_cursor_add_touch_point (PhocCursor *self, struct wlr_touch_down_event *event)"""),
    ("""  touch_point = phoc_cursor_add_touch_point (self, event);
  lx = touch_point->lx;
  ly = touch_point->ly;
  handle_gestures_for_event_at (self, lx, ly, PHOC_EVENT_TOUCH_BEGIN, event, sizeof (*event));
""",
     """  touch_point = phoc_cursor_add_touch_point (self, event);
  lx = touch_point->lx;
  ly = touch_point->ly;
  handle_gestures_for_event_at (self, lx, ly, PHOC_EVENT_TOUCH_BEGIN, event, sizeof (*event));

  if (g_hash_table_size (priv->touch_points) >= 3)
    duo_touch_avale = TRUE;
  if (duo_touch_avale) {
    duo_annuler_toucher_clients (seat);
    return;
  }
"""),
    ("""  phoc_cursor_remove_touch_point (self, event->touch_id);

  if (self->seat->touch_id == event->touch_id)
    self->seat->touch_id = -1;
""",
     """  phoc_cursor_remove_touch_point (self, event->touch_id);
  if (g_hash_table_size (priv->touch_points) == 0)
    duo_touch_avale = FALSE;

  if (self->seat->touch_id == event->touch_id)
    self->seat->touch_id = -1;
"""),
])

view = os.path.join(src, "view.c")
patch(view, [
    ("""bool
phoc_view_move_to_next_output (PhocView *view, enum wlr_direction direction)
{""",
     """/* DUO-GESTES : glissement animé d'un écran à l'autre */
typedef struct {
  PhocView   *view;
  PhocOutput *output;
  double      x0, y0, x1, y1;
  gint64      debut;
} DuoGlisse;

#define DUO_GLISSE_US 280000

static gboolean
duo_glisse_pas (gpointer data)
{
  DuoGlisse *g = data;
  double t = (double)(g_get_monotonic_time () - g->debut) / DUO_GLISSE_US;

  if (!phoc_view_is_mapped (g->view) || t >= 1.0) {
    if (phoc_view_is_mapped (g->view))
      phoc_view_arrange (g->view, g->output, TRUE);
    g_object_unref (g->view);
    g_free (g);
    return G_SOURCE_REMOVE;
  }
  t = 1.0 - (1.0 - t) * (1.0 - t) * (1.0 - t);   /* départ rapide, arrivée en douceur */
  phoc_view_move (g->view, g->x0 + (g->x1 - g->x0) * t, g->y0 + (g->y1 - g->y0) * t);
  return G_SOURCE_CONTINUE;
}

bool
phoc_view_move_to_next_output (PhocView *view, enum wlr_direction direction)
{"""),
    ("""  if (phoc_view_is_fullscreen (view)) {
    phoc_view_set_fullscreen (view, true, PHOC_OUTPUT (new_output->data));
    return true;
  }

  phoc_view_arrange (view, output, TRUE);
  return true;
}""",
     """  if (phoc_view_is_fullscreen (view)) {
    phoc_view_set_fullscreen (view, true, PHOC_OUTPUT (new_output->data));
    return true;
  }

  if (phoc_view_is_maximized (view)) {
    struct wlr_box cible, geom;
    PhocViewPrivate *priv = phoc_view_get_instance_private (view);

    if (phoc_view_get_maximized_box (view, output, &cible)) {
      DuoGlisse *g = g_new0 (DuoGlisse, 1);

      phoc_view_get_geometry (view, &geom);
      g->view = g_object_ref (view);
      g->output = output;
      g->x0 = view->box.x;
      g->y0 = view->box.y;
      g->x1 = cible.x - geom.x / priv->scale;
      g->y1 = cible.y - geom.y / priv->scale;
      g->debut = g_get_monotonic_time ();
      g_timeout_add (16, duo_glisse_pas, g);
      return true;
    }
  }

  phoc_view_arrange (view, output, TRUE);
  return true;
}"""),
])
