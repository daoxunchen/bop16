#define _CRT_SECURE_NO_WARNINGS

#define NOT_COMPETE_MODE

#define AGG_DEBUG_

#ifndef NOT_COMPETE_MODE
#undef AGG_DEBUG_
#endif

#ifdef AGG_DEBUG_
#include <iostream>
#include <thread>
#endif // AGG_DEBUG_

#ifdef NOT_COMPETE_MODE
#include <ctime>
#include <fstream>
#endif // !COMPETE_MODE

#include <vector>
#include <Windows.h>

#include <cpprest\http_listener.h>
#include <cpprest\json.h>

using namespace std;
using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;

using Id_Type = __int64;

typedef vector<Id_Type>(*FUNA)(Id_Type&, Id_Type&);

const char* dllName = "findPath.dll";
const char* funName = "findPath";
FUNA fp = nullptr;


#ifdef NOT_COMPETE_MODE
template<typename T>
void RSLog(const T str)
{
	wofstream logfile("REST.log", wofstream::app);
	time_t Time = time(nullptr);
	auto strTime = ctime(&Time);
	wstring res(strTime + 4, strTime + 19);	// remove year and week
	res.append(L"    \t");
	logfile << res << str << endl;
#ifdef AGG_DEBUG_
	wcout << res << str << endl;
#endif // AGG_DEBUG_
	logfile.close();
}
#endif

void handleRequest(http_request req)
{
#ifdef NOT_COMPETE_MODE
	RSLog("**handling request...");

#ifdef AGG_DEBUG_
	RSLog(req.to_string());
#endif // AGG_DEBUG_
#endif // NOT_COMPETE_MODE

	auto q = uri::split_query(req.relative_uri().query());

	auto iter = q.cbegin();

	Id_Type start = stoll(iter->second);
	Id_Type end = stoll((++iter)->second);

	json::value res;
	auto path = fp(start, end);
	for (size_t i = 0; i < path.size() / 4; ++i) {
		json::value tmp;
		for (size_t j = 0; j < 4; ++j) {
			auto tmpid = path[4 * i + j];
			if (tmpid == 0) break;
			tmp[j] = tmpid;
		}
		res[i] = tmp;
	}

	req.reply(status_codes::OK, res);

#ifdef NOT_COMPETE_MODE
	wstring strLog(L"input:");
	strLog += to_wstring(start) + L":" + to_wstring(end);
	RSLog(strLog);
	strLog = L"\n&output json:";
	strLog += res.serialize();
	RSLog(strLog);
	RSLog("**handling over.\n");
#endif // !COMPETE_MODE
}

#ifdef AGG_DEBUG_
void handleLog(http_request req)
{
	ifstream logfile("REST.log");
	if (!logfile) {
		req.reply(status_codes::OK);
		return;
	}
	string tmp((istreambuf_iterator<char>(logfile)), istreambuf_iterator<char>());
	req.reply(status_codes::OK, tmp);
	logfile.close();
}
#endif // AGG_DEBUG_

int main()
{
#ifdef AGG_DEBUG_
	http_listener loger(U("http://*/log"));
	http_listener cleaner(U("http://*/clean"));

	loger.support(handleLog);
	cleaner.support([](http_request req) {remove("REST.log"); });
#endif // AGG_DEBUG_
	
	http_listener listener(U("http://*/test"));
	listener.support(methods::GET, handleRequest);

	HMODULE hDll = LoadLibrary(dllName);
	if (hDll == NULL) {
#ifdef NOT_COMPETE_MODE
		RSLog("Load Dll Err.");
#endif // COMPETE_MODE
		return 1;
	}
	fp = FUNA(GetProcAddress(hDll, funName));
	if (fp == nullptr) {
#ifdef NOT_COMPETE_MODE
		RSLog("Load Function Err.");
#endif
		FreeLibrary(hDll);
		return 2;
	}

	try {

#ifdef AGG_DEBUG_
		auto threadLog = loger.open();
		thread httpLog([&] { threadLog.wait(); });
		auto threadClean = cleaner.open();
		thread httpClean([&] {threadClean.wait(); });

		httpLog.join();
		httpClean.join();
#endif // AGG_DEBUG_
		
		listener
			.open()
#ifdef NOT_COMPETE_MODE
			.then([=]() 
		{
			RSLog("###### start listening...");
		})
#endif // !NOT_COMPETE_MODE
			
			.get();
		
		while (true);
	}
	catch (const std::exception& e) {
		RSLog("@@@@@@Exception: ");
		RSLog(e.what());
	}

	listener.close();
	FreeLibrary(hDll);

#ifdef AGG_DEBUG_
	loger.close();
	cleaner.close();
	system("pause");
#endif // AGG_DEBUG_
	return 0;
}