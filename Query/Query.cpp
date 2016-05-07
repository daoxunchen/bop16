#include "../Query.h"

#define ACY
#define QUERY_DEBUG_

#include <cpprest/http_client.h>
#include <cpprest/uri_builder.h>

using namespace std;

using namespace utility;
using namespace web;
using namespace web::http;
using namespace web::http::client;
using namespace web::json;

pplx::task<http_response> baseHttpClient(const wstring &expr, size_t count, size_t offset)
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
	builder.append_query(U("attributes"), U("Id,F.FId,AA.AuId,AA.AfId,J.JId,C.CId,RId"));
	
	////////////////////////////////////////////////////////
	builder.append_query(U("count"), to_wstring(count));
	builder.append_query(U("offset"), to_wstring(offset));
	///////////////////////////////////////////////////////
	
	builder.append_query(U("expr"), expr);
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
	}
}

void JsonToEntities(const json::value &val, vector<entity> &ents)
{
	if (val.is_null()) return;

	auto valarray = val.as_array();
	for (auto iter = valarray.cbegin(); iter != valarray.cend(); ++iter) {
		auto ent = iter->as_object();
		entity e;
		for (auto iter2 = ent.cbegin(); iter2 != ent.cend(); ++iter2) {
			switch (Attri(iter2->first)) {
			case QueryAttri::Id:
				e.Id = iter2->second.as_integer();
				break;
			case QueryAttri::FId: {
				auto Fs = iter2->second.as_array();
				for (auto iterF = Fs.cbegin(); iterF != Fs.cend(); ++iterF) {
					e.F_Id.push_back(iterF->at(U("FId")).as_integer());
				}
			}break;
			case QueryAttri::JId:
				e.J_Id = (iter2->second).as_object().at(U("JId")).as_integer();
				break;
			case QueryAttri::CId:
				e.C_Id = (iter2->second).as_object().at(U("CId")).as_integer();
				break;
			case QueryAttri::AA: {
				auto As = iter2->second.as_array();
				for (auto iterAA = As.cbegin(); iterAA != As.cend(); ++iterAA) {
					AA a;
					auto aa = iterAA->as_object();
					for (auto iteraa = aa.cbegin(); iteraa != aa.cend(); ++iteraa) {
						switch (Attri(iteraa->first)) {
						case QueryAttri::AfId:
							a.AfId = iteraa->second.as_integer();
							break;
						case QueryAttri::AuId:
							a.AuId = iteraa->second.as_integer();
							break;
						}
					}
					e.AAs.push_back(a);
				}
			}break;
			case QueryAttri::RId:{
				auto Rs = iter2->second.as_array();
				for (auto iterR = Rs.cbegin(); iterR != Rs.cend(); ++iterR) {
					e.R_Id.push_back(iterR->as_integer());
				}
			}break;
			}
		}
		ents.push_back(e);
	}
}

void queryEntity(QueryAttri qa, Id_type id, Entity_List &ents, size_t count, size_t offset)
{
	string_t expr;
	switch (qa) {
	case QueryAttri::Id: expr = U("Id=") + to_wstring(id);
	break;
	case QueryAttri::FId: expr = U("Composite(F.FId=") + to_wstring(id) + U(")");
	break;
	case QueryAttri::JId: expr = U("Composite(J.JId=") + to_wstring(id) + U(")");
	break;
	case QueryAttri::CId: expr = U("Composite(C.CId=") + to_wstring(id) + U(")");
	break;
	case QueryAttri::AuId: expr = U("Composite(AA.AuId=") + to_wstring(id) + U(")");
	break;
	case QueryAttri::AfId: expr = U("Composite(AA.AfId=") + to_wstring(id) + U(")");
	break;
	case QueryAttri::RId: expr = U("RId=") + to_wstring(id);
	break;
	default:
	break;
	}
	queryCustom(expr, ents, count, offset);
}

void queryCustom(const wstring &expr, Entity_List &ents, size_t count, size_t offset)
{
	auto query = baseHttpClient(expr, count, offset).then(
		extractResponse).then(extractJson).then(
			[&](json::value val) {return JsonToEntities(val, ents); });
	try {
		query.get();
	}
	catch (exception &e) {
		printf("Exception: %s\r\n", e.what());
	}
}

void queryCustomAll(const wstring &expr, Entity_List &ents)
{
	//	TODO
	queryCustom(expr, ents, 10000);
}
