/*********************************************
Id1  --> Id2		Id1.RId == Id2
Id1 <--> FId2		Id1.FId == FId2
Id1 <--> CId2		Id1.CId == CId2
Id1 <--> JId2		Id1.JId == JId2
Id1 <--> AuId2		Id1.AuId == AuId2
AuId1 <--> AfId2	AuId1.AfId == AfId2
**********************************************/

#include "../Entity.h"
#include "../Query.h"

#include <string>
#include <algorithm>
#include <iterator>
#include <set>
#include <thread>
#include <mutex>
#include <ctime>

#define AGG_DEBUG_

#ifdef AGG_DEBUG_
	#include <iostream>
#endif // AGG_DEBUG_

using namespace std;

//	function declaration
paths_t _2hop_Id_Id_Id(const entity &ent1, const entity &ent2, Entity_List &queryRes = Entity_List());
paths_t _2hop_Id_Others_Id(const entity &ent1, const entity &ent2);
void _2hop_Id_AuId();	// note: only can be used in Id-AuId-2hop
void _2hop_AuId_Id();
paths_t _2hop_AuId_AfId_AuId(Id_type id1, Id_type id2, Entity_List &ls1 = Entity_List(), Entity_List &ls2 = Entity_List());
paths_t _2hop_AuId_Id_AuId(Id_type id1, Id_type id2);

void _3hop_Id_Others_Id_AuId();
void _3hop_Id_Id_Id_AuId();
void _3hop_AuId_Id_Others_Id();
void _3hop_AuId_Id_Id_Id();
void _3hop_AuId_AuId();
//	end declaration

Id_type startId, endId;
QueryAttri label[2];
Entity_List startEntities, endEntities;
Entity_List RIdEntities;	// whose RId == id2
paths_t AGG_Path;	// final result
mutex AGG_mtx;		// mutex for AGG_Path

