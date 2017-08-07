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
_winsize WINSIZE;

#ifdef _WIN32
void get_win_size(_winsize &size)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	size.cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	size.rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}
#else
void get_win_size(_winsize &size)
{
	struct winsize win;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &win);
	size.cols = win.ws_col;
	size.rows = win.ws_row;
}
#endif

void on_resize(int)
{
	get_win_size(WINSIZE);
}

void init_signal()
{
#ifndef _WIN32
	signal(SIGWINCH, on_resize);
#endif
	on_resize(0);
}

static std::atomic<uint32_t> cur, count;
enum ProgState { CREATED, RUNNING, TERM };
static std::atomic<ProgState> state;
static std::ostream *progos;
inline void printprog()
{
	float percent = (float)cur / (float)count;

	int barwidth = WINSIZE.cols - 9;

	*progos << "[";
	int pos = barwidth * percent;
	for (int i = 0; i < barwidth; ++i) {
		if (i < pos) *progos << "=";
		else if (i == pos) *progos << ">";
		else *progos << " ";
	}
	*progos << "] " << (int)(percent * 100.0f) << " %";
	*progos << '\r';
	progos->flush();
}
static void progthread()
{
	while (1) {
		if (state == TERM) break;
		if (state == RUNNING) printprog();
		std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60fps
	}
}

void createprog(std::ostream &os)
{
	progos = &os;
	init_signal();
	state = CREATED;
}

void startprog(uint32_t c = 0)
{
	cur = 0; count = c;
	state = RUNNING;
	std::thread(progthread).detach();
}
void endprog()
{
	cur = (uint32_t)count;
	state = TERM;
	printprog();
	*progos << std::endl;
}
bool progterm()
{
	return state == TERM;
}
void setprog(uint32_t cu)
{
	if (cu > count) count = cu;
	cur = cu;
}
void setprog(uint32_t cu, uint32_t co)
{
	cur = cu;
	count = co;
}

// simple progress handle
struct handle {
	handle(std::ostream &os = std::cout)
	{
		createprog(os);
	}
	handle(std::ostream &os, uint32_t c) : handle(os)
	{
		start(c);
	}
	~handle()
	{
		end();
	}
	void start(uint32_t c)
	{
		startprog(c);
	}
	void end()
	{
		if (!progterm()) endprog();
	}
	void init(uint32_t co)
	{
	}
	void operator()(uint32_t cu)
	{
		setprog(cu);
	}
	void operator()(uint32_t cu, uint32_t co)
	{
		setprog(cu, co);
	}
};

// write progress to void
struct voidhandle {
	voidhandle(std::ostream& = std::cout, uint32_t = 0) {}
	void start(uint32_t) {}
	void end() {}
	void operator()(uint32_t, uint32_t = 0) {}
};

}
