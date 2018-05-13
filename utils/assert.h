/*
 * Copyright (C) 2017, Max von Buelow
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#pragma once

#include <cstdlib>

// #define DBG

#define HAVE_ASSERT

#ifdef DBG
#define assert(C) do { if (!(C)) { std::cerr << "Assertion \"" << #C << "\" failed on line " << __LINE__ << " " << __FILE__ << std::endl; assert::err(); } } while (0)
#define assert_lt(A, B) do { if (!((A) < (B))) { std::cerr << "Assertion \"" << #A << "\" (" << (A) << ") < \"" << #B << "\" (" << (B) << ") failed on line " << __LINE__ << " " << __FILE__ << std::endl; assert::err(); } } while (0)
#define assert_le(A, B) do { if (!((A) <= (B))) { std::cerr << "Assertion \"" << #A << "\" (" << (A) << ") <= \"" << #B << "\" (" << (B) << ") failed on line " << __LINE__ << " " << __FILE__ << std::endl; assert::err(); } } while (0)
#define assert_gt(A, B) do { if (!((A) > (B))) { std::cerr << "Assertion \"" << #A << "\" (" << (A) << ") > \"" << #B << "\" (" << (B) << ") failed on line " << __LINE__ << " " << __FILE__ << std::endl; assert::err(); } } while (0)
#define assert_ge(A, B) do { if (!((A) >= (B))) { std::cerr << "Assertion \"" << #A << "\" (" << (A) << ") >= \"" << #B << "\" (" << (B) << ") failed on line " << __LINE__ << " " << __FILE__ << std::endl; assert::err(); } } while (0)
#define assert_eq(A, B) do { if (!((A) == (B))) { std::cerr << "Assertion \"" << #A << "\" (" << (A) << ") == \"" << #B << "\" (" << (B) << ") failed on line " << __LINE__ << " " << __FILE__ << std::endl; assert::err(); } } while (0)
#define assert_ne(A, B) do { if (!((A) != (B))) { std::cerr << "Assertion \"" << #A << "\" (" << (A) << ") != \"" << #B << "\" (" << (B) << ") failed on line " << __LINE__ << " " << __FILE__ << std::endl; assert::err(); } } while (0)
#define assert_fail do { std::cerr << "False assertion on line " << __LINE__ << " " << __FILE__ << std::endl; assert::err(); } while (0)
#else
#define assert(C) ((void)0)
#define assert_lt(A, B) ((void)0)
#define assert_le(A, B) ((void)0)
#define assert_gt(A, B) ((void)0)
#define assert_ge(A, B) ((void)0)
#define assert_eq(A, B) ((void)0)
#define assert_ne(A, B) ((void)0)
#define assert_fail ((void)0)
#endif

namespace assert {

inline void err()
{
	std::exit(EXIT_FAILURE);
}

}

