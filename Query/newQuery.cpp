#include <Windows.h>
#include <winhttp.h>
#include <string>
#include <vector>
#include <algorithm>
#include "rapidjson/document.h"

#include "../newQuery.h"

#pragma comment(lib,"winhttp.lib")

using namespace std;
using namespace rapidjson;
//   /academic/v1.0/evaluate?expr=Composite(AA.AuN==%27jaime%20teevan%27)&count=10000&attributes=Id,F.FId,AA.AuId,AA.AfId,J.JId,C.CId,RId&subscription-key=f7cc29509a8443c5b3a5e56b0e38b5a6
vector<char> baseHttp(const wstring &para)
{
	DWORD dwSize = 0;
	DWORD dwDownloaded = 0;
	vector<char> pszOutBuffer;
	BOOL  bResults = FALSE;
	HINTERNET  hSession = NULL,
		hConnect = NULL,
		hRequest = NULL;

	// Use WinHttpOpen to obtain a session handle.
	hSession = WinHttpOpen(NULL,
		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME,
		WINHTTP_NO_PROXY_BYPASS, 0);

	// Specify an HTTP server.
	if (hSession)
		hConnect = WinHttpConnect(hSession, L"oxfordhk.azure-api.net",
			INTERNET_DEFAULT_HTTPS_PORT, 0);

	// Create an HTTP request handle.
	if (hConnect)
		hRequest = WinHttpOpenRequest(hConnect, L"GET", para.c_str(),
			NULL, WINHTTP_NO_REFERER,
			WINHTTP_DEFAULT_ACCEPT_TYPES,
			WINHTTP_FLAG_SECURE);

	// Send a request.
	if (hRequest)
		bResults = WinHttpSendRequest(hRequest,
			WINHTTP_NO_ADDITIONAL_HEADERS, 0,
			WINHTTP_NO_REQUEST_DATA, 0,
			0, 0);


	// End the request.
	if (bResults)
		bResults = WinHttpReceiveResponse(hRequest, NULL);

	// Keep checking for data until there is nothing left.
	if (bResults) {
		do {
			// Check for available data.
			dwSize = 0;
			if (!WinHttpQueryDataAvailable(hRequest, &dwSize))
				printf("Error %u in WinHttpQueryDataAvailable.\n",
					GetLastError());

			// Allocate space for the buffer.
			pszOutBuffer.insert(pszOutBuffer.end(), dwSize, 0);
			
			// Read the data.
			if (!WinHttpReadData(hRequest, (LPVOID)(pszOutBuffer.data() + pszOutBuffer.size() - dwSize),
				dwSize, &dwDownloaded))
				printf("Error %u in WinHttpReadData.\n", GetLastError());
		} while (dwSize > 0);
	}

	// Report any errors.
	if (!bResults)
		printf("Error %d has occurred.\n", GetLastError());

	// Close any open handles.
	if (hRequest) WinHttpCloseHandle(hRequest);
	if (hConnect) WinHttpCloseHandle(hConnect);
	if (hSession) WinHttpCloseHandle(hSession);
	pszOutBuffer.push_back(0);
	return pszOutBuffer;
}

wstring queryPara(const wstring &expr, const wstring &attr, size_t count, size_t offset)
{
	wstring para(L"/academic/v1.0/evaluate?subscription-key=f7cc29509a8443c5b3a5e56b0e38b5a6&expr=");
	para += expr;
	para += L"&attributes=";
	para += attr;
	para += L"&count=" + to_wstring(count);
	para += L"&offset=" + to_wstring(offset);
	return para;
}

QueryAttri Attri(const char *attr)
{
	switch (attr[0]) {
	case 'I': return QueryAttri::Id;
	case 'R': return QueryAttri::RId;
	case 'F': return QueryAttri::FId;
	case 'C': return QueryAttri::CId;
	case 'J': return QueryAttri::JId;
	case 'A': {
		switch (attr[1]) {
		case 'A': return QueryAttri::AA;
		case 'u': return QueryAttri::AuId;
		case 'f': return QueryAttri::AfId;
		}
	}
	default:return QueryAttri::Log;
	}
}

void JsonTo_1_Entities(const vector<char> &val, entity &e)
{
	if (val.size() <= 2) { printf("val empty"); return; }
	Document doc;
	if (!doc.Parse(val.data()).IsObject()) { printf("doc not object"); return; }
	auto entities = doc["entities"].GetArray();
	auto iter = entities.Begin();
	for (auto iter2 = iter->MemberBegin(); iter2 != iter->MemberEnd(); ++iter2) {
		switch (Attri(iter2->name.GetString())) {
		case QueryAttri::Id:
			e.Id = iter2->value.GetInt64();
			break;
		case QueryAttri::FId: {
			auto Fs = iter2->value.GetArray();
			for (auto iterF = Fs.Begin(); iterF != Fs.End(); ++iterF) {
				e.F_Id.emplace_back(iterF->MemberBegin()->value.GetInt64());
			}
		}break;
		case QueryAttri::JId:
			e.J_Id = iter2->value.MemberBegin()->value.GetInt64();
			break;
		case QueryAttri::CId:
			e.C_Id = iter2->value.MemberBegin()->value.GetInt64();
			break;
		case QueryAttri::AA: {
			auto As = iter2->value.GetArray();
			for (auto iterAA = As.Begin(); iterAA != As.End(); ++iterAA) {
				Id_type AfId = 0, AuId = 0;
				for (auto iteraa = iterAA->MemberBegin(); iteraa != iterAA->MemberEnd(); ++iteraa) {
					switch (Attri(iteraa->name.GetString())) {
					case QueryAttri::AfId:
						AfId = iteraa->value.GetInt64();
						break;
					case QueryAttri::AuId:
						AuId = iteraa->value.GetInt64();
						break;
					}
				}
				e.AAs[AuId] = AfId;
			}
		}break;
		case QueryAttri::RId: {
			auto Rs = iter2->value.GetArray();
			for (auto iterR = Rs.Begin(); iterR != Rs.End(); ++iterR) {
				e.R_Id.emplace_back(iterR->GetInt64());
			}
		}break;
		}
	}
}

