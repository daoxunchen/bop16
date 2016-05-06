#include "../Entity.h"
#include "../Query.h"

using namespace std;

vector<vector<Id_type>> findPath(Id_type& id1, Id_type& id2)
{
	vector<vector<Id_type>> res;
	res.push_back(vector<Id_type>({ 1,2,2,3 }));
	res.push_back(vector<Id_type>({ 2,2,2,3 }));
	res.push_back(vector<Id_type>({ 1,2,3 }));
	return res;
}