import sys
p = sys.argv[1]; s = open(p).read()
old = """  output = get_output_from_settings (self, device);

  if (!output)
    output = get_output_from_wlroots (self, device);"""
new = """  output = NULL;
  /* Surface Duo : PHOC_TOUCH_MAP="nom périphérique=sortie;..." (les deux dalles ont la même identité EDID) */
  {
    const char *map = g_getenv ("PHOC_TOUCH_MAP");
    const char *devname = phoc_input_device_get_name (device);

    if (map && devname) {
      g_auto (GStrv) pairs = g_strsplit (map, ";", -1);

      for (int i = 0; pairs[i] && !output; i++) {
        g_auto (GStrv) kv = g_strsplit (pairs[i], "=", 2);

        if (kv[0] && kv[1] && g_str_equal (kv[0], devname))
          output = phoc_desktop_find_output_by_name (desktop, kv[1]);
      }
    }
  }

  if (!output)
    output = get_output_from_settings (self, device);

  if (!output)
    output = get_output_from_wlroots (self, device);"""
assert old in s, 'motif introuvable'
open(p, 'w').write(s.replace(old, new, 1)); print('patch phoc OK')
