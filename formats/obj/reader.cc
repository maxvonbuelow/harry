
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
	0, 0, 1, 4, 6, 7, 11, 13, 
	17, 21, 23, 29, 30, 31, 32, 33, 
	34, 35, 36, 37, 38, 39, 40, 43, 
	53, 56, 58, 63, 73, 76, 78, 83, 
	93, 96, 98, 105, 106, 118, 121, 123, 
	130, 133, 135, 137, 142, 150, 152, 154, 
	156, 158, 160, 162, 164, 167, 169, 171, 
	174, 176, 178, 183, 191, 193, 195, 197, 
	199, 201, 203, 205, 208, 210, 212, 215, 
	217, 219, 222, 228, 230, 232, 234, 236, 
	238, 240, 242, 243, 245, 247, 248, 250, 
	252, 255, 261, 263, 265, 267, 269, 271, 
	273, 275, 276, 278, 280, 281, 282, 292, 
	295, 297, 302, 312, 315, 317, 322, 332, 
	335, 337, 344, 345, 348, 350, 352, 357, 
	365, 367, 369, 371, 373, 375, 377, 379, 
	382, 384, 386, 389, 391, 393, 396, 402, 
	404, 406, 408, 410, 412, 414, 416, 417, 
	419, 421, 422, 424, 426, 429, 435, 437, 
	439, 441, 443, 445, 447, 449, 450, 452, 
	454, 455, 456, 466, 469, 471, 476, 486, 
	489, 491, 498, 499, 511, 514, 516, 523, 
	526, 528, 530, 535, 543, 545, 547, 549, 
	551, 553, 555, 557, 560, 562, 564, 567, 
	569, 571, 576, 584, 586, 588, 590, 592, 
	594, 596, 598, 601, 603, 605, 608, 610, 
	612, 615, 621, 623, 625, 627, 629, 631, 
	633, 635, 636, 638, 640, 641, 642, 648, 
	655, 657, 663, 669, 671, 676, 681, 683, 
	687, 691, 693, 696, 707, 718, 729, 740
};

static const char _ObjParser_trans_keys[] = {
	10, 10, 13, 32, 10, 13, 32, 32, 
	45, 48, 57, 48, 57, 32, 47, 48, 
	57, 32, 45, 48, 57, 48, 57, 10, 
	13, 32, 47, 48, 57, 32, 116, 108, 
	108, 105, 98, 115, 101, 109, 116, 108, 
	32, 110, 116, 32, 43, 45, 46, 73, 
	78, 105, 110, 48, 57, 46, 48, 57, 
	48, 57, 32, 69, 101, 48, 57, 32, 
	43, 45, 46, 73, 78, 105, 110, 48, 
	57, 46, 48, 57, 48, 57, 32, 69, 
	101, 48, 57, 32, 43, 45, 46, 73, 
	78, 105, 110, 48, 57, 46, 48, 57, 
	48, 57, 10, 13, 32, 69, 101, 48, 
	57, 10, 10, 13, 32, 43, 45, 46, 
	73, 78, 105, 110, 48, 57, 46, 48, 
	57, 48, 57, 10, 13, 32, 69, 101, 
	48, 57, 10, 13, 32, 43, 45, 48, 
	57, 10, 13, 32, 48, 57, 10, 13, 
	32, 46, 69, 101, 48, 57, 78, 110, 
	70, 102, 73, 105, 78, 110, 73, 105, 
	84, 116, 89, 121, 10, 13, 32, 65, 
	97, 78, 110, 10, 13, 32, 43, 45, 
	48, 57, 10, 13, 32, 48, 57, 10, 
	13, 32, 46, 69, 101, 48, 57, 78, 
	110, 70, 102, 73, 105, 78, 110, 73, 
	105, 84, 116, 89, 121, 10, 13, 32, 
	65, 97, 78, 110, 10, 13, 32, 43, 
	45, 48, 57, 32, 48, 57, 32, 46, 
	69, 101, 48, 57, 78, 110, 70, 102, 
	73, 105, 78, 110, 73, 105, 84, 116, 
	89, 121, 32, 65, 97, 78, 110, 32, 
	43, 45, 48, 57, 32, 48, 57, 32, 
	46, 69, 101, 48, 57, 78, 110, 70, 
	102, 73, 105, 78, 110, 73, 105, 84, 
	116, 89, 121, 32, 65, 97, 78, 110, 
	32, 32, 32, 43, 45, 46, 73, 78, 
	105, 110, 48, 57, 46, 48, 57, 48, 
	57, 32, 69, 101, 48, 57, 32, 43, 
	45, 46, 73, 78, 105, 110, 48, 57, 
	46, 48, 57, 48, 57, 32, 69, 101, 
	48, 57, 32, 43, 45, 46, 73, 78, 
	105, 110, 48, 57, 46, 48, 57, 48, 
	57, 10, 13, 32, 69, 101, 48, 57, 
	10, 10, 13, 32, 43, 45, 48, 57, 
	10, 13, 32, 48, 57, 10, 13, 32, 
	46, 69, 101, 48, 57, 78, 110, 70, 
	102, 73, 105, 78, 110, 73, 105, 84, 
	116, 89, 121, 10, 13, 32, 65, 97, 
	78, 110, 10, 13, 32, 43, 45, 48, 
	57, 32, 48, 57, 32, 46, 69, 101, 
	48, 57, 78, 110, 70, 102, 73, 105, 
	78, 110, 73, 105, 84, 116, 89, 121, 
	32, 65, 97, 78, 110, 32, 43, 45, 
	48, 57, 32, 48, 57, 32, 46, 69, 
	101, 48, 57, 78, 110, 70, 102, 73, 
	105, 78, 110, 73, 105, 84, 116, 89, 
	121, 32, 65, 97, 78, 110, 32, 32, 
	32, 43, 45, 46, 73, 78, 105, 110, 
	48, 57, 46, 48, 57, 48, 57, 32, 
	69, 101, 48, 57, 32, 43, 45, 46, 
	73, 78, 105, 110, 48, 57, 46, 48, 
	57, 48, 57, 10, 13, 32, 69, 101, 
	48, 57, 10, 10, 13, 32, 43, 45, 
	46, 73, 78, 105, 110, 48, 57, 46, 
	48, 57, 48, 57, 10, 13, 32, 69, 
	101, 48, 57, 10, 13, 32, 43, 45, 
	48, 57, 10, 13, 32, 48, 57, 10, 
	13, 32, 46, 69, 101, 48, 57, 78, 
	110, 70, 102, 73, 105, 78, 110, 73, 
	105, 84, 116, 89, 121, 10, 13, 32, 
	65, 97, 78, 110, 10, 13, 32, 43, 
	45, 48, 57, 10, 13, 32, 48, 57, 
	10, 13, 32, 46, 69, 101, 48, 57, 
	78, 110, 70, 102, 73, 105, 78, 110, 
	73, 105, 84, 116, 89, 121, 10, 13, 
	32, 65, 97, 78, 110, 10, 13, 32, 
	43, 45, 48, 57, 32, 48, 57, 32, 
	46, 69, 101, 48, 57, 78, 110, 70, 
	102, 73, 105, 78, 110, 73, 105, 84, 
	116, 89, 121, 32, 65, 97, 78, 110, 
	32, 10, 10, 13, 32, 45, 48, 57, 
	10, 13, 32, 45, 47, 48, 57, 48, 
	57, 10, 13, 32, 47, 48, 57, 10, 
	13, 32, 45, 48, 57, 48, 57, 10, 
	13, 32, 48, 57, 32, 45, 47, 48, 
	57, 48, 57, 32, 47, 48, 57, 32, 
	45, 48, 57, 48, 57, 32, 48, 57, 
	10, 13, 32, 35, 102, 103, 109, 111, 
	115, 117, 118, 10, 13, 32, 35, 102, 
	103, 109, 111, 115, 117, 118, 10, 13, 
	32, 35, 102, 103, 109, 111, 115, 117, 
	118, 10, 13, 32, 35, 102, 103, 109, 
	111, 115, 117, 118, 10, 13, 32, 35, 
	102, 103, 109, 111, 115, 117, 118, 0
};

