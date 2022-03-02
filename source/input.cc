#include "input.hh"

namespace Input
{
static int
read_special_from_escape ()
{
  static constexpr int CTRL_FLAG = '5';
  //static constexpr int SHIFT_FLAG = '2';
  //static constexpr int ALT_FLAG = '3';
  int first, second = 0, third = 0;
  // This could have just been the escape key so check if there is more in the
  // input buffer using a non-blocking call. If there was more this consumed
  // the '[' of the escape sequence.
  nodelay (stdscr, true);
  if (getch () == ERR)
    {
      nodelay (stdscr, false);
      return Special::Escape;
    }
  nodelay (stdscr, false);
  first = getch ();
  switch (first)
    {
      case 'A': return KEY_UP;
      case 'B': return KEY_DOWN;
      case 'C': return KEY_RIGHT;
      case 'D': return KEY_LEFT;
      case 'F': return KEY_END;
      case 'H': return KEY_HOME;
    }
  if (getch () == ';')
    {
      second = getch ();
      third = getch ();
    }
  switch (first)
    {
      case '1':
        if (second != CTRL_FLAG)
          return Special::Unused;
        switch (third)
          {
            case 'A': return Special::CtrlUp;
            case 'B': return Special::CtrlDown;
            case 'C': return Special::CtrlRight;
            case 'D': return Special::CtrlLeft;
            default: return Special::Unused;
          }
      case '3':
        if (second)
          return second == CTRL_FLAG ? Special::CtrlDelete : Special::Unused;
        return Special::Delete;
      case '5':
        return second ? Special::Unused : KEY_PPAGE;
      case '6':
        return second ? Special::Unused : KEY_NPAGE;
    }
  return Special::Unused;
}

int
get_char ()
{
  int ch;
  switch (ch = getch ())
    {
      case 0: return Special::CtrlSpace;
      case 1: return Special::CtrlA;
      case 8: return Special::CtrlBackspace;
      case 27: return read_special_from_escape ();
      case 127: return Special::Backspace;
      default: return ch;
    }
}

std::string_view
get_line (History *history, GetLineCallback callback)
{
  std::string *str = history->start ();
  auto read_unicode = [&](char first) {
    str->push_back (first);
    switch (first >> 4)
      {
        case 0xf: str->push_back (getch ()); [[fallthrough]];
        case 0xe: str->push_back (getch ()); [[fallthrough]];
        case 0xd:
        case 0xc: str->push_back (getch ());
      }
  };
  int ch;
  bool change;
  while ((ch = get_char ()) != '\n')
    {
      change = true;
      // UTF8 sequence
      if (ch & 0x80)
        read_unicode (ch);
      // Ascii character
      else if (std::isprint (ch))
        str->push_back (ch);
      // Control character
      else
        {
          switch (ch)
            {
              case KEY_UP:
                str = history->prev ();
                break;
              case KEY_DOWN:
                str = history->next ();
                break;
              case Special::Backspace:
                if (!str->empty ())
                  str->pop_back ();
                break;
              case Special::CtrlBackspace:
                // Delete either anything after '/' or '.', or the entire
                // string if neither is found.
                str->erase (str->find_last_of ("/."sv, str->size () - 2) + 1);
                break;
              case Special::Escape:
                return ""sv;
              default:
                change = false;
                break;
            }
        }
      if (change && callback)
        callback (*str);
    }
  History::reference line = history->commit ();
  return { line.c_str (), line.size () };
}

}
