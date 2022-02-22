#include "stdafx.hh"
#include "options.hh"
#include "space_info.hh"
#include "display.hh"
#include "select.hh"

static void
show_progress (const SpaceInfo &si)
{
  Display::space_info (si, false);
  Display::footer (si);
  Display::refresh ();
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
  const fs::path dev_path = "/dev";
  SpaceInfo *si;
  const char *const arg = parse_args (argc, argv);
  fs::path path = (arg
                   ? fs::canonical (fs::path (arg))
                   : fs::current_path ());
  fs::path pending_path;
  bool sort_ascending = false;

  // The program gets stuck when entering the /dev/ directory (at least on my
  // machine :^) ) so we do not permit entering it.
  // It also reports a bogus size when showing the root directory (in my case
  // 128 TB) so we display 'Not supported' instead of the size.
  if (path == dev_path)
    {
      std::fputs ("The /dev directory is not supported.\n", stderr);
      return 1;
    }

  Select::clear_selection ();

  Display::begin ();
  Display::header (path);
  si = process_dir (path, show_progress);
  if (si == nullptr)
    {
      Display::end ();
      fail ();
    }
  Display::space_info (*si);
  Display::footer (*si);
  Display::refresh ();

  std::string_view search = ""sv;

  int ch;
  while ((ch = getch ()) != 'q')
    {
      switch (ch)
        {
          case KEY_UP:
          case 'k':
key_up:
            Display::move_cursor (-1, si->item_count () + 1);
            break;
          case KEY_DOWN:
          case 'j':
key_down:
            Display::move_cursor (1, si->item_count () + 1);
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
            Display::set_cursor (0);
            break;
          case 'G':
            Display::set_cursor (si->item_count ());
            break;
          case 10: // Enter
          case ' ':
            pending_path = Display::select (*si, path);
            if (fs::is_directory (pending_path) && pending_path != dev_path)
              {
                path.swap (pending_path);
                Display::clear ();
                Display::header (path);
                si = process_dir (path, show_progress);
                if (si == nullptr)
                  {
                    path.swap (pending_path);
                    Display::header (path);
                    si = process_dir (path);
                  }
                else
                  {
                    Display::set_cursor (0);
                    Select::select (search, *si);
                  }
                Display::footer (*si);
              }
            break;
          case 'r':
          case 'i':
            sort_ascending = !sort_ascending;
            si->sort (sort_ascending);
            break;
          case '/':
            search = Display::input ("Search");
            Display::footer (*si);
            Select::select (search, *si);
            break;
          case 'n':
            Select::next ();
            break;
          case 'N':
            Select::prev ();
            break;
          case 'c':  // ToDo: probably use a differeny key/way to clear
            Select::clear_selection ();
            break;
        }
      Display::space_info (*si);
      Display::refresh ();
    }
  Display::end ();
}
