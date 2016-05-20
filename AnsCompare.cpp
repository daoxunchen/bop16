#define _CRT_SECURE_NO_WARNINGS

#include <cstdio>
#include <vector>
#include <algorithm>
#include <thread>
#include "rapidjson\document.h"
#include "rapidjson\filereadstream.h"

using namespace std;
using namespace rapidjson;

using Id_type = long long;
using path_t = vector<Id_type>;
using paths_t = vector<path_t>;

paths_t jsonToVec(const Document &doc);
bool comparePath(const path_t &a, const path_t &b);
void find_rmRepeat(paths_t &p, const char *ch);

int main(int argc, char *argv[])
{
	FILE *fp1 = fopen(argv[1], "rb");
	if (!fp1) {
		printf("read file 1 err\r\n");
		return 1;
	}
	char readBuffer1[65536];
	FileReadStream is1(fp1, readBuffer1, sizeof(readBuffer1));
	Document d1, d2;
	paths_t ans1, ans2;
	d1.ParseStream(is1);
	ans1 = jsonToVec(d1);
	sort(ans1.begin(), ans1.end(), comparePath);
	FILE *fp2;
	if (argc < 3 || !(fp2 = fopen(argv[2], "rb"))) {
		printf("ans1 : %u\r\n", ans1.size());
		printf("*****************************\r\nrepeat:\r\n*****************************\r\n");
		find_rmRepeat(ans1, "ans1");
		fclose(fp1);
		return 1;
	}
	char readBuffer2[65536];
	FileReadStream is2(fp2, readBuffer2, sizeof(readBuffer2));
	d2.ParseStream(is2);
	ans2 = jsonToVec(d2);
	sort(ans2.begin(), ans2.end(), comparePath);
	if (ans1.size() == ans2.size()) {
		printf("both ans have %d paths\r\n", ans1.size());
	} else {
		printf("ans1 : %u\r\n", ans1.size());
		printf("ans2 : %u\r\n", ans2.size());
	}
	
	printf("*****************************\r\nrepeat:\r\n*****************************\r\n");
	find_rmRepeat(ans1, "ans1");
	printf("*****************************\r\n");
	find_rmRepeat(ans2, "ans2");
	printf("*****************************\r\ndiffererce:");

	paths_t dif;
	set_difference(ans1.begin(), ans1.end(), ans2.begin(), ans2.end(), back_inserter(dif), comparePath);
	for (const auto &p : dif) {
		printf("ans1: [ ");
		for (size_t i = 0; i < p.size(); ++i) {
			printf("%lld ", p[i]);
		}
		printf("]\t");
	}
	printf("\r\n*****************************\r\n");
	dif.clear();
	set_difference(ans2.begin(), ans2.end(), ans1.begin(), ans1.end(), back_inserter(dif), comparePath);
	for (const auto &p : dif) {
		printf("ans2: [ ");
		for (size_t i = 0; i < p.size(); ++i) {
			printf("%lld ", p[i]);
		}
		printf("]\t");
	}
	printf("\r\n*****************************\r\n");
	fclose(fp1);
	fclose(fp2);
	return 0;
}

paths_t jsonToVec(const Document &doc)
{
	paths_t res;
	auto vals = doc.GetArray();
	for (auto iter = vals.Begin(); iter != vals.End(); ++iter) {
		auto p = iter->GetArray();
		path_t tmp;
		for (auto iter1 = p.Begin(); iter1 != p.End(); ++iter1) {
			tmp.push_back(iter1->GetInt64());
		}
		res.push_back(tmp);
	}
	return res;
}

bool comparePath(const path_t &a, const path_t &b)
{
	if (a.size() != b.size())
		return a.size() < b.size();
	switch (a.size()) {
	case 2:return a[1] < b[1]; break;
	case 3:
		if (a[1] != b[1])
			return a[1] < b[1];
		else
			return a[2] < b[2];
		break;
	default:
		if (a[1] != b[1])
			return a[1] < b[1];
		else
			if (a[2] != b[2])
				return a[2] < b[2];
			else
				return a[3] < b[3];
		break;
	}
}

void find_rmRepeat(paths_t & p, const char * ch)
{
	while (true) {
		auto iter = adjacent_find(p.begin(), p.end());
		if (iter == p.end()) break;
		auto iter2 = iter + 1;
		size_t cnt = 2;
		while ((*++iter2) == (*iter)) ++cnt;
		printf("%s\t[ ", ch);
		for (size_t i = 0; i < iter->size(); ++i) {
			printf("%lld ", (*iter)[i]);
		}
		printf("]\trepeat %u times\r\n", cnt);
		p.erase(iter + 1, iter2);
	}
}