paths_t findPath(Id_type id1, Id_type id2)
{
#ifdef AGG_DEBUG_
	auto start_time = clock();
#endif // AGG_DEBUG_
	
	AGG_Path.clear();	// clear All for a new process

	thread startThread([&]() {
		startEntities.clear();
		startId = id1;
		queryCustom(OR_ID_AUID(id1), startEntities);
	});
	thread endThread([&]() {
		endEntities.clear();
		endId = id2;
		queryCustom(OR_ID_AUID(id2), endEntities);
	});
	
	startThread.join();
	endThread.join();

	if (startEntities.empty() || endEntities.empty()) return AGG_Path;	//	sth wrong happened (network or bad id)

	if (startEntities.size() == 1) label[0] = QueryAttri::Id;	// id1 is Id
	else {
		label[0] = QueryAttri::AuId;	//	id1 is Auid
		startEntities.erase(startEntities.begin());
	}
	if (endEntities.size() == 1) label[1] = QueryAttri::Id;	// id2
	else {
		label[1] = QueryAttri::AuId;
		endEntities.erase(endEntities.begin());
	}

#ifdef AGG_DEBUG_
	cout << "query size: " << startEntities.size() << ":" << endEntities.size() << endl;
	cout << "query time: " << clock() - start_time << endl << "query type: ";
	auto find_time = clock();
#endif // AGG_DEBUG_

	//	find path process
	if (label[0] == QueryAttri::Id) {
		if (label[1] == QueryAttri::Id) {	//	Id-Id
#ifdef AGG_DEBUG_
			cout << "Id-Id" << endl;
#endif // AGG_DEBUG_
			thread step1_1([&]() {
				//	1-hop Id-Id
				if (startEntities[0].R_Id.cend() !=
					find(startEntities[0].R_Id.cbegin(), startEntities[0].R_Id.cend(), id2)) {
					AGG_Path.push_back(path_t({ id1,id2 }));
				}
#ifdef AGG_DEBUG_
				cout << "1-hop :" << AGG_Path.size() << endl;
#endif // AGG_DEBUG_
				auto res = _2hop_Id_Others_Id(startEntities[0], endEntities[0]);
				copy(res.cbegin(), res.cend(), back_inserter(AGG_Path));
#ifdef AGG_DEBUG_
				cout << "2-hop Id-Others-Id over: " << res.size() << endl;
#endif // AGG_DEBUG_
			});
			// 2-hop Id-Id-Id
			Entity_List startRIdEnts;
			thread step1_2([&]() {
				auto res = _2hop_Id_Id_Id(startEntities[0], endEntities[0], startRIdEnts);
				if (res.empty()) return;
				AGG_mtx.lock();
				copy(res.cbegin(), res.cend(), back_inserter(AGG_Path));
				AGG_mtx.unlock();
#ifdef AGG_DEBUG_
				cout << "2-hop over" << endl;
#endif // AGG_DEBUG_
			});
			step1_1.join();
			step1_2.join();

			thread step2_1([&]() {	//	3-hop Id-Id-Others-Id
				for each (auto var in startRIdEnts) {
					auto res = _2hop_Id_Others_Id(var, endEntities[0]);
					paths_t res2;
					for each (auto var1 in res) {
						res2.emplace_back(path_t({ startId,var1[0],var1[1],var1[2] }));
					}
					if (res2.empty()) return;
					AGG_mtx.lock();
					copy(res2.cbegin(), res2.cend(), back_inserter(AGG_Path));
					AGG_mtx.unlock();
				}
#ifdef AGG_DEBUG_
				cout << "3-hop Id-Id-Others-Id over" << endl;
#endif // AGG_DEBUG_
			});
			/*
			thread step2_2([&]() {	// 3-hop Id-Id-Id-Id
				vector<thread> ths(startRIdEnts.size());
				for (size_t i = 0; i < ths.size(); ++i) {
					ths[i] = thread([&]() {
						paths_t res2;
						auto res = _2hop_Id_Id_Id(startRIdEnts[i], endEntities[0]);
						for each (auto var in res) {
							res2.emplace_back(path_t({ startId,var[0],var[1] ,var[2] }));
						}
						AGG_mtx.lock();
						copy(res2.cbegin(), res2.cend(), back_inserter(AGG_Path));
						AGG_mtx.unlock();
					});
				}
				for_each(ths.begin(), ths.end(), [](thread &th) {th.join(); });
			});
			//thread step2_3([&]() {	// 3-hop Id-Others-Id-Id
			//	int num = startEntities[0].F_Id.size() + startEntities[0].AAs.size();
			//	if (startEntities[0].C_Id != 0) ++num;
			//	if (startEntities[0].J_Id != 0) ++num;
			//	vector<thread> queryThreads(num);
			//});
			//thread step2_4([&]() {	// 3-hop Id-Others-Id-Id
			//	int num = startEntities[0].F_Id.size() + startEntities[0].AAs.size();
			//	if (startEntities[0].C_Id != 0) ++num;
			//	if (startEntities[0].J_Id != 0) ++num;
			//	vector<thread> queryThreads(num);
			//});*/
			step2_1.join();
			//step2_2.join();
			//step2_3.join();
			//step2_4.join();

#ifdef AGG_DEBUG_
			cout << "3-hop over" << endl;
#endif // AGG_DEBUG_
		} else {	//	Id-Auid
#ifdef AGG_DEBUG_
			cout << "Id-AuId" << endl;
#endif // AGG_DEBUG_
			//	1-hop
			thread step1_1([&]() {	//	Id-AuId-1-hop
				if (find_if(startEntities[0].AAs.cbegin(), startEntities[0].AAs.cend(),
					[&](AA v) {return v.AuId == endId; }) != startEntities[0].AAs.cend()) {
					AGG_mtx.lock();
					AGG_Path.emplace_back(path_t({ startId,endId }));
					AGG_mtx.unlock();
				}				
			});
			//	2-hop
			thread step1_2(_2hop_Id_AuId);
			//	3-hop
			thread step1_3(_3hop_Id_Others_Id_AuId);	//	Id-Others-Id-AuId
			thread step1_4(_3hop_Id_Id_Id_AuId);	//	Id-Others-Id-AuId
			step1_1.join();
			step1_2.join();
			step1_3.join();
			step1_4.join();
		}
	} else {
		if (label[1] == QueryAttri::Id) {	//	Auid-Id
#ifdef AGG_DEBUG_
			cout << "AuId-Id" << endl;
#endif // AGG_DEBUG_
			//	1-hop
			thread step1_1([&]() {	//	AuId-Id
				if (startEntities.cend() !=
					find_if(startEntities.cbegin(), startEntities.cend(),
						[&](entity var) {return var.Id == id2; })) {
					AGG_mtx.lock();
					AGG_Path.emplace_back(path_t({ startId,endId }));
					AGG_mtx.unlock();
				}
			});
			//	2-hop
			thread step1_2(_2hop_AuId_Id);	//	AuId-Id-Id
			//	3-hop
			thread step1_3([&]() {	//	AuId-AfId-AuId-Id
				vector<thread> ths(endEntities[0].AAs.size());
				for (size_t i = 0; i < endEntities[0].AAs.size(); ++i) {
					ths[i] = thread([&, i]() {
						auto res1 = _2hop_AuId_AfId_AuId(startId, endEntities[0].AAs[i].AuId, startEntities);
						if (res1.empty()) return;
						paths_t res;
						for each (auto var1 in res1) {
							res.emplace_back(path_t({ startId,var1[1],var1[2],endId }));
						}
						AGG_mtx.lock();
						copy(res.cbegin(), res.cend(), back_inserter(AGG_Path));
						AGG_mtx.unlock();
					});
				}
				for_each(ths.begin(), ths.end(), [](thread &th) {th.join(); });
			});
			thread step1_4(_3hop_AuId_Id_Others_Id);
			thread step1_5(_3hop_AuId_Id_Id_Id);
			step1_1.join();
			step1_2.join();
			step1_3.join();
			step1_4.join();
			step1_5.join();

		} else {	//	Auid-Auid
#ifdef AGG_DEBUG_
			cout << "AuId-AuId" << endl;
#endif // AGG_DEBUG_
			//	no 1-hop
			//	2-hop
			thread step1_1([&]() {	//	AuId-Id-AuId
				auto res = _2hop_AuId_Id_AuId(startId, endId);
				if (res.empty()) return;
				AGG_mtx.lock();
				copy(res.cbegin(), res.cend(), back_inserter(AGG_Path));
				AGG_mtx.unlock();
			});
			thread step1_2([&]() {	//	AuId-AfId-AuId
				auto res = _2hop_AuId_AfId_AuId(startId, endId, startEntities, endEntities);
				if (res.empty()) return;
				AGG_mtx.lock();
				copy(res.cbegin(), res.cend(), back_inserter(AGG_Path));
				AGG_mtx.unlock();
			});
			//	3-hop
			thread step1_3(_3hop_AuId_AuId);
			
			step1_1.join();
			step1_2.join();
			step1_3.join();
		}
	}

#ifdef AGG_DEBUG_
	cout << "running time: " << clock() - find_time << endl;
#endif // AGG_DEBUG_
	
	return AGG_Path;
}

