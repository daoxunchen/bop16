#pragma once

#include <string>
#include <mutex>
#include "Entity.h"

#define N(n) to_wstring(n)
#define ID(id) L"Id="+N(id)
#define FID(id) L"Composite(F.FId="+N(id)+L")"
#define JID(id) L"Composite(J.JId="+N(id)+L")"
#define CID(id) L"Composite(C.CId="+N(id)+L")"
#define RID(id) L"RId="+N(id)
#define AUID(id) L"Composite(AA.AuId="+N(id)+L")"
#define AFID(id) L"Composite(AA.AfId="+N(id)+L")"

#define AND_ID_RID(x,y) L"AND(Id="+N(x)+L",RId="+N(y)+L")"
#define AND_AUID_AUID(x,y) L"AND(Composite(AA.AuId="+N(x)+L"),Composite(AA.AuId="+N(y)+L"))"
#define AND_CID_RID(x,y) L"AND(Composite(C.CId="+N(x)+L"),RId="+N(y)+L")"
#define AND_JID_RID(x,y) L"AND(Composite(J.JId="+N(x)+L"),RId="+N(y)+L")"
#define AND_FID_RID(x,y) L"AND(Composite(F.FId="+N(x)+L"),RId="+N(y)+L")"
#define AND_AUID_RID(x,y) L"AND(Composite(AA.AuId="+N(x)+L"),RId="+N(y)+L")"

enum class QueryAttri
{
	Id, FId, JId, CId, AuId, AfId, RId, // QueryType
	AA, Log
};

void queryOne(const std::wstring &expr, entity &ent1, 
	const std::wstring &attr = L"Id,F.FId,AA.AuId,AA.AfId,J.JId,C.CId,RId");

const size_t defaultCount = 10000;
//	count=0 will query all
void queryCustom(const std::wstring &expr, Entity_List &ents,
	const std::wstring &attr = L"Id,F.FId,AA.AuId,AA.AfId,J.JId,C.CId,RId",
	size_t count = defaultCount, size_t offset = 0);

void queryCustomLock(const std::wstring &expr, Entity_List &ents, std::mutex &mtx,
	const std::wstring &attr = L"Id,F.FId,AA.AuId,AA.AfId,J.JId,C.CId,RId",
	size_t count = defaultCount, size_t offset = 0);
