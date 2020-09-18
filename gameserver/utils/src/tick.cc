/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Simon Sandström
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "tick.h"

#ifndef __EMSCRIPTEN__
#include <boost/date_time/posix_time/posix_time.hpp>
#else
#include <emscripten.h>
#endif

namespace utils
{

#ifndef __EMSCRIPTEN__

using ms_clock = boost::posix_time::microsec_clock;
static const decltype(ms_clock::universal_time()) START = ms_clock::universal_time();

std::int64_t Tick::now()
{
  static_assert(sizeof(std::int64_t) == sizeof(boost::posix_time::microsec_clock::time_duration_type::tick_type),
                "boost::posix_time::microsec_clock::time_duration_type::tick_type needs to be 64 bits");
  return (ms_clock::universal_time() - START).total_milliseconds();
}

#else

std::int64_t Tick::now()
{
  // TODO
  return 0;
}

#endif

}  // namespace utils