paths_t _2hop_Id_Id_Id(const entity &ent1, const entity &ent2, Entity_List &queryRes)
{
	paths_t res;
	if (ent1.R_Id.empty()) return res;
	mutex RIdEntsMtx;
	vector<thread> queryThreads(ent1.R_Id.size());
	for (size_t i = 0; i < queryThreads.size(); ++i) {
		queryThreads[i] = thread([&, i]() {
			queryCustomLock(AND_ID_RID(ent1.R_Id[i], ent2.Id), queryRes, RIdEntsMtx);
		});
	}
	for_each(queryThreads.begin(), queryThreads.end(), 
		[](thread &th) {if(th.joinable()) th.join(); });
	
	for each (auto var in queryRes) {
		if (find(var.R_Id.cbegin(), var.R_Id.cend(), ent2.Id) != var.R_Id.cend()) {
			res.emplace_back(path_t({ ent1.Id,var.Id,ent2.Id }));
		}
	}
	return res;
}

paths_t _2hop_Id_Others_Id(const entity & ent1, const entity & ent2)
{
	paths_t res;

	//	Id-FId-Id
	for each (auto var in ent1.F_Id) {
		if ((var != 0) && (find(ent2.F_Id.cbegin(), ent2.F_Id.cend(), var) != ent2.F_Id.cend())) {
			res.emplace_back(path_t({ ent1.Id,var,ent2.Id }));
		}
	}

	//	Id-CId-Id
	if ((ent1.C_Id != 0) && (ent1.C_Id == ent2.C_Id)) {
		res.emplace_back(path_t({ ent1.Id,ent1.C_Id,ent2.Id }));
	}

	//	Id-JId-Id
	if ((ent1.J_Id != 0) && (ent1.J_Id == ent2.J_Id)) {
		res.emplace_back(path_t({ ent1.Id,ent1.J_Id,ent2.Id }));
	}

	//	Id-AuId-Id
	for each (auto var in ent1.AAs) {
		if (ent2.AAs.cend() !=
			find_if(ent2.AAs.cbegin(), ent2.AAs.cend(),
				[&](AA var1) {return var1.AuId == var.AuId; })) {
			res.emplace_back(path_t({ ent1.Id,var.AuId,ent2.Id }));
		}
	}
	return res;
}

//	this function only can be used in Id-AuId-2hop
void _2hop_Id_AuId()
{
	if (startEntities[0].R_Id.empty()) return;
	paths_t res;
	for each (auto var in endEntities) {
		if (find(startEntities[0].R_Id.cbegin(), startEntities[0].R_Id.cend(), var.Id) 
			!= startEntities[0].R_Id.cend()) {
			res.emplace_back(path_t({ startId,var.Id,endId }));
		}
	}
	if (res.empty()) return;
	AGG_mtx.lock();
	copy(res.cbegin(), res.cend(), back_inserter(AGG_Path));
	AGG_mtx.unlock();
}

void _2hop_AuId_Id()
{
	paths_t res;
	for each (auto var in startEntities) {
		if (find(var.R_Id.cbegin(), var.R_Id.cend(), endId) != var.R_Id.cend()) {
			res.emplace_back(path_t({ startId,var.Id,endId }));
		}
	}
	if (res.empty()) return;
	AGG_mtx.lock();
	copy(res.cbegin(), res.cend(), back_inserter(AGG_Path));
	AGG_mtx.unlock();
}

