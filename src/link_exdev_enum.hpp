/*
  ISC License

  Copyright (c) 2021, Antonio SJ Musumeci <trapexit@spawn.link>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#pragma once

#include "toml.hpp"

enum class LinkEXDEV
  {
   INVALID,
   PASSTHROUGH,
   REL_SYMLINK,
   ABS_BASE_SYMLINK,
   ABS_POOL_SYMLINK
  };

namespace toml
{
  template<>
  struct from<LinkEXDEV>
  {
    static
    LinkEXDEV
    from_toml(const toml::value &v_)
    {
      const std::string &s = v_.as_string();

      if(s == "passthrough")
        return LinkEXDEV::PASSTHROUGH;
      if(s == "rel-symlink")
        return LinkEXDEV::REL_SYMLINK;
      if(s == "abs-base-symlink")
        return LinkEXDEV::ABS_BASE_SYMLINK;
      if(s == "abs-pool-symlink")
        return LinkEXDEV::ABS_POOL_SYMLINK;

      return LinkEXDEV::INVALID;
    }
  };
}
