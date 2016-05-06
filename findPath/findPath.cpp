#include "../Entity.h"
#include "../Query.h"

#include <string>
#include <thread>

using namespace std;

Entity_List startEntities, endEntities;
vector<vector<Id_type>> res;	// final result

QueryAttri label[2];

vector<vector<Id_type>> findPath(Id_type id1, Id_type id2)
{
	// clear All for a new process
	res.clear();
	startEntities.clear();
	endEntities.clear();

	//	query id is Id or AA.Auid
	queryCustom(L"OR(Id=" + to_wstring(id1) + L",Composite(AA.AuId=" + to_wstring(id1) + L"))", startEntities);
	queryCustom(L"OR(Id=" + to_wstring(id2) + L",Composite(AA.AuId=" + to_wstring(id2) + L"))", endEntities);

	if (startEntities[0].Id == id1) label[0] = QueryAttri::Id;	// id1 is Id
	else label[0] = QueryAttri::AuId;	//	id1 is Auid
	if (endEntities[0].Id == id2) label[1] = QueryAttri::Id;	// id2
	else label[1] = QueryAttri::AuId;

	//	find path process
	if (label[0] == QueryAttri::Id) {
		if (label[1] == QueryAttri::Id) {	//	Id-Id

		} else {	//	Id-Auid

		}
	} else {
		if (label[1] == QueryAttri::Id) {	//	Auid-Id

		} else {	//	Auid-Auid

		}
	}

	//res.push_back(vector<Id_type>({ id1,id2 }));
	return res;
}
