#pragma once

#include <initializer_list>

#include "Entity.h"

using Id_List = std::initializer_list<Id_type>;

enum class QueryAttri
{
	Id, FId, JId, CId, AuId, AfId, RId, // QueryType
	AA
};

// this function didn't clear ents, just append the new entity
// eg : queryEntity(QueryAttri::Id, 123, a);
void queryEntity(QueryAttri qa, Id_type &id,
	Entity_List &ents,
	size_t count = 100, size_t offset = 0);	// these two params is not used for now

// note : uncomplete
// eg : queryEntity(QueryAttri::Id, Id_List{123, 456}, a);
void queryEntity(QueryAttri qa, Id_List &ids,
	Entity_List &ents,
	size_t count = 100, size_t offset = 0);

// 
//void queryCustom(const string expr, std::vector<entity> ents);
