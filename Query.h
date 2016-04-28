#pragma once

#include "Entity.h"

// single query
// thread edition(recommended)
void queryId(const Id_type &Id, entity &ent);
void queryFId(const Id_type &FId, std::vector<entity> ents);
void queryJId(const Id_type &JId, std::vector<entity> ents);
void queryCId(const Id_type &CId, std::vector<entity> ents);
void queryAuId(const Id_type &AuId, std::vector<entity> ents);
void queryAfId(const Id_type &AfId, std::vector<entity> ents);
void queryRId(const Id_type &RId, std::vector<entity> ents);

// normal edition
entity queryId(const Id_type &Id);
std::vector<entity> queryFId(const Id_type &FId);
std::vector<entity> queryJId(const Id_type &JId);
std::vector<entity> queryCId(const Id_type &CId);
std::vector<entity> queryAuId(const Id_type &AuId);
std::vector<entity> queryAfId(const Id_type &AfId);
std::vector<entity> queryRId(const Id_type &RId);