static const char _ObjParser_single_lengths[] = {
	0, 1, 3, 2, 1, 2, 0, 2, 
	2, 0, 4, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 3, 8, 
	1, 0, 3, 8, 1, 0, 3, 8, 
	1, 0, 5, 1, 10, 1, 0, 5, 
	3, 2, 0, 3, 6, 2, 2, 2, 
	2, 2, 2, 2, 3, 2, 2, 3, 
	2, 0, 3, 6, 2, 2, 2, 2, 
	2, 2, 2, 3, 2, 2, 3, 2, 
	0, 1, 4, 2, 2, 2, 2, 2, 
	2, 2, 1, 2, 2, 1, 2, 0, 
	1, 4, 2, 2, 2, 2, 2, 2, 
	2, 1, 2, 2, 1, 1, 8, 1, 
	0, 3, 8, 1, 0, 3, 8, 1, 
	0, 5, 1, 3, 2, 0, 3, 6, 
	2, 2, 2, 2, 2, 2, 2, 3, 
	2, 2, 3, 2, 0, 1, 4, 2, 
	2, 2, 2, 2, 2, 2, 1, 2, 
	2, 1, 2, 0, 1, 4, 2, 2, 
	2, 2, 2, 2, 2, 1, 2, 2, 
	1, 1, 8, 1, 0, 3, 8, 1, 
	0, 5, 1, 10, 1, 0, 5, 3, 
	2, 0, 3, 6, 2, 2, 2, 2, 
	2, 2, 2, 3, 2, 2, 3, 2, 
	0, 3, 6, 2, 2, 2, 2, 2, 
	2, 2, 3, 2, 2, 3, 2, 0, 
	1, 4, 2, 2, 2, 2, 2, 2, 
	2, 1, 2, 2, 1, 1, 4, 5, 
	0, 4, 4, 0, 3, 3, 0, 2, 
	2, 0, 1, 11, 11, 11, 11, 11
};

static const char _ObjParser_range_lengths[] = {
	0, 0, 0, 0, 0, 1, 1, 1, 
	1, 1, 1, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 0, 1, 1, 1, 1, 
	0, 0, 1, 1, 1, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 1, 1, 1, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	1, 1, 1, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 1, 
	1, 1, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 0, 0, 0, 1, 1, 1, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 1, 1, 1, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 1, 1, 1, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 1, 1, 1, 1, 1, 1, 
	1, 1, 0, 1, 1, 1, 1, 0, 
	0, 1, 1, 1, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	1, 1, 1, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 1, 
	1, 1, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 0, 0, 0, 0, 0
};

static const short _ObjParser_index_offsets[] = {
	0, 0, 2, 6, 9, 11, 15, 17, 
	21, 25, 27, 33, 35, 37, 39, 41, 
	43, 45, 47, 49, 51, 53, 55, 59, 
	69, 72, 74, 79, 89, 92, 94, 99, 
	109, 112, 114, 121, 123, 135, 138, 140, 
	147, 151, 154, 156, 161, 169, 172, 175, 
	178, 181, 184, 187, 190, 194, 197, 200, 
	204, 207, 209, 214, 222, 225, 228, 231, 
	234, 237, 240, 243, 247, 250, 253, 257, 
	260, 262, 265, 271, 274, 277, 280, 283, 
	286, 289, 292, 294, 297, 300, 302, 305, 
	307, 310, 316, 319, 322, 325, 328, 331, 
	334, 337, 339, 342, 345, 347, 349, 359, 
	362, 364, 369, 379, 382, 384, 389, 399, 
	402, 404, 411, 413, 417, 420, 422, 427, 
	435, 438, 441, 444, 447, 450, 453, 456, 
	460, 463, 466, 470, 473, 475, 478, 484, 
	487, 490, 493, 496, 499, 502, 505, 507, 
	510, 513, 515, 518, 520, 523, 529, 532, 
	535, 538, 541, 544, 547, 550, 552, 555, 
	558, 560, 562, 572, 575, 577, 582, 592, 
	595, 597, 604, 606, 618, 621, 623, 630, 
	634, 637, 639, 644, 652, 655, 658, 661, 
	664, 667, 670, 673, 677, 680, 683, 687, 
	690, 692, 697, 705, 708, 711, 714, 717, 
	720, 723, 726, 730, 733, 736, 740, 743, 
	745, 748, 754, 757, 760, 763, 766, 769, 
	772, 775, 777, 780, 783, 785, 787, 793, 
	800, 802, 808, 814, 816, 821, 826, 828, 
	832, 836, 838, 841, 853, 865, 877, 889
};

static const unsigned char _ObjParser_trans_targs[] = {
	235, 0, 235, 1, 2, 0, 235, 1, 
	3, 5, 0, 5, 6, 7, 0, 7, 
	0, 8, 229, 7, 0, 8, 9, 10, 
	0, 10, 0, 236, 221, 222, 223, 10, 
	0, 3, 0, 13, 0, 14, 0, 15, 
	0, 16, 0, 11, 0, 18, 0, 19, 
	0, 20, 0, 21, 0, 11, 0, 23, 
	101, 161, 0, 23, 24, 24, 25, 90, 
	98, 90, 98, 89, 0, 25, 89, 0, 
	26, 0, 27, 86, 86, 26, 0, 27, 
	28, 28, 29, 75, 83, 75, 83, 74, 
	0, 29, 74, 0, 30, 0, 31, 71, 
	71, 30, 0, 31, 32, 32, 33, 60, 
	68, 60, 68, 59, 0, 33, 59, 0, 
	34, 0, 237, 35, 36, 56, 56, 34, 
	0, 237, 0, 237, 35, 36, 37, 37, 
	38, 45, 53, 45, 53, 44, 0, 38, 
	44, 0, 39, 0, 237, 35, 40, 41, 
	41, 39, 0, 237, 35, 40, 0, 42, 
	42, 0, 43, 0, 237, 35, 40, 43, 
	0, 237, 35, 40, 39, 41, 41, 44, 
	0, 46, 46, 0, 47, 47, 0, 48, 
	48, 0, 49, 49, 0, 50, 50, 0, 
	51, 51, 0, 52, 52, 0, 237, 35, 
	40, 0, 54, 54, 0, 55, 55, 0, 
	237, 35, 40, 0, 57, 57, 0, 58, 
	0, 237, 35, 36, 58, 0, 237, 35, 
	36, 34, 56, 56, 59, 0, 61, 61, 
	0, 62, 62, 0, 63, 63, 0, 64, 
	64, 0, 65, 65, 0, 66, 66, 0, 
	67, 67, 0, 237, 35, 36, 0, 69, 
	69, 0, 70, 70, 0, 237, 35, 36, 
	0, 72, 72, 0, 73, 0, 31, 73, 
	0, 31, 30, 71, 71, 74, 0, 76, 
	76, 0, 77, 77, 0, 78, 78, 0, 
	79, 79, 0, 80, 80, 0, 81, 81, 
	0, 82, 82, 0, 31, 0, 84, 84, 
	0, 85, 85, 0, 31, 0, 87, 87, 
	0, 88, 0, 27, 88, 0, 27, 26, 
	86, 86, 89, 0, 91, 91, 0, 92, 
	92, 0, 93, 93, 0, 94, 94, 0, 
	95, 95, 0, 96, 96, 0, 97, 97, 
	0, 27, 0, 99, 99, 0, 100, 100, 
	0, 27, 0, 102, 0, 102, 103, 103, 
	104, 150, 158, 150, 158, 149, 0, 104, 
	149, 0, 105, 0, 106, 146, 146, 105, 
	0, 106, 107, 107, 108, 135, 143, 135, 
	143, 134, 0, 108, 134, 0, 109, 0, 
	110, 131, 131, 109, 0, 110, 111, 111, 
	112, 120, 128, 120, 128, 119, 0, 112, 
	119, 0, 113, 0, 238, 114, 115, 116, 
	116, 113, 0, 238, 0, 238, 114, 115, 
	0, 117, 117, 0, 118, 0, 238, 114, 
	115, 118, 0, 238, 114, 115, 113, 116, 
	116, 119, 0, 121, 121, 0, 122, 122, 
	0, 123, 123, 0, 124, 124, 0, 125, 
	125, 0, 126, 126, 0, 127, 127, 0, 
	238, 114, 115, 0, 129, 129, 0, 130, 
	130, 0, 238, 114, 115, 0, 132, 132, 
	0, 133, 0, 110, 133, 0, 110, 109, 
	131, 131, 134, 0, 136, 136, 0, 137, 
	137, 0, 138, 138, 0, 139, 139, 0, 
	140, 140, 0, 141, 141, 0, 142, 142, 
	0, 110, 0, 144, 144, 0, 145, 145, 
	0, 110, 0, 147, 147, 0, 148, 0, 
	106, 148, 0, 106, 105, 146, 146, 149, 
	0, 151, 151, 0, 152, 152, 0, 153, 
	153, 0, 154, 154, 0, 155, 155, 0, 
	156, 156, 0, 157, 157, 0, 106, 0, 
	159, 159, 0, 160, 160, 0, 106, 0, 
	162, 0, 162, 163, 163, 164, 210, 218, 
	210, 218, 209, 0, 164, 209, 0, 165, 
	0, 166, 206, 206, 165, 0, 166, 167, 
	167, 168, 195, 203, 195, 203, 194, 0, 
	168, 194, 0, 169, 0, 239, 170, 171, 
	191, 191, 169, 0, 239, 0, 239, 170, 
	171, 172, 172, 173, 180, 188, 180, 188, 
	179, 0, 173, 179, 0, 174, 0, 239, 
	170, 175, 176, 176, 174, 0, 239, 170, 
	175, 0, 177, 177, 0, 178, 0, 239, 
	170, 175, 178, 0, 239, 170, 175, 174, 
	176, 176, 179, 0, 181, 181, 0, 182, 
	182, 0, 183, 183, 0, 184, 184, 0, 
	185, 185, 0, 186, 186, 0, 187, 187, 
	0, 239, 170, 175, 0, 189, 189, 0, 
	190, 190, 0, 239, 170, 175, 0, 192, 
	192, 0, 193, 0, 239, 170, 171, 193, 
	0, 239, 170, 171, 169, 191, 191, 194, 
	0, 196, 196, 0, 197, 197, 0, 198, 
	198, 0, 199, 199, 0, 200, 200, 0, 
	201, 201, 0, 202, 202, 0, 239, 170, 
	171, 0, 204, 204, 0, 205, 205, 0, 
	239, 170, 171, 0, 207, 207, 0, 208, 
	0, 166, 208, 0, 166, 165, 206, 206, 
	209, 0, 211, 211, 0, 212, 212, 0, 
	213, 213, 0, 214, 214, 0, 215, 215, 
	0, 216, 216, 0, 217, 217, 0, 166, 
	0, 219, 219, 0, 220, 220, 0, 166, 
	0, 236, 0, 236, 221, 222, 9, 10, 
	0, 236, 221, 222, 224, 226, 225, 0, 
	225, 0, 236, 221, 222, 226, 225, 0, 
	236, 221, 222, 227, 228, 0, 228, 0, 
	236, 221, 222, 228, 0, 8, 230, 232, 
	231, 0, 231, 0, 8, 232, 231, 0, 
	8, 233, 234, 0, 234, 0, 8, 234, 
	0, 235, 1, 2, 3, 4, 11, 12, 
	11, 11, 17, 22, 0, 235, 1, 2, 
	3, 4, 11, 12, 11, 11, 17, 22, 
	0, 235, 1, 2, 3, 4, 11, 12, 
	11, 11, 17, 22, 0, 235, 1, 2, 
	3, 4, 11, 12, 11, 11, 17, 22, 
	0, 235, 1, 2, 3, 4, 11, 12, 
	11, 11, 17, 22, 0, 0
};

