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
  fs::path path = (arg
                   ? fs::canonical (fs::path (arg))
                   : fs::current_path ());
  fs::path pending_path;
  bool sort_ascending = false;
  const fs::path dev_path = "/dev";

  // The program gets stuck when entering the /dev/ directory (at least on my
  // machine :^) ) so we do not permit entering it.
  // It also reports a bogus size when showing the root directory (in my case
  // 128 TB) so we display 'Not supported' instead of the size.
  // (ToDo: maybe just hide it?)
  if (path == dev_path)
    {
      std::fputs ("The /dev directory is not supported.\n", stderr);
      return 1;
    }

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
            display::move_cursor (-1, si->item_count () + 1);
            break;
          case KEY_DOWN:
          case 'j':
key_down:
            display::move_cursor (1, si->item_count () + 1);
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
            display::set_cursor (si->item_count ());
            break;
          case 10: // Enter
          case ' ':
            pending_path = display::select (*si);
            if (fs::is_directory (pending_path) && pending_path != dev_path)
              {
                path.swap (pending_path);
                display::clear ();
                display::header (path);
                si = process_dir (path, show_progress);
                if (si == nullptr)
                  {
                    path.swap (pending_path);
                    display::header (path);
                    si = process_dir (path);
                  }
                else
                  display::set_cursor (0);
                display::footer (*si);
              }
            break;
          case 'r':
          case 'i':
            sort_ascending = !sort_ascending;
            si->sort (sort_ascending);
            break;
        }
      display::space_info (*si);
      display::refresh ();
    }
  display::end ();
}
