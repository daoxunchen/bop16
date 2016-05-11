#include "../Query.h"

#define ACY
#ifdef ACY
#include <iostream>
#endif

#define QUERY_DEBUG_

#include <iterator>
#include <thread>
#include <cpprest/http_client.h>
#include <cpprest/uri_builder.h>

using namespace std;

using namespace utility;
using namespace web;
using namespace web::http;
using namespace web::http::client;
using namespace web::json;

pplx::task<http_response> baseHttpClient(const wstring &expr, const wstring &attr, size_t count, size_t offset)
{
#ifndef ACY
	http_client queryClient(U("https://oxfordhk.azure-api.net"));
#else
	http_client_config quertConfig;
	quertConfig.set_proxy(web_proxy(U("http://127.0.0.1:1080")));
	http_client queryClient(U("https://oxfordhk.azure-api.net"), quertConfig);
#endif // ACY

	uri_builder builder(U("/academic/v1.0/evaluate"));
	builder.append_query(U("subscription-key"), U("f7cc29509a8443c5b3a5e56b0e38b5a6"));
	
	////////////////////////////////////////////////////////
	builder.append_query(U("expr"), expr);
	builder.append_query(U("count"), to_wstring(count));
	builder.append_query(U("offset"), to_wstring(offset));
	builder.append_query(U("attributes"), attr);
	///////////////////////////////////////////////////////

	return queryClient.request(methods::GET, builder.to_string());
}

json::value extractResponse(const http_response &res)
{
	if (res.status_code() == status_codes::OK) return res.extract_json().get();
#ifdef QUERY_DEBUG_
	else cout << "http_response status_code: " << res.status_code() << endl;
#endif // QUERY_DEBUG_
	return json::value();
}

json::value extractJson(const json::value &val)
{
	if (val.is_null() || !val.is_object()) {
#ifdef QUERY_DEBUG_
		cout << "val is null or is not a object" << endl;
#endif // QUERY_DEBUG_

		return json::value();
	}
	
	auto obj = val.as_object();
	return obj.at(U("entities"));
}

QueryAttri Attri(string_t attr)
{
	switch (attr[0]) {
	case U('I'): return QueryAttri::Id;
	case U('R'): return QueryAttri::RId;
	case U('F'): return QueryAttri::FId;
	case U('C'): return QueryAttri::CId;
	case U('J'): return QueryAttri::JId;
	case U('A'): {
		switch (attr[1]) {
		case U('A'): return QueryAttri::AA;
		case U('u'): return QueryAttri::AuId;
		case U('f'): return QueryAttri::AfId;
		}
	}
	default:return QueryAttri::Log;
	}
}

void JsonTo_1_Entities(const json::value &val, entity &ent1)
{
	if (val.is_null()) return;
	auto valarray = val.as_array();
	if (valarray.size() == 0) return;
	auto ent = valarray.cbegin()->as_object();
	for (auto iter2 = ent.cbegin(); iter2 != ent.cend(); ++iter2) {
		switch (Attri(iter2->first)) {
		case QueryAttri::Id:
			ent1.Id = iter2->second.as_number().to_int64();
			break;
		case QueryAttri::FId: {
			auto Fs = iter2->second.as_array();
			for (auto iterF = Fs.cbegin(); iterF != Fs.cend(); ++iterF) {
				ent1.F_Id.emplace_back(iterF->at(U("FId")).as_number().to_int64());
			}
		}break;
		case QueryAttri::JId:
			ent1.J_Id = (iter2->second).as_object().at(U("JId")).as_number().to_int64();
			break;
		case QueryAttri::CId:
			ent1.C_Id = (iter2->second).as_object().at(U("CId")).as_number().to_int64();
			break;
		case QueryAttri::AA: {
			auto As = iter2->second.as_array();
			for (auto iterAA = As.cbegin(); iterAA != As.cend(); ++iterAA) {
				AA a;
				auto aa = iterAA->as_object();
				for (auto iteraa = aa.cbegin(); iteraa != aa.cend(); ++iteraa) {
					switch (Attri(iteraa->first)) {
					case QueryAttri::AfId:
						a.AfId = iteraa->second.as_number().to_int64();
						break;
					case QueryAttri::AuId:
						a.AuId = iteraa->second.as_number().to_int64();
						break;
					}
				}
				ent1.AAs.emplace_back(a);
			}
		}break;
		case QueryAttri::RId: {
			auto Rs = iter2->second.as_array();
			for (auto iterR = Rs.cbegin(); iterR != Rs.cend(); ++iterR) {
				ent1.R_Id.emplace_back(iterR->as_number().to_int64());
			}
		}break;
		}
	}
}

