#include "space_info.hh"

std::error_code G_error;

void
SpaceInfo::add (const fs::path &path, u64 size, u64 file_count)
{
  file_count_ += file_count;
  insert_sorted (Item { path, size });
  total_ += size;
  if (size > biggest_)
    biggest_ = size;
}

void
SpaceInfo::sort (bool ascending)
{
  auto comp = [ascending] (const Item &a, const Item &b) {
    // ToDo: should ascending affect aphabetical sorting as well?
    return (a.size == b.size
            ? a.path < b.path
            : (a.size > b.size) ^ ascending);
  };
  std::sort (items_.begin (), items_.end (), comp);
}

void
SpaceInfo::insert_sorted (Item &&item)
{
  items_.insert (
    std::upper_bound (items_.begin (), items_.end (), item,
                      [](const Item &a, const Item &b) {
                        return (a.size == b.size
                                ? a.path < b.path
                                : a.size > b.size);
                      }),
    std::move (item)
  );
}

bool
can_get_size (const fs::file_status &stat)
{
  switch (stat.type ())
    {
      break; case fs::file_type::none: return true;
      break; case fs::file_type::unknown: return true;
      break; case fs::file_type::regular: return true;
      break; case fs::file_type::symlink: return true;
      break; default: return false;
    }
}

template <class It, class UnaryFunction>
static bool
safe_directory_iterator (It &&dir_it, UnaryFunction f)
{
  const auto end = fs::end (dir_it);
  for (auto it = fs::begin (dir_it); it != end; it.increment (G_error))
    {
      if (G_error)
        return false;
      f (*it);
    }
  return true;
}

bool
directory_size_and_file_count (const fs::path &path, u64 &size, u64 &count)
{
  size = count = 0;
  return safe_directory_iterator (
    fs::recursive_directory_iterator (path),
    [&](const fs::directory_entry &entry) {
      if (entry.exists () && can_get_size (entry.status ()))
        {
          size += entry.file_size ();
          ++count;
        }
    }
  );
}

SpaceInfo *
process_dir (const fs::path &path, ProcessingCallback callback)
{
  u64 size, count;

  if (G_dirs.contains (path))
    return &G_dirs[path];

  SpaceInfo *const si
    = &G_dirs.emplace (std::make_pair (path, SpaceInfo {path})).first->second;

  if (!safe_directory_iterator (
        fs::directory_iterator (path),
        [&](const fs::directory_entry &entry) {
          if (entry.is_directory ())
            {
              if (!directory_size_and_file_count (entry.path (), size, count))
                // ToDo: allow SpaceInfo::Item to represent errors
                si->add (entry.path (), -1, -1);
              else
                si->add (entry.path (), size, count);
            }
          else if (entry.exists () && can_get_size (entry.status ()))
            si->add (entry.path (), entry.file_size ());
          if (callback)
            callback (*si);
        }
      ))
    return nullptr;
  return si;
}
