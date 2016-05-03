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

json::value handleJson(json::value val)
{
	json::value res;
	
	if (val.is_array() && val.size() >= 2) {
		auto ents = val.as_array();
		Id_Type ids[2];
		int i = 0;
		for (auto iter = ents.cbegin(); iter != ents.cend(); ++iter) {
			ids[i++] = iter->as_integer();
		}
		auto path = fp(ids[0], ids[1]);
		for (size_t i = 0; i < path.size(); ++i) {
			json::value tmp;
			for (size_t j = 0; j < path[i].size(); ++j) {
				tmp[j] = path[i].at(j);
			}
			res[i] = tmp;
		}
	}

	return res;
}

void handleRequest(http_request req)
{
	RSLog("**handling request...");
	RSLog(req.to_string());
	auto ents = req.extract_json().get();
	auto path = handleJson(ents);
	RSLog("&input json:");
	RSLog(ents.serialize());
	RSLog("&output json:");
	RSLog(path.serialize());
	
	req.reply(status_codes::OK, path);
	RSLog("**handling over.");
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

void handleClean(http_request req)
{
	remove("REST.log");
}

int main()
{
	http_listener listener(U("http://*"));
	http_listener loger(U("http://*/log"));
	http_listener cleaner(U("http://*/clean"));

	listener.support(handleRequest);
	loger.support(handleLog);
	cleaner.support(handleClean);

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