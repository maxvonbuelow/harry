
#line 1 "formats/obj/reader.rl"
/*
 * Copyright (C) 2017, Max von Buelow
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <stdexcept>
#include <vector>
#include <string>
#include <cmath>

#include "reader.h"

#include "structs/mixing.h"
#include "structs/types.h"
#include "structs/quant.h"
#include "utils/io.h"
#include "utils/progress.h"

#define BUFSIZE 16384


#line 74 "formats/obj/reader.rl"


namespace obj {
namespace reader {


#line 36 "/home/max/repos/harry/formats/obj/reader.cc"
static const char _ObjParser_actions[] = {
	0, 1, 0, 1, 2, 1, 3, 1, 
	4, 1, 12, 1, 18, 1, 19, 1, 
	20, 1, 21, 1, 22, 1, 23, 1, 
	24, 2, 0, 1, 2, 0, 3, 2, 
	6, 17, 2, 7, 17, 2, 8, 17, 
	2, 9, 10, 2, 11, 12, 2, 13, 
	14, 2, 13, 15, 2, 13, 16, 2, 
	18, 0, 2, 19, 24, 2, 20, 24, 
	2, 21, 24, 2, 23, 24, 3, 5, 
	6, 17, 3, 9, 11, 12, 3, 13, 
	15, 16, 3, 18, 0, 1, 3, 18, 
	0, 3
};

static const short _ObjParser_key_offsets[] = {
	0, 0, 4, 5, 7, 9, 14, 16, 
	21, 26, 28, 35, 42, 44, 45, 46, 
	47, 48, 49, 50, 51, 52, 53, 54, 
	58, 69, 72, 74, 80, 91, 94, 96, 
	102, 113, 116, 118, 126, 139, 140, 143, 
	145, 153, 157, 159, 161, 167, 176, 178, 
	180, 186, 188, 190, 192, 194, 198, 200, 
	202, 206, 208, 210, 216, 225, 227, 229, 
	235, 237, 239, 241, 243, 247, 249, 251, 
	255, 257, 259, 263, 270, 272, 274, 278, 
	280, 282, 284, 286, 288, 290, 292, 294, 
	296, 298, 302, 309, 311, 313, 317, 319, 
	321, 323, 325, 327, 329, 331, 333, 335, 
	346, 349, 351, 357, 368, 371, 373, 379, 
	390, 393, 395, 403, 407, 408, 410, 412, 
	418, 427, 429, 431, 437, 439, 441, 443, 
	445, 449, 451, 453, 457, 459, 461, 465, 
	472, 474, 476, 480, 482, 484, 486, 488, 
	490, 492, 494, 496, 498, 500, 504, 511, 
	513, 515, 519, 521, 523, 525, 527, 529, 
	531, 533, 535, 537, 548, 551, 553, 559, 
	570, 573, 575, 583, 596, 597, 600, 602, 
	610, 614, 616, 618, 624, 633, 635, 637, 
	643, 645, 647, 649, 651, 655, 657, 659, 
	663, 665, 667, 673, 682, 684, 686, 692, 
	694, 696, 698, 700, 704, 706, 708, 712, 
	714, 716, 720, 727, 729, 731, 735, 737, 
	739, 741, 743, 745, 747, 749, 751, 752, 
	760, 762, 769, 776, 778, 784, 790, 792, 
	797, 802, 804, 808, 820, 832, 844, 856
};

static const char _ObjParser_trans_keys[] = {
	9, 10, 13, 32, 10, 10, 13, 9, 
	32, 9, 32, 45, 48, 57, 48, 57, 
	9, 32, 47, 48, 57, 9, 32, 45, 
	48, 57, 48, 57, 9, 10, 13, 32, 
	47, 48, 57, 9, 10, 13, 32, 45, 
	48, 57, 9, 32, 116, 108, 108, 105, 
	98, 115, 101, 109, 116, 108, 9, 32, 
	110, 116, 9, 32, 43, 45, 46, 73, 
	78, 105, 110, 48, 57, 46, 48, 57, 
	48, 57, 9, 32, 69, 101, 48, 57, 
	9, 32, 43, 45, 46, 73, 78, 105, 
	110, 48, 57, 46, 48, 57, 48, 57, 
	9, 32, 69, 101, 48, 57, 9, 32, 
	43, 45, 46, 73, 78, 105, 110, 48, 
	57, 46, 48, 57, 48, 57, 9, 10, 
	13, 32, 69, 101, 48, 57, 9, 10, 
	13, 32, 43, 45, 46, 73, 78, 105, 
	110, 48, 57, 10, 46, 48, 57, 48, 
	57, 9, 10, 13, 32, 69, 101, 48, 
	57, 9, 10, 13, 32, 43, 45, 48, 
	57, 9, 10, 13, 32, 48, 57, 9, 
	10, 13, 32, 46, 69, 101, 48, 57, 
	78, 110, 70, 102, 9, 10, 13, 32, 
	73, 105, 78, 110, 73, 105, 84, 116, 
	89, 121, 9, 10, 13, 32, 65, 97, 
	78, 110, 9, 10, 13, 32, 43, 45, 
	48, 57, 9, 10, 13, 32, 48, 57, 
	9, 10, 13, 32, 46, 69, 101, 48, 
	57, 78, 110, 70, 102, 9, 10, 13, 
	32, 73, 105, 78, 110, 73, 105, 84, 
	116, 89, 121, 9, 10, 13, 32, 65, 
	97, 78, 110, 9, 10, 13, 32, 43, 
	45, 48, 57, 9, 32, 48, 57, 9, 
	32, 46, 69, 101, 48, 57, 78, 110, 
	70, 102, 9, 32, 73, 105, 78, 110, 
	73, 105, 84, 116, 89, 121, 9, 32, 
	65, 97, 78, 110, 9, 32, 43, 45, 
	48, 57, 9, 32, 48, 57, 9, 32, 
	46, 69, 101, 48, 57, 78, 110, 70, 
	102, 9, 32, 73, 105, 78, 110, 73, 
	105, 84, 116, 89, 121, 9, 32, 65, 
	97, 78, 110, 9, 32, 9, 32, 9, 
	32, 43, 45, 46, 73, 78, 105, 110, 
	48, 57, 46, 48, 57, 48, 57, 9, 
	32, 69, 101, 48, 57, 9, 32, 43, 
	45, 46, 73, 78, 105, 110, 48, 57, 
	46, 48, 57, 48, 57, 9, 32, 69, 
	101, 48, 57, 9, 32, 43, 45, 46, 
	73, 78, 105, 110, 48, 57, 46, 48, 
	57, 48, 57, 9, 10, 13, 32, 69, 
	101, 48, 57, 9, 10, 13, 32, 10, 
	43, 45, 48, 57, 9, 10, 13, 32, 
	48, 57, 9, 10, 13, 32, 46, 69, 
	101, 48, 57, 78, 110, 70, 102, 9, 
	10, 13, 32, 73, 105, 78, 110, 73, 
	105, 84, 116, 89, 121, 9, 10, 13, 
	32, 65, 97, 78, 110, 9, 10, 13, 
	32, 43, 45, 48, 57, 9, 32, 48, 
	57, 9, 32, 46, 69, 101, 48, 57, 
	78, 110, 70, 102, 9, 32, 73, 105, 
	78, 110, 73, 105, 84, 116, 89, 121, 
	9, 32, 65, 97, 78, 110, 9, 32, 
	43, 45, 48, 57, 9, 32, 48, 57, 
	9, 32, 46, 69, 101, 48, 57, 78, 
	110, 70, 102, 9, 32, 73, 105, 78, 
	110, 73, 105, 84, 116, 89, 121, 9, 
	32, 65, 97, 78, 110, 9, 32, 9, 
	32, 9, 32, 43, 45, 46, 73, 78, 
	105, 110, 48, 57, 46, 48, 57, 48, 
	57, 9, 32, 69, 101, 48, 57, 9, 
	32, 43, 45, 46, 73, 78, 105, 110, 
	48, 57, 46, 48, 57, 48, 57, 9, 
	10, 13, 32, 69, 101, 48, 57, 9, 
	10, 13, 32, 43, 45, 46, 73, 78, 
	105, 110, 48, 57, 10, 46, 48, 57, 
	48, 57, 9, 10, 13, 32, 69, 101, 
	48, 57, 9, 10, 13, 32, 43, 45, 
	48, 57, 9, 10, 13, 32, 48, 57, 
	9, 10, 13, 32, 46, 69, 101, 48, 
	57, 78, 110, 70, 102, 9, 10, 13, 
	32, 73, 105, 78, 110, 73, 105, 84, 
	116, 89, 121, 9, 10, 13, 32, 65, 
	97, 78, 110, 9, 10, 13, 32, 43, 
	45, 48, 57, 9, 10, 13, 32, 48, 
	57, 9, 10, 13, 32, 46, 69, 101, 
	48, 57, 78, 110, 70, 102, 9, 10, 
	13, 32, 73, 105, 78, 110, 73, 105, 
	84, 116, 89, 121, 9, 10, 13, 32, 
	65, 97, 78, 110, 9, 10, 13, 32, 
	43, 45, 48, 57, 9, 32, 48, 57, 
	9, 32, 46, 69, 101, 48, 57, 78, 
	110, 70, 102, 9, 32, 73, 105, 78, 
	110, 73, 105, 84, 116, 89, 121, 9, 
	32, 65, 97, 78, 110, 9, 32, 10, 
	9, 10, 13, 32, 45, 47, 48, 57, 
	48, 57, 9, 10, 13, 32, 47, 48, 
	57, 9, 10, 13, 32, 45, 48, 57, 
	48, 57, 9, 10, 13, 32, 48, 57, 
	9, 32, 45, 47, 48, 57, 48, 57, 
	9, 32, 47, 48, 57, 9, 32, 45, 
	48, 57, 48, 57, 9, 32, 48, 57, 
	9, 10, 13, 32, 35, 102, 103, 109, 
	111, 115, 117, 118, 9, 10, 13, 32, 
	35, 102, 103, 109, 111, 115, 117, 118, 
	9, 10, 13, 32, 35, 102, 103, 109, 
	111, 115, 117, 118, 9, 10, 13, 32, 
	35, 102, 103, 109, 111, 115, 117, 118, 
	9, 10, 13, 32, 35, 102, 103, 109, 
	111, 115, 117, 118, 0
};

static const char _ObjParser_single_lengths[] = {
	0, 4, 1, 2, 2, 3, 0, 3, 
	3, 0, 5, 5, 2, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 4, 
	9, 1, 0, 4, 9, 1, 0, 4, 
	9, 1, 0, 6, 11, 1, 1, 0, 
	6, 4, 2, 0, 4, 7, 2, 2, 
	6, 2, 2, 2, 2, 4, 2, 2, 
	4, 2, 0, 4, 7, 2, 2, 6, 
	2, 2, 2, 2, 4, 2, 2, 4, 
	2, 0, 2, 5, 2, 2, 4, 2, 
	2, 2, 2, 2, 2, 2, 2, 2, 
	0, 2, 5, 2, 2, 4, 2, 2, 
	2, 2, 2, 2, 2, 2, 2, 9, 
	1, 0, 4, 9, 1, 0, 4, 9, 
	1, 0, 6, 4, 1, 2, 0, 4, 
	7, 2, 2, 6, 2, 2, 2, 2, 
	4, 2, 2, 4, 2, 0, 2, 5, 
	2, 2, 4, 2, 2, 2, 2, 2, 
	2, 2, 2, 2, 0, 2, 5, 2, 
	2, 4, 2, 2, 2, 2, 2, 2, 
	2, 2, 2, 9, 1, 0, 4, 9, 
	1, 0, 6, 11, 1, 1, 0, 6, 
	4, 2, 0, 4, 7, 2, 2, 6, 
	2, 2, 2, 2, 4, 2, 2, 4, 
	2, 0, 4, 7, 2, 2, 6, 2, 
	2, 2, 2, 4, 2, 2, 4, 2, 
	0, 2, 5, 2, 2, 4, 2, 2, 
	2, 2, 2, 2, 2, 2, 1, 6, 
	0, 5, 5, 0, 4, 4, 0, 3, 
	3, 0, 2, 12, 12, 12, 12, 12
};

static const char _ObjParser_range_lengths[] = {
	0, 0, 0, 0, 0, 1, 1, 1, 
	1, 1, 1, 1, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 0, 1, 1, 
	1, 0, 0, 1, 1, 1, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 1, 1, 1, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 1, 1, 1, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	1, 1, 1, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 0, 0, 0, 1, 1, 
	1, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 1, 1, 1, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 1, 1, 1, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 0, 1, 1, 1, 
	0, 0, 1, 1, 1, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 1, 1, 1, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	1, 1, 1, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 0, 0, 0, 0, 0
};

static const short _ObjParser_index_offsets[] = {
	0, 0, 5, 7, 10, 13, 18, 20, 
	25, 30, 32, 39, 46, 49, 51, 53, 
	55, 57, 59, 61, 63, 65, 67, 69, 
	74, 85, 88, 90, 96, 107, 110, 112, 
	118, 129, 132, 134, 142, 155, 157, 160, 
	162, 170, 175, 178, 180, 186, 195, 198, 
	201, 208, 211, 214, 217, 220, 225, 228, 
	231, 236, 239, 241, 247, 256, 259, 262, 
	269, 272, 275, 278, 281, 286, 289, 292, 
	297, 300, 302, 306, 313, 316, 319, 324, 
	327, 330, 333, 336, 339, 342, 345, 348, 
	351, 353, 357, 364, 367, 370, 375, 378, 
	381, 384, 387, 390, 393, 396, 399, 402, 
	413, 416, 418, 424, 435, 438, 440, 446, 
	457, 460, 462, 470, 475, 477, 480, 482, 
	488, 497, 500, 503, 510, 513, 516, 519, 
	522, 527, 530, 533, 538, 541, 543, 547, 
	554, 557, 560, 565, 568, 571, 574, 577, 
	580, 583, 586, 589, 592, 594, 598, 605, 
	608, 611, 616, 619, 622, 625, 628, 631, 
	634, 637, 640, 643, 654, 657, 659, 665, 
	676, 679, 681, 689, 702, 704, 707, 709, 
	717, 722, 725, 727, 733, 742, 745, 748, 
	755, 758, 761, 764, 767, 772, 775, 778, 
	783, 786, 788, 794, 803, 806, 809, 816, 
	819, 822, 825, 828, 833, 836, 839, 844, 
	847, 849, 853, 860, 863, 866, 871, 874, 
	877, 880, 883, 886, 889, 892, 895, 897, 
	905, 907, 914, 921, 923, 929, 935, 937, 
	942, 947, 949, 953, 966, 979, 992, 1005
};

static const unsigned char _ObjParser_trans_targs[] = {
	1, 235, 2, 1, 0, 235, 0, 235, 
	2, 3, 5, 5, 0, 5, 5, 6, 
	7, 0, 7, 0, 8, 8, 229, 7, 
	0, 8, 8, 9, 10, 0, 10, 0, 
	11, 236, 222, 11, 223, 10, 0, 11, 
	236, 222, 11, 9, 10, 0, 3, 3, 
	0, 14, 0, 15, 0, 16, 0, 17, 
	0, 12, 0, 19, 0, 20, 0, 21, 
	0, 22, 0, 12, 0, 24, 24, 102, 
	162, 0, 24, 24, 25, 25, 26, 91, 
	99, 91, 99, 90, 0, 26, 90, 0, 
	27, 0, 28, 28, 87, 87, 27, 0, 
	28, 28, 29, 29, 30, 76, 84, 76, 
	84, 75, 0, 30, 75, 0, 31, 0, 
	32, 32, 72, 72, 31, 0, 32, 32, 
	33, 33, 34, 61, 69, 61, 69, 60, 
	0, 34, 60, 0, 35, 0, 36, 237, 
	37, 36, 57, 57, 35, 0, 36, 237, 
	37, 36, 38, 38, 39, 46, 54, 46, 
	54, 45, 0, 237, 0, 39, 45, 0, 
	40, 0, 41, 237, 37, 41, 42, 42, 
	40, 0, 41, 237, 37, 41, 0, 43, 
	43, 0, 44, 0, 41, 237, 37, 41, 
	44, 0, 41, 237, 37, 41, 40, 42, 
	42, 45, 0, 47, 47, 0, 48, 48, 
	0, 41, 237, 37, 41, 49, 49, 0, 
	50, 50, 0, 51, 51, 0, 52, 52, 
	0, 53, 53, 0, 41, 237, 37, 41, 
	0, 55, 55, 0, 56, 56, 0, 41, 
	237, 37, 41, 0, 58, 58, 0, 59, 
	0, 36, 237, 37, 36, 59, 0, 36, 
	237, 37, 36, 35, 57, 57, 60, 0, 
	62, 62, 0, 63, 63, 0, 36, 237, 
	37, 36, 64, 64, 0, 65, 65, 0, 
	66, 66, 0, 67, 67, 0, 68, 68, 
	0, 36, 237, 37, 36, 0, 70, 70, 
	0, 71, 71, 0, 36, 237, 37, 36, 
	0, 73, 73, 0, 74, 0, 32, 32, 
	74, 0, 32, 32, 31, 72, 72, 75, 
	0, 77, 77, 0, 78, 78, 0, 32, 
	32, 79, 79, 0, 80, 80, 0, 81, 
	81, 0, 82, 82, 0, 83, 83, 0, 
	32, 32, 0, 85, 85, 0, 86, 86, 
	0, 32, 32, 0, 88, 88, 0, 89, 
	0, 28, 28, 89, 0, 28, 28, 27, 
	87, 87, 90, 0, 92, 92, 0, 93, 
	93, 0, 28, 28, 94, 94, 0, 95, 
	95, 0, 96, 96, 0, 97, 97, 0, 
	98, 98, 0, 28, 28, 0, 100, 100, 
	0, 101, 101, 0, 28, 28, 0, 103, 
	103, 0, 103, 103, 104, 104, 105, 151, 
	159, 151, 159, 150, 0, 105, 150, 0, 
	106, 0, 107, 107, 147, 147, 106, 0, 
	107, 107, 108, 108, 109, 136, 144, 136, 
	144, 135, 0, 109, 135, 0, 110, 0, 
	111, 111, 132, 132, 110, 0, 111, 111, 
	112, 112, 113, 121, 129, 121, 129, 120, 
	0, 113, 120, 0, 114, 0, 115, 238, 
	116, 115, 117, 117, 114, 0, 115, 238, 
	116, 115, 0, 238, 0, 118, 118, 0, 
	119, 0, 115, 238, 116, 115, 119, 0, 
	115, 238, 116, 115, 114, 117, 117, 120, 
	0, 122, 122, 0, 123, 123, 0, 115, 
	238, 116, 115, 124, 124, 0, 125, 125, 
	0, 126, 126, 0, 127, 127, 0, 128, 
	128, 0, 115, 238, 116, 115, 0, 130, 
	130, 0, 131, 131, 0, 115, 238, 116, 
	115, 0, 133, 133, 0, 134, 0, 111, 
	111, 134, 0, 111, 111, 110, 132, 132, 
	135, 0, 137, 137, 0, 138, 138, 0, 
	111, 111, 139, 139, 0, 140, 140, 0, 
	141, 141, 0, 142, 142, 0, 143, 143, 
	0, 111, 111, 0, 145, 145, 0, 146, 
	146, 0, 111, 111, 0, 148, 148, 0, 
	149, 0, 107, 107, 149, 0, 107, 107, 
	106, 147, 147, 150, 0, 152, 152, 0, 
	153, 153, 0, 107, 107, 154, 154, 0, 
	155, 155, 0, 156, 156, 0, 157, 157, 
	0, 158, 158, 0, 107, 107, 0, 160, 
	160, 0, 161, 161, 0, 107, 107, 0, 
	163, 163, 0, 163, 163, 164, 164, 165, 
	211, 219, 211, 219, 210, 0, 165, 210, 
	0, 166, 0, 167, 167, 207, 207, 166, 
	0, 167, 167, 168, 168, 169, 196, 204, 
	196, 204, 195, 0, 169, 195, 0, 170, 
	0, 171, 239, 172, 171, 192, 192, 170, 
	0, 171, 239, 172, 171, 173, 173, 174, 
	181, 189, 181, 189, 180, 0, 239, 0, 
	174, 180, 0, 175, 0, 176, 239, 172, 
	176, 177, 177, 175, 0, 176, 239, 172, 
	176, 0, 178, 178, 0, 179, 0, 176, 
	239, 172, 176, 179, 0, 176, 239, 172, 
	176, 175, 177, 177, 180, 0, 182, 182, 
	0, 183, 183, 0, 176, 239, 172, 176, 
	184, 184, 0, 185, 185, 0, 186, 186, 
	0, 187, 187, 0, 188, 188, 0, 176, 
	239, 172, 176, 0, 190, 190, 0, 191, 
	191, 0, 176, 239, 172, 176, 0, 193, 
	193, 0, 194, 0, 171, 239, 172, 171, 
	194, 0, 171, 239, 172, 171, 170, 192, 
	192, 195, 0, 197, 197, 0, 198, 198, 
	0, 171, 239, 172, 171, 199, 199, 0, 
	200, 200, 0, 201, 201, 0, 202, 202, 
	0, 203, 203, 0, 171, 239, 172, 171, 
	0, 205, 205, 0, 206, 206, 0, 171, 
	239, 172, 171, 0, 208, 208, 0, 209, 
	0, 167, 167, 209, 0, 167, 167, 166, 
	207, 207, 210, 0, 212, 212, 0, 213, 
	213, 0, 167, 167, 214, 214, 0, 215, 
	215, 0, 216, 216, 0, 217, 217, 0, 
	218, 218, 0, 167, 167, 0, 220, 220, 
	0, 221, 221, 0, 167, 167, 0, 236, 
	0, 11, 236, 222, 11, 224, 226, 225, 
	0, 225, 0, 11, 236, 222, 11, 226, 
	225, 0, 11, 236, 222, 11, 227, 228, 
	0, 228, 0, 11, 236, 222, 11, 228, 
	0, 8, 8, 230, 232, 231, 0, 231, 
	0, 8, 8, 232, 231, 0, 8, 8, 
	233, 234, 0, 234, 0, 8, 8, 234, 
	0, 1, 235, 2, 1, 3, 4, 12, 
	13, 12, 12, 18, 23, 0, 1, 235, 
	2, 1, 3, 4, 12, 13, 12, 12, 
	18, 23, 0, 1, 235, 2, 1, 3, 
	4, 12, 13, 12, 12, 18, 23, 0, 
	1, 235, 2, 1, 3, 4, 12, 13, 
	12, 12, 18, 23, 0, 1, 235, 2, 
	1, 3, 4, 12, 13, 12, 12, 18, 
	23, 0, 0
};

static const char _ObjParser_trans_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 19, 19, 0, 19, 19, 40, 
	74, 0, 43, 0, 46, 46, 46, 9, 
	0, 0, 0, 40, 74, 0, 43, 0, 
	46, 46, 46, 46, 46, 9, 0, 0, 
	0, 0, 0, 40, 74, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 82, 82, 55, 11, 
	11, 11, 11, 86, 0, 0, 5, 0, 
	3, 0, 31, 31, 0, 0, 3, 0, 
	0, 0, 25, 25, 1, 0, 0, 0, 
	0, 28, 0, 0, 5, 0, 3, 0, 
	31, 31, 0, 0, 3, 0, 0, 0, 
	25, 25, 1, 0, 0, 0, 0, 28, 
	0, 0, 5, 0, 3, 0, 31, 31, 
	31, 31, 0, 0, 3, 0, 0, 0, 
	0, 0, 25, 25, 1, 0, 0, 0, 
	0, 28, 0, 0, 0, 0, 5, 0, 
	3, 0, 31, 31, 31, 31, 0, 0, 
	3, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 7, 0, 70, 70, 70, 70, 
	7, 0, 31, 31, 31, 31, 0, 0, 
	0, 5, 0, 0, 0, 0, 0, 0, 
	0, 37, 37, 37, 37, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 37, 37, 37, 37, 
	0, 0, 0, 0, 0, 0, 0, 34, 
	34, 34, 34, 0, 0, 0, 0, 7, 
	0, 70, 70, 70, 70, 7, 0, 31, 
	31, 31, 31, 0, 0, 0, 5, 0, 
	0, 0, 0, 0, 0, 0, 37, 37, 
	37, 37, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 37, 37, 37, 37, 0, 0, 0, 
	0, 0, 0, 0, 34, 34, 34, 34, 
	0, 0, 0, 0, 7, 0, 70, 70, 
	7, 0, 31, 31, 0, 0, 0, 5, 
	0, 0, 0, 0, 0, 0, 0, 37, 
	37, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	37, 37, 0, 0, 0, 0, 0, 0, 
	0, 34, 34, 0, 0, 0, 0, 7, 
	0, 70, 70, 7, 0, 31, 31, 0, 
	0, 0, 5, 0, 0, 0, 0, 0, 
	0, 0, 37, 37, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 37, 37, 0, 0, 0, 
	0, 0, 0, 0, 34, 34, 0, 0, 
	0, 0, 0, 0, 82, 82, 55, 11, 
	11, 11, 11, 86, 0, 0, 5, 0, 
	3, 0, 31, 31, 0, 0, 3, 0, 
	0, 0, 25, 25, 1, 0, 0, 0, 
	0, 28, 0, 0, 5, 0, 3, 0, 
	31, 31, 0, 0, 3, 0, 0, 0, 
	25, 25, 1, 0, 0, 0, 0, 28, 
	0, 0, 5, 0, 3, 0, 31, 31, 
	31, 31, 0, 0, 3, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	7, 0, 70, 70, 70, 70, 7, 0, 
	31, 31, 31, 31, 0, 0, 0, 5, 
	0, 0, 0, 0, 0, 0, 0, 37, 
	37, 37, 37, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 37, 37, 37, 37, 0, 0, 
	0, 0, 0, 0, 0, 34, 34, 34, 
	34, 0, 0, 0, 0, 7, 0, 70, 
	70, 7, 0, 31, 31, 0, 0, 0, 
	5, 0, 0, 0, 0, 0, 0, 0, 
	37, 37, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 37, 37, 0, 0, 0, 0, 0, 
	0, 0, 34, 34, 0, 0, 0, 0, 
	7, 0, 70, 70, 7, 0, 31, 31, 
	0, 0, 0, 5, 0, 0, 0, 0, 
	0, 0, 0, 37, 37, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 37, 37, 0, 0, 
	0, 0, 0, 0, 0, 34, 34, 0, 
	0, 0, 0, 0, 0, 82, 82, 55, 
	11, 11, 11, 11, 86, 0, 0, 5, 
	0, 3, 0, 31, 31, 0, 0, 3, 
	0, 0, 0, 25, 25, 1, 0, 0, 
	0, 0, 28, 0, 0, 5, 0, 3, 
	0, 31, 31, 31, 31, 0, 0, 3, 
	0, 0, 0, 0, 0, 25, 25, 1, 
	0, 0, 0, 0, 28, 0, 0, 0, 
	0, 5, 0, 3, 0, 31, 31, 31, 
	31, 0, 0, 3, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 7, 0, 70, 
	70, 70, 70, 7, 0, 31, 31, 31, 
	31, 0, 0, 0, 5, 0, 0, 0, 
	0, 0, 0, 0, 37, 37, 37, 37, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 37, 
	37, 37, 37, 0, 0, 0, 0, 0, 
	0, 0, 34, 34, 34, 34, 0, 0, 
	0, 0, 7, 0, 70, 70, 70, 70, 
	7, 0, 31, 31, 31, 31, 0, 0, 
	0, 5, 0, 0, 0, 0, 0, 0, 
	0, 37, 37, 37, 37, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 37, 37, 37, 37, 
	0, 0, 0, 0, 0, 0, 0, 34, 
	34, 34, 34, 0, 0, 0, 0, 7, 
	0, 70, 70, 7, 0, 31, 31, 0, 
	0, 0, 5, 0, 0, 0, 0, 0, 
	0, 0, 37, 37, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 37, 37, 0, 0, 0, 
	0, 0, 0, 0, 34, 34, 0, 0, 
	0, 0, 0, 0, 0, 40, 0, 74, 
	0, 43, 0, 78, 78, 78, 78, 49, 
	9, 0, 0, 0, 0, 0, 40, 74, 
	0, 43, 0, 52, 52, 52, 52, 9, 
	0, 0, 0, 40, 0, 74, 0, 43, 
	0, 78, 78, 49, 9, 0, 0, 0, 
	40, 74, 0, 43, 0, 52, 52, 9, 
	0, 23, 23, 23, 23, 23, 23, 23, 
	23, 23, 23, 23, 23, 0, 67, 67, 
	67, 67, 67, 67, 67, 67, 67, 67, 
	67, 67, 0, 58, 58, 58, 58, 58, 
	58, 58, 58, 58, 58, 58, 58, 0, 
	64, 64, 64, 64, 64, 64, 64, 64, 
	64, 64, 64, 64, 0, 61, 61, 61, 
	61, 61, 61, 61, 61, 61, 61, 61, 
	61, 0, 0
};

static const char _ObjParser_eof_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 21, 13, 17, 15
};

static const int ObjParser_start = 235;
static const int ObjParser_first_final = 235;
static const int ObjParser_error = 0;

static const int ObjParser_en_main = 235;


#line 80 "formats/obj/reader.rl"

typedef float real;
static const mixing::Type REALMT = mixing::FLOAT;
enum Attrs { VERTEX, TEX, NORMAL };

inline int objidx(int n, int size)
{
	if (n == 0) throw std::runtime_error("index cannot be 0");
	if (n > size) throw std::runtime_error("n too big");
	if (n < 0) {
		if (size + n < 0) {
			throw std::runtime_error("n too small");
		}
		return size + n;
	}

	return n - 1;
}

static const int lut_interp[] = { mixing::POS, mixing::TEX, mixing::NORMAL };

#define IL 5 /*std::numeric_limits<mesh::listidx_t>::max()*/
#define IR std::numeric_limits<mesh::regidx_t>::max()

