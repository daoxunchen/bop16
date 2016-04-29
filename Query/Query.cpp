#include "../Query.h"

#include <iostream>
#include <string>
#include <cpprest/http_client.h>
#include <cpprest/uri_builder.h>

#define QUERY_DEBUG_

using namespace std;

using namespace utility;
using namespace web;
using namespace web::http;
using namespace web::http::client;
using namespace web::json;

enum class QueryType
{
	Id, FId, JId, CId, AuId, AfId, RId
};

void responseToJson(http_response res)
{
	auto data = res.extract_json();

	try {
		data.wait();
	}
	catch (const std::exception &e) {
		printf("Error exception:%s\n", e.what());
	}
}

void ttt(json::value &val)
{
	if (!val.is_null()) {
		auto b = val.as_object();
		for (auto iter = b.cbegin(); iter != b.cend(); ++iter) {
			if (iter->first != U("entities")) continue;
			json::array entities = iter->second.as_array();
			cout << "size is " << entities.size();
			for (auto iter2 = entities.cbegin(); iter2 != entities.cend(); ++iter2) {

			}
		}
	}
}
/*
void queryId(const Id_type &Id, entity &ent)
{
	builder.append_query(U("expr"), U("Composite(AA.AuN=='jaime teevan')"));
	pplx::task<void> resp = queryClient.request(methods::GET, builder.to_string()).then([=](http_response res)
	{ return res.extract_json(); }).then(ttt);
		//.then([=](json::value val) {std::wcout<<"bcd" << val.serialize(); });
	try {
		resp.wait();
	}
	catch (const std::exception &e) {
		printf("aError exception:%s\n", e.what());
	}
}*/

pplx::task<http_response> baseHttpClient(QueryType qt, Id_type id)
{
#ifndef QUERY_DEBUG_
	http_client queryClient(U("https://oxfordhk.azure-api.net"));
#else
	http_client_config quertConfig;
	quertConfig.set_proxy(web_proxy(U("http://127.0.0.1:1080")));
	http_client queryClient(U("https://oxfordhk.azure-api.net"), quertConfig);
#endif // QUERY_DEBUG_

	uri_builder builder(U("/academic/v1.0/evaluate"));
	builder.append_query(U("subscription-key"), U("f7cc29509a8443c5b3a5e56b0e38b5a6"));
	builder.append_query(U("attributes"), U("Id,F.FId,AA.AuId,AA.AfId,J.JId,C.CId,RId"));
	
	////////////////////////////////////////////////////////
	builder.append_query(U("count"), U("10000"));
	builder.append_query(U("offset"), U("0"));
	///////////////////////////////////////////////////////

	string_t expr;
	switch (qt) {
	case QueryType::Id: expr = U("Id=") + to_wstring(id);
		break;
	case QueryType::FId: expr = U("Composite(F.FId=") + to_wstring(id) + U(")");
		break;
	case QueryType::JId: expr = U("Composite(J.JId=") + to_wstring(id) + U(")");
		break;
	case QueryType::CId: expr = U("Composite(C.CId=") + to_wstring(id) + U(")");
		break;
	case QueryType::AuId: expr = U("Composite(AA.AuId=") + to_wstring(id) + U(")");
		break;
	case QueryType::AfId: expr = U("Composite(AA.AfId=") + to_wstring(id) + U(")");
		break;
	case QueryType::RId: expr = U("RId=") + to_wstring(id);
		break;
	default:
		break;
	}
	builder.append_query(U("expr"), expr);
	return queryClient.request(methods::GET, builder.to_string());
}

json::value Response(http_response res)
{
	json::value obj;
	if (res.status_code() == status_codes::OK) return res.extract_json();
#ifdef QUERY_DEBUG_
	else cout << "http_response status_code: " << res.status_code() << endl;
#endif // QUERY_DEBUG_
	return obj;
}

void QueryEntity(QueryType qt, initializer_list<Id_type> ids, vector<entity> entities, size_t count = 100, size_t offset = 0)
{
}

void queryId(const Id_type &Id, entity &ent)
{
	auto tmp = baseHttpClient(QueryType::Id, Id).then([=](http_response res) {
		cout << res.status_code() << endl;
		wcout << res.to_string();
	});
	tmp.get();
}
