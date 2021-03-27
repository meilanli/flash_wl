//=============================================================================
//文件名称：flash_wl.c
//功能概要：flash磨损平衡 wear-leveling,4字节对齐
//更新时间：2020-12-15 	meilanli
//=============================================================================
#include "flash_wl.h"
#include "flash_wl_config.h"

//结束页开始地址，包含此页
#define FLASH_WL_END_ADDR       (FLASH_WL_START_ADDR+ (FLASH_WL_PAGE_NUM-1)*FLASH_PAGE_SIZE)    

struct flash_wl_struct
{
    uint32_t data_addr; //最后保存的有效数据的起始地址
    uint32_t free_addr; //当前空闲空间起始地址
    uint32_t page_start_addr;   //当前操作的页起始地址
};

typedef union
{
    uint32_t word;
    uint8_t byte[4];
}un_byte_word;

static struct flash_wl_struct s_flash=
{
    FLASH_WL_START_ADDR,
    FLASH_WL_START_ADDR,
    FLASH_WL_START_ADDR
};

//擦、写解锁并清除标志
static void dev_unlock(void)
{
    DEV_UNLOCK();
}
                        
//擦、写上锁
static void dev_lock(void)
{
    DEV_LOCK();
}

//页擦除，传入页首地址
static int8_t dev_page_erase(uint32_t addr)
{
    if( DEV_PAGE_ERASE(addr) )
    {
        return -1;
    }
    return 0;
}

//写一个字
static int8_t dev_word_write(uint32_t addr, uint32_t word)
{
    if( DEV_WORD_WRITE(addr, word) )
    {
        return -1;
    }
    return 0;
}

//读一个字
static uint32_t dev_word_read(uint32_t addr)
{
    return DEV_WORD_READ(addr);
}

//磨损平衡写数据
//数据长度+补位字节 小于 页大小/2+4
//补位字节为补齐4字节整数倍需要的长度
//保存格式:  data0 ... dataN [4字节补位] 55 LenH LenL AA
FlashWlStatusFlag flash_wl_write(uint8_t *data, uint32_t data_len)
{
    FlashWlStatusFlag ret = FWL_NO_ERROR;
    uint32_t i,j;
    un_byte_word temp;
    uint8_t rem;    //余数
    uint32_t len;   //加上补位字节长度
    
    //数据长度4字节对齐
    rem = data_len%4;
    if(rem)
        len = data_len +4 -rem;
    else
        len = data_len;
    
    //最终数据长度大于半页,无法磨损平衡
    if(len+4 > FLASH_PAGE_SIZE/2) return FWL_LONG_ERR;    

    dev_unlock();
    
    //如果数据超过本页空闲空间，切换页
    if( s_flash.free_addr+len > s_flash.page_start_addr+FLASH_PAGE_SIZE)
    {
        if( dev_page_erase(s_flash.page_start_addr) <0 )  //擦除当前使用的页
        {
            ret = FWL_ERASE_ERR;
            goto err1;
        }
    
        s_flash.page_start_addr += FLASH_PAGE_SIZE; //下一页开始地址
        if(s_flash.page_start_addr > FLASH_WL_END_ADDR) //超过设置的页区间
        {
            s_flash.page_start_addr = FLASH_WL_START_ADDR; //回到起始页
        }
        
        s_flash.free_addr = s_flash.page_start_addr;    //空闲地址等于页起始地址
    }

    //写入数据到空闲地址
    for(i=0; i<len; i+=4)
    {
        if( (len-i) >= 4 )
        {
            temp.word = *(uint32_t *)(data+i);
        }
        else    //如果最后一次不足4字节
        {
            temp.word = 0xffffffff;
            for(j=0;j<rem;j++)
            {
                temp.byte[j] = data[i+j];
            }
        }
        
        if( dev_word_write(s_flash.free_addr+i, temp.word ) <0)
        {
            ret = FWL_WRITE_ERR;
            goto err1;
        }
    }
    
    //写存储标志
    temp.byte[0] = 0x55;
    temp.byte[1] = data_len >>8 &0xFF;   //有效数据长度
    temp.byte[2] = data_len &0xFF;
    temp.byte[3] = 0xAA;
    if( dev_word_write(s_flash.free_addr+len, temp.word ) <0)
    {
        ret = FWL_WRITE_ERR;
        goto err1;
    }
    
    #if FLASH_WL_WRITE_CHECK
    //检查写入值
    if(temp.word != dev_word_read(s_flash.free_addr+len) )
    {
        ret = FWL_CHECK_ERR;
        goto err1;
    }
    
    for (i = 0; i < len; i+=4)
    {
        temp.word = dev_word_read(s_flash.free_addr+i);
        if( (len-i) >= 4 )
        {
            if(temp.word != *(uint32_t *)(data+i) )
            {
                ret = FWL_CHECK_ERR;
                goto err1;
            }
        }
        else
        {
            for(j=0;j<rem;j++)
            {
                if(temp.byte[j] != data[i+j] )
                {
                    ret = FWL_CHECK_ERR;
                    goto err1;
                }
            }
        }
    }
    #endif
    
    s_flash.data_addr = s_flash.free_addr;  //空闲地址变成新数据地址
    s_flash.free_addr += len+4; //空闲地址移到下一个未使用的空间

    mb_set_register_word(ADDR_TEST+3, s_flash.data_addr&0xffff);
    mb_set_register_word(ADDR_TEST+4, s_flash.free_addr&0xffff);
    mb_set_register_word(ADDR_TEST+5, s_flash.page_start_addr&0xffff);
err1:
    dev_lock();
	return ret;
}

