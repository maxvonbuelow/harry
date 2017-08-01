#pragma once

#include <functional>
#include <chrono>
#include <thread>

#define assert(C) do { if (!(C)) { std::cerr << "Assertion \"" << #C << "\" failed on line " << __LINE__ << " " << __FILE__ << std::endl; AssertMngr::err(); } } while(0)
#define assert_lt(A, B) do { if (!((A) < (B))) { std::cerr << "Assertion \"" << #A << "\" (" << (A) << ") < \"" << #B << "\" (" << (B) << ") failed on line " << __LINE__ << " " << __FILE__ << std::endl; AssertMngr::err(); } } while(0)
#define assert_le(A, B) do { if (!((A) <= (B))) { std::cerr << "Assertion \"" << #A << "\" (" << (A) << ") <= \"" << #B << "\" (" << (B) << ") failed on line " << __LINE__ << " " << __FILE__ << std::endl; AssertMngr::err(); } } while(0)
#define assert_gt(A, B) do { if (!((A) > (B))) { std::cerr << "Assertion \"" << #A << "\" (" << (A) << ") > \"" << #B << "\" (" << (B) << ") failed on line " << __LINE__ << " " << __FILE__ << std::endl; AssertMngr::err(); } } while(0)
#define assert_ge(A, B) do { if (!((A) >= (B))) { std::cerr << "Assertion \"" << #A << "\" (" << (A) << ") >= \"" << #B << "\" (" << (B) << ") failed on line " << __LINE__ << " " << __FILE__ << std::endl; AssertMngr::err(); } } while(0)
#define assert_eq(A, B) do { if (!((A) == (B))) { std::cerr << "Assertion \"" << #A << "\" (" << (A) << ") == \"" << #B << "\" (" << (B) << ") failed on line " << __LINE__ << " " << __FILE__ << std::endl; AssertMngr::err(); } } while(0)
#define assert_ne(A, B) do { if (!((A) != (B))) { std::cerr << "Assertion \"" << #A << "\" (" << (A) << ") != \"" << #B << "\" (" << (B) << ") failed on line " << __LINE__ << " " << __FILE__ << std::endl; AssertMngr::err(); } } while(0)
#define assert_fail do { std::cerr << "False assertion on line " << __LINE__ << " " << __FILE__ << std::endl; AssertMngr::err(); } while(0)

#define bigassert(EXP) EXP

using namespace std::chrono_literals;

struct AssertMngr {
	static std::function<void(void)> f;

	static void err()
	{
		if (f) f();
	}

	template <typename T>
	static void set(T &&h)
	{
		f = h;
	}

	static void keepalive()
	{
		while (1) {
			std::this_thread::sleep_for(512ms);
		}
	}
};