struct OBJReader {
	mesh::Builder &builder;
	mesh::listidx_t attr_lists[3][5]; // VERTEX, TEX, NORMAL
	mesh::regidx_t vtx_reg[5];
	mesh::regidx_t face_reg[64];
	std::vector<std::pair<mesh::listidx_t, mesh::attridx_t>> tex_loc, normal_loc;
	std::vector<std::string> mtllibs;

	OBJReader(mesh::Builder &_builder) : builder(_builder),
		attr_lists{ { IL, IL, IL, IL, IL }, { IL, IL, IL, IL, IL }, { IL, IL, IL, IL, IL } },
		vtx_reg{ IR, IR, IR, IR, IR },
		face_reg{ IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR,
		          IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR,
		          IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR,
		          IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR }
	{}

	// helper methods
	int init_attr(Attrs attr, int n)
	{
		mesh::listidx_t &list = attr_lists[attr][n];
		if (list == IL) {
			mixing::Fmt fmt;
			mixing::Interps interps;
			for (int i = 0; i < n; ++i) {
				fmt.add(REALMT);
				interps.append(lut_interp[attr], i);
			}
			list = builder.add_list(fmt, interps, attr == VERTEX ? mesh::attr::VTX : mesh::attr::CORNER);
		}
		return list;
	}
	mesh::attridx_t write_attr(int list, real *coords, int n)
	{
		mesh::attridx_t row = builder.alloc_attr(list);
		for (int i = 0; i < n; ++i) {
			unsigned char *dst = builder.elem(list, row, i);
			std::copy((unsigned char*)(coords + i), (unsigned char*)(coords + i + 1), dst);
		}
		return row;
	}

