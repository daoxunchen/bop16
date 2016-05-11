#define _CRT_SECURE_NO_WARNINGS

#include "Test.h"

#include <cpprest\http_listener.h>
#include <cpprest\json.h>

using namespace std;
using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;

#ifdef SEPARATE
#include <Windows.h>
#include <vector>
using Id_type = long long;
typedef vector<vector<Id_type>>(*FUNA)(Id_type, Id_type);
const char* dllName = "findPath.dll";
const char* funName = "findPath";
FUNA fp = nullptr;
#else
#include "Entity.h"
extern paths_t findPath(Id_type id1, Id_type id2);
#endif

void handleRequest(http_request req)
{
#ifndef COMPETE_TIME
	AGGLog("**handling request...");

#ifdef AGG_DEBUG_
	//RSLog(L"\n" + req.to_string());
	AGGLog(req.absolute_uri().to_string());
#endif // AGG_DEBUG_
#endif // COMPETE_TIME

	auto q = uri::split_query(req.relative_uri().query());
	
	Id_type start = stoll(q[U("id1")]);
	Id_type end = stoll(q[U("id2")]);
	
#ifdef SEPARATE
	auto path = fp(start, end);	//	findPath
#else
	auto path = findPath(start, end);	//	findPath
#endif

	json::value res;
	for (size_t i = 0; i < path.size(); ++i) {
		json::value tmp;
		for (size_t j = 0; j < path[i].size(); ++j) {
			tmp[j] = path[i].at(j);
		}
		res[i] = tmp;
	}

	req.reply(status_codes::OK, res);

#ifndef COMPETE_TIME
	/*wstring strLog(L"input:");
	strLog += to_wstring(start) + L":" + to_wstring(end);
	RSLog(strLog);
	strLog = L"\n&output json:";
	strLog += res.serialize();
	RSLog(strLog);*/
	AGGLog("**handling over.\n");
#endif // !COMPETE_TIME
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

#ifdef SEPARATE
	HMODULE hDll = LoadLibrary(dllName);
	if (hDll == NULL) {
#ifndef COMPETE_TIME
		AGGLog("Load Dll Err.");
#endif // COMPETE_TIME
		return 1;
	}
	fp = FUNA(GetProcAddress(hDll, funName));
	if (fp == nullptr) {
#ifndef COMPETE_TIME
		AGGLog("Load Function Err.");
#endif
		FreeLibrary(hDll);
		return 2;
	}
#endif
	try {

#ifdef AGG_DEBUG_
		auto threadLog = loger.open();
		thread httpLog([&] { threadLog.wait(); });
		auto threadClean = cleaner.open();
		thread httpClean([&] {threadClean.wait(); });

		httpLog.join();
		httpClean.join();
#endif // AGG_DEBUG_
		
		listener.open()
#ifndef COMPETE_TIME
			.then([=]() {AGGLog("###### start listening..."); })
#endif // !COMPETE_TIME
			.get();
		
		while (true);
	}
	catch (const std::exception& e) {
		AGGLog("@@@@@@Exception: ");
		AGGLog(e.what());
#ifdef SEPARATE
		FreeLibrary(hDll);
#endif
	}

	listener.close();
#ifdef SEPARATE
	FreeLibrary(hDll);
#endif

#ifdef AGG_DEBUG_
	loger.close();
	cleaner.close();
	system("pause");
#endif // AGG_DEBUG_
	return 0;
}