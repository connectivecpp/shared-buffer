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
#include <bit> // std::bit_cast

#include "buffer/shared_buffer.hpp"

#include "utility/repeat.hpp"
#include "utility/make_byte_array.hpp"

constexpr std::byte Harhar { 42 };
constexpr int N = 11;


template <typename SB, typename PT>
SB generic_pointer_construction_test() {
  auto arr = chops::make_byte_array( 40, 41, 42, 43, 44, 60, 59, 58, 57, 56, 42, 42 );
  auto ptr = std::bit_cast<const PT *>(arr.data());
  SB sb(ptr, arr.size());
  REQUIRE_FALSE (sb.empty());
  chops::repeat(static_cast<int>(arr.size()), [&sb, arr] (int i) { REQUIRE(*(sb.data()+i) == arr[i]); } );
  return sb;
}

template <typename PT>
void generic_pointer_append_test() {
  auto sb { generic_pointer_construction_test<chops::mutable_shared_buffer, PT>() };
  auto sav_sz { sb.size() };
  const PT arr[] { 5, 6, 7 };
  sb.append (arr, 3);
  REQUIRE (sb.size() == (sav_sz + 3));
//  sb.append (std::span<PT>(arr, 3));
//  REQUIRE (sb.size() == (sav_sz + 6));
}

template <typename SB>
void common_methods_test(const std::byte* buf, typename SB::size_type sz) {

  REQUIRE (sz > 2);

  SB sb(buf, sz);
  REQUIRE_FALSE (sb.empty());
  {
    SB sb2(buf, sz);
    REQUIRE_FALSE (sb2.empty());
    REQUIRE (sb == sb2);
  }
  {
    std::list<std::byte> lst (buf, buf+sz);
    SB sb2(lst.cbegin(), lst.cend());
    REQUIRE_FALSE (sb2.empty());
    REQUIRE (sb == sb2);
  }
  {
    auto ba = chops::make_byte_array(buf[0], buf[1]);
    SB sb2(ba.cbegin(), ba.cend());
    REQUIRE_FALSE (sb2.empty());
    REQUIRE (((sb2 < sb) != 0)); // uses spaceship operator
    REQUIRE (sb2 != sb);
  }
  {
    auto ba = chops::make_byte_array(0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
    SB sb2(ba.cbegin(), ba.cend());
    REQUIRE_FALSE (sb2.empty());
    REQUIRE (sb2 != sb);
  }
}

template <typename SB>
void byte_vector_move_test() {

  auto arr = chops::make_byte_array (0x01, 0x02, 0x03, 0x04, 0x05);

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

                     
TEMPLATE_TEST_CASE ( "Shared buffer common methods",
                     "[const_shared_buffer] [common]",
                     chops::mutable_shared_buffer, chops::const_shared_buffer ) {
  auto arr = chops::make_byte_array ( 80, 81, 82, 83, 84, 90, 91, 92 );
  common_methods_test<TestType>(arr.data(), arr.size());
}

TEST_CASE ( "Mutable shared buffer copy construction and assignment",
            "[mutable_shared_buffer] [copy]" ) {

  auto arr = chops::make_byte_array ( 80, 81, 82, 83, 84, 90, 91, 92 );

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

  chops::mutable_shared_buffer sb;

  sb.resize(N);
  REQUIRE (sb.size() == N);
  chops::repeat(N, [&sb] (int i) { REQUIRE (*(sb.data() + i) == std::byte{0} ); } );

  SECTION ( "Compare two resized mutable shared buffer with same size" ) {
    chops::mutable_shared_buffer sb2(N);
    REQUIRE (sb == sb2);
    chops::repeat(N, [&sb, &sb2] (int i) {
      REQUIRE (*(sb.data() + i) == std::byte{0} );
      REQUIRE (*(sb2.data() + i) == std::byte{0} );
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

  REQUIRE (*(sb1.data()+0) == *(arr2.data()+0));
  REQUIRE (*(sb1.data()+1) == *(arr2.data()+1));
  REQUIRE (*(sb1.data()+2) == *(arr2.data()+2));
  REQUIRE (*(sb1.data()+3) == *(arr2.data()+3));
  REQUIRE (*(sb1.data()+4) == *(arr2.data()+4));

  REQUIRE (*(sb2.data()+0) == *(arr1.data()+0));
  REQUIRE (*(sb2.data()+1) == *(arr1.data()+1));
  REQUIRE (*(sb2.data()+2) == *(arr1.data()+2));
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
  REQUIRE (r == bv);
  r[0] = std::byte(0xdd);
  REQUIRE_FALSE (r == bv);
}

