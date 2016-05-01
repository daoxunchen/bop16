#define _CRT_SECURE_NO_WARNINGS

#define AGG_DEBUG_

#ifdef AGG_DEBUG_
#include <iostream>
#endif // AGG_DEBUG_

#include <vector>
#include <fstream>
#include <ctime>
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
	ofstream logfile("logfile.log", ofstream::app);
	auto t = time(nullptr);
	logfile << ctime(&t) << '\t' << str << endl;
#ifdef AGG_DEBUG_
	cout << ctime(&t) << '\t' << str << endl;
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
	RSLog("handling GET...");
	auto ents = req.extract_json().get();
	auto path = handleJson(ents);
	RSLog("input json:");
	RSLog(ents.serialize().c_str());
	RSLog("output json:");
	RSLog(path.serialize().c_str());
	
	req.reply(status_codes::OK, path);
	RSLog("handling over.");
}

int main()
{
	http_listener listener(U("http://*"));
	listener.support(methods::GET, handleRequest);

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
		listener
			.open()
			.then([=]() {
			RSLog("start listening...");
			}).wait();

		while (true);
	}
	catch (const std::exception& e) {
		RSLog("\t\t\tException: ");
		RSLog(e.what());
	}

	listener.close();
	FreeLibrary(hDll);

#ifdef AGG_DEBUG_
	system("pause");
#endif // AGG_DEBUG_
	return 0;
}