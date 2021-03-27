//=============================================================================
//�ļ����ƣ�flash_wl.c
//���ܸ�Ҫ��flashĥ��ƽ�� wear-leveling,4�ֽڶ���
//����ʱ�䣺2020-12-15 	meilanli
//=============================================================================
#include "flash_wl.h"
#include "flash_wl_config.h"

//����ҳ��ʼ��ַ��������ҳ
#define FLASH_WL_END_ADDR       (FLASH_WL_START_ADDR+ (FLASH_WL_PAGE_NUM-1)*FLASH_PAGE_SIZE)    

struct flash_wl_struct
{
    uint32_t data_addr; //��󱣴����Ч���ݵ���ʼ��ַ
    uint32_t free_addr; //��ǰ���пռ���ʼ��ַ
    uint32_t page_start_addr;   //��ǰ������ҳ��ʼ��ַ
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

//����д�����������־
static void dev_unlock(void)
{
    DEV_UNLOCK();
}
                        
//����д����
static void dev_lock(void)
{
    DEV_LOCK();
}

//ҳ����������ҳ�׵�ַ
static int8_t dev_page_erase(uint32_t addr)
{
    if( DEV_PAGE_ERASE(addr) )
    {
        return -1;
    }
    return 0;
}

//дһ����
static int8_t dev_word_write(uint32_t addr, uint32_t word)
{
    if( DEV_WORD_WRITE(addr, word) )
    {
        return -1;
    }
    return 0;
}

//��һ����
static uint32_t dev_word_read(uint32_t addr)
{
    return DEV_WORD_READ(addr);
}

//ĥ��ƽ��д����
//���ݳ���+��λ�ֽ� С�� ҳ��С/2+4
//��λ�ֽ�Ϊ����4�ֽ���������Ҫ�ĳ���
//�����ʽ:  data0 ... dataN [4�ֽڲ�λ] 55 LenH LenL AA
FlashWlStatusFlag flash_wl_write(uint8_t *data, uint32_t data_len)
{
    FlashWlStatusFlag ret = FWL_NO_ERROR;
    uint32_t i,j;
    un_byte_word temp;
    uint8_t rem;    //����
    uint32_t len;   //���ϲ�λ�ֽڳ���
    
    //���ݳ���4�ֽڶ���
    rem = data_len%4;
    if(rem)
        len = data_len +4 -rem;
    else
        len = data_len;
    
    //�������ݳ��ȴ��ڰ�ҳ,�޷�ĥ��ƽ��
    if(len+4 > FLASH_PAGE_SIZE/2) return FWL_LONG_ERR;    

    dev_unlock();
    
    //������ݳ�����ҳ���пռ䣬�л�ҳ
    if( s_flash.free_addr+len > s_flash.page_start_addr+FLASH_PAGE_SIZE)
    {
        if( dev_page_erase(s_flash.page_start_addr) <0 )  //������ǰʹ�õ�ҳ
        {
            ret = FWL_ERASE_ERR;
            goto err1;
        }
    
        s_flash.page_start_addr += FLASH_PAGE_SIZE; //��һҳ��ʼ��ַ
        if(s_flash.page_start_addr > FLASH_WL_END_ADDR) //�������õ�ҳ����
        {
            s_flash.page_start_addr = FLASH_WL_START_ADDR; //�ص���ʼҳ
        }
        
        s_flash.free_addr = s_flash.page_start_addr;    //���е�ַ����ҳ��ʼ��ַ
    }

    //д�����ݵ����е�ַ
    for(i=0; i<len; i+=4)
    {
        if( (len-i) >= 4 )
        {
            temp.word = *(uint32_t *)(data+i);
        }
        else    //������һ�β���4�ֽ�
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
    
    //д�洢��־
    temp.byte[0] = 0x55;
    temp.byte[1] = data_len >>8 &0xFF;   //��Ч���ݳ���
    temp.byte[2] = data_len &0xFF;
    temp.byte[3] = 0xAA;
    if( dev_word_write(s_flash.free_addr+len, temp.word ) <0)
    {
        ret = FWL_WRITE_ERR;
        goto err1;
    }
    
    #if FLASH_WL_WRITE_CHECK
    //���д��ֵ
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
    
    s_flash.data_addr = s_flash.free_addr;  //���е�ַ��������ݵ�ַ
    s_flash.free_addr += len+4; //���е�ַ�Ƶ���һ��δʹ�õĿռ�

    mb_set_register_word(ADDR_TEST+3, s_flash.data_addr&0xffff);
    mb_set_register_word(ADDR_TEST+4, s_flash.free_addr&0xffff);
    mb_set_register_word(ADDR_TEST+5, s_flash.page_start_addr&0xffff);
err1:
    dev_lock();
	return ret;
}

//ĥ��ƽ�������
//���ض������ݳ���
//û�����ݷ���0
uint32_t flash_wl_read(uint8_t *data, uint32_t data_len)
{
    uint32_t i,len;
    un_byte_word temp;
    uint8_t j;
    
    //δ��ʼ����û�б�������
    if(s_flash.data_addr == s_flash.free_addr) return 0;
    
    temp.word = dev_word_read(s_flash.free_addr-4);
    if(0x55 != temp.byte[0] || 0xAA != temp.byte[3]) return 0;  //����־
    
    len = temp.byte[1]*256+temp.byte[2];    //���ݳ���
    if( len > data_len ) len = data_len;
    
    for(i=0; i<len; i+=4)
    {
        if( (len-i) >= 4 )
        {
            *(uint32_t *)(data+i) = dev_word_read(s_flash.data_addr+i);
        }
        else //������һ�β���4�ֽ�
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

//��ʼ��,��λ������λ��
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
    //���û���ҵ��洢���ݣ���������ҳ�����������ݣ��Ѿ�������ҳ�ٲ�����Ӱ������
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
