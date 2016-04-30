#pragma once

#include "Entity.h"

enum class QueryAttri
{
	Id, FId, JId, CId, AuId, AfId, RId, // QueryType
	AA
};

void queryEntity(QueryAttri qa, initializer_list<Id_type> &ids, 
	std::vector<entity> &ents, 
	size_t count = 100, size_t offset = 0);
//void queryCustom(const string expr, std::vector<entity> ents);
