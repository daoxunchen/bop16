# bop16

这是编程之美2016复赛的代码，小组排名68。。。和平时评测水平差不多

代码由于没有考虑在一篇paper中一个AuId可以对应多个AfId的情况，所以在极少数情况下会缺失部分路径。
幸运的是在复赛最终评测中好像没有遇到这种情况。。。

* master分支是比赛提交的程序代码
采用cpprest作restful服务和query查询及cpprest内置的json parser来解析和发送
* devA分支是修正上述bug，并且参照进决赛的队伍的代码优化过的版本（未完成）
采用winhttp重写query，采用rapidjson解析json
优化部分：1、采用http长连接，减少查询时间 2、每次查询都查两次，避免网络波动
* AnsCompare分支答案比对工具（未完成）
* MyQuery分支自己搭建的评测系统（未完成）