#pragma once

#include <vector>

typedef __int64 Id_type;

struct AA
{
	Id_type Auid;
	Id_type Afid;
};

struct entity{
	Id_type Id;
	Id_type J_Id;
	Id_type C_Id;
	std::vector<AA> AAs;
	std::vector<Id_type> R_Id;
	std::vector<Id_type> F_Id;
};
