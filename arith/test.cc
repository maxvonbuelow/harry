#include <iostream>
#include <sstream>
#include <string.h>

#include "coder.h"
#include "stat_adaptive.h"

int main()
{
	arith::AdaptiveStatisticsModule<> stat(256);
	for (int i = 0; i < 256; ++i) stat.init(i);

	arith::AdaptiveStatisticsModule<> stat2(256);
	for (int i = 0; i < 256; ++i) stat2.init(i);

	int len = 10000000;
	char *data = new char[len];
	const char *src = "Hello, my name is Max!#NIP3ifn)H#f08hf108hp0HF3";
	for (int i = 0; i < len; ++i) {
		data[i] = (src[i % strlen(src)] + i) % 256;
	}

	std::stringstream str;

	arith::Encoder<> e(str);
	for (int i = 0; i < len; ++i) {
		e(stat, (unsigned char)data[i]);
		stat.inc((unsigned char)data[i]);
	}
	e.flush();
	std::cout << "Encoded" << std::endl;

	arith::Decoder<> d(str);
	char *dst = new char[len];
	for (int i = 0; i < len; ++i) {
		unsigned char c = d(stat2);
		stat2.inc((unsigned char)c);
// 		std::cout << c;
		dst[i] = c;
	}
// 	std::cout << std::endl;
	for (int i = 0; i < len; ++i) {
		if (data[i] != dst[i]) std::cout << "ERROR" << std::endl;
	}

	std::cout << "Size: " << str.tellp() << std::endl;
}

// #include <array>
// #include <iostream>
// 
// struct test {
// 	int b;
// 	test(int a) : b(a)
// 	{
// 	}
// };
// int main()
// {
// // 	std::array<test, 10> arr1;
// // 	arr1.fill(1);
// 
// 	test arr2[10] = { test(1) };
// 	for (int i = 0; i < 10; ++i) {
// 		std::cout << /*arr1[i] << " " <<*/ arr2[i] << std::endl;
// 	}
// }
