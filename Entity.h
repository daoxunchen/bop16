#pragma once

#include <vector>

using Id_type = __int64;

struct AA
{
	Id_type AuId = 0;
	Id_type AfId = 0;
};

struct entity{
	Id_type Id = 0;
	Id_type J_Id = 0;
	Id_type C_Id = 0;
	std::vector<AA> AAs;
	std::vector<Id_type> R_Id;
	std::vector<Id_type> F_Id;
};

using Entity_List = std::vector<entity>;