static const char _ObjParser_trans_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 19, 0, 19, 40, 74, 0, 43, 
	0, 46, 46, 9, 0, 0, 40, 74, 
	0, 43, 0, 46, 46, 46, 46, 9, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 82, 82, 55, 11, 
	11, 11, 11, 86, 0, 0, 5, 0, 
	3, 0, 31, 0, 0, 3, 0, 0, 
	25, 25, 1, 0, 0, 0, 0, 28, 
	0, 0, 5, 0, 3, 0, 31, 0, 
	0, 3, 0, 0, 25, 25, 1, 0, 
	0, 0, 0, 28, 0, 0, 5, 0, 
	3, 0, 31, 31, 31, 0, 0, 3, 
	0, 0, 0, 0, 0, 0, 25, 25, 
	1, 0, 0, 0, 0, 28, 0, 0, 
	5, 0, 3, 0, 31, 31, 31, 0, 
	0, 3, 0, 0, 0, 0, 0, 0, 
	0, 0, 7, 0, 70, 70, 70, 7, 
	0, 31, 31, 31, 0, 0, 0, 5, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 37, 37, 
	37, 0, 0, 0, 0, 0, 0, 0, 
	34, 34, 34, 0, 0, 0, 0, 7, 
	0, 70, 70, 70, 7, 0, 31, 31, 
	31, 0, 0, 0, 5, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 37, 37, 37, 0, 0, 
	0, 0, 0, 0, 0, 34, 34, 34, 
	0, 0, 0, 0, 7, 0, 70, 7, 
	0, 31, 0, 0, 0, 5, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 37, 0, 0, 0, 
	0, 0, 0, 0, 34, 0, 0, 0, 
	0, 7, 0, 70, 7, 0, 31, 0, 
	0, 0, 5, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 37, 0, 0, 0, 0, 0, 0, 
	0, 34, 0, 0, 0, 0, 82, 82, 
	55, 11, 11, 11, 11, 86, 0, 0, 
	5, 0, 3, 0, 31, 0, 0, 3, 
	0, 0, 25, 25, 1, 0, 0, 0, 
	0, 28, 0, 0, 5, 0, 3, 0, 
	31, 0, 0, 3, 0, 0, 25, 25, 
	1, 0, 0, 0, 0, 28, 0, 0, 
	5, 0, 3, 0, 31, 31, 31, 0, 
	0, 3, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 7, 0, 70, 70, 
	70, 7, 0, 31, 31, 31, 0, 0, 
	0, 5, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	37, 37, 37, 0, 0, 0, 0, 0, 
	0, 0, 34, 34, 34, 0, 0, 0, 
	0, 7, 0, 70, 7, 0, 31, 0, 
	0, 0, 5, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 37, 0, 0, 0, 0, 0, 0, 
	0, 34, 0, 0, 0, 0, 7, 0, 
	70, 7, 0, 31, 0, 0, 0, 5, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 37, 0, 
	0, 0, 0, 0, 0, 0, 34, 0, 
	0, 0, 0, 82, 82, 55, 11, 11, 
	11, 11, 86, 0, 0, 5, 0, 3, 
	0, 31, 0, 0, 3, 0, 0, 25, 
	25, 1, 0, 0, 0, 0, 28, 0, 
	0, 5, 0, 3, 0, 31, 31, 31, 
	0, 0, 3, 0, 0, 0, 0, 0, 
	0, 25, 25, 1, 0, 0, 0, 0, 
	28, 0, 0, 5, 0, 3, 0, 31, 
	31, 31, 0, 0, 3, 0, 0, 0, 
	0, 0, 0, 0, 0, 7, 0, 70, 
	70, 70, 7, 0, 31, 31, 31, 0, 
	0, 0, 5, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 37, 37, 37, 0, 0, 0, 0, 
	0, 0, 0, 34, 34, 34, 0, 0, 
	0, 0, 7, 0, 70, 70, 70, 7, 
	0, 31, 31, 31, 0, 0, 0, 5, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 37, 37, 
	37, 0, 0, 0, 0, 0, 0, 0, 
	34, 34, 34, 0, 0, 0, 0, 7, 
	0, 70, 7, 0, 31, 0, 0, 0, 
	5, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 37, 
	0, 0, 0, 0, 0, 0, 0, 34, 
	0, 0, 0, 0, 0, 0, 40, 74, 
	0, 0, 0, 0, 40, 0, 74, 0, 
	43, 0, 78, 78, 78, 49, 9, 0, 
	0, 0, 0, 40, 74, 0, 43, 0, 
	52, 52, 52, 9, 0, 0, 40, 0, 
	74, 0, 43, 0, 78, 49, 9, 0, 
	0, 40, 74, 0, 43, 0, 52, 9, 
	0, 23, 23, 23, 23, 23, 23, 23, 
	23, 23, 23, 23, 0, 67, 67, 67, 
	67, 67, 67, 67, 67, 67, 67, 67, 
	0, 58, 58, 58, 58, 58, 58, 58, 
	58, 58, 58, 58, 0, 64, 64, 64, 
	64, 64, 64, 64, 64, 64, 64, 64, 
	0, 61, 61, 61, 61, 61, 61, 61, 
	61, 61, 61, 61, 0, 0
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

		
#line 701 "/home/max/repos/harry/formats/obj/reader.cc"
	{
	cs = ObjParser_start;
	}

#line 227 "formats/obj/reader.rl"

		while (!is.eof()) {
			char *p = buf;
			is.read(p, BUFSIZE);
			char *pe = p + is.gcount();
			char *eof = is.eof() ? pe : nullptr;

			
#line 715 "/home/max/repos/harry/formats/obj/reader.cc"
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
#line 888 "/home/max/repos/harry/formats/obj/reader.cc"
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
#line 920 "/home/max/repos/harry/formats/obj/reader.cc"
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

