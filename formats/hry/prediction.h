#pragma once

#include "transform.h"

namespace pred {

template <typename T>
int quant2bits(int q)
{
	return q == 0 ? sizeof(T) << 3 : q;
}


template <class T>
T mask(int bits)
{
	return bits == (sizeof(T) << 3) ? T(-1) : T((1 << bits) - 1);
}

// template <class T>
// T decodeDelta(const T delta, const T pred, const T min, const T max)
// {
//        const T delta_mask[2] = { T(0), ~T(0) };
//        const T max_pos = max - pred;
//        const T max_neg = pred - min;
//        // this is a corner case where the whole range is positive only
//        if (max_neg == T(0)) return pred + delta;
//  
//        const T balanced_max = std::min(max_neg - T(1), max_pos);
//        if ((delta >> 1) > balanced_max)
//        {
//               if (max_pos >= max_neg)
//               {
//                      return pred + delta - balanced_max - T(1);
//               }
//               else
//               {
//                      return pred - delta + balanced_max;
//               }
//        }
//        return pred + (T(delta >> 1) ^ delta_mask[delta & 1]);
// }
//  
// template <class T>
// T encodeDelta(const T raw, const T pred, const T min, const T max)
// {
//        const T max_pos = max - pred;
//        const T max_neg = pred - min;
//        // this is a corner case where the whole range is positive only
//        if (max_neg == T(0)) return raw - min;
//  
//        const T balanced_max = std::min(max_neg - T(1), max_pos);
//        if (raw < pred)
//        {
//               const T dlt = pred - raw;
//               if (dlt > balanced_max) return dlt + balanced_max;
//               // dlt can never be 0 here
//               return T(dlt << 1) - 1;
//        }
//        else
//        {
//               const T dlt = raw - pred;
//               if (dlt > balanced_max) return dlt + balanced_max;
//               return T(dlt << 1);
//        }
// }


static const uint64_t masks[] = { 0x80ull, 0x8000ull, 0, 0x80000000ull, 0, 0, 0, 0x8000000000000000ull };

template <typename T>
transform::uint_t<sizeof(T)> int2uint(transform::int_t<sizeof(T)> val)
{
	return ((transform::uint_t<sizeof(T)>)val) ^ masks[sizeof(T)];
}
template <typename T>
transform::int_t<sizeof(T)> uint2int(transform::uint_t<sizeof(T)> val)
{
	return ((transform::int_t<sizeof(T)>)val) ^ masks[sizeof(T)];
}
 
template <class T>
T decodeDelta(const T delta, const T pred, const int bits, std::false_type)
{
       const T delta_mask[2] = { T(0), ~T(0) };
       const T max_pos = mask<T>(bits) - pred;
       // this is a corner case where the whole range is positive only
       if (pred == T(0)) return delta;
 
       const T balanced_max = std::min(T(pred - T(1)), max_pos);
// 	  std::cout << "bm: " << balanced_max << std::endl;
       if ((delta >> 1) > balanced_max)
       {
              if (max_pos >= pred)
              {
                     return pred + delta - balanced_max - T(1);
              }
              else
              {
                     return pred - delta + balanced_max;
              }
       }
       return pred + (T(delta >> 1) ^ delta_mask[delta & 1]);
}
template <class T>
T decodeDelta(const T delta, const T pred, const int bits, std::true_type)
{
	transform::int_t<sizeof(T)> predint = transform::float2int(pred);
	transform::uint_t<sizeof(T)> deltaint = *((transform::uint_t<sizeof(T)>*)&delta);
	transform::int_t<sizeof(T)> res = uint2int<T>(decodeDelta(deltaint, int2uint<T>(predint), bits, std::false_type()));
	T res2 = transform::int2float(res);
	return res2;
}

template <class T>
T decodeDelta(const T delta, const T pred, const int q)
{
	return decodeDelta(delta, pred, quant2bits<T>(q), std::is_floating_point<T>());
}

 
template <class T>
T encodeDelta(const T raw, const T pred,  int bits, std::false_type)
{
       const T max_pos = mask<T>(bits) - pred;
       // this is a corner case where the whole range is positive only
       if (pred == T(0)) return raw;
 
       const T balanced_max = std::min(T(pred), max_pos);
       if (raw < pred)
       {
              const T dlt = pred - raw;
              if (dlt > balanced_max) return dlt + balanced_max;
              // dlt can never be 0 here
              return T(dlt << 1) - 1;
       }
       else
       {
              const T dlt = raw - pred;
              if (dlt > balanced_max) return dlt + balanced_max;
              return T(dlt << 1);
       }
}

template <class T>
T encodeDelta(const T raw, const T pred, const int bits, std::true_type)
{
	transform::int_t<sizeof(T)> predint = transform::float2int(pred);
	transform::int_t<sizeof(T)> rawint = transform::float2int(raw);
	transform::uint_t<sizeof(T)> res = encodeDelta(int2uint<T>(rawint), int2uint<T>(predint), bits, std::false_type());
	T r = *((T*)&res);

	assert_eq(raw, decodeDelta(r, pred, bits));
	
	return r;
}
template <class T>
T encodeDelta(const T raw, const T pred, const int q)
{
	return encodeDelta(raw, pred, quant2bits<T>(q), std::is_floating_point<T>());
}
 
// template <class T>
// T predict(const int bits)
// {
//        return T(1 << (bits - 1));
// }
//  
// template <class T>
// T predict(const T v0, const int bits)
// {
//        return v0;
// }
//  
// template <class T>
// T predict(const T v0, const T v1, const int bits)
// {
//        return (v0 >> 1) + (v1 >> 1) + (((v0 & 1) + (v1 & 1)) >> 1);
// }

 
template <class T>
T predict(const T v0, const T v1, const T v2, const int bits, std::false_type)
{
       const T max = mask<T>(bits);
       if (v1 < v2)
       {
              // v0 - (v2 - v1)
              const T tmp = v2 - v1;
              if (tmp > v0) return T(0);
              else return v0 - tmp;
       }
       else
       {
              // v0 + (v1 - v2)
              const T tmp = v1 - v2;
              const T v = v0 + tmp;
              if ((v > max) || (v < v0)) return max;
              return v;
       }
}
template <class T>
T predict(const T v0, const T v1, const T v2, const int bits, std::true_type)
{
// 	T pred = v0 + (v1-v2);
	return v0 + (v1-v2);
}
template <class T>
T predict(const T v0, const T v1, const T v2, const int q)
{
	return predict(v0, v1, v2, quant2bits<T>(q), std::is_floating_point<T>());
}

template <typename T>
T predict_face(const T v0, const int q, std::false_type)
{
	return v0;
}
template <typename T>
T predict_face(const T v0, const int q, std::true_type)
{
	return v0;
}

template <typename T>
T predict_face(const T v0, const int q)
{
	return predict_face(v0, q, std::is_floating_point<T>());
}

// template <class T>
// T predictRange(const T min, const T max)
// {
//        return (min >> 1) + (max >> 1) + (((min & 1) + (max & 1)) >> 1);
// }
//  
// template <class T>
// T predictRange(const T v0, const T min, const T max)
// {
//        return v0;
// }
 
// template <class T>
// T predictRange(const T v0, const T v1, const T min, const T max)
// {
//        return (v0 >> 1) + (v1 >> 1) + (((v0 & 1) + (v1 & 1)) >> 1);
// }
//  
// template <class T>
// T predictRange(const T v0, const T v1, const T v2, const T min, const T max)
// {
//        if (v1 < v2)
//        {
//               // v0 - (v2 - v1)
//               const T tmp = v2 - v1;
//               if (tmp > v0 - min) return min;
//               else return v0 - tmp;
//        }
//        else
//        {
//               // v0 + (v1 - v2)
//               const T tmp = v1 - v2;
//               const T v = v0 + tmp;
//               if ((v > max) || (v < v0)) return max;
//               return v;
//        }
// }
 
// // encoding (full range of 0 to (1 << bits) - 1)
// template <class T> T encode(const T raw, const int bits)                                     { return encodeDelta(raw, predict<T>(bits),          bits); }
// template <class T> T encode(const T raw, const T v0, const int bits)                         { return encodeDelta(raw, predict(v0, bits),         bits); }
// template <class T> T encode(const T raw, const T v0, const T v1, const int bits)             { return encodeDelta(raw, predict(v0, v1, bits),     bits); }
// int maxdel = 0;
// template <class T> T encode(const T raw, const T v0, const T v1, const T v2, const int bits, std::false_type) { T d = encodeDelta(raw, predict(v0, v1, v2, bits), bits); /*maxdel = std::max((int)d, maxdel); std::cout << maxdel << std::endl;*/ /*std::cout << d << std::endl; */return d; }
// template <class T> T encode(const T raw, const T v0, const T v1, const T v2, const int bits, std::true_type) {
// // 	T pred = v0 + v1 - v2;
// 	T pred = v0 + (v1-v2);
// 	transform::int_t<sizeof(T)> predint = float2int(pred);
// 	transform::int_t<sizeof(T)> rawint = float2int(raw);
// 	transform::int_t<sizeof(T)> res = encodeDelta((transform::uint_t<sizeof(T)>)rawint, (transform::uint_t<sizeof(T)>)predint, bits);
// // 	transform::int_t<sizeof(T)> res = zigzag_encode(rawint - predint);
// 	return *((T*)&res);
// 	
// }
// template <class T> T encode(const T raw, const T v0, const T v1, const T v2, const int bits) { return encode(raw, v0, v1, v2, bits, std::is_floating_point<T>()); }
//  
// // encoding (range min to max)
// template <class T> T encodeRange(const T raw, const T min, const T max)                                     { return encodeDelta(raw, predictRange(min, max),             min, max); }
// template <class T> T encodeRange(const T raw, const T v0, const T min, const T max)                         { return encodeDelta(raw, predictRange(v0, min, max),         min, max); }
// template <class T> T encodeRange(const T raw, const T v0, const T v1, const T min, const T max)             { return encodeDelta(raw, predictRange(v0, v1, min, max),     min, max); }
// template <class T> T encodeRange(const T raw, const T v0, const T v1, const T v2, const T min, const T max) { return encodeDelta(raw, predictRange(v0, v1, v2, min, max), min, max); }
//  
// // decoding (full range of 0 to (1 << bits) - 1)
// template <class T> T decode(const T delta, const int bits)                                     { return decodeDelta(delta, predict<T>(bits),          bits); }
// template <class T> T decode(const T delta, const T v0, const int bits)                         { return decodeDelta(delta, predict(v0, bits),         bits); }
// template <class T> T decode(const T delta, const T v0, const T v1, const int bits)             { return decodeDelta(delta, predict(v0, v1, bits),     bits); }
// template <class T> T decode(const T delta, const T v0, const T v1, const T v2, const int bits, std::false_type) { return decodeDelta(delta, predict(v0, v1, v2, bits), bits); }
// template <class T> T decode(const T delta, const T v0, const T v1, const T v2, const int bits, std::true_type) {
// 	// 	T pred = v0 + v1 - v2;
// 	T pred = v0 + (v1-v2);
// 	transform::int_t<sizeof(T)> predint = float2int(pred);
// 	transform::uint_t<sizeof(T)> deltaint = *((transform::uint_t<sizeof(T)>*)&delta);
// 	transform::int_t<sizeof(T)> res = decodeDelta(deltaint, (transform::uint_t<sizeof(T)>)predint, bits);
// 	transform::int_t<sizeof(T)> res2 = int2float(res);
// // 	transform::int_t<sizeof(T)> res = zigzag_encode(rawint - predint);
// 	return *((T*)&res2);
// 	
// }
// template <class T> T decode(const T delta, const T v0, const T v1, const T v2, const int bits) { return decode(delta, v0, v1, v2, bits, std::is_floating_point<T>()); }
// 
// // decoding (range min to max)
// template <class T> T decodeRange(const T delta, const T min, const T max)                                     { return decodeDelta(delta, predictRange(min, max),             min, max); }
// template <class T> T decodeRange(const T delta, const T v0, const T min, const T max)                         { return decodeDelta(delta, predictRange(v0, min, max),         min, max); }
// template <class T> T decodeRange(const T delta, const T v0, const T v1, const T min, const T max)             { return decodeDelta(delta, predictRange(v0, v1, min, max),     min, max); }
// template <class T> T decodeRange(const T delta, const T v0, const T v1, const T v2, const T min, const T max) { return decodeDelta(delta, predictRange(v0, v1, v2, min, max), min, max); }

}