void JsonToEntities(const json::value &val, vector<entity> &ents, mutex &mtx)
{
	if (val.is_null()) return;
	auto valarray = val.as_array();
	vector<entity> tmpEnts;
	for (auto iter = valarray.cbegin(); iter != valarray.cend(); ++iter) {
		auto ent = iter->as_object();
		entity e;
		for (auto iter2 = ent.cbegin(); iter2 != ent.cend(); ++iter2) {
			switch (Attri(iter2->first)) {
			case QueryAttri::Id:
				e.Id = iter2->second.as_number().to_int64();
				break;
			case QueryAttri::FId: {
				auto Fs = iter2->second.as_array();
				for (auto iterF = Fs.cbegin(); iterF != Fs.cend(); ++iterF) {
					e.F_Id.emplace_back(iterF->at(U("FId")).as_number().to_int64());
				}
			}break;
			case QueryAttri::JId:
				e.J_Id = (iter2->second).as_object().at(U("JId")).as_number().to_int64();
				break;
			case QueryAttri::CId:
				e.C_Id = (iter2->second).as_object().at(U("CId")).as_number().to_int64();
				break;
			case QueryAttri::AA: {
				auto As = iter2->second.as_array();
				for (auto iterAA = As.cbegin(); iterAA != As.cend(); ++iterAA) {
					AA a;
					auto aa = iterAA->as_object();
					for (auto iteraa = aa.cbegin(); iteraa != aa.cend(); ++iteraa) {
						switch (Attri(iteraa->first)) {
						case QueryAttri::AfId:
							a.AfId = iteraa->second.as_number().to_int64();
							break;
						case QueryAttri::AuId:
							a.AuId = iteraa->second.as_number().to_int64();
							break;
						}
					}
					e.AAs.emplace_back(a);
				}
			}break;
			case QueryAttri::RId: {
				auto Rs = iter2->second.as_array();
				for (auto iterR = Rs.cbegin(); iterR != Rs.cend(); ++iterR) {
					e.R_Id.emplace_back(iterR->as_number().to_int64());
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

void queryOne(const wstring &expr, entity &ent, const wstring &attr)
{
	auto query = baseHttpClient(expr, attr, 10, 0)
		.then(extractResponse)
		.then(extractJson)
		.then([&](json::value val) {JsonTo_1_Entities(val, ent); });

	try {
		query.get();
	}
	catch (exception &e) {
		printf("Exception: %s\r\n", e.what());
	}
}

const size_t countNum = 400;
const size_t mexThreads = 50;
void queryCustom(const wstring &expr, Entity_List &ents, const wstring &attr, size_t count, size_t offset)
{
	size_t cnt = count == 0 ? defaultCount : count;
	size_t oldSize = ents.size();
	size_t threadNums = (cnt + countNum - 1) / countNum;
	mutex queryMtx;
	vector<thread> ths(threadNums);
	for (size_t i = 0; i < threadNums; ++i) {
		ths[i] = thread([&, i]() {
			auto query = baseHttpClient(expr, attr, countNum, offset + i * countNum)
				.then(extractResponse)
				.then(extractJson)
				.then([&](json::value val) {return JsonToEntities(val, ents, queryMtx); });

			try {
				query.get();
			}
			catch (exception &e) {
				printf("Exception: %s\r\n", e.what());
			}
		});
	}
	for_each(ths.begin(), ths.end(), [](thread &th) {th.join(); });

	if (count == 0 && (ents.size() - oldSize) == defaultCount) {
		queryCustom(expr, ents, attr, 0, offset + defaultCount);
	}
}

void queryCustomLock(const wstring &expr, Entity_List &ents, mutex &mtx, const wstring &attr, size_t count, size_t offset)
{
	size_t cnt = count == 0 ? defaultCount : count;
	size_t oldSize = ents.size();
	size_t threadNums = (cnt + countNum - 1) / countNum;
	vector<thread> ths(threadNums);
	for (size_t i = 0; i < threadNums; ++i) {
		ths[i] = thread([&, i]() {
			auto query = baseHttpClient(expr, attr, countNum, offset + i * countNum)
				.then(extractResponse)
				.then(extractJson)
				.then([&](json::value val) { JsonToEntities(val, ents, mtx); });

			try {
				query.get();
			}
			catch (exception &e) {
				printf("Exception: %s\r\n", e.what());
			}
		});
	}
	for_each(ths.begin(), ths.end(), [](thread &th) {th.join(); });
	if (count == 0 && (ents.size() - oldSize) == defaultCount) {
		queryCustomLock(expr, ents, mtx, attr, 0, offset + defaultCount);
	}
}
