Arithmetic Coder
======

This is an implementation of a Arithmetic Coder, which is based on the description of Moffat et. al. [1998].

Usage Example
------

```
#include <iostream>
#include <sstream>

#include "coder.h"
#include "stat_adaptive.h"

void encode(std::ostream &os, const std::vector<char> &src)
{
	arith::AdaptiveStatisticsModule<> stat(256); // create statistics module with 256 symbols
	for (int i = 0; i < 256; ++i) stat.init(i); // initialize all possible symbols (all probabilities will be distributed uniformly)

	arith::Encoder<> e(os);
	for (int i = 0; i < src.size(); ++i) {
		e(stat, (unsigned char)src[i]); // encode the symbol
		stat.inc((unsigned char)src[i]); // increment the frequency of that symbol
	}
	e.flush(); // flush all remaining bits
}

void decode(std::istream &is, std::vector<char> &dst)
{
	arith::AdaptiveStatisticsModule<> stat(256);
	for (int i = 0; i < 256; ++i) stat.init(i);

	arith::Decoder<> d(is);
	for (int i = 0; i < dst.size(); ++i) {
		unsigned char c = d(stat);
		stat.inc((unsigned char)c);
		dst[i] = c;
	}
}


int main()
{
	std::string mystr = "Hello, my name is Max!";
	std::vector<char> data(mystr.begin(), mystr.end());

	std::stringstream stream;
	encode(stream, data);

	std::vector<char> dst(data.size());
	decode(stream, dst);

	for (int i = 0; i < data.size(); ++i) {
		if (data[i] != dst[i]) std::cout << "ERROR" << std::endl;
	}

	std::cout << "Original size: " << data.size() << " Bytes" << std::endl;
	std::cout << "Encoded size: " << stream.tellp() << " Bytes" << std::endl;
}
```
