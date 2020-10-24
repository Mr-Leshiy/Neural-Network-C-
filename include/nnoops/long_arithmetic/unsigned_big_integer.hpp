#ifndef NNOOPS_LIB_CPP_LONG_ARITHMETIC_BIG_INTEGER_HPP_
#define NNOOPS_LIB_CPP_LONG_ARITHMETIC_BIG_INTEGER_HPP_

#include <stdint.h>

#include <array>
#include <string>
#include <type_traits>

#include "nnoops/long_arithmetic/arith_error.hpp"
#include "nnoops/strutil.hpp"

namespace nnoops {

// Representation on the unsigned integer with the arbitrary size
// SIZE should be multiple of 8 (1 byte)
template <uint64_t SIZE = 64,
          typename = typename std::enable_if<SIZE % 8 == 0 && SIZE != 0>::type>
struct UBigInteger {
  UBigInteger() = default;

  UBigInteger(const UBigInteger<SIZE>& val) : data(val.data) {}

  UBigInteger(UBigInteger<SIZE>&& val) : data(std::move(val.data)) {}

  UBigInteger<SIZE>& operator=(const UBigInteger<SIZE>& val) {
    this->data = val.data;
    return *this;
  }

  UBigInteger<SIZE>& operator=(UBigInteger<SIZE>&& val) {
    this->data = std::move(val.data);
    return *this;
  }

  UBigInteger(uint8_t val) { init(val); }

  UBigInteger(uint16_t val) { init(val); }

  UBigInteger(uint32_t val) { init(val); }

  UBigInteger(uint64_t val) { init(val); }

  UBigInteger(int8_t val) {
    assert(val >= 0 && "value should be positive");
    init((uint8_t)val);
  }

  UBigInteger(int16_t val) {
    assert(val >= 0 && "value should be positive");
    init((uint16_t)val);
  }

  UBigInteger(int32_t val) {
    assert(val >= 0 && "value should be positive");
    init((uint32_t)val);
  }

  UBigInteger(int64_t val) {
    assert(val >= 0 && "value should be positive");
    init((uint64_t)val);
  }

  UBigInteger<SIZE> operator-() const {
    UBigInteger<SIZE> ret = *this;
    for (size_t i = 0; i < data.size(); ++i) {
      ret.data[i] = ~this->data[i];
    }
    ++ret;
    return ret;
  }

  UBigInteger<SIZE>& operator++() {
    // prefix operator
    for (size_t i = 0; i < data.size() && ++data[i] == 0; ++i) {
    }
    return *this;
  }

  UBigInteger<SIZE> operator++(int) {
    // postfix operator
    UBigInteger<SIZE> ret = *this;
    ++(*this);
    return ret;
  }

  UBigInteger<SIZE>& operator--() {
    // prefix operator
    for (size_t i = 0; i < data.size() && --data[i] == BASE; ++i) {
    }
    return *this;
  }

  UBigInteger<SIZE> operator--(int) {
    UBigInteger<SIZE> ret = *this;
    --(*this);
    return ret;
  }

  UBigInteger<SIZE>& operator+=(const UBigInteger<SIZE>& b) {
    uint64_t carry = 0;
    for (size_t i = 0; i < data.size(); ++i) {
      uint64_t n = carry + this->data[i] + b.data[i];
      this->data[i] = n & BASE;
      carry = n >> 8;
    }
    return *this;
  }

  UBigInteger<SIZE>& operator-=(const UBigInteger<SIZE>& b) {
    uint64_t carry = 0;
    for (size_t i = 0; i < data.size(); ++i) {
      uint64_t n = carry + this->data[i] - b.data[i];
      this->data[i] = n & BASE;
      carry = n >> 8;
    }
    return *this;
  }

  UBigInteger<SIZE>& operator*=(const UBigInteger<SIZE>& b) {
    UBigInteger<SIZE> a;
    for (size_t i = 0; i < data.size(); ++i) {
      uint64_t carry = 0;
      for (size_t j = 0; i + j < data.size(); ++j) {
        uint64_t n = carry + a.data[i + j] + this->data[i] * b.data[j];
        a.data[i + j] = n & BASE;
        carry = n >> 8;
      }
    }
    this->data = std::move(a.data);
    return *this;
  }

  UBigInteger<SIZE>& operator/=(const UBigInteger<SIZE>& b) noexcept(false) {
    UBigInteger<SIZE> divisor = b;
    UBigInteger<SIZE> dividend = *this;
    UBigInteger<SIZE> result = 0;
    UBigInteger<SIZE> remainder = 0;

    if (b > *this) {
      return result;
    }

    if (b == 0) {
      throw arith_error("devide by zero");
    }

    // normalize
    size_t m = 0, n = 0, d = 0;
    for (size_t i = SIZE - 1;; --i) {
      if (divisor.data[i] != 0 && m == 0) {
        d = divisor.data[i];
        n = i;
      }

      if (dividend.data[i] != 0 && n == 0) {
        n = i - m;
      }

      if ((m == 0 && n == 0) || i == 0) {
        break;
      }
    }

    d = BASE / d;
    if (d != 1) {
      dividend *= d;
      divisor *= d;
    }

    for (int64_t j = m;; --j) {
      // Calculate q
      uint64_t el1 = dividend.data[j + n + 1] * BASE + dividend.data[j + n];
      uint64_t r = el1 & divisor.data[n];
      uint64_t q = el1 / divisor.data[n];

      // Test
      if (q == BASE ||
          q * divisor.data[n - 1] > (b * r + dividend.data[j + n - 1])) {
      }

      if (j == 0) {
        break;
      }
    }

    return result;
  }

  friend inline UBigInteger<SIZE> operator+(const UBigInteger<SIZE>& a,
                                            const UBigInteger<SIZE>& b) {
    return UBigInteger<SIZE>(a) += b;
  }

  friend inline UBigInteger<SIZE> operator-(const UBigInteger<SIZE>& a,
                                            const UBigInteger<SIZE>& b) {
    return UBigInteger<SIZE>(a) -= b;
  }

  friend inline UBigInteger<SIZE> operator*(const UBigInteger<SIZE>& a,
                                            const UBigInteger<SIZE>& b) {
    return UBigInteger<SIZE>(a) *= b;
  }

  bool operator==(const UBigInteger<SIZE>& val) const {
    return this->data == val.data;
  }

  bool operator!=(const UBigInteger<SIZE>& val) const {
    return !(*this == val);
  }

  // return -1 if this less than b,
  // return 1 if this bigger than b
  // return 0 if this equal to b
  int compareTo(const UBigInteger<SIZE>& b) const {
    for (int64_t i = (int64_t)(data.size() - 1); i != 0; --i) {
      if (this->data[i] < b.data[i]) {
        return -1;
      }
      if (this->data[i] > b.data[i]) {
        return 1;
      }
    }

    return 0;
  }

  friend bool operator>(const UBigInteger<SIZE>& a,
                        const UBigInteger<SIZE>& b) {
    return a.compareTo(b) > 0;
  }

  friend bool operator<(const UBigInteger<SIZE>& a,
                        const UBigInteger<SIZE>& b) {
    return a.compareTo(b) < 0;
  }

  friend bool operator>=(const UBigInteger<SIZE>& a,
                         const UBigInteger<SIZE>& b) {
    return a.compareTo(b) >= 0;
  }

  friend bool operator<=(const UBigInteger<SIZE>& a,
                         const UBigInteger<SIZE>& b) {
    return a.compareTo(b) <= 0;
  }

  friend std::string toPrettyString(const UBigInteger<SIZE>& val) {
    std::string ret = HexStr(val.data.rbegin(), val.data.rend());
    removeZeros(ret);
    return ret;
  }

 protected:
  template <typename T,
            typename = typename std::enable_if<
                std::is_integral<T>::value && std::is_unsigned<T>::value>::type>
  void init(T value) {
    size_t val_size = sizeof(value);
    assert(data.size() >= val_size && "data has a small size");
    for (size_t i = 0; i < data.size(); ++i) {
      if (i < val_size) {
        data[i] = (uint8_t)(value >> 8 * i);
      } else {
        data[i] = 0;
      }
    }
  }

 private:
  static const uint8_t BASE = 0xff;
  std::array<uint8_t, SIZE / 8> data{};
};

}  // namespace nnoops

#endif
