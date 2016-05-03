#define _CRT_SECURE_NO_WARNINGS

#define AGG_DEBUG_

#ifdef AGG_DEBUG_
#include <iostream>
#endif // AGG_DEBUG_

#include <vector>
#include <fstream>
#include <ctime>
#include <thread>
#include <Windows.h>

#include <cpprest\http_listener.h>
#include <cpprest\json.h>

using namespace std;
using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;

using Id_Type = __int64;

typedef vector<vector<Id_Type>>(*FUNA)(Id_Type&, Id_Type&);

const char* dllName = "findPath.dll";
const char* funName = "findPath";
FUNA fp = nullptr;

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

void handleRequest(http_request req)
{
	RSLog("**handling request...");

#ifdef AGG_DEBUG_
	RSLog(req.to_string());
#endif // AGG_DEBUG_

	auto q = uri::split_query(req.relative_uri().query());

	auto iter = q.cbegin();

	Id_Type start = stoll(iter->second);
	Id_Type end = stoll((++iter)->second);

	json::value res;
	auto path = fp(start, end);
	for (size_t i = 0; i < path.size(); ++i) {
		json::value tmp;
		for (size_t j = 0; j < path[i].size(); ++j) {
			tmp[j] = path[i].at(j);
		}
		res[i] = tmp;
	}

	req.reply(status_codes::OK, res);

	wstring strLog(L"input:");
	strLog += to_wstring(start) + L":" + to_wstring(end);
	strLog += L"\n&output json:";
	strLog += res.serialize();
	strLog += L"**handling over.\n";
	RSLog(strLog);
}

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

int main()
{
	http_listener listener(U("http://*/test"));
	http_listener loger(U("http://*/log"));
	http_listener cleaner(U("http://*/clean"));

	listener.support(methods::GET, handleRequest);
	loger.support(handleLog);
	cleaner.support([](http_request req) {remove("REST.log"); });

	HMODULE hDll = LoadLibrary(dllName);
	if (hDll == NULL) {
		RSLog("Load Dll Err.");
		return 1;
	}
	fp = FUNA(GetProcAddress(hDll, funName));
	if (fp == nullptr) {
		RSLog("Load Function Err.");
		FreeLibrary(hDll);
		return 2;
	}

	try {
		auto threadLog = loger.open();
		thread httpLog([&] { threadLog.wait(); });
		auto threadClean = cleaner.open();
		thread httpClean([&] {threadClean.wait(); });
		
		httpLog.join();
		httpClean.join();

		listener
			.open()
			.then([=]() {
			RSLog("###### start listening...");
			}).get();
		
		while (true);
	}
	catch (const std::exception& e) {
		RSLog("@@@@@@Exception: ");
		RSLog(e.what());
	}

	listener.close();
	FreeLibrary(hDll);

#ifdef AGG_DEBUG_
	system("pause");
#endif // AGG_DEBUG_
	return 0;
}