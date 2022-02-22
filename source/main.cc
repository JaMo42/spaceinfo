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

static void
fail ()
{
  std::fputs (G_error.message ().c_str (), stderr);
  std::fputc ('\n', stderr);
  std::exit (1);
}

int
main (const int argc, const char **argv)
{
  SpaceInfo *si;
  const char *const arg = parse_args (argc, argv);
  fs::path path = arg ? arg : fs::current_path (), pending_path;

  display::begin ();
  display::header (path);
  si = process_dir (path, show_progress);
  if (si == nullptr)
    {
      display::end ();
      fail ();
    }
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
                si = process_dir (path, show_progress);
                // ToDo: stay in previous directory and inform user about error
                //       instead of just quitting
                if (si == nullptr)
                  {
                    display::end ();
                    fail ();
                  }
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
