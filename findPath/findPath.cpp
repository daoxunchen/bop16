#include "../Entity.h"
#include "../Query.h"

#include <string>
#include <ctime>
#include <algorithm>
#include <thread>

using namespace std;

#define U(x) L ## x

Entity_List startEntities, endEntities;
vector<vector<Id_type>> res;	// final result

QueryAttri label[2];

vector<vector<Id_type>> findPath(Id_type id1, Id_type id2)
{
	auto start_time = clock();

	// clear All for a new process
	res.clear();
	startEntities.clear();
	endEntities.clear();

	//	query id is Id or AA.Auid
	queryCustom(L"OR(Id=" + to_wstring(id1) + L",Composite(AA.AuId=" + to_wstring(id1) + L"))", startEntities);
	queryCustom(L"OR(Id=" + to_wstring(id2) + L",Composite(AA.AuId=" + to_wstring(id2) + L"))", endEntities);

	if (startEntities.empty() || endEntities.empty()) return res;	//	sth wrong happened (network or bad id)

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
				res.push_back(vector<Id_type>({ id1,id2 }));
			}
			//TODO: find 2 and 3
		} else {	//	Id-Auid
			//	1-hop
			if (startEntities[0].AAs.cend() != 
				find_if(startEntities[0].AAs.cbegin(), startEntities[0].AAs.cend(), 
					[&](AA varA) {return varA.AuId == id2; })) {
				res.push_back(vector<Id_type>({ id1,id2 }));
			}
			//TODO: find 2 and 3
		}
	} else {
		if (label[1] == QueryAttri::Id) {	//	Auid-Id
			//	1-hop
			if (startEntities.cend() != 
				find(startEntities.cbegin(), startEntities.cend(), endEntities[0])) {
				res.push_back(vector<Id_type>({ id1,id2 }));
			}
			//TODO: find 2 and 3
		} else {	//	Auid-Auid
			//	no 1-hop
			//TODO: find 2 and 3
		}
	}

	auto end_time = clock();

	return res;
}
