/*********************************************
Id1  --> Id2		Id1.RId == Id2
Id1 <--> FId2		Id1.FId == FId2
Id1 <--> CId2		Id1.CId == CId2
Id1 <--> JId2		Id1.JId == JId2
Id1 <--> AuId2		Id1.AuId == AuId2
AuId1 <--> AfId2	AuId1.AfId == AfId2
**********************************************/
/*********************************************
[Id-Id]:
1--Id-Id
2--Id-Id-Id, Id-others-Id
3--Id-Id-Id-Id, Id-others-Id-Id, Id-Id-others-Id
[Id-AuId]:
1--Id-AuId
2--Id-Id-AuId
3--Id-Id-Id-AuId, Id-others-Id-AuId, Id-AuId-AfId-AuId
[AuId-Id]:
1--AuId-Id
2--AuId-Id-Id
3--AuId-AfId-AuId-Id, AuId-Id-Id-AuId, AuId-Id-others-Id
[AuId-AuId]:
2--AuId-Id-AuId, AuId-AfId-AuId
3--AuId-Id-Id-AuId
**********************************************/

#include "../Test.h"
#include "../Entity.h"
#include "../Query.h"
//#include "../newQuery.h"

#include <string>
#include <algorithm>
#include <iterator>
#include <set>
#include <thread>
#include <mutex>
#include <ctime>

#ifdef AGG_DEBUG_
	#include <iostream>
#endif // AGG_DEBUG_

using namespace std;

//	function declaration
paths_t _2hop_Id_Id_Id(const entity &ent1, const entity &ent2);
paths_t _2hop_Id_Others_Id(const entity &ent1, const entity &ent2);
void _2hop_Id_Id_AuId();	// note: only can be used in Id-Id-AuId-2hop
void _2hop_AuId_Id();
paths_t _2hop_AuId_AfId_AuId(Id_type id1, Id_type id2, Entity_List &ls1);
paths_t _2hop_AuId_AfId_AuId(Id_type id1, Id_type id2, Entity_List &ls1, Entity_List &ls2);
paths_t _2hop_AuId_Id_AuId(Id_type id1, Id_type id2, Entity_List &ls1 = Entity_List(), Entity_List &ls2 = Entity_List());

void _3hop_Id_Others_Id_AuId();
void _3hop_Id_Id_Id_AuId();
void _3hop_Id_AuId_AfId_AuId();
void _3hop_AuId_Id_Others_Id();
void _3hop_AuId_Id_Id_Id();
void _3hop_AuId_AuId();
//	add path to AGG_path
inline void addAGG_path(const paths_t &path);
inline void addAGG_path(Id_type id, paths_t &path);
inline void addAGG_path(paths_t &path, Id_type id);
//	end declaration

Id_type startId, endId;
Entity_List startEntities, endEntities;
mutex startMtx, endMtx;
paths_t AGG_Path;	// final result
mutex AGG_mtx;		// mutex for AGG_Path

bool finding;	// finding process is over?

int detectID_AUID()
{
#ifdef AGG_DEBUG_
	auto start_time = clock();
#endif // AGG_DEBUG_
	int flag = 0;	//	00--id1-id2	11--auid1-auid2
	entity e[2];
	vector<thread> ths(2);
	ths[0] = thread([&]() {
		queryOne(ID(startId), e[0]);
		if (!(e[0].AAs.empty())) {
			startEntities = { e[0] };
		} else {
			flag += 2;
			startEntities.clear();
		}
	});
	ths[1] = thread([&]() {
		queryOne(ID(endId), e[1], L"Id,F.FId,AA.AuId,AA.AfId,J.JId,C.CId");
		if (!(e[1].AAs.empty())) {
			endEntities = { e[1] };
		} else {
			flag += 1;
			endEntities.clear();
		}
	});
	ths[0].join(); ths[1].join();
#ifdef AGG_DEBUG_
	auto end_time = clock();
	cout << "\tdectect time: " << end_time - start_time << "ms" << endl;
#endif // AGG_DEBUG_
	return flag;
}

