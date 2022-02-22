#include "space_info.hh"

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

std::pair<u64, u64>
directory_size_and_file_count (const fs::path &path)
{
  u64 size = 0, count = 0;
  // ToDo: same as in process_dir
  for (const auto &entry : fs::recursive_directory_iterator (path))
    {
      if (entry.exists () && can_get_size (entry.status ()))
        {
          size += entry.file_size ();
          ++count;
        }
    }
  return std::make_pair (size, count);
}

SpaceInfo *
process_dir (const fs::path &path, ProcessingCallback callback)
{
  if (G_dirs.contains (path))
    return &G_dirs[path];

  SpaceInfo *const si
    = &G_dirs.emplace (std::make_pair (path, SpaceInfo {})).first->second;

  // ToDO: Iterate manually so we can catch individual errors and use
  //       std::error_code
  for (const auto &entry : fs::directory_iterator (path))
    {
      if (entry.is_directory ())
        {
          auto [size, count] = directory_size_and_file_count (entry.path ());
          si->add (entry.path (), size, count);
        }
      else if (entry.exists () && can_get_size (entry.status ()))
        si->add (entry.path (), entry.file_size ());
      if (callback)
        callback (*si);
    }
  return si;
}