	// OBJ methods
	void vertex(real *coords, int n)
	{
		mesh::listidx_t list = init_attr(VERTEX, n);
		mesh::attridx_t aidx = write_attr(list, coords, n);

		mesh::regidx_t r = vtx_reg[n];
		if (r == IR) {
			r = vtx_reg[n] = builder.add_vtx_region(1);
			builder.bind_reg_vtxlist(r, 0, list);
		}
		mesh::vtxidx_t vidx = builder.alloc_vtx();
		builder.vtx_reg(vidx, r);
		builder.bind_vtx_attr(vidx, 0, aidx);
	}
	void tex(real *coords, int n)
	{
		mesh::listidx_t list = init_attr(TEX, n);
		mesh::attridx_t aidx = write_attr(list, coords, n);
		tex_loc.push_back(std::make_pair(list, aidx));
	}
	void normal(real *coords, int n)
	{
		mesh::listidx_t list = init_attr(NORMAL, n);
		mesh::attridx_t aidx = write_attr(list, coords, n);
		normal_loc.push_back(std::make_pair(list, aidx));
	}
	void face(bool has_v, int *vi, bool has_t, int *ti, bool has_n, int *ni, int corners)
	{
		if (!has_v) throw std::runtime_error("Face has no vertex index; required for connectivity");
		mesh::listidx_t tex_l = IL, normal_l = IL;

		if (has_t) tex_l = tex_loc[ti[0]].first;
		if (has_n) normal_l = normal_loc[ni[0]].first;

		for (int c = 1; c < corners; ++c) {
			if (has_t && tex_loc[ti[c]].first != tex_l) throw std::runtime_error("Inconsistent texture attribute types in face");
			if (has_n && normal_loc[ni[c]].first != normal_l) throw std::runtime_error("Inconsistent normal attribute types in face");
		}

		int reglookup = (tex_l << 3) | normal_l;
		mesh::regidx_t r = face_reg[reglookup];
		mesh::listidx_t num_attrs = (mesh::listidx_t)has_t + (mesh::listidx_t)has_n;
		mesh::listidx_t tex_a = 0, normal_a = (mesh::listidx_t)has_t;
		if (r == IR) {
			r = face_reg[reglookup] = builder.add_face_region(0, num_attrs);
			if (has_t) builder.bind_reg_cornerlist(r, tex_a, tex_l);
			if (has_n) builder.bind_reg_cornerlist(r, normal_a, normal_l);
		}

		// add face
		mesh::faceidx_t fidx = builder.alloc_face(1, corners);
		builder.face_reg(fidx, r);
		builder.face_begin(corners);
		for (int i = 0; i < corners; ++i) {
			builder.set_org(vi[i]);
			if (has_t) builder.bind_corner_attr(fidx, i, tex_a, tex_loc[ti[i]].second);
			if (has_n) builder.bind_corner_attr(fidx, i, normal_a, normal_loc[ni[i]].second);
		}
		builder.face_end();
	}

