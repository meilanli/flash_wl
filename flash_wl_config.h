#ifndef __FLASH_WL_CONFIG_H
#define __FLASH_WL_CONFIG_H

#include "gd32e230.h"

//ҳ��С
#define FLASH_PAGE_SIZE         1024
//ƽ���д��ʼҳ��ʼ��ַ
#define FLASH_WL_START_ADDR     (0x08000000+FLASH_PAGE_SIZE*12)     
//ƽ���дʹ�õ�ҳ��
#define FLASH_WL_PAGE_NUM       2       
//д����Ƿ�������ݼ��
#define FLASH_WL_WRITE_CHECK    1   

//����д�����������־
#define DEV_UNLOCK()    do{ \
                            fmc_unlock(); \
                            fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGERR); \
                        }while(0)

//����д����
#define DEV_LOCK()    do{ \
                            fmc_lock(); \
                        }while(0)


//ҳ����������ҳ�׵�ַ��ʧ��1���ɹ�0
#define DEV_PAGE_ERASE(addr)    (( FMC_READY != fmc_page_erase(addr) )?1:0 )

//дһ���֡�ʧ��1���ɹ�0
#define DEV_WORD_WRITE(addr, word)      (( FMC_READY != fmc_word_program(addr, word) )?1:0 )

//��һ���֣�����word
#define DEV_WORD_READ(addr)     (*(uint32_t *)(addr))

#endif
