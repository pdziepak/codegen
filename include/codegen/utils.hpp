/*
 * Copyright © 2019 Paweł Dziepak
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <atomic>

#include <llvm/Support/Error.h>
#include <llvm/Support/raw_ostream.h>

namespace codegen {

namespace detail {

static inline std::atomic<unsigned> id_counter{};

}

class llvm_error : public std::runtime_error {
public:
  explicit llvm_error(llvm::Error err)
      : std::runtime_error([&] {
          auto str = std::string{"LLVM Error: "};
          auto os = llvm::raw_string_ostream(str);
          os << err;
          os.str();
          return str;
        }()) {}
};

inline void throw_on_error(llvm::Error err) {
  if (err) { throw llvm_error(std::move(err)); }
}

template<typename T> T unwrap(llvm::Expected<T> value) {
  if (!value) { throw llvm_error(value.takeError()); }
  return std::move(*value);
}

} // namespace codegen