void findingPath()
{	
	auto flag = detectID_AUID();

#ifdef AGG_DEBUG_
	auto find_time = clock();
#endif // AGG_DEBUG_

	//	find path process
	switch (flag) {
	case 0: {		//	Id-Id
#ifdef AGG_DEBUG_
		cout << "Id-Id" << endl;
#endif // AGG_DEBUG_
		thread step0_1([&]() {
			//	1-hop Id-Id
			if (startEntities[0].R_Id.cend() !=
				find(startEntities[0].R_Id.cbegin(), startEntities[0].R_Id.cend(), endId)) {
				AGG_mtx.lock();
				AGG_Path.push_back(path_t({ startId,endId }));
				AGG_mtx.unlock();
			}
			//	2-hop Id-Others-Id
			auto res = _2hop_Id_Others_Id(startEntities[0], endEntities[0]);
			if (res.empty()) return;
			addAGG_path(res);
		});
		//	3-hop Id-Others-Id-Id
		thread step0_2([&]() {
#ifdef AGG_DEBUG_
			auto step02time = clock();
#endif
			thread th1([&]() {	//	Id-CId-Id-Id
				if (startEntities[0].C_Id == 0) return;
				Entity_List tmp;
				queryCustom(AND_CID_RID(startEntities[0].C_Id, endId), tmp, L"Id");
				if (tmp.empty()) return;
				paths_t res;
				for (const auto &var : tmp) {
					res.emplace_back(path_t({ startId,startEntities[0].C_Id,var.Id,endId }));
				}
				addAGG_path(res);
			});
			thread th2([&]() {	//	Id-JId-Id-Id
				if (startEntities[0].J_Id == 0) return;
				Entity_List tmp;
				queryCustom(AND_JID_RID(startEntities[0].J_Id, endId), tmp, L"Id");
				if (tmp.empty()) return;
				paths_t res;
				for (const auto &var : tmp) {
					res.emplace_back(path_t({ startId,startEntities[0].J_Id,var.Id,endId }));
				}
				addAGG_path(res);
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
						for (const auto &var : tmp) {
							res.emplace_back(path_t({ startId,startEntities[0].F_Id[i],var.Id,endId }));
						}
						addAGG_path(res);
					});
				}
				for_each(ths.begin(), ths.end(), [](thread &th) {th.join(); });
			});
			thread th4([&]() {	//	Id-AuId-Id-Id
				vector<thread> ths(startEntities[0].AAs.size());
				auto iterAA = startEntities[0].AAs.cbegin();
				for (size_t i = 0; i < startEntities[0].AAs.size(); ++i) {
					ths[i] = thread([&, iterAA]() {
						Entity_List tmp;
						queryCustom(AND_AUID_RID(iterAA->first, endId), tmp, L"Id");
						if (tmp.empty()) return;
						paths_t res;
						for (const auto &var : tmp) {
							res.emplace_back(path_t({ startId,iterAA->first,var.Id,endId }));
						}
						addAGG_path(res);
					});
					auto cnt = startEntities[0].AAs.count(iterAA->first);
					while (cnt--)++iterAA;					
				}
				for_each(ths.begin(), ths.end(), [](thread &th) {th.join(); });
			});
			th1.join(); th2.join(); th3.join(); th4.join();
#ifdef AGG_DEBUG_
			auto step02etime = clock();
			cout << "step02:" << step02etime - step02time << "ms" << endl;
#endif // AGG_DEBUG_
		});

		thread step0_3([&]() {	// 2-hop Id-Id-*-Id
#ifdef AGG_DEBUG_
			auto step03time = clock();
#endif // AGG_DEBUG_
			if (startEntities[0].R_Id.empty()) return;
			vector<thread> ths(startEntities[0].R_Id.size());
			for (size_t i = 0; i < startEntities[0].R_Id.size(); ++i) {
				ths[i] = thread([&, i]() {
					entity tmp;
					queryOne(ID(startEntities[0].R_Id[i]), tmp);
					thread th1([&]() {	//	Id-Id-Id
#ifdef AGG_DEBUG_
						auto step031time = clock();
#endif // AGG_DEBUG_
						if (tmp.R_Id.cend() !=
							find(tmp.R_Id.cbegin(), tmp.R_Id.cend(), endId)) {
							AGG_mtx.lock();
							AGG_Path.push_back(path_t({ startId,tmp.Id,endId }));
							AGG_mtx.unlock();
						}
						//	Id-Id-Others-Id
						auto res = _2hop_Id_Others_Id(tmp, endEntities[0]);
						if (res.empty()) return;
						addAGG_path(startId, res);
#ifdef AGG_DEBUG_
						auto step031etime = clock();
						cout << "step031:" << step031etime - step031time << "ms" << endl;
#endif // AGG_DEBUG_
					});
					thread th2([&]() {	//	Id-Id-Id-Id
#ifdef AGG_DEBUG_
						auto step032time = clock();
#endif // AGG_DEBUG_
						auto res = _2hop_Id_Id_Id(tmp, endEntities[0]);
						if (res.empty()) return;
						addAGG_path(startId, res);
#ifdef AGG_DEBUG_
						auto step032etime = clock();
						cout << "step032:" << step032etime - step032time << "ms" << endl;
#endif // AGG_DEBUG_
					});
					th1.join();
					th2.join();
				});
			}
			for_each(ths.begin(), ths.end(), [](thread &th) {th.join(); });
#ifdef AGG_DEBUG_
			auto step03etime = clock();
			cout << "step03:" << step03etime - step03time << "ms" << endl;
#endif // AGG_DEBUG_
		});
		step0_1.join();
		step0_2.join();
		step0_3.join();
	}break;
	case 1: {		//	Id-AuId
#ifdef AGG_DEBUG_
		cout << "Id-AuId" << endl;
		auto case1_time = clock();
#endif // AGG_DEBUG_
		thread th1([&]() {
			queryCustom(AUID(endId), endEntities, L"Id,F.FId,AA.AuId,AA.AfId,J.JId,C.CId");
		});
		//	1-hop
		thread step1_1([&]() {	//	Id-AuId-1-hop
			if (startEntities[0].AAs.find(endId) != startEntities[0].AAs.end()) {
				AGG_mtx.lock();
				AGG_Path.emplace_back(path_t({ startId,endId }));
				AGG_mtx.unlock();
			}
		});
		th1.join(); step1_1.join();
#ifdef AGG_DEBUG_
		auto case1_qtime = clock();
		cout << "query time:" << case1_qtime - case1_time << "ms" << endl;
#endif // AGG_DEBUG_
		//	2-hop
		thread step1_2(_2hop_Id_Id_AuId);	
		//	3-hop
		thread step1_3(_3hop_Id_Others_Id_AuId);	
		thread step1_4(_3hop_Id_Id_Id_AuId);
		thread step1_5(_3hop_Id_AuId_AfId_AuId);	//	Id-AuId-AfId-AuId
		step1_2.join();
		step1_3.join();
		step1_4.join();
		step1_5.join();
	}break;
	case 2: {		//	AuId-Id
#ifdef AGG_DEBUG_
		cout << "AuId-Id" << endl;
		auto case2_time = clock();
#endif // AGG_DEBUG_
		thread th1([&]() {
			queryCustom(AUID(startId), startEntities);
		});
		//	1-hop
		thread step2_1([&]() {	//	AuId-Id
			if (endEntities[0].AAs.find(startId) != endEntities[0].AAs.end()) {
				AGG_mtx.lock();
				AGG_Path.emplace_back(path_t({ startId,endId }));
				AGG_mtx.unlock();
			}
		});
		th1.join();
		step2_1.join();

#ifdef AGG_DEBUG_
		auto case2_qtime = clock();
		cout << "query time:" << case2_qtime - case2_time << "ms" << endl;
#endif // AGG_DEBUG_
		//	2-hop
		thread step2_2(_2hop_AuId_Id);	//	AuId-Id-Id
		//	3-hop
		thread step2_3([&]() {	//	AuId-AfId-AuId-Id
#ifdef AGG_DEBUG_
			auto step23time = clock();
#endif // AGG_DEBUG_
			vector<thread> ths(endEntities[0].AAs.size());
			auto iterAA = endEntities[0].AAs.cbegin();
			for (size_t i = 0; i < endEntities[0].AAs.size(); ++i) {
				ths[i] = thread([&, iterAA]() {
					auto res = _2hop_AuId_AfId_AuId(startId, iterAA->first, startEntities);
					if (res.empty()) return;
					addAGG_path(res, endId);
				});
				++iterAA;
			}
			for_each(ths.begin(), ths.end(), [](thread &th) {th.join(); });
#ifdef AGG_DEBUG_
			auto step23etime = clock();
			cout << "step23:" << step23etime - step23time << "ms" << endl;
#endif // AGG_DEBUG_
		});
		thread step2_4(_3hop_AuId_Id_Others_Id);
		thread step2_5(_3hop_AuId_Id_Id_Id);

		step2_2.join();
		step2_3.join();
		step2_4.join();
		step2_5.join();
	}break;
	case 3: {		//	AuId-AuId
#ifdef AGG_DEBUG_
		cout << "AuId-AuId" << endl;
		auto case3_time = clock();
#endif // AGG_DEBUG_
		thread ths1([&]() {queryCustom(AUID(startId), startEntities, L"Id,AA.AuId,AA.AfId,RId"); });
		thread ths2([&]() {queryCustom(AUID(endId), endEntities, L"Id,AA.AuId,AA.AfId"); });
		ths1.join(); ths2.join();
#ifdef AGG_DEBUG_
		auto case3_qtime = clock();
		cout << "query time:" << case3_qtime - case3_time << "ms" << endl;
#endif // AGG_DEBUG_
		//	no 1-hop
		//	2-hop
		thread step3_1([&]() {	//	AuId-Id-AuId
			auto res = _2hop_AuId_Id_AuId(startId, endId, startEntities, endEntities);
			if (res.empty()) return;
			addAGG_path(res);
		});
		thread step3_2([&]() {	//	AuId-AfId-AuId
			auto res = _2hop_AuId_AfId_AuId(startId, endId, startEntities, endEntities);
			if (res.empty()) return;
			addAGG_path(res);
		});
		//	3-hop
		thread step3_3(_3hop_AuId_AuId);
		step3_1.join();
		step3_2.join();
		step3_3.join();
	}break;
	}

