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

void findingPath();

Id_type startId, endId;
Entity_List startEntities, endEntities;
paths_t AGG_Path;	// final result
mutex AGG_mtx;		// mutex for AGG_Path

volatile bool finding;	// finding process is over?

clock_t start_time;

paths_t findPath(Id_type id1, Id_type id2)
{
	start_time = clock();
	finding = true;
	startId = id1;
	endId = id2;
	thread findThread(findingPath);
	findThread.detach();
	AGG_mtx.lock();
	AGG_Path.clear();	// clear All for a new process
	AGG_mtx.unlock();
	while (finding && ((clock() - start_time) < 280000));

#ifdef AGG_DEBUG_
	if (finding) {
		cout << "running Time Out" << endl;
	}
	else {
		cout << "Total running time: " << clock() - start_time << "ms" << endl;
		cout << "Total path numbers:" << AGG_Path.size() << endl;
	}
#endif // AGG_DEBUG_

	return AGG_Path;
}

void findingPath()
{	
	QueryAttri label[2];
	thread startThread([&]() {
		startEntities.clear();
		queryCustom(OR_ID_AUID(startId), startEntities);
		if (startEntities.size() == 1) label[0] = QueryAttri::Id;	// id1 is Id
		else if(startEntities.size() > 1){
			label[0] = QueryAttri::AuId;	//	id1 is Auid
			startEntities.erase(startEntities.begin());
		}
	});
	thread endThread([&]() {
		endEntities.clear();
		queryCustom(OR_ID_AUID(endId), endEntities);
		if (endEntities.size() == 1) label[1] = QueryAttri::Id;	// id2
		else if (endEntities.size() > 1) {
			label[1] = QueryAttri::AuId;
			endEntities.erase(endEntities.begin());
		}
	});
	
	startThread.join();
	endThread.join();

	if (startEntities.empty() || endEntities.empty()) {	//	sth wrong happened (network or bad id)
		finding = false;
		return;
	}

#ifdef AGG_DEBUG_
	cout << "query size: " << startEntities.size() << ":" << endEntities.size() << endl;
	cout << "query time: " << clock() - start_time << "ms" << endl << "query type: ";
	auto find_time = clock();
#endif // AGG_DEBUG_

	//	find path process
	if (label[0] == QueryAttri::Id) {
		if (label[1] == QueryAttri::Id) {	//	Id-Id
#ifdef AGG_DEBUG_
			cout << "Id-Id" << endl;
#endif // AGG_DEBUG_
			thread step1_1([&]() {
#ifdef AGG_DEBUG_
				auto thstart = clock();
#endif
				//	1-hop Id-Id
				if (startEntities[0].R_Id.cend() !=
					find(startEntities[0].R_Id.cbegin(), startEntities[0].R_Id.cend(), endId)) {
					AGG_mtx.lock();
					AGG_Path.push_back(path_t({ startId,endId }));
					AGG_mtx.unlock();
				}
				//	2-hop Id-Others-Id
				auto res = _2hop_Id_Others_Id(startEntities[0], endEntities[0]);
				AGG_mtx.lock();
				copy(res.cbegin(), res.cend(), back_inserter(AGG_Path));
				AGG_mtx.unlock();
#ifdef AGG_DEBUG_
				auto thend = clock();
				cout << "Id-Id th1:" << thend - thstart << "ms" << endl;
#endif
			});
			//	3-hop Id-Others-Id-Id
			thread step1_2([&]() {
#ifdef AGG_DEBUG_
				auto thstart = clock();
#endif
				thread th1([&]() {	//	Id-CId-Id-Id
					if (startEntities[0].C_Id == 0) return;
					Entity_List tmp;
					queryCustom(AND_CID_RID(startEntities[0].C_Id, endId), tmp, L"Id");
					if (tmp.empty()) return;
					paths_t res;
					for each (auto var in tmp) {
						res.emplace_back(path_t({ startId,startEntities[0].C_Id,var.Id,endId }));
					}
					AGG_mtx.lock();
					copy(res.begin(), res.end(), back_inserter(AGG_Path));
					AGG_mtx.unlock();
				});
				thread th2([&]() {	//	Id-JId-Id-Id
					if (startEntities[0].J_Id == 0) return;
					Entity_List tmp;
					queryCustom(AND_JID_RID(startEntities[0].J_Id, endId), tmp, L"Id");
					if (tmp.empty()) return;
					paths_t res;
					for each (auto var in tmp) {
						res.emplace_back(path_t({ startId,startEntities[0].J_Id,var.Id,endId }));
					}
					AGG_mtx.lock();
					copy(res.begin(), res.end(), back_inserter(AGG_Path));
					AGG_mtx.unlock();
				});
				thread th3([&]() {	//	Id-FId-Id-Id
					if (startEntities[0].F_Id.empty()) return;
					vector<thread> ths(startEntities[0].F_Id.size());
					for (size_t i = 0; i < startEntities[0].F_Id.size(); ++i) {
						ths[i] = thread([&, i]() {
							Entity_List tmp;
							queryCustom(AND_FID_RID(startEntities[0].F_Id[i], endId), tmp, L"Id");
							if (tmp.empty()) return;
							paths_t res;
							for each (auto var in tmp) {
								res.emplace_back(path_t({ startId,startEntities[0].F_Id[i],var.Id,endId }));
							}
							AGG_mtx.lock();
							copy(res.begin(), res.end(), back_inserter(AGG_Path));
							AGG_mtx.unlock();
						});
					}
					for_each(ths.begin(), ths.end(), [](thread &th) {th.join(); });
				});
				thread th4([&]() {	//	Id-AuId-Id-Id
					vector<thread> ths(startEntities[0].AAs.size());
					for (size_t i = 0; i < startEntities[0].AAs.size(); ++i) {
						ths[i] = thread([&, i]() {
							Entity_List tmp;
							queryCustom(AND_AUID_RID(startEntities[0].AAs[i].AuId, endId), tmp, L"Id");
							if (tmp.empty()) return;
							paths_t res;
							for each (auto var in tmp) {
								res.emplace_back(path_t({ startId,startEntities[0].AAs[i].AuId,var.Id,endId }));
							}
							AGG_mtx.lock();
							copy(res.begin(), res.end(), back_inserter(AGG_Path));
							AGG_mtx.unlock();
						});
					}
					for_each(ths.begin(), ths.end(), [](thread &th) {th.join(); });
				});
				th1.join(); th2.join(); th3.join(); th4.join();
#ifdef AGG_DEBUG_
				auto thend = clock();
				cout << "Id-Id th2:" << thend - thstart << "ms" << endl;
#endif
			});
			
			thread step1_3([&]() {	// 2-hop Id-Id-*-Id
#ifdef AGG_DEBUG_
				auto thstart = clock();
#endif
				if (startEntities[0].R_Id.empty()) return;
				vector<thread> ths(startEntities[0].R_Id.size());
				for (size_t i = 0; i < startEntities[0].R_Id.size(); ++i) {
					ths[i] = thread([&, i]() {
						entity tmp;
						queryOne(ID(startEntities[0].R_Id[i]), tmp);
						thread th1([&]() {	//	Id-Id-Id
#ifdef AGG_DEBUG_
							auto th1start = clock();
#endif
							if (tmp.R_Id.cend() !=
								find(tmp.R_Id.cbegin(), tmp.R_Id.cend(), endId)) {
								AGG_mtx.lock();
								AGG_Path.push_back(path_t({ startId,tmp.Id,endId }));
								AGG_mtx.unlock();
							}
							//	Id-Id-Others-Id
							auto res1 = _2hop_Id_Others_Id(tmp, endEntities[0]);
							if (res1.empty()) return;
							paths_t res;
							for each (auto var in res1) {
								res.emplace_back(path_t({ startId,var[0],var[1],endId }));
							}
							AGG_mtx.lock();
							copy(res.cbegin(), res.cend(), back_inserter(AGG_Path));
							AGG_mtx.unlock();
#ifdef AGG_DEBUG_
							auto th1end = clock();
							cout << "Id-Id th2--th1:" << th1end - th1start << "ms" << endl;
#endif
						});
						thread th2([&]() {	//	Id-Id-Id-Id
#ifdef AGG_DEBUG_
							auto th1start = clock();
#endif
							auto res1 = _2hop_Id_Id_Id(tmp, endEntities[0]);
							if (res1.empty()) return;
							paths_t res;
							for each (auto var in res1) {
								res.emplace_back(path_t({ startId,var[0],var[1],endId }));
							}
							AGG_mtx.lock();
							copy(res.cbegin(), res.cend(), back_inserter(AGG_Path));
							AGG_mtx.unlock();
#ifdef AGG_DEBUG_
							auto th1end = clock();
							cout << "Id-Id th2--th2:" << th1end - th1start << "ms" << endl;
#endif
						});
						th1.join();
						th2.join();
					});
				}
				for_each(ths.begin(), ths.end(), [](thread &th) {th.join(); });
#ifdef AGG_DEBUG_
				auto thend = clock();
				cout << "Id-Id th3:" << thend - thstart << "ms" << endl;
#endif
			});
			step1_1.join();
			step1_2.join();
			step1_3.join();
		} else {	//	Id-Auid
#ifdef AGG_DEBUG_
			cout << "Id-AuId" << endl;
#endif // AGG_DEBUG_
			//	1-hop
			thread step1_1([&]() {	//	Id-AuId-1-hop
#ifdef AGG_DEBUG_
				auto thstart = clock();
#endif
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
						[&](entity var) {return var.Id == endId; })) {
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
	cout << "finding time: " << clock() - find_time << "ms" << endl;
#endif // AGG_DEBUG_
	finding = false;
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
	mutex mtx;
	thread F([&]() {	//	Id-FId-Id
		for each (auto var in ent1.F_Id) {
			if ((var != 0) && (find(ent2.F_Id.cbegin(), ent2.F_Id.cend(), var) != ent2.F_Id.cend())) {
				mtx.lock();
				res.emplace_back(path_t({ ent1.Id,var,ent2.Id }));
				mtx.unlock();
			}
		}
	});

	thread AAth([&]() {
		for (size_t i = 0; i < ent1.AAs.size(); ++i) {
			if (find_if(ent2.AAs.begin(), ent2.AAs.end(), [&, i](AA va) {return va.AuId == ent1.AAs[i].AuId; })
				!= ent2.AAs.end()) {
				mtx.lock();
				res.emplace_back(path_t({ ent1.Id,ent1.AAs[i].AuId,ent2.Id }));
				mtx.unlock();
			}
		}
	});

	////	Id-AuId-Id
	//for each (auto var in ent1.AAs) {
	//	if (find_if(ent2.AAs.begin(), ent2.AAs.end(), 
	//		[&](AA var1) {return var1.AuId == var.AuId; })
	//		!= ent2.AAs.end()) {
	//		mtx.lock();
	//		res.emplace_back(path_t({ ent1.Id,var.AuId,ent2.Id }));
	//		mtx.unlock();
	//	}
	//}


	//	Id-CId-Id
	if ((ent1.C_Id != 0) && (ent1.C_Id == ent2.C_Id)) {
		mtx.lock();
		res.emplace_back(path_t({ ent1.Id,ent1.C_Id,ent2.Id }));
		mtx.unlock();
	}

	//	Id-JId-Id
	if ((ent1.J_Id != 0) && (ent1.J_Id == ent2.J_Id)) {
		mtx.lock();
		res.emplace_back(path_t({ ent1.Id,ent1.J_Id,ent2.Id }));
		mtx.unlock();
	}

	F.join(); AAth.join();
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
	if (ls2.empty()) queryCustom(AUID(id2), ls2, L"AA.AuId,AA.AfId");

	//	AuId-AfId-AuId
	set<Id_type> AfIds;
	thread th1([&]() {
		for each (auto var in ls1) {
			auto it = find_if(var.AAs.cbegin(), var.AAs.cend(), [&](AA a) {return a.AuId == id1; });
			if (it != var.AAs.cend()) {
				AfIds.insert(it->AfId);
			}
		}
		AfIds.erase(0);
	});
	set<Id_type> AfIds2;
	thread th2([&]() {
		for each (auto var in ls2) {
			auto it = find_if(var.AAs.cbegin(), var.AAs.cend(), [&](AA a) {return a.AuId == id2; });
			if (it != var.AAs.cend()) {
				AfIds2.insert(it->AfId);
			}
		}
	});
	th1.join(); th2.join();

	for each (auto var in AfIds2) {
		if (AfIds.find(var) != AfIds.end()) {
			res.emplace_back(path_t({ id1,var,id2 }));
		}
	}
	
	return res;
}

paths_t _2hop_AuId_Id_AuId(Id_type id1, Id_type id2)	// second way???
{
	Entity_List a;
	paths_t res;
	queryCustom(AND_AUID_AUID(id1, id2), a, L"Id");
	for each (auto var in a) {
		res.emplace_back(path_t({ id1,var.Id,id2 }));
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