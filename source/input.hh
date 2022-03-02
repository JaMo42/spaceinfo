#pragma once
#include "stdafx.hh"
#include "options.hh"

namespace Input
{
enum Special
{
  // random value that's hopefully above anything ncurses uses :^)
  Unused = 0x10000,
  CtrlUp,
  CtrlDown,
  CtrlRight,
  CtrlLeft,
  CtrlA,
  Backspace,
  CtrlBackspace,
  Delete,
  CtrlDelete,
  Escape,
  CtrlSpace
};

class History
{
  using container_type = std::list<std::string>;
public:
  using value_type = container_type::value_type;
  using reference = container_type::reference;
  using const_reference = container_type::const_reference;
  using pointer = container_type::pointer;
  using iterator = container_type::iterator;

public:
  pointer start ()
  {
    new_val_.clear ();
    cursor_ = data_.end ();
    return &new_val_;
  }

  reference commit ()
  {
    if (data_.size () == Options::history_size)
      data_.pop_front ();
    return data_.emplace_back (cursor_ == data_.end ()
                               ? std::move (new_val_)
                               : std::move (hist_val_));
  }

  pointer prev ()
  {
    if (data_.empty ())
      return &new_val_;
    if (cursor_ != data_.begin ())
      --cursor_;
    hist_val_.assign (*cursor_);
    return &hist_val_;
  }

  pointer next ()
  {
    if (cursor_ == data_.end ())
      return &new_val_;
    ++cursor_;
    if (cursor_ == data_.end ())
      return &new_val_;
    hist_val_.assign (*cursor_);
    return &hist_val_;
  }

private:
  container_type data_;
  iterator cursor_;
  value_type hist_val_;
  value_type new_val_;
};

using GetLineCallback = std::function<void (History::const_reference)>;

int get_char ();

std::string_view get_line (History *history = nullptr,
                           GetLineCallback callback = nullptr);
}
