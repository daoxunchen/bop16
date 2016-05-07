/*********************************************
Id1  --> Id2		Id1.RId == Id2
Id1 <--> FId2		Id1.FId == FId2
Id1 <--> CId2		Id1.CId == CId2
Id1 <--> JId2		Id1.JId == JId2
Id1 <--> AuId2		Id1.AuId == AuId2
AuId1 <--> AfId2	AuId1.AfId == AfId2
**********************************************/

#include "../Entity.h"
#include "../Query.h"

#include <string>
#include <ctime>
#include <algorithm>
#include <iterator>
#include <set>
#include <thread>

using namespace std;

#define U(x) L ## x
#define N(n) to_wstring(n)
#define ID(id) U("Id=")+N(id)
#define RID(id) U("RId=")+N(id)
#define AUID(id) U("Composite(AA.AuId=")+N(id)+U(")")
#define AND(x,y) U("AND(")+x+U(",")+y+U(")")

//	typename
using path_t = vector<Id_type>;
using paths_t = vector<path_t>;

//	function declaration
paths_t _2hop_Id_Id(Id_type id1, Id_type id2);
paths_t _2hop_Id_Id(const entity &ent1, Id_type id2);
paths_t _2hop_Id_Id(const entity &ent1, const entity &ent2);
paths_t _2hop_Id_AuId(Id_type id1, Id_type id2);
paths_t _2hop_Id_AuId(const entity &ent1, Id_type id2);
paths_t _2hop_AuId_Id(Id_type id1, Id_type id2);
paths_t _2hop_AuId_AfId_AuId(Id_type id1, Id_type id2, Entity_List &ls1 = Entity_List(), Entity_List &ls2 = Entity_List());
paths_t _2hop_AuId_AuId(Id_type id1, Id_type id2);
//paths_t _2hop_AuId_AuId(const Entity_List &ls1, const Entity_List &ls2);
void _3hop_Id_Id();
void _3hop_Id_AuId();
void _3hop_AuId_Id();
void _3hop_AuId_AuId();
//	end declaration

Id_type startId, endId;
QueryAttri label[2];
Entity_List startEntities, endEntities;
paths_t AGG_Path;	// final result

paths_t findPath(Id_type id1, Id_type id2)
{
	auto start_time = clock();

	// clear All for a new process
	AGG_Path.clear();
	startEntities.clear();
	endEntities.clear();

	//	query id is Id or AA.Auid
	queryCustom(L"OR(Id=" + to_wstring(id1) + L",Composite(AA.AuId=" + to_wstring(id1) + L"))", startEntities);
	queryCustom(L"OR(Id=" + to_wstring(id2) + L",Composite(AA.AuId=" + to_wstring(id2) + L"))", endEntities);

	startId = id1; endId = id2;

	if (startEntities.empty() || endEntities.empty()) return AGG_Path;	//	sth wrong happened (network or bad id)

	if (startEntities[0].Id == id1) label[0] = QueryAttri::Id;	// id1 is Id
	else label[0] = QueryAttri::AuId;	//	id1 is Auid
	if (endEntities[0].Id == id2) label[1] = QueryAttri::Id;	// id2
	else label[1] = QueryAttri::AuId;

	//	find path process
	if (label[0] == QueryAttri::Id) {
		if (label[1] == QueryAttri::Id) {	//	Id-Id
			//	1-hop
			if (startEntities[0].R_Id.cend() != 
				find(startEntities[0].R_Id.cbegin(), startEntities[0].R_Id.cend(), id2)) {
				AGG_Path.push_back(path_t({ id1,id2 }));
			}
			//	2-hop
			auto res = _2hop_Id_Id(startEntities[0], endEntities[0]);
			copy(res.cbegin(), res.cend(), back_inserter(AGG_Path));
			//TODO: find 3
		} else {	//	Id-Auid
			//	1-hop
			if (startEntities[0].AAs.cend() != 
				find_if(startEntities[0].AAs.cbegin(), startEntities[0].AAs.cend(), 
					[&](AA varA) {return varA.AuId == id2; })) {
				AGG_Path.push_back(path_t({ id1,id2 }));
			}
			//	2-hop
			auto res = _2hop_Id_AuId(id1, id2);
			copy(res.cbegin(), res.cend(), back_inserter(AGG_Path));
			//	3-hop
			_3hop_Id_AuId();
		}
	} else {
		if (label[1] == QueryAttri::Id) {	//	Auid-Id
			//	1-hop
			if (startEntities.cend() != 
				find_if(startEntities.cbegin(), startEntities.cend(), 
					[&](entity var) {return var.Id == id2; })) {
				AGG_Path.push_back(path_t({ id1,id2 }));
			}
			//	2-hop
			auto res = _2hop_AuId_Id(id1, id2);
			copy(res.cbegin(), res.cend(), back_inserter(AGG_Path));
			//	3-hop
			_3hop_AuId_Id();
		} else {	//	Auid-Auid
			//	no 1-hop
			//	2-hop
			auto res = _2hop_AuId_AuId(id1, id2);
			copy(res.cbegin(), res.cend(), back_inserter(AGG_Path));
			//	3-hop
			_3hop_AuId_AuId();
		}
	}

	auto end_time = clock();

	return AGG_Path;
}

paths_t _2hop_Id_Id(Id_type id1, Id_type id2)
{
	Entity_List a;
	//queryEntity(QueryAttri::Id, id1, a);
	//queryEntity(QueryAttri::Id, id2, a);
	queryCustom(ID(id1), a);
	queryCustom(ID(id2), a);
	return _2hop_Id_Id(a[0], a[1]);
}

