#ifndef __FLASH_WL_CONFIG_H
#define __FLASH_WL_CONFIG_H

#include "gd32e230.h"

//页大小
#define FLASH_PAGE_SIZE         1024
//平衡擦写起始页开始地址
#define FLASH_WL_START_ADDR     (0x08000000+FLASH_PAGE_SIZE*12)     
//平衡擦写使用的页数
#define FLASH_WL_PAGE_NUM       2       
//写入后是否读出数据检查
#define FLASH_WL_WRITE_CHECK    1   

//擦、写解锁并清除标志
#define DEV_UNLOCK()    do{ \
                            fmc_unlock(); \
                            fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGERR); \
                        }while(0)

//擦、写上锁
#define DEV_LOCK()    do{ \
                            fmc_lock(); \
                        }while(0)


//页擦除，传入页首地址。失败1，成功0
#define DEV_PAGE_ERASE(addr)    (( FMC_READY != fmc_page_erase(addr) )?1:0 )

//写一个字。失败1，成功0
#define DEV_WORD_WRITE(addr, word)      (( FMC_READY != fmc_word_program(addr, word) )?1:0 )

//读一个字，返回word
#define DEV_WORD_READ(addr)     (*(uint32_t *)(addr))

#endif
