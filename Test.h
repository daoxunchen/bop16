#pragma once

//#define SEPARATE
//#define ACY
//#define COMPETE_TIME

#ifndef COMPETE_TIME
#define AGG_DEBUG_
#endif // !COMPETE_TIME

#ifndef COMPETE_TIME
#include <ctime>
#include <fstream>
#endif // !COMPETE_TIME

#ifdef AGG_DEBUG_
#include <iostream>
#endif // AGG_DEBUG_

#ifndef COMPETE_TIME
template<typename T>
void AGGLog(const T str)
{
	std::wofstream logfile("REST.log", wofstream::app);
	time_t Time = time(nullptr);
	auto strTime = ctime(&Time);
	std::wstring res(strTime + 4, strTime + 19);	// remove year and week
	res.append(L"    \t");
	logfile << res << str << std::endl;
#ifdef AGG_DEBUG_
	std::wcout << res << str << std::endl;
#endif // AGG_DEBUG_
	logfile.close();
}
#endif // !COMPETE_TIME

