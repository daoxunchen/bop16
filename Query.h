#pragma once

#include <string>

#include "Entity.h"

enum class QueryAttri
{
	Id, FId, JId, CId, AuId, AfId, RId, // QueryType
	AA
};

// this function didn't clear ents, just append the new entity
// eg : queryEntity(QueryAttri::Id, 123, a);
void queryEntity(QueryAttri qa, Id_type &id,
	Entity_List &ents,
	size_t count = 100, size_t offset = 0);

// 
void queryCustom(const std::wstring &expr, Entity_List &ents,
	size_t count = 100, size_t offset = 0);
