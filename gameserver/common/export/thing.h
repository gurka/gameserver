/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Simon Sandström
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

#ifndef COMMON_EXPORT_THING_H_
#define COMMON_EXPORT_THING_H_

#include <functional>
#include <type_traits>
#include <variant>

namespace common
{

class Creature;
class Item;

struct Thing
{
  Thing(const Creature* creature)  // NOLINT
      : thing(creature)
  {
  }

  Thing(const Item* item)  // NOLINT
      : thing(item)
  {
  }

  const Creature* creature() const
  {
    return thing.index() == 0 ? std::get<0>(thing) : nullptr;
  }

  const Item* item() const
  {
    return thing.index() == 1 ? std::get<1>(thing) : nullptr;
  }

  void visit(const std::function<void(const Creature*)>& creature_func,
             const std::function<void(const Item*)>& item_func) const
  {
    std::visit([&creature_func, &item_func](auto&& arg)
    {
      using T = std::decay_t<decltype(arg)>;
      if constexpr (std::is_same_v<T, const Creature*>)  // NOLINT clang-tidy bug?
      {
        if (creature_func)
        {
          creature_func(arg);
        }
      }
      else if constexpr (std::is_same_v<T, const Item*>)
      {
        if (item_func)
        {
          item_func(arg);
        }
      }
      else
      {
        static_assert(AlwaysFalse<T>::value, "Invalid type T");
      }
    }, thing);
  }


  std::variant<const Creature*, const Item*> thing;

 private:
  template<typename T>
  struct AlwaysFalse : std::false_type {};
};

}  // namespace common

#endif  // COMMON_EXPORT_THING_H_
