#pragma once

#include "module_builder.hpp"

namespace codegen::literals {

value<int8_t> operator""_i8(unsigned long long v) {
  return constant<int8_t>(v);
}
value<int16_t> operator""_i16(unsigned long long v) {
  return constant<int16_t>(v);
}
value<int32_t> operator""_i32(unsigned long long v) {
  return constant<int32_t>(v);
}
value<int64_t> operator""_i64(unsigned long long v) {
  return constant<int64_t>(v);
}

value<uint8_t> operator""_u8(unsigned long long v) {
  return constant<uint8_t>(v);
}
value<uint16_t> operator""_u16(unsigned long long v) {
  return constant<uint16_t>(v);
}
value<uint32_t> operator""_u32(unsigned long long v) {
  return constant<uint32_t>(v);
}
value<uint64_t> operator""_u64(unsigned long long v) {
  return constant<uint64_t>(v);
}

value<float> operator""_f32(long double v) {
  return constant<float>(v);
}
value<double> operator""_f64(long double v) {
  return constant<double>(v);
}

} // namespace codegen::literals