#pragma once
#include "stdafx.hh"

class SpaceInfo
{
  struct Item
  {
    fs::path path;
    u64 size;
  };

public:
  using items_type = std::vector<Item>;
  using value_type = Item;
  using iterator = items_type::iterator;
  using const_iterator = items_type::const_iterator;

public:
  void
  add (const fs::path &path, u64 size, u64 file_count = 1);

  void
  sort (bool ascending = false);

  u64 total () const { return total_; }
  u64 biggest () const { return biggest_; }
  u64 total_file_count () const { return file_count_; }
  u64 item_count () const { return items_.size (); }

  const_iterator begin () const { return items_.cbegin (); }
  const_iterator end () const { return items_.cend (); }

  const Item &
  operator[] (usize idx) const
  { return items_[idx]; }

  f64
  size_relative_to_biggest (const Item &item) const
  { return static_cast<f64> (item.size) / biggest_; }

  f64
  size_relative_to_total (const Item &item) const
  { return static_cast<f64> (item.size) / total_; }

private:
  void insert_sorted (Item &&);

private:
  u64 file_count_ {0};
  u64 biggest_ {0};
  u64 total_ {0};
  items_type items_ {};
};

using ProcessingCallback = std::function<void (const SpaceInfo &)>;

inline std::map<fs::path, SpaceInfo> G_dirs;

extern std::error_code G_error;

bool can_get_size (const fs::file_status &stat);

std::pair<u64, u64> directory_size_and_file_count (const fs::path &path);

SpaceInfo * process_dir (const fs::path &path,
                         ProcessingCallback callback = nullptr);
