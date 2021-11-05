##10.30～10.31 Sat~Sun
* 胡乱摸索，写了select meta、drop table和update

##11.1  Mon
* 第一次提交过代码后，英雄榜和测试结果上都没有队伍的名字，问王运来老师后才得知要把编译产生的文件都删掉再上传代码。
* 下午的增量测试中只有select meta和drop table过了，update挂了，原因不明。
* 翘了语文课，尝试写了select_tables，结果不能正确处理where t1.id = t2.id的情况，最终的查询结果中会把id不等的两个表的记录连接在一起，原因是自带的do_select不能正确处理表间的比较条件，感觉需要自己写一个。

##11.2 Tue
* update过了，之前挂的原因是在where里有不存在的列名时会返回failure，但实际上应该返回success并且什么也不做

##11.3 Wed
* 继续尝试select_tables，试了好几种思路，最后都是写了一半，发现无法实现。

##11.4 Thur
* select_tables终于过了，不过是最裸的暴力，就是直接把所有表的数据存到一个大tupleset里，把filter的代码搬过来，然后暴力判断每条tuple是否满足where条件。在处理t.name = 'qwe'的where条件时会server core，原因是condition里的CHARS类型是用char *类型实现的，但tuple里的CHARS类型是用string实现的，要先把char *转成string以后再转换成void *
