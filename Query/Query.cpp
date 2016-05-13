#include "../Query.h"
#include "../Test.h"

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
	if (res.status_code() != status_codes::OK) {
#ifdef AGG_DEBUG_
		cout << "http_response status_code: " << res.status_code() << endl;
#endif // AGG_DEBUG_
		return json::value();
	} 
	auto val = res.extract_json().get();

	if (val.is_null() || !val.is_object()) {
#ifdef AGG_DEBUG_
		cout << "val is null or is not a object" << endl;
#endif // AGG_DEBUG_
		return json::value();
	}

	return val.as_object().at(U("entities"));
}

QueryAttri Attri(const string_t &attr)
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
				Id_type AfId = 0, AuId = 0;
				auto aa = iterAA->as_object();
				for (auto iteraa = aa.cbegin(); iteraa != aa.cend(); ++iteraa) {
					switch (Attri(iteraa->first)) {
					case QueryAttri::AfId:
						AfId = iteraa->second.as_number().to_int64();
						break;
					case QueryAttri::AuId:
						AuId = iteraa->second.as_number().to_int64();
						break;
					}
				}
				ent1.AAs[AuId] = AfId;
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
					Id_type AfId = 0, AuId = 0;
					auto aa = iterAA->as_object();
					for (auto iteraa = aa.cbegin(); iteraa != aa.cend(); ++iteraa) {
						switch (Attri(iteraa->first)) {
						case QueryAttri::AfId:
							AfId = iteraa->second.as_number().to_int64();
							break;
						case QueryAttri::AuId:
							AuId = iteraa->second.as_number().to_int64();
							break;
						}
					}
					e.AAs[AuId] = AfId;
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
	auto queryTask = baseHttpClient(expr, attr, 10, 0)
		.then(extractResponse)
		.then([&](json::value val) {JsonTo_1_Entities(val, ent); });

	try {
		queryTask.get();
	}
	catch (exception &e) {
		printf("Exception: %s\r\n", e.what());
	}
}

const size_t minQNum = 200;
const size_t maxThreads = 50;
const size_t thresholdNum = minQNum * maxThreads;
void queryCustom(const wstring &expr, Entity_List &ents, const wstring &attr, size_t count, size_t offset)
{
	size_t threadNums, countNum;
	if (count == 0) {
		threadNums = maxThreads;
		countNum = minQNum;
	} else if (count > thresholdNum) {
		threadNums = maxThreads;
		countNum = (count + threadNums - 1) / threadNums;
	} else {
		countNum = minQNum;
		threadNums = (count + countNum - 1) / countNum;
	}

	mutex queryMtx;
	size_t oldSize = ents.size();
	vector<thread> ths(threadNums);
	for (size_t i = 0; i < threadNums; ++i) {
		ths[i] = thread([&, i]() {
			auto query = baseHttpClient(expr, attr, countNum, offset + i * countNum)
				.then(extractResponse)
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

	if (count == 0 && (ents.size() - oldSize) == thresholdNum) {
		queryCustom(expr, ents, attr, 0, offset + thresholdNum);
	}
}

void queryCustomLock(const wstring &expr, Entity_List &ents, mutex &mtx, const wstring &attr, size_t count, size_t offset)
{
	size_t threadNums, countNum;
	if (count == 0) {
		threadNums = maxThreads;
		countNum = minQNum;
	} else if (count > thresholdNum) {
		threadNums = maxThreads;
		countNum = (count + threadNums - 1) / threadNums;
	} else {
		countNum = minQNum;
		threadNums = (count + countNum - 1) / countNum;
	}

	size_t oldSize = ents.size();
	vector<thread> ths(threadNums);
	for (size_t i = 0; i < threadNums; ++i) {
		ths[i] = thread([&, i]() {
			auto query = baseHttpClient(expr, attr, countNum, offset + i * countNum)
				.then(extractResponse)
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
	if (count == 0 && (ents.size() - oldSize) == thresholdNum) {
		queryCustomLock(expr, ents, mtx, attr, 0, offset + thresholdNum);
	}
}