	// reader
	void read_obj(std::istream &is, const std::string &dir)
	{
		progress::handle prog;
		prog.start(util::linenum_approx(is));
		builder.init_bindings(0, 1, 2);

		double fraction, denom, sign, val, exp, expmul;
		int idx, idx_sign;
		std::vector<int> fi[3];
		real coords[4];
		int ncoord;
		int vi[3] = { 0 };
		int line = 0;

		char buf[BUFSIZE];
		int cs;

		
#line 746 "/home/max/repos/harry/formats/obj/reader.cc"
	{
	cs = ObjParser_start;
	}

#line 227 "formats/obj/reader.rl"

		while (!is.eof()) {
			char *p = buf;
			is.read(p, BUFSIZE);
			char *pe = p + is.gcount();
			char *eof = is.eof() ? pe : nullptr;

			
#line 760 "/home/max/repos/harry/formats/obj/reader.cc"
	{
	int _klen;
	unsigned int _trans;
	const char *_acts;
	unsigned int _nacts;
	const char *_keys;

	if ( p == pe )
		goto _test_eof;
	if ( cs == 0 )
		goto _out;
_resume:
	_keys = _ObjParser_trans_keys + _ObjParser_key_offsets[cs];
	_trans = _ObjParser_index_offsets[cs];

	_klen = _ObjParser_single_lengths[cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + _klen - 1;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + ((_upper-_lower) >> 1);
			if ( (*p) < *_mid )
				_upper = _mid - 1;
			else if ( (*p) > *_mid )
				_lower = _mid + 1;
			else {
				_trans += (unsigned int)(_mid - _keys);
				goto _match;
			}
		}
		_keys += _klen;
		_trans += _klen;
	}

	_klen = _ObjParser_range_lengths[cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + (_klen<<1) - 2;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + (((_upper-_lower) >> 1) & ~1);
			if ( (*p) < _mid[0] )
				_upper = _mid - 2;
			else if ( (*p) > _mid[1] )
				_lower = _mid + 2;
			else {
				_trans += (unsigned int)((_mid - _keys)>>1);
				goto _match;
			}
		}
		_trans += _klen;
	}

_match:
	cs = _ObjParser_trans_targs[_trans];

	if ( _ObjParser_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _ObjParser_actions + _ObjParser_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
#line 30 "formats/obj/reader.rl"
	{ sign = 1; val = 0; fraction = 0; denom = 1; exp = 0; expmul = 1; }
	break;
	case 1:
#line 31 "formats/obj/reader.rl"
	{ sign = (*p) == '-' ? -1 : 1; }
	break;
	case 2:
#line 32 "formats/obj/reader.rl"
	{ fraction *= 10; fraction += (*p) - '0'; denom *= 10; }
	break;
	case 3:
#line 33 "formats/obj/reader.rl"
	{ val *= 10; val += (*p) - '0'; }
	break;
	case 4:
#line 34 "formats/obj/reader.rl"
	{ exp *= 10; exp += (*p) - '0'; }
	break;
	case 5:
#line 35 "formats/obj/reader.rl"
	{ expmul = std::pow(10.0, exp); }
	break;
	case 6:
#line 36 "formats/obj/reader.rl"
	{ val += fraction / denom; val *= sign * expmul; }
	break;
	case 7:
#line 37 "formats/obj/reader.rl"
	{ val = NAN; }
	break;
	case 8:
#line 38 "formats/obj/reader.rl"
	{ val = INFINITY * sign; }
	break;
	case 9:
#line 50 "formats/obj/reader.rl"
	{ idx_sign = 0; }
	break;
	case 10:
#line 50 "formats/obj/reader.rl"
	{ idx_sign = 1; }
	break;
	case 11:
#line 50 "formats/obj/reader.rl"
	{ idx = 0; }
	break;
	case 12:
#line 50 "formats/obj/reader.rl"
	{ idx *= 10; idx += (*p) - '0'; }
	break;
	case 13:
#line 50 "formats/obj/reader.rl"
	{ idx = idx_sign ? -idx : idx; }
	break;
	case 14:
#line 52 "formats/obj/reader.rl"
	{ fi[VERTEX].push_back(objidx(idx, vi[VERTEX])); }
	break;
	case 15:
#line 53 "formats/obj/reader.rl"
	{ fi[TEX].push_back(objidx(idx, vi[TEX])); }
	break;
	case 16:
#line 54 "formats/obj/reader.rl"
	{ fi[NORMAL].push_back(objidx(idx, vi[NORMAL])); }
	break;
	case 17:
#line 56 "formats/obj/reader.rl"
	{ coords[ncoord++] = val; }
	break;
	case 18:
#line 57 "formats/obj/reader.rl"
	{ ncoord = 0; }
	break;
	case 19:
#line 63 "formats/obj/reader.rl"
	{ ++vi[VERTEX]; vertex(coords, ncoord); }
	break;
	case 20:
#line 64 "formats/obj/reader.rl"
	{ ++vi[TEX]; tex(coords, ncoord); }
	break;
	case 21:
#line 65 "formats/obj/reader.rl"
	{ ++vi[NORMAL]; normal(coords, ncoord); }
	break;
	case 22:
#line 66 "formats/obj/reader.rl"
	{ fi[0].clear(); fi[1].clear(); fi[2].clear(); }
	break;
	case 23:
#line 66 "formats/obj/reader.rl"
	{ face(!fi[VERTEX].empty(), fi[VERTEX].data(), !fi[TEX].empty(), fi[TEX].data(), !fi[NORMAL].empty(), fi[NORMAL].data(), fi[VERTEX].size()); }
	break;
	case 24:
#line 73 "formats/obj/reader.rl"
	{ prog(line++); }
	break;
#line 933 "/home/max/repos/harry/formats/obj/reader.cc"
		}
	}

_again:
	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	const char *__acts = _ObjParser_actions + _ObjParser_eof_actions[cs];
	unsigned int __nacts = (unsigned int) *__acts++;
	while ( __nacts-- > 0 ) {
		switch ( *__acts++ ) {
	case 19:
#line 63 "formats/obj/reader.rl"
	{ ++vi[VERTEX]; vertex(coords, ncoord); }
	break;
	case 20:
#line 64 "formats/obj/reader.rl"
	{ ++vi[TEX]; tex(coords, ncoord); }
	break;
	case 21:
#line 65 "formats/obj/reader.rl"
	{ ++vi[NORMAL]; normal(coords, ncoord); }
	break;
	case 23:
#line 66 "formats/obj/reader.rl"
	{ face(!fi[VERTEX].empty(), fi[VERTEX].data(), !fi[TEX].empty(), fi[TEX].data(), !fi[NORMAL].empty(), fi[NORMAL].data(), fi[VERTEX].size()); }
	break;
#line 965 "/home/max/repos/harry/formats/obj/reader.cc"
		}
	}
	}

	_out: {}
	}

#line 235 "formats/obj/reader.rl"

			if (cs == ObjParser_error) throw std::runtime_error("Unable to parse this OBJ file");
		}
		prog.end();
	}
};

void read(std::istream &is, const std::string &dir, mesh::Mesh &mesh)
{
	mesh::Builder builder(mesh);

	OBJReader reader(builder);
	reader.read_obj(is, dir);

	quant::set_bounds(mesh.attrs);

	std::cout << "Used face regions: " << mesh.attrs.num_regs_face() << std::endl;
	std::cout << "Used vertex regions: " << mesh.attrs.num_regs_vtx() << std::endl;
}

}
}