#ifdef AGG_DEBUG_
	cout << "finding time: " << clock() - find_time << "ms" << endl;
#endif // AGG_DEBUG_
	finding = false;
}

paths_t _2hop_Id_Id_Id(const entity &ent1, const entity &ent2)
{
	paths_t res;
	if (ent1.R_Id.empty()) return res;
	mutex resMtx;
	vector<thread> queryThreads(ent1.R_Id.size());
	for (size_t i = 0; i < queryThreads.size(); ++i) {
		queryThreads[i] = thread([&, i]() {
			entity tmp;
			queryOne(AND_ID_RID(ent1.R_Id[i], ent2.Id), tmp, L"Id");
			if (tmp.Id != 0) {
				resMtx.lock();
				res.push_back(path_t({ ent1.Id, tmp.Id, ent2.Id }));
				resMtx.unlock();
			}
		});
	}
	for_each(queryThreads.begin(), queryThreads.end(),
		[](thread &th) {th.join(); });

	return res;
}


paths_t _2hop_Id_Others_Id(const entity & ent1, const entity & ent2)
{
	paths_t res;
	if (ent1.Id == ent2.Id) {
		if (ent1.C_Id != 0) {
			res.push_back(path_t({ ent1.Id,ent1.C_Id,ent2.Id }));
		}
		if (ent1.J_Id != 0) {
			res.push_back(path_t({ ent1.Id,ent1.J_Id,ent2.Id }));
		}
		for (const auto &fid : ent1.F_Id) {
			res.push_back(path_t({ ent1.Id,fid,ent2.Id }));
		}
		for (const auto &aa : ent1.AAs) {
			res.push_back(path_t({ ent1.Id,aa.first,ent2.Id }));
		}
		return res;
	}
	
	mutex mtx;
	thread F([&]() {	//	Id-FId-Id
		for (auto &var : ent1.F_Id) {
			if ((var != 0) && (find(ent2.F_Id.cbegin(), ent2.F_Id.cend(), var) != ent2.F_Id.cend())) {
				mtx.lock();
				res.emplace_back(path_t({ ent1.Id,var,ent2.Id }));
				mtx.unlock();
			}
		}
	});

	thread AAth([&]() {	//	Id-AuId-Id
		for (auto it = ent1.AAs.cbegin(); it != ent1.AAs.cend(); ++it) {
			if (ent2.AAs.find(it->first) != ent2.AAs.end()) {
				mtx.lock();
				res.emplace_back(path_t({ ent1.Id,it->first,ent2.Id }));
				mtx.unlock();
				auto cnt = ent1.AAs.count(it->first);
				while (--cnt) ++it;
			}
		}
	});

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

//	this function only can be used in Id-Id-AuId-2hop
void _2hop_Id_Id_AuId()
{
	if (startEntities[0].R_Id.empty()) return;
	paths_t res;
	for (auto &var : endEntities) {
		if (find(startEntities[0].R_Id.cbegin(), startEntities[0].R_Id.cend(), var.Id) 
			!= startEntities[0].R_Id.cend()) {
			res.emplace_back(path_t({ startId,var.Id,endId }));
		}
	}
	if (res.empty()) return;
	addAGG_path(res);
}

void _2hop_AuId_Id()
{
	paths_t res;
	for (auto &var : startEntities) {
		if (find(var.R_Id.cbegin(), var.R_Id.cend(), endId) != var.R_Id.cend()) {
			res.emplace_back(path_t({ startId,var.Id,endId }));
		}
	}
	if (res.empty()) return;
	addAGG_path(res);
}

paths_t _2hop_AuId_AfId_AuId(Id_type id1, Id_type id2, Entity_List & ls1)
{
	paths_t res;
	mutex resMtx;
	set<Id_type> AfIds;
	mutex setMtx;

	for (auto &var : ls1) {
		auto cnt = var.AAs.count(id1);
		auto it = var.AAs.find(id1);
		while (cnt) {
			AfIds.insert(it->second);
			++it;
			--cnt;
		}
	}
	AfIds.erase(0);
	if (id1 == id2) {		
		for (const auto &var : AfIds) {
			res.emplace_back(path_t({ id1,var,id2 }));
		}
	} else {
		vector<thread> ths(AfIds.size());
		auto it = AfIds.begin();
		for (size_t i = 0; i < ths.size(); ++i) {
			ths[i] = thread([&]() {
				Id_type af = *it;
				entity a;
				queryOne(AND_AUID_AFID(id2, *it), a, L"Id");
				if (a.Id != 0) {
					resMtx.lock();
					res.emplace_back(path_t({ id1,*it,id2 }));
					resMtx.unlock();
				}
			});
			++it;
		}
		for_each(ths.begin(), ths.end(), [](thread &th) {th.join(); });
	}

	return res;
}

paths_t _2hop_AuId_AfId_AuId(Id_type id1, Id_type id2, Entity_List & ls1, Entity_List & ls2)
{
	paths_t res;
	set<Id_type> AfIds;
	
	if (id1 == id2) {
		for (auto &var : ls1) {
			auto cnt = var.AAs.count(id1);
			auto it = var.AAs.find(id1);
			while (cnt) {
				AfIds.insert(it->second);
				++it;
				--cnt;
			}
		}
		AfIds.erase(0);
		for (const auto &var : AfIds) {
			res.emplace_back(path_t({ id1,var,id2 }));
		}
	} else {
		set<Id_type> AfIds2;
		thread th1([&]() {
			for (auto &var : ls1) {
				auto cnt = var.AAs.count(id1);
				auto it = var.AAs.find(id1);
				while (cnt) {
					AfIds.insert(it->second);
					++it;
					--cnt;
				}
			}
			AfIds.erase(0);
		});
		thread th2([&]() {
			for (auto &var : ls2) {
				auto cnt = var.AAs.count(id2);
				auto it = var.AAs.find(id2);
				while (cnt) {
					AfIds.insert(it->second);
					++it;
					--cnt;
				}
			}
		});
		th1.join(); th2.join();
		for (const auto &var : AfIds2) {
			if (AfIds.find(var) != AfIds.end()) {
				res.emplace_back(path_t({ id1,var,id2 }));
			}
		}
	}

	return res;
}

paths_t _2hop_AuId_Id_AuId(Id_type id1, Id_type id2, Entity_List &ls1, Entity_List &ls2)
{
	paths_t res;
	if (id1 == id2) {
		if (ls1.empty()) {
			queryCustom(AUID(id1), ls1, L"Id");
		}
		for (const auto &var : ls1) {
			res.emplace_back(path_t({ id1,var.Id,id2 }));
		}
	} else {
		if (ls1.empty() || ls2.empty()) {
			Entity_List tmp;
			queryCustom(AND_AUID_AUID(id1, id2), tmp, L"Id");
			for (auto &var : tmp) {
				res.emplace_back(path_t({ id1,var.Id,id2 }));
			}
		} else {
			for (auto &var : ls1) {
				if (ls2.cend() !=
					find_if(ls2.cbegin(), ls2.cend(), [&](entity a) {return a.Id == var.Id; })) {
					res.push_back(path_t({ id1, var.Id, id2 }));
				}
			}
		}
	}
	return res;
}

void _3hop_Id_Others_Id_AuId()
{
	vector<thread> ths(endEntities.size());
	for (size_t i = 0; i < ths.size(); ++i) {
		ths[i] = thread([&, i]() {
			auto res = _2hop_Id_Others_Id(startEntities[0], endEntities[i]);
			if (res.empty()) return;
			addAGG_path(res, endId);
		});
	}
	for_each(ths.begin(), ths.end(), [](thread &th) {th.join(); });
}

void _3hop_Id_Id_Id_AuId()
{
	paths_t res;
	for (auto &var : endEntities) {
		auto res1 = _2hop_Id_Id_Id(startEntities[0], var);
		for each (auto var1 in res1) {
			res.emplace_back(path_t({ startId,var1[1],var1[2],endId }));
		}
	}
	if (res.empty()) return;
	addAGG_path(res);
}

void _3hop_Id_AuId_AfId_AuId()
{
	vector<thread> ths(startEntities[0].AAs.size());
	auto iterAA = startEntities[0].AAs.cbegin();
	for (auto &th : ths) {
		th = thread([&, iterAA]() {
			auto res = _2hop_AuId_AfId_AuId(endId, iterAA->first, endEntities);	// reverse
			if (res.empty()) return;
			paths_t res1;
			for (auto p : res) {
				res1.push_back(path_t({ startId,p[2],p[1],p[0] }));
			}
			addAGG_path(res1);
		});
		++iterAA;
	}
	for_each(ths.begin(), ths.end(), [](thread &th) {th.join(); });
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
	addAGG_path(res);
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
	addAGG_path(res);
}

void _3hop_AuId_AuId()	//	AuId-Id-Id-AuId
{
	vector<thread> ths(endEntities.size());
	for (size_t i = 0; i < endEntities.size(); ++i) {
		ths[i] = thread([&, i]() {
			for (auto &var1 : startEntities) {
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

// Add path to final result
void addAGG_path(const paths_t &path)
{
	AGG_mtx.lock();
	copy(path.cbegin(), path.cend(), back_inserter(AGG_Path));
	AGG_mtx.unlock();
}

void addAGG_path(Id_type id, paths_t &path)
{
	for (auto &var : path) {
		var.insert(var.begin(), id);
	}
	AGG_mtx.lock();
	copy(path.cbegin(), path.cend(), back_inserter(AGG_Path));
	AGG_mtx.unlock();
}

void addAGG_path(paths_t &path, Id_type id)
{
	for (auto &var : path) {
		var.push_back(id);
	}
	AGG_mtx.lock();
	copy(path.cbegin(), path.cend(), back_inserter(AGG_Path));
	AGG_mtx.unlock();
}
/***********************Dll interface********************************************/
clock_t gstart_time;
paths_t findPath(Id_type id1, Id_type id2)
{
	gstart_time = clock();
	finding = true;
#ifdef AGG_DEBUG_
	//if (id1 != 57898110 || id2 != 2014261844) return AGG_Path;
#endif // AGG_DEBUG_
	startId = id1;
	endId = id2;
	AGG_mtx.lock();
	thread findThread(findingPath);
	findThread.detach();
	AGG_Path.clear();	// clear All for a new process
	AGG_mtx.unlock();
	while (finding && ((clock() - gstart_time) < 280000));

#ifdef AGG_DEBUG_
	if (finding) {
		cout << "running Time Out" << endl;
	} else {
		cout << "Total running time: " << clock() - gstart_time << "ms" << endl;
		cout << "Total path numbers:" << AGG_Path.size() << endl;
	}
#endif // AGG_DEBUG_

	return AGG_Path;
}
