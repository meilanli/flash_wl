#ifndef __FLASH_WL_H
#define __FLASH_WL_H
#include "stdint.h"

typedef enum
{
    FWL_NO_ERROR = 0,
    //FWL_ERROR = -1,             //����
    //FWL_PAR_ERR = -2,           //��������
    FWL_LONG_ERR = -3,          //����̫��
    FWL_ERASE_ERR = -4,         //����ʧ��
    FWL_WRITE_ERR = -5,         //д��ʧ��
    //FWL_DATA_ERR = -6,          //���ݴ���
    FWL_CHECK_ERR = -7,         //д������ݶ���������
    //FWL_ADDR_ERR = -8,          //��ַ����
}FlashWlStatusFlag;

//д����
FlashWlStatusFlag flash_wl_write(uint8_t *data, uint32_t data_len);

//������
//���ض������ݳ���
//û�����ݷ���0
uint32_t flash_wl_read(uint8_t *data, uint32_t data_len);  

//��ʼ��,��λ�����һ������λ��
void flash_wl_init(void);   

#endif