//磨损平衡读数据
//返回读出数据长度
//没有数据返回0
uint32_t flash_wl_read(uint8_t *data, uint32_t data_len)
{
    uint32_t i,len;
    un_byte_word temp;
    uint8_t j;
    
    //未初始化或没有保存数据
    if(s_flash.data_addr == s_flash.free_addr) return 0;
    
    temp.word = dev_word_read(s_flash.free_addr-4);
    if(0x55 != temp.byte[0] || 0xAA != temp.byte[3]) return 0;  //检查标志
    
    len = temp.byte[1]*256+temp.byte[2];    //数据长度
    if( len > data_len ) len = data_len;
    
    for(i=0; i<len; i+=4)
    {
        if( (len-i) >= 4 )
        {
            *(uint32_t *)(data+i) = dev_word_read(s_flash.data_addr+i);
        }
        else //如果最后一次不足4字节
        {
            temp.word = dev_word_read(s_flash.data_addr+i);
            for(j=0;j<len%4;j++)
            {
                data[i+j] = temp.byte[j];
            }
        }
    }
    return len;
}

//初始化,定位到数据位置
void flash_wl_init(void)
{
    int16_t i;
    uint32_t addr;
    uint32_t page_addr;
    un_byte_word temp;
    uint32_t len;
    uint8_t rem;

    page_addr = FLASH_WL_START_ADDR;

    for(i=FLASH_WL_PAGE_NUM-1; i>=0; i--)
    {
        page_addr = FLASH_WL_START_ADDR + FLASH_PAGE_SIZE*i;
        
        for(addr=FLASH_PAGE_SIZE-4; addr>4; addr-=4)
        {
            temp.word = dev_word_read(addr+page_addr);

            if( temp.byte[0] == 0x55 && temp.byte[3] == 0xAA)
            {
                len = temp.byte[1]*256 +temp.byte[2];
                rem = len%4;
                if(rem)
                {
                    len = len + (4-rem);
                }
                s_flash.data_addr = addr+page_addr - len;
                s_flash.free_addr = addr+page_addr+4;
                s_flash.page_start_addr = page_addr;
                
                mb_set_register_word(ADDR_TEST+6, s_flash.data_addr&0xffff);
                mb_set_register_word(ADDR_TEST+7, s_flash.free_addr&0xffff);
                mb_set_register_word(ADDR_TEST+8, s_flash.page_start_addr&0xffff);
                
                return ;
            }
        }
    }
    
    dev_unlock();
    //如果没有找到存储数据，擦除所有页，避免脏数据，已经擦除的页再擦除不影响寿命
    for(i=0; i<FLASH_WL_PAGE_NUM; i++)
    {
        page_addr = FLASH_WL_START_ADDR + FLASH_PAGE_SIZE*i;
        dev_page_erase(page_addr);  
    }
    dev_lock();
    
    s_flash.free_addr = FLASH_WL_START_ADDR;
    s_flash.data_addr = FLASH_WL_START_ADDR;
    s_flash.page_start_addr = FLASH_WL_START_ADDR;
}
