/* 
 * Progress bar
 * 
 * Copyright (c) 2017, Max von Buelow
 * All rights reserved.
 * Contact: https://maxvonbuelow.de
 * 
 * This file is part of the HOTools project.
 * https://github.com/magcks/hotools
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <stdint.h> // or cstdint
#include <ostream>
#include <cstring>
#include <thread>
#include <future>
#include <atomic>
#include <chrono>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#include <signal.h>
#endif

namespace progress {

struct _winsize {
	std::atomic<uint32_t> cols;
	std::atomic<uint32_t> rows;
};
static _winsize WINSIZE;

#ifdef _WIN32
inline void get_win_size(_winsize &size)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	size.cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	size.rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}
#else
inline void get_win_size(_winsize &size)
{
	struct winsize win;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &win);
	size.cols = win.ws_col;
	size.rows = win.ws_row;
}
#endif

static void on_resize(int)
{
	get_win_size(WINSIZE);
}

inline void init_signal()
{
#ifndef _WIN32
	signal(SIGWINCH, on_resize);
#endif
	on_resize(0);
}

enum ProgState { CREATED, RUNNING, TERM };
struct ProgressData {
	std::atomic<uint32_t> cur, count;
	std::atomic<ProgState> state;
	std::future<void> future;
};

inline void printprog(ProgressData *pd)
{
	float percent = (float)pd->cur / (float)pd->count;

	int barwidth = WINSIZE.cols - 9;

	std::cout << "[";
	int pos = barwidth * percent;
	for (int i = 0; i < barwidth; ++i) {
		if (i < pos) std::cout << "=";
		else if (i == pos) std::cout << ">";
		else std::cout << " ";
	}
	std::cout << "] " << (int)(percent * 100.0f) << " %";
	std::cout << '\r';
	std::cout.flush();
}
static void progthread(ProgressData *pd)
{
	while (1) {
		if (pd->state == RUNNING || pd->state == TERM) printprog(pd);
		if (pd->state == TERM) {
			std::cout << std::endl;
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60fps
	}
}

inline void createprog(ProgressData *pd)
{
	init_signal();
	pd->state = CREATED;
	pd->future = std::async(std::launch::async, progthread, pd);
}

inline void startprog(ProgressData *pd, uint32_t c = 0)
{
	pd->cur = 0;
	pd->count = c;
	pd->state = RUNNING;
}
inline void endprog(ProgressData *pd)
{
	pd->cur = (uint32_t)pd->count;
	pd->state = TERM;
	pd->future.wait();
}
inline bool progterm(ProgressData *pd)
{
	return pd->state == TERM;
}
inline void setprog(ProgressData *pd, uint32_t cu)
{
	if (cu > pd->count) pd->count = cu;
	pd->cur = cu;
}
inline void setprog(ProgressData *pd, uint32_t cu, uint32_t co)
{
	pd->cur = cu;
	pd->count = co;
}

// simple progress handle
struct handle {
	ProgressData pd;
	inline handle()
	{
		createprog(&pd);
	}
	inline handle(uint32_t c) : handle()
	{
		start(c);
	}
	inline ~handle()
	{
		end();
	}
	inline void start(uint32_t c)
	{
		startprog(&pd, c);
	}
	inline void end()
	{
		if (!progterm(&pd)) endprog(&pd);
	}
	inline void init(uint32_t co)
	{
	}
	inline void operator()(uint32_t cu)
	{
		setprog(&pd, cu);
	}
	inline void operator()(uint32_t cu, uint32_t co)
	{
		setprog(&pd, cu, co);
	}
};

// write progress to void
struct voidhandle {
	voidhandle(uint32_t = 0) {}
	void start(uint32_t) {}
	void end() {}
	void operator()(uint32_t, uint32_t = 0) {}
};

}
