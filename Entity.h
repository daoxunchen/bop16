#pragma once

#include <vector>
#include <map>

using Id_type = long long;
//using AA = std::map<Id_type, Id_type>;

struct entity
{
	Id_type Id = 0;
	Id_type J_Id = 0;
	Id_type C_Id = 0;
	std::multimap<Id_type, Id_type> AAs;
	std::vector<Id_type> R_Id;
	std::vector<Id_type> F_Id;
};

using Entity_List = std::vector<entity>;
using path_t = std::vector<Id_type>;
using paths_t = std::vector<path_t>;
