/** @file
 *
 * @brief Test scenarios for @c mutable_shared_buffer and
 * @c const_shared_buffer classes.
 *
 * @author Cliff Green
 *
 * @copyright (c) 2017-2024 by Cliff Green
 *
 * Distributed under the Boost Software License, Version 1.0. 
 * (See accompanying file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#include "catch2/catch_test_macros.hpp"
#include "catch2/catch_template_test_macros.hpp"


#include <cstddef> // std::byte
#include <list>
#include <string_view>
#include <span>
#include <array>
#include <algorithm> // std::copy
#include <bit> // std::bit_cast

#include "buffer/shared_buffer.hpp"

#include "utility/repeat.hpp"
#include "utility/byte_array.hpp"

constexpr std::size_t test_data_size { 12u };
using test_data_type = std::array<std::byte, test_data_size>;
constexpr test_data_type test_data { chops::make_byte_array( 40, 41, 42, 43, 44, 60, 59, 58, 57, 56, 42, 42 ) };
char test_data_char[test_data_size] { 40, 41, 42, 43, 44, 60, 59, 58, 57, 56, 42, 42 };
const char* test_data_char_ptr {test_data_char};

template <typename SB>
bool check_sb_against_test_data(SB sb) {
  REQUIRE (sb.size() == test_data_size);
  test_data_type buf;
  std::copy(sb.data(), sb.data()+sb.size(), buf.begin());
  return chops::compare_byte_arrays(buf, test_data);
}

template <typename SB, typename PT>
SB generic_pointer_construction_test() {
  auto ptr { std::bit_cast<const PT *>(test_data.data()) };
  SB sb(ptr, test_data_size);
  REQUIRE_FALSE (sb.empty());
  REQUIRE (check_sb_against_test_data(sb));
  return sb;
}

template <typename PT>
void generic_pointer_append_test() {
  auto sb { generic_pointer_construction_test<chops::mutable_shared_buffer, PT>() };
  auto sav_sz { sb.size() };
  const PT arr[] { 5, 6, 7 };
  const PT* ptr_arr { arr };
  sb.append (ptr_arr, 3);
  REQUIRE (sb.size() == (sav_sz + 3));
  std::span<const PT, 3> sp { arr };
  sb.append (sp);
  REQUIRE (sb.size() == (sav_sz + 6));
}

template <typename SB>
void check_sb(SB sb) {
  REQUIRE_FALSE (sb.empty());
  REQUIRE (sb.size() == test_data_size);
  REQUIRE (check_sb_against_test_data(sb));
}

template <typename SB>
void common_ctor_test() {

  {
    std::span<const std::byte, test_data_size> sp { test_data };
    SB sb{sp};
    check_sb(sb);
  }
  {
    std::span<const std::byte> sp { test_data.data(), test_data.size() };
    SB sb{sp};
    check_sb(sb);
  }
  {
    SB sb{test_data.data(), test_data.size()};
    check_sb(sb);
  }
  {
    std::span<const char, test_data_size> sp { test_data_char };
    SB sb{sp};
    check_sb(sb);
  }
  {
    std::span<const char> sp { test_data_char, test_data_char+test_data_size };
    SB sb{sp};
    check_sb(sb);
  }
  {
    SB sb{test_data_char_ptr, test_data_size};
    check_sb(sb);
  }

  {
    std::list<std::byte> lst {test_data.cbegin(), test_data.cend()};
    SB sb {lst.cbegin(), lst.cend()};
    check_sb(sb);
  }
  {
    SB sb1{test_data.data(), test_data.size()};
    SB sb2{test_data.data(), test_data.size()};
    REQUIRE (sb1 == sb2);
  }
  {
    SB sb1{test_data.data(), test_data.size()};
    SB sb2{sb1};
    REQUIRE (sb1 == sb2);
  }

}

template <typename SB>
void common_comparison_test() {
  auto ba1 { chops::make_byte_array(0x00, 0x00, 0x00) };
  auto ba2 { chops::make_byte_array(0x00, 0x22, 0x33) };

  SB sb1(ba1.cbegin(), ba1.cend());
  SB sb2(ba2.cbegin(), ba2.cend());
  REQUIRE_FALSE (sb1.empty());
  REQUIRE_FALSE (sb2.empty());
  REQUIRE_FALSE (sb1 == sb2);
  REQUIRE (((sb1 < sb2) != 0)); // uses spaceship operator
}

template <typename SB>
void byte_vector_move_test() {

  auto arr { chops::make_byte_array (0x01, 0x02, 0x03, 0x04, 0x05) };

  std::vector<std::byte> bv { arr.cbegin(), arr.cend() };
  SB sb(std::move(bv));
  REQUIRE (sb == SB(arr.cbegin(), arr.cend()));
  REQUIRE_FALSE (bv.size() == sb.size());
}
 
TEMPLATE_TEST_CASE ( "Generic pointer construction",
                     "[common]",
                     char, unsigned char, signed char, std::uint8_t ) {
  generic_pointer_construction_test<chops::mutable_shared_buffer, TestType>();
  generic_pointer_construction_test<chops::const_shared_buffer, TestType>();
}

                     
TEMPLATE_TEST_CASE ( "Shared buffer common ctor methods",
                     "[const_shared_buffer] [mutable_shared_buffer] [common]",
                     chops::mutable_shared_buffer, chops::const_shared_buffer ) {
  common_ctor_test<TestType>();
}

TEMPLATE_TEST_CASE ( "Shared buffer common comparison methods",
                     "[const_shared_buffer] [mutable_shared_buffer] [common]",
                     chops::mutable_shared_buffer, chops::const_shared_buffer ) {
  common_comparison_test<TestType>();
}

TEST_CASE ( "Mutable shared buffer copy construction and assignment",
            "[mutable_shared_buffer] [copy]" ) {

  constexpr std::byte Harhar { 42 };

  auto arr { chops::make_byte_array ( 80, 81, 82, 83, 84, 90, 91, 92 ) };

  chops::mutable_shared_buffer sb;
  REQUIRE (sb.empty());

  SECTION ("Assign mutable shared buffer into default constructed mutable shared buffer") {
    chops::mutable_shared_buffer sb2(arr.cbegin(), arr.cend());
    sb = sb2;
    REQUIRE (sb.size() == arr.size());
    REQUIRE (sb == sb2);
  }
  SECTION ("Assign mutable shared buffer, then copy construct") {
    sb = chops::mutable_shared_buffer(arr.cbegin(), arr.cend());
    chops::mutable_shared_buffer sb2(sb);
    REQUIRE (sb == sb2);
    *(sb.data()+0) = Harhar;
    *(sb.data()+1) = Harhar;
    REQUIRE (sb == sb2);
  }
}

TEST_CASE ( "Mutable shared buffer resize and clear",
            "[mutable_shared_buffer] [resize_and_clear]" ) {

  constexpr int N = 11;

  chops::mutable_shared_buffer sb;
  REQUIRE (sb.empty());
  REQUIRE (sb.size() == 0);

  sb.resize(N);
  REQUIRE (sb.size() == N);
  chops::repeat(N, [&sb] (int i) { REQUIRE (std::to_integer<int>(*(sb.data() + i)) == 0 ); } );

  SECTION ( "Compare two resized mutable shared buffer with same size" ) {
    chops::mutable_shared_buffer sb2(N);
    REQUIRE (sb == sb2);
    chops::repeat(N, [&sb, &sb2] (int i) {
      REQUIRE (std::to_integer<int>(*(sb.data() + i)) == 0 );
      REQUIRE (std::to_integer<int>(*(sb2.data() + i)) == 0 );
    } );
  }
  SECTION ( "Clear, check size" ) {
    sb.clear();
    REQUIRE (sb.size() == 0);
    REQUIRE (sb.empty());
  } // end given
}

TEST_CASE ( "Mutable shared buffer swap",
            "[mutable_shared_buffer] [swap]" ) {

  auto arr1 = chops::make_byte_array (0xaa, 0xbb, 0xcc);
  auto arr2 = chops::make_byte_array (0x01, 0x02, 0x03, 0x04, 0x05);

  chops::mutable_shared_buffer sb1(arr1.cbegin(), arr1.cend());
  chops::mutable_shared_buffer sb2(arr2.cbegin(), arr2.cend());

  chops::swap(sb1, sb2);
  REQUIRE (sb1.size() == arr2.size());
  REQUIRE (sb2.size() == arr1.size());

  REQUIRE (std::to_integer<int>(*(sb1.data()+0)) == std::to_integer<int>(*(arr2.data()+0)));
  REQUIRE (std::to_integer<int>(*(sb1.data()+1)) == std::to_integer<int>(*(arr2.data()+1)));
  REQUIRE (std::to_integer<int>(*(sb1.data()+2)) == std::to_integer<int>(*(arr2.data()+2)));
  REQUIRE (std::to_integer<int>(*(sb1.data()+3)) == std::to_integer<int>(*(arr2.data()+3)));
  REQUIRE (std::to_integer<int>(*(sb1.data()+4)) == std::to_integer<int>(*(arr2.data()+4)));

  REQUIRE (std::to_integer<int>(*(sb2.data()+0)) == std::to_integer<int>(*(arr1.data()+0)));
  REQUIRE (std::to_integer<int>(*(sb2.data()+1)) == std::to_integer<int>(*(arr1.data()+1)));
  REQUIRE (std::to_integer<int>(*(sb2.data()+2)) == std::to_integer<int>(*(arr1.data()+2)));
}

TEST_CASE ( "Mutable shared buffer append",
            "[mutable_shared_buffer] [append]" ) {

  auto arr = chops::make_byte_array (0xaa, 0xbb, 0xcc);
  auto arr2 = chops::make_byte_array (0xaa, 0xbb, 0xcc, 0xaa, 0xbb, 0xcc);
  chops::mutable_shared_buffer ta(arr.cbegin(), arr.cend());
  chops::mutable_shared_buffer ta2(arr2.cbegin(), arr2.cend());

  chops::mutable_shared_buffer sb;
  REQUIRE (sb.empty());

  SECTION ( "Append array to default constructed mutable shared_buffer" ) {
    sb.append(arr.data(), arr.size());
    REQUIRE (sb == ta);
  }

  SECTION ( "Append mutable shared buffer" ) {
    sb.append(ta);
    REQUIRE (sb == ta);
  }

  SECTION ( "Call append twice" ) {
    sb.append(ta);
    sb.append(ta);
    REQUIRE (sb == ta2);
  }

  SECTION ( "Append with single byte" ) {
    sb.append(std::byte(0xaa));
    sb.append(std::byte(0xbb));
    sb += std::byte(0xcc);
    REQUIRE (sb == ta);
  }

  SECTION ( "Append with templated append" ) {
    std::string_view sv("Haha, Bro!");
    chops::mutable_shared_buffer cb(sv.data(), sv.size());
    sb.append(sv.data(), sv.size());
    REQUIRE (sb == cb);
  } 
}

TEMPLATE_TEST_CASE ( "Generic pointer append",
                     "[mutable_shared_buffer] [pointer] [append]",
                     char, unsigned char, signed char, std::uint8_t ) {
  generic_pointer_append_test<TestType>();
}

TEST_CASE ( "Compare a mutable shared_buffer with a const shared buffer",
            "[mutable_shared_buffer] [const_shared_buffer] [compare]" ) {

  auto arr = chops::make_byte_array (0xaa, 0xbb, 0xcc);
  chops::mutable_shared_buffer msb(arr.cbegin(), arr.cend());
  chops::const_shared_buffer csb(arr.cbegin(), arr.cend());
  REQUIRE (msb == csb);
  REQUIRE (csb == msb);
}

TEST_CASE ( "Mutable shared buffer move into const shared buffer",
            "[mutable_shared_buffer] [const_shared_buffer] [move]" ) {

  auto arr1 = chops::make_byte_array (0xaa, 0xbb, 0xcc);
  auto arr2 = chops::make_byte_array (0x01, 0x02, 0x03, 0x04, 0x05);

  chops::mutable_shared_buffer msb(arr1.cbegin(), arr1.cend());
  chops::const_shared_buffer csb(std::move(msb));
  REQUIRE (csb == chops::const_shared_buffer(arr1.cbegin(), arr1.cend()));
  REQUIRE_FALSE (msb == csb);
  msb.clear();
  msb.resize(arr2.size());
  msb.append(arr2.data(), arr2.size());
  REQUIRE_FALSE (msb == csb);
}

TEMPLATE_TEST_CASE ( "Move a vector of bytes into a shared buffer",
                     "[common] [move_byte_vec]",
                     chops::mutable_shared_buffer, chops::const_shared_buffer ) {

  byte_vector_move_test<TestType>();

}

TEST_CASE ( "Use get_byte_vec for external modification of buffer",
            "[mutable_shared_buffer] [get_byte_vec]" ) {

  auto arr = chops::make_byte_array (0xaa, 0xbb, 0xcc);
  chops::mutable_shared_buffer::byte_vec bv (arr.cbegin(), arr.cend());

  chops::mutable_shared_buffer msb(bv.cbegin(), bv.cend());

  auto r = msb.get_byte_vec();
//  REQUIRE (r == bv); // Catch2 build problems on MSVC
  r[0] = std::byte(0xdd);
//  REQUIRE_FALSE (r == bv); // Catch2 build problems on MSVC
}

