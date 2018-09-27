/*
 * This file is part of crunch
 * Copyright © 2013 Rachel Mant (dx-mon@users.sourceforge.net)
 *
 * crunch is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * crunch is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Core.h"
#include "Logger.h"
#ifndef _MSC_VER
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#else
#include <io.h>
#include <sys/locking.h>
#endif
#include <stdio.h>

#ifndef _MSC_VER
#define COL(val) val - 8
#else
#define COL(val) int16_t(val - 9)
#endif
#define WCOL(val) val - 2

#ifdef _MSC_VER
	#define TTY	"CON"
HANDLE console;
	#if _MSC_VER < 1900L
FILE *stdout;
	#endif
#else
	#define TTY	"/dev/tty"
#endif

FILE *realStdout = nullptr;
bool logging = false;
testLog *logger = nullptr;
uint8_t isTTY = 1;

int getColumns()
{
#ifndef _MSC_VER
	struct winsize win;
	ioctl(STDIN_FILENO, TIOCGWINSZ, &win);
	return (win.ws_col == 0 ? 80 : win.ws_col);
#else
	CONSOLE_SCREEN_BUFFER_INFO window;
	GetConsoleScreenBufferInfo(console, &window);
	return window.dwSize.X;
#endif
}

size_t vaTestPrintf(const char *format, va_list args)
{
	if (realStdout == nullptr)
		realStdout = stdout;
	return vfprintf(realStdout, format, args);
}

size_t testPrintf(const char *format, ...)
{
	size_t ret;
	va_list args;
	va_start(args, format);
	ret = vaTestPrintf(format, args);
	va_end(args);
	return ret;
}

void printOk() { testPrintf(" [  OK  ]\n"); }
void printFailure() { testPrintf(" [ FAIL ]\n"); }
void printSkip() { testPrintf(" [ SKIP ]\n"); }
void printAborted() { testPrintf("[ **** ABORTED **** ]\n"); }

void normal()
{
#ifndef _MSC_VER
	testPrintf(NORMAL);
#else
	SetConsoleTextAttribute(console, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#endif
}

#ifndef _MSC_VER
void echoOk()
{
	if (isTTY)
		testPrintf(CURS_UP SET_COL BRACKET "[" SUCCESS "  OK  " BRACKET "]" NEWLINE, COL(getColumns()));
	else
		printOk();
	++passes;
}

void echoFailure()
{
	if (isTTY)
		testPrintf(" " SET_COL BRACKET "[" FAILURE " FAIL " BRACKET "]" NEWLINE, COL(getColumns()));
	else
		printFailure();
	++failures;
}

void echoSkip()
{
	if (isTTY)
		testPrintf(" " SET_COL BRACKET "[" WARNING " SKIP " BRACKET "]" NEWLINE, COL(getColumns()));
	else
		printSkip();
	++passes;
}

void echoAborted()
{
	if (isTTY)
		testPrintf("\n" BRACKET "[" FAILURE " **** ABORTED **** " BRACKET "]" NEWLINE);
	else
		printAborted();
	throw threadExit_t(2);
}
#else
void echoOk()
{
	if (isTTY)
	{
		CONSOLE_SCREEN_BUFFER_INFO cursor;
		GetConsoleScreenBufferInfo(console, &cursor);
		cursor.dwCursorPosition.Y--;
		cursor.dwCursorPosition.X = COL(getColumns());
		SetConsoleCursorPosition(console, cursor.dwCursorPosition);
		SetConsoleTextAttribute(console, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
		testPrintf("[");
		SetConsoleTextAttribute(console, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		testPrintf("  OK  ");
		SetConsoleTextAttribute(console, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
		testPrintf("]");
		SetConsoleTextAttribute(console, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
		testPrintf("\n");
	}
	else
		printOk();
	passes++;
}

void echoFailure()
{
	if (isTTY)
	{
		CONSOLE_SCREEN_BUFFER_INFO cursor;
		GetConsoleScreenBufferInfo(console, &cursor);
		cursor.dwCursorPosition.X = COL(getColumns());
		SetConsoleCursorPosition(console, cursor.dwCursorPosition);
		SetConsoleTextAttribute(console, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
		testPrintf("[");
		SetConsoleTextAttribute(console, FOREGROUND_RED | FOREGROUND_INTENSITY);
		testPrintf(" FAIL ");
		SetConsoleTextAttribute(console, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
		testPrintf("]");
		SetConsoleTextAttribute(console, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
		testPrintf("\n");
	}
	else
		printFailure();
	failures++;
}

void echoSkip()
{
	if (isTTY)
	{
		CONSOLE_SCREEN_BUFFER_INFO cursor;
		GetConsoleScreenBufferInfo(console, &cursor);
		cursor.dwCursorPosition.X = COL(getColumns());
		SetConsoleCursorPosition(console, cursor.dwCursorPosition);
		SetConsoleTextAttribute(console, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
		testPrintf("[");
		SetConsoleTextAttribute(console, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		testPrintf(" SKIP ");
		SetConsoleTextAttribute(console, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
		testPrintf("]");
		SetConsoleTextAttribute(console, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
		testPrintf("\n");
	}
	else
		printSkip();
}

void echoAborted()
{
	if (isTTY)
	{
		SetConsoleTextAttribute(console, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
		testPrintf("[");
		SetConsoleTextAttribute(console, FOREGROUND_RED | FOREGROUND_INTENSITY);
		testPrintf(" **** ABORTED **** ");
		SetConsoleTextAttribute(console, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
		testPrintf("]");
		SetConsoleTextAttribute(console, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
		testPrintf("\n");
	}
	else
		printAborted();
	throw threadExit_t(2);
}
#endif

void logResult(resultType type, const char *message, ...)
{
	va_list args;

	if (isTTY)
		normal();
	va_start(args, message);
	vaTestPrintf(message, args);
	va_end(args);
	switch (type)
	{
		case RESULT_SUCCESS:
			echoOk();
			break;
		case RESULT_FAILURE:
			echoFailure();
			break;
		case RESULT_SKIP:
			echoSkip();
			break;
		default:
			echoAborted();
	}
}

testLog *startLogging(const char *fileName)
{
	testLog *ret;
	if (logging)
		return nullptr;
	ret = new testLog();
	logging = true;
#ifndef _MSC_VER
	ret->stdout = dup(STDOUT_FILENO);
#else
	ret->stdout = dup(fileno(stdout));
#endif
	realStdout = stdout;
	ret->file = fopen(fileName, "w");
	ret->fd = fileno(ret->file);
#ifndef _MSC_VER
	flock(ret->fd, LOCK_EX);
	dup2(STDOUT_FILENO, ret->fd);
#else
//	locking(ret->fd, LK_LOCK, -1);
	dup2(fileno(stdout), ret->fd);
#endif
	logger = ret;
	stdout = ret->file;
	return ret;
}

void stopLogging(testLog *logFile)
{
	if (logFile == nullptr)
		return;
#ifndef _MSC_VER
	flock(logFile->fd, LOCK_UN);
	dup2(logFile->stdout, STDOUT_FILENO);
#else
//	locking(logFile->fd, LK_UNLCK, -1);
	dup2(logFile->stdout, fileno(stdout));
#endif
	fclose(logFile->file);
	stdout = realStdout;
	realStdout = nullptr;
	delete logFile;
	logger = nullptr;
	logging = false;
}