paths_t _2hop_Id_Id(const entity &ent1, Id_type id2)
{
	Entity_List a;
	//queryEntity(QueryAttri::Id, id2, a);
	queryCustom(ID(id2), a);
	return _2hop_Id_Id(ent1, a[0]);
}

paths_t _2hop_Id_Id(const entity &ent1, const entity &ent2)
{
	paths_t res;
	Entity_List a;
	//	Id-Id-Id
	queryEntity(QueryAttri::RId, ent2.Id, a);
	//queryCustom(RID(ent2.Id), a);
	for each (auto var in a)
	{
		if (find(ent1.R_Id.cbegin(), ent1.R_Id.cend(), var.Id) != ent1.R_Id.cend()) {
			res.push_back(path_t({ ent1.Id,var.Id,ent2.Id }));
		}
	}
	//	Id-FId-Id
	for each (auto var in ent1.F_Id) {
		if (find(ent2.F_Id.cbegin(), ent2.F_Id.cend(), var) != ent2.F_Id.cend()) {
			res.push_back(path_t({ ent1.Id,var,ent2.Id }));
		}
	}

	//	Id-CId-Id
	if (ent1.C_Id == ent2.C_Id) {
		res.push_back(path_t({ ent1.Id,ent1.C_Id,ent2.Id }));
	}
	//	Id-JId-Id
	if (ent1.J_Id == ent2.J_Id) {
		res.push_back(path_t({ ent1.Id,ent1.J_Id,ent2.Id }));
	}
	//	Id-AuId-Id
	for each (auto var in ent1.AAs) {
		if (ent2.AAs.cend() != 
			find_if(ent2.AAs.cbegin(), ent2.AAs.cend(), 
				[&](AA var1) {return var1.AuId == var.AuId; })) {
			res.push_back(path_t({ ent1.Id,var.AuId,ent2.Id }));
		}
	}
	return res;
}

paths_t _2hop_Id_AuId(const entity &ent1, Id_type id2)
{
	paths_t res;
	Entity_List ents;
	for each (auto var in ent1.R_Id)
	{
		queryCustom(U("AND(Id=") + N(var) + U(",Composite(AA.AuId=") + N(id2) + U("))"), ents);
		if (ents.size() > 0) res.push_back(path_t({ ent1.Id,var,id2 }));
		ents.clear();
	}
	return res;
}

paths_t _2hop_Id_AuId(Id_type id1, Id_type id2)
{
	Entity_List a;
	//queryEntity(QueryAttri::Id, id1, a);
	queryCustom(ID(id1), a);
	return _2hop_Id_AuId(a[0], id2);
}

paths_t _2hop_AuId_Id(Id_type id1, Id_type id2)
{
	return _2hop_Id_AuId(id2, id1);
}

paths_t _2hop_AuId_AfId_AuId(Id_type id1, Id_type id2, Entity_List & ls1, Entity_List & ls2)
{
	paths_t res;
	if (ls1.empty()) queryCustom(U("Composite(AA.AuId=") + N(id1) + U(")"), ls1);
	if (ls2.empty()) queryCustom(U("Composite(AA.AuId=") + N(id2) + U(")"), ls2);

	//	AuId-AfId-AuId
	set<Id_type> AfIds;
	for each (auto var in ls1) {
		auto it = find_if(var.AAs.cbegin(), var.AAs.cend(), [&](AA a) {return a.AuId == id1; });
		if (it != var.AAs.cend()) {
			AfIds.insert(it->AfId);
		}
	}
	for each (auto var in ls2) {
		auto it = find_if(var.AAs.cbegin(), var.AAs.cend(), [&](AA a) {return a.AuId == id2; });
		if (it != var.AAs.cend() && AfIds.find(it->AfId) != AfIds.end()) {
			res.push_back(path_t({ id1,it->AuId,id2 }));
		}
	}
	return res;
}

paths_t _2hop_AuId_AuId(Id_type id1, Id_type id2)
{
	Entity_List a, b;

	//	AuId-AfId-AuId
	paths_t res = _2hop_AuId_AfId_AuId(id1, id2, a, b);

	//	AuId-Id-AuId
	a.clear();
	queryCustom(U("AND(Composite(AA.AuId=") + N(id1) + U("),Composite(AA.AuId=") + N(id2) + U("))"), a);
	for each (auto var in a) {
		res.push_back(path_t({ id1,var.Id,id2 }));
	}
	return res;
}

void _3hop_Id_Id()
{
}

void _3hop_Id_AuId()
{
	for each (auto var in endEntities) {
		auto res = _2hop_Id_Id(startEntities[0], var);
		for each (auto var1 in res) {
			AGG_Path.push_back(path_t({ var1[0],var1[1],var1[2],endId }));
		}
	}
}

void _3hop_AuId_Id()
{
	//	AuId-AfId-AuId-Id
	for each (auto var in endEntities[0].AAs) {
		auto res1 = _2hop_AuId_AfId_AuId(startId, var.AuId, startEntities);
		for each (auto var1 in res1) {
			AGG_Path.push_back(path_t({ var1[0],var1[1],var1[2],endId }));
		}
	}

	//	Auid-Id-(2-hop)-Id
	for each (auto var in startEntities){
		auto res2 = _2hop_Id_Id(var, endEntities[0]);
		for each (auto var2 in res2) {
			AGG_Path.push_back(path_t({ startId,var2[0],var2[1],var2[2] }));
		}
	}
}

void _3hop_AuId_AuId()
{
	//	AuId-Id-Id-AuId
	for each (auto var in endEntities) {
		for each (auto var1 in startEntities) {
			if (find(var1.R_Id.cbegin(), var1.R_Id.cend(), var.Id) != var1.R_Id.cend()) {
				AGG_Path.push_back(path_t({ startId,var1.Id,var.Id,endId }));
			}
		}
	}
}