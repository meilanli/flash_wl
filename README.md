
flash磨损平衡 wear-leveling

## 原理应用
	
	应用于数据量不大，但需要经常保存数据的场景，比如防止突然掉电丢失数据
	写之前擦除一页，写满一页再擦除下一页或者擦除当前页。
## 移植：
	修改flash_wl_config.h
	1, 修改5个flash操作接口: 解锁、上锁、擦页、写、读
	2, 修改宏定义: 页大小，将用于平衡擦写的页起始，页数
	
## 使用：
	1,实现flash_wl_config.h里面相关flash接口
	1,包含flash_wl.c
	2,初始化flash_wl_init()
	3,保存数据flash_wl_write()
	4,读出最后一次保存的数据flash_wl_read()

## 数据存取
	存储格式 data0 ... dataN [4字节补位] 55 LenH LenL AA
	初始从有效flash段末尾倒序查找 标志AA
	获取长度，提取数据
	
##	注意
	建议数据长度4字节整数倍,因为4字节补位未验证