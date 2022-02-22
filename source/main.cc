#include "stdafx.hh"
#include "space_info.hh"
#include "display.hh"
#include "options.hh"

static void
show_progress (const SpaceInfo &si)
{
  display::space_info (si, false);
  display::footer (si);
  display::refresh ();
}

static SpaceInfo *
do_process_dir (const fs::path &p)
{
  try
    {
      return process_dir (p, show_progress);
    }
  catch (const fs::filesystem_error &e)
    {
      display::end ();
      std::fputs ("Permission denied\n", stderr);
      std::exit (1);
    }
}

int
main (const int argc, const char **argv)
{
  SpaceInfo *si;
  const char *const arg = parse_args (argc, argv);
  fs::path path = arg ? arg : fs::current_path (), pending_path;

  display::begin ();
  display::header (path);
  si = do_process_dir (path);
  display::space_info (*si);
  display::footer (*si);
  display::refresh ();

  int ch;
  while ((ch = getch ()) != 'q')
    {
      switch (ch)
        {
          case KEY_UP:
          case 'k':
key_up:
            display::move_cursor (-1, si->item_count ());
            break;
          case KEY_DOWN:
          case 'j':
key_down:
            display::move_cursor (1, si->item_count ());
            break;
          case 27:
            (void)getch ();
            ch = getch ();
            if (ch == 'A')
              goto key_up;
            else if (ch == 'B')
              goto key_down;
            break;
          case 'g':
            display::set_cursor (0);
            break;
          case 'G':
            display::set_cursor (si->item_count () - 1);
            break;
          case 10: // Enter
            pending_path = display::select (*si, path);
            if (fs::is_directory (pending_path))
              {
                path.swap (pending_path);
                display::clear ();
                display::header (path);
                si = do_process_dir (path);
                display::footer (*si);
                display::set_cursor (0);
              }
            break;
        }
      display::space_info (*si);
      display::refresh ();
    }
  display::end ();
}
