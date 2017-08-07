// #include <iostream>
// #include <sstream>
// #include <string.h>
// 
// #include "coder.h"
// #include "stat_adaptive.h"
// 
// int main()
// {
// 	arith::AdaptiveStatisticsModule<> stat(256);
// 	for (int i = 0; i < 256; ++i) stat.init(i);
// 
// 	arith::AdaptiveStatisticsModule<> stat2(256);
// 	for (int i = 0; i < 256; ++i) stat2.init(i);
// 
// 	int len = 100;
// 	char *data = new char[len];
// 	const char *src = "Hello, my name is Max!";
// 	for (int i = 0; i < len; ++i) {
// 		data[i] = src[i % strlen(src)];
// 	}
// 
// 	std::stringstream str;
// 
// 	arith::Encoder<> e(str);
// 	for (int i = 0; i < len; ++i) {
// 		e(stat, data[i]);
// 		stat.inc(data[i]);
// 	}
// 	e.flush();
// 	std::cout << "Encoded" << std::endl;
// 
// 	arith::Decoder<> d(str);
// 	for (int i = 0; i < len; ++i) {
// 		char c = d(stat2);
// 		stat2.inc(c);
// 		std::cout << c;
// 	}
// 	std::cout << std::endl;
// 
// 	std::cout << "Size: " << str.tellp() << std::endl;
// }

#include <array>
#include <iostream>

struct test {
	int b;
	test(int a) : b(a)
	{
	}
};
int main()
{
// 	std::array<test, 10> arr1;
// 	arr1.fill(1);

	test arr2[10] = { test(1) };
	for (int i = 0; i < 10; ++i) {
		std::cout << /*arr1[i] << " " <<*/ arr2[i] << std::endl;
	}
}
