/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Simon Sandström
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

#include "item.h"

const ItemId ItemData::INVALID_ID = 0;

template<>
std::string Item::getAttribute(const std::string& name) const
{
  return itemData_->attributes.at(name);
}

template<>
int Item::getAttribute(const std::string& name) const
{
  return std::stoi(itemData_->attributes.at(name));
}

template<>
float Item::getAttribute(const std::string& name) const
{
  return std::stof(itemData_->attributes.at(name));
}

bool Item::operator==(const Item& other) const
{
  if (this->itemData_ == nullptr && other.itemData_ == nullptr)
  {
    // Both are invalid
    return true;
  }
  else if (this->itemData_ == nullptr || other.itemData_ == nullptr)
  {
    // One of them is invalid
    return false;
  }
  else
  {
    return this->itemData_->id == other.itemData_->id;
  }
}

bool Item::operator!=(const Item& other) const
{
  return !(*this == other);
}