paths_t _2hop_AuId_AfId_AuId(Id_type id1, Id_type id2, Entity_List & ls1, Entity_List & ls2)
{
	paths_t res;
	//if (ls1.empty()) queryCustom(AUID(id1), ls1);
	if (ls2.empty()) queryCustom(AUID(id2), ls2);

	//	AuId-AfId-AuId
	set<Id_type> AfIds;
	for each (auto var in ls1) {
		auto it = find_if(var.AAs.cbegin(), var.AAs.cend(), [&](AA a) {return a.AuId == id1; });
		if (it != var.AAs.cend()) {
			AfIds.insert(it->AfId);
		}
	}
	AfIds.erase(0);
	set<Id_type> AfIdCommon;
	for each (auto var in ls2) {
		auto it = find_if(var.AAs.cbegin(), var.AAs.cend(), [&](AA a) {return a.AuId == id2; });
		if (it != var.AAs.cend() && AfIds.find(it->AfId) != AfIds.end()) {
			AfIdCommon.insert(it->AfId);
		}
	}
	for each (auto var in AfIdCommon) {
		res.emplace_back(path_t({ id1,var,id2 }));
	}
	
	return res;
}

paths_t _2hop_AuId_Id_AuId(Id_type id1, Id_type id2)
{
	Entity_List a;
	paths_t res;
	queryCustom(AND_AUID_AUID(id1, id2), a);
	for each (auto var in a) {
		res.push_back(path_t({ id1,var.Id,id2 }));
	}
	return res;
}

void _3hop_Id_Others_Id_AuId()
{
	paths_t res;
	for each (auto var in endEntities) {
		auto res1 = _2hop_Id_Others_Id(startEntities[0], var);
		for each (auto var1 in res1) {
			res.emplace_back(path_t({ startId,var1[1],var1[2],endId }));
		}
	}
	if (res.empty()) return;
	AGG_mtx.lock();
	copy(res.cbegin(), res.cend(), back_inserter(AGG_Path));
	AGG_mtx.unlock();
}

void _3hop_Id_Id_Id_AuId()
{
	paths_t res;
	for each (auto var in endEntities) {
		auto res1 = _2hop_Id_Id_Id(startEntities[0], var);
		for each (auto var1 in res1) {
			res.emplace_back(path_t({ startId,var1[1],var1[2],endId }));
		}
	}
	if (res.empty()) return;
	AGG_mtx.lock();
	copy(res.cbegin(), res.cend(), back_inserter(AGG_Path));
	AGG_mtx.unlock();
}

void _3hop_AuId_Id_Others_Id()
{
	paths_t res;
	for each (auto var in startEntities) {
		auto res2 = _2hop_Id_Others_Id(var, endEntities[0]);
		for each (auto var2 in res2) {
			res.emplace_back(path_t({ startId,var2[0],var2[1],var2[2] }));
		}
	}
	if (res.empty()) return;
	AGG_mtx.lock();
	copy(res.cbegin(), res.cend(), back_inserter(AGG_Path));
	AGG_mtx.unlock();
}

void _3hop_AuId_Id_Id_Id()
{
	paths_t res;
	mutex resMtx;
	vector<thread> ths(startEntities.size());
	for (size_t i = 0; i < startEntities.size(); ++i) {
		ths[i] = thread([&, i]() {
			paths_t res1 = _2hop_Id_Id_Id(startEntities[i], endEntities[0]);
			if (res1.empty()) return;
			for each (auto var2 in res1) {
				resMtx.lock();
				res.emplace_back(path_t({ startId,var2[0],var2[1],endId }));
				resMtx.unlock();
			}
		});
	}
	for_each(ths.begin(), ths.end(), [](thread &th) {th.join(); });

	if (res.empty()) return;
	AGG_mtx.lock();
	copy(res.cbegin(), res.cend(), back_inserter(AGG_Path));
	AGG_mtx.unlock();
}

void _3hop_AuId_AuId()	//	AuId-Id-Id-AuId
{
	vector<thread> ths(endEntities.size());
	for (size_t i = 0; i < endEntities.size(); ++i) {
		ths[i] = thread([&, i]() {
			for each (auto var1 in startEntities) {
				if (var1.R_Id.empty()) continue;
				if (find(var1.R_Id.cbegin(), var1.R_Id.cend(), endEntities[i].Id) != var1.R_Id.cend()) {
					AGG_mtx.lock();
					AGG_Path.emplace_back(path_t({ startId,var1.Id,endEntities[i].Id,endId }));
					AGG_mtx.unlock();
				}
			}
		});
	}
	for_each(ths.begin(), ths.end(), [](thread &th) {th.join(); });
}