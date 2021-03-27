#ifndef __FLASH_WL_H
#define __FLASH_WL_H
#include "stdint.h"

typedef enum
{
    FWL_NO_ERROR = 0,
    //FWL_ERROR = -1,             //错误
    //FWL_PAR_ERR = -2,           //参数错误
    FWL_LONG_ERR = -3,          //数据太长
    FWL_ERASE_ERR = -4,         //擦除失败
    FWL_WRITE_ERR = -5,         //写入失败
    //FWL_DATA_ERR = -6,          //数据错误
    FWL_CHECK_ERR = -7,         //写入的数据读出检查错误
    //FWL_ADDR_ERR = -8,          //地址错误
}FlashWlStatusFlag;

//写数据
FlashWlStatusFlag flash_wl_write(uint8_t *data, uint32_t data_len);

//读数据
//返回读出数据长度
//没有数据返回0
uint32_t flash_wl_read(uint8_t *data, uint32_t data_len);  

//初始化,定位到最后一次数据位置
void flash_wl_init(void);   

#endif