void JsonToEntities(const vector<char> &val, vector<entity> &ents, mutex &mtx)
{
	if (val.size() <= 2) { printf("val empty"); return; }
	Document doc;
	if (!doc.Parse(val.data()).IsObject()) { printf("doc not object"); return; }
	vector<entity> tmpEnts;
	auto entities = doc["entities"].GetArray();
	for (auto iter = entities.Begin(); iter != entities.End(); ++iter) {
		entity e;
		for (auto iter2 = iter->MemberBegin(); iter2 != iter->MemberEnd(); ++iter2) {
			switch (Attri(iter2->name.GetString())) {
			case QueryAttri::Id:
				e.Id = iter2->value.GetInt64();
				break;
			case QueryAttri::FId: {
				auto Fs = iter2->value.GetArray();
				for (auto iterF = Fs.Begin(); iterF != Fs.End(); ++iterF) {
					e.F_Id.emplace_back(iterF->MemberBegin()->value.GetInt64());
				}
			}break;
			case QueryAttri::JId:
				e.J_Id = iter2->value.MemberBegin()->value.GetInt64();
				break;
			case QueryAttri::CId:
				e.C_Id = iter2->value.MemberBegin()->value.GetInt64();
				break;
			case QueryAttri::AA: {
				auto As = iter2->value.GetArray();
				for (auto iterAA = As.Begin(); iterAA != As.End(); ++iterAA) {
					Id_type AfId = 0, AuId = 0;
					for (auto iteraa = iterAA->MemberBegin(); iteraa != iterAA->MemberEnd(); ++iteraa) {
						switch (Attri(iteraa->name.GetString())) {
						case QueryAttri::AfId:
							AfId = iteraa->value.GetInt64();
							break;
						case QueryAttri::AuId:
							AuId = iteraa->value.GetInt64();
							break;
						}
					}
					e.AAs[AuId] = AfId;
				}
			}break;
			case QueryAttri::RId: {
				auto Rs = iter2->value.GetArray();
				for (auto iterR = Rs.Begin(); iterR != Rs.End(); ++iterR) {
					e.R_Id.emplace_back(iterR->GetInt64());
				}
			}break;
			}
		}
		tmpEnts.emplace_back(e);
	}
	if (tmpEnts.empty()) return;
	mtx.lock();
	copy(tmpEnts.begin(), tmpEnts.end(), back_inserter(ents));
	mtx.unlock();
}

//////////////////////////////////////////////////////////////
void queryOne(const std::wstring &expr, entity &ent1, const std::wstring &attr)
{
	auto res = baseHttp(queryPara(expr, attr, 10, 0));
	JsonTo_1_Entities(res, ent1);
}

const size_t minQNum = 200;
const size_t maxThreads = 20;
const size_t thresholdNum = minQNum * maxThreads;
void queryCustom(const std::wstring &expr, Entity_List &ents, 
	const std::wstring &attr, size_t count, size_t offset)
{
	size_t threadNums, countNum;
	threadNums = maxThreads;
	countNum = count / maxThreads + 1;

	mutex queryMtx;
	size_t oldSize = ents.size();
	vector<thread> ths(threadNums);
	for (size_t i = 0; i < threadNums; ++i) {
		ths[i] = thread([&, i]() {
			auto res = baseHttp(queryPara(expr, attr, countNum, offset + i * countNum));
			JsonToEntities(res, ents, queryMtx);
		});
	}
	for_each(ths.begin(), ths.end(), [](thread &th) {th.join(); });

	if ((ents.size() - oldSize) == (countNum*threadNums)) {
#ifdef AGG_DEBUG_
		cout << "second time query88888888888888888888888888" << endl;
#endif // AGG_DEBUG_
		queryCustom(expr, ents, attr, count, offset + countNum*threadNums);
	}
}

void queryCustomLock(const std::wstring &expr, Entity_List &ents, std::mutex &mtx,
	const std::wstring &attr, size_t count, size_t offset)
{
	size_t threadNums, countNum;
	threadNums = maxThreads;
	countNum = count / maxThreads + 1;

	size_t oldSize = ents.size();
	vector<thread> ths(threadNums);
	for (size_t i = 0; i < threadNums; ++i) {
		ths[i] = thread([&, i]() {
			auto res = baseHttp(queryPara(expr, attr, countNum, offset + i * countNum));
			JsonToEntities(res, ents, mtx);
		});
	}
	for_each(ths.begin(), ths.end(), [](thread &th) {th.join(); });
	if ((ents.size() - oldSize) == (countNum*threadNums)) {
#ifdef AGG_DEBUG_
		cout << "second time query88888888888888888888888888" << endl;
#endif // AGG_DEBUG_
		queryCustomLock(expr, ents, mtx, attr, count, offset + countNum*threadNums);
	}
}