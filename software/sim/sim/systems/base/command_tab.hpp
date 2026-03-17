/* -------------------------------------------------------------------------------------------------------------------------------------- */
/* AUTO-GENERATED FILE. Source: BD.xlsx                                                                                                   */
/* Do not edit manually.                                                                                                                  */
/* -------------------------------------------------------------------------------------------------------------------------------------- */

#pragma once                                                                                                                                
#include <stdint.h>                                                                                                                         

#include "sys_list.hpp"                                                                                                                     
#include "type_list.hpp"                                                                                                                    
#include "alg_list.hpp"                                                                                                                     
#include "param_comm_list.hpp"                                                                                                              

#define	TAR_NONE	0                                                                                                                          

struct S_commtab{                                                                                                                           
	char      	sys_id;             	/** sys_id                                                                                    id системы */
	char      	type;               	/** type                                                                                      тип данных */
	char      	sign;               	/** sign                                                                         TYPE_SIGN / TYPE_UNSIGN */
	char      	tar_a;              	/** tar_a                                                                                    тарировка A */
	char      	tar_b;              	/** tar_b                                                                                    тарировка B */
	char      	tar_c;              	/** tar_c                                                                                    тарировка C */
	uint16_t  	msg_offset;         	/** msg_offset                                                           глобальное смещение в сообщении */
	char      	msg_block_offset;   	/** msg_block_offset                                                             смещение в блоке данных */
	char      	msg_block_n;        	/** msg_block_n                                                                       номер блока данных */
	uint16_t  	msg_cs_offset;      	/** msg_cs_offset                                                          глобальное смещение внутри CS */
	char      	msg_cs_block_n;     	/** msg_cs_block_n                                                                 номер блока внутри CS */
	const char*	key;                	/** key                                                                           идентификатор команды */
	char      	alg;                	/** alg                                                                               алгоритм обработки */
};                                                                                                                                          

inline S_commtab S_basecommtab[Comm_max]={                                                                                                  
	{SYS_PSS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 0, 0, 0, 0, 0, "CPSSINIT", ALG_AN},         	/** "CPSSINIT"                 "запуск PSS системы" */
	{SYS_PSS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 1, 1, 0, 1, 0, "CPSSPWRON", ALG_AN},        	/** "CPSSPWRON"                           "PSS вкл" */
	{SYS_PSS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 2, 2, 0, 2, 0, "CPSSPWROFF", ALG_AN},       	/** "CPSSPWROFF"                         "PSS выкл" */
	{SYS_PSS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 3, 3, 0, 3, 0, "CPSSPWR12ON", ALG_AN},      	/** "CPSSPWR12ON"         "питание контура 12V вкл" */
	{SYS_PSS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 4, 4, 0, 4, 0, "CPSSPWR12OFF", ALG_AN},     	/** "CPSSPWR12OFF"       "питание контура 12V выкл" */
	{SYS_PSS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 5, 5, 0, 5, 0, "CPSSPWR6ON", ALG_AN},       	/** "CPSSPWR6ON"           "питание контура 6V вкл" */
	{SYS_PSS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 6, 6, 0, 6, 0, "CPSSPWR6OFF", ALG_AN},      	/** "CPSSPWR6OFF"         "питание контура 6V выкл" */
	{SYS_PSS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 7, 7, 0, 7, 0, "CPSSPWR5ON", ALG_AN},       	/** "CPSSPWR5ON"           "питание контура 5V вкл" */
	{SYS_PSS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 8, 8, 0, 8, 0, "CPSSPWR5OFF", ALG_AN},      	/** "CPSSPWR5OFF"         "питание контура 5V выкл" */
	{SYS_PSS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 9, 9, 0, 9, 0, "CPSSPWR33ON", ALG_AN},      	/** "CPSSPWR33ON"        "питание контура 3.3V вкл" */
	{SYS_PSS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 10, 10, 0, 10, 0, "CPSSPWR33OFF", ALG_AN},  	/** "CPSSPWR33OFF"      "питание контура 3.3V выкл" */
	{SYS_PSS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 11, 11, 0, 11, 0, "CPSSECOON", ALG_AN},     	/** "CPSSECOON"        "режим экономии энергии вкл" */
	{SYS_PSS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 12, 12, 0, 12, 0, "CPSSECOOFF", ALG_AN},    	/** "CPSSECOOFF"      "режим экономии энергии выкл" */
	{SYS_PSS, TYPE_AP, TYPE_SIGN, 0, 1, 0, 16, 0, 1, 16, 1, "CPSSMAXT1", ALG_AN},       	/** "CPSSMAXT1"        "макс температура контура 1" */
	{SYS_PSS, TYPE_AP, TYPE_SIGN, 0, 1, 0, 32, 0, 2, 32, 2, "CPSSMAXT2", ALG_AN},       	/** "CPSSMAXT2"        "макс температура контура 2" */
	{SYS_PSS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 48, 0, 3, 48, 3, "CPSSMAXA", ALG_AN},       	/** "CPSSMAXA"           "макс сила тока A батареи" */
	{SYS_PSS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 56, 8, 3, 56, 3, "CPSSMAXW", ALG_AN},       	/** "CPSSMAXW"            "макс мощность W батареи" */
	{SYS_PSS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 64, 0, 4, 64, 4, "CPSSMINC", ALG_AN},       	/** "CPSSMINC"                "мин заряд C батареи" */
	{SYS_PSS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 72, 8, 4, 72, 4, "CPSSMINV", ALG_AN},       	/** "CPSSMINV"           "мин напряжение V батареи" */
	{SYS_PSS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 80, 0, 5, 80, 5, "CPSSMAXA12", ALG_AN},     	/** "CPSSMAXA12"     "макс сила тока A контура 12V" */
	{SYS_PSS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 88, 8, 5, 88, 5, "CPSSMAXA6", ALG_AN},      	/** "CPSSMAXA6"       "макс сила тока A контура 6V" */
	{SYS_PSS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 96, 0, 6, 96, 6, "CPSSMAXA5", ALG_AN},      	/** "CPSSMAXA5"       "макс сила тока A контура 5V" */
	{SYS_PSS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 104, 8, 6, 104, 6, "CPSSMAXA33", ALG_AN},   	/** "CPSSMAXA33"    "макс сила тока A контура 3.3V" */
	{SYS_PSS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 112, 0, 7, 112, 7, "CPSSCLOCK", ALG_AN},    	/** "CPSSCLOCK"                "задать счетчик PSS" */
	{SYS_TCS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 0, 0, 0, 128, 8, "CTCSINIT", ALG_AN},       	/** "CTCSINIT"                 "запуск TCS системы" */
	{SYS_TCS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 1, 1, 0, 129, 8, "CTCSPWRON", ALG_AN},      	/** "CTCSPWRON"                           "TCS вкл" */
	{SYS_TCS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 2, 2, 0, 130, 8, "CTCSPWROFF", ALG_AN},     	/** "CTCSPWROFF"                         "TCS выкл" */
	{SYS_TCS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 3, 3, 0, 131, 8, "CTCSPSSON", ALG_AN},      	/** "CTCSPSSON"                     "опрос PSS вкл" */
	{SYS_TCS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 4, 4, 0, 132, 8, "CTCSPSSOFF", ALG_AN},     	/** "CTCSPSSOFF"                   "опрос PSS выкл" */
	{SYS_TCS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 5, 5, 0, 133, 8, "CTCSTMSON", ALG_AN},      	/** "CTCSTMSON"                     "опрос TMS вкл" */
	{SYS_TCS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 6, 6, 0, 134, 8, "CTCSTMSOFF", ALG_AN},     	/** "CTCSTMSOFF"                   "опрос TMS выкл" */
	{SYS_TCS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 7, 7, 0, 135, 8, "CTCSMNSON", ALG_AN},      	/** "CTCSMNSON"                     "опрос MNS вкл" */
	{SYS_TCS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 8, 8, 0, 136, 8, "CTCSMNSOFF", ALG_AN},     	/** "CTCSMNSOFF"                   "опрос MNS выкл" */
	{SYS_TCS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 9, 9, 0, 137, 8, "CTCSLSON", ALG_AN},       	/** "CTCSLSON"                       "опрос LS вкл" */
	{SYS_TCS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 10, 10, 0, 138, 8, "CTCSLSOFF", ALG_AN},    	/** "CTCSLSOFF"                     "опрос LS выкл" */
	{SYS_TCS, TYPE_AP, TYPE_UNSIGN, 0, 1, 0, 16, 0, 1, 144, 9, "CTCSFRQ", ALG_AN},      	/** "CTCSFRQ"       "задать скорость опроса систем" */
	{SYS_TCS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 32, 0, 2, 160, 10, "CTCSCLOCK", ALG_AN},    	/** "CTCSCLOCK"                "задать счетчик TCS" */
	{SYS_TMS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 0, 0, 0, 176, 11, "CTMSINIT", ALG_AN},      	/** "CTMSINIT"                 "запуск TMS системы" */
	{SYS_TMS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 1, 1, 0, 177, 11, "CTMSPWRON", ALG_AN},     	/** "CTMSPWRON"                           "TMS вкл" */
	{SYS_TMS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 2, 2, 0, 178, 11, "CTMSPWROFF", ALG_AN},    	/** "CTMSPWROFF"                         "TMS выкл" */
	{SYS_TMS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 8, 8, 0, 184, 11, "CTMSCLOCK", ALG_AN},     	/** "CTMSCLOCK"                "задать счетчик TMS" */
	{SYS_TMS, TYPE_A, TYPE_UNSIGN, 0, 1, -10, 16, 0, 1, 192, 12, "CTMSPWRFAN1", ALG_AC},	/** "CTMSPWRFAN1"                "питание кулера 1" */
	{SYS_TMS, TYPE_A, TYPE_UNSIGN, 0, 1, -10, 24, 8, 1, 200, 12, "CTMSPWRFAN2", ALG_AC},	/** "CTMSPWRFAN2"                "питание кулера 2" */
	{SYS_TMS, TYPE_A, TYPE_UNSIGN, 0, 1, -10, 32, 0, 2, 208, 13, "CTMSPWRFAN3", ALG_AC},	/** "CTMSPWRFAN3"                "питание кулера 3" */
	{SYS_TMS, TYPE_A, TYPE_UNSIGN, 0, 1, -10, 40, 8, 2, 216, 13, "CTMSPWRFAN4", ALG_AC},	/** "CTMSPWRFAN4"                "питание кулера 4" */
	{SYS_MNS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 0, 0, 0, 224, 14, "CMNSINIT", ALG_AN},      	/** "CMNSINIT"                 "запуск TMS системы" */
	{SYS_MNS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 1, 1, 0, 225, 14, "CMNSPWRON", ALG_AN},     	/** "CMNSPWRON"                           "TMS вкл" */
	{SYS_MNS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 2, 2, 0, 226, 14, "CMNSPWROFF", ALG_AN},    	/** "CMNSPWROFF"                         "TMS выкл" */
	{SYS_MNS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 8, 8, 0, 232, 14, "CMNSCLOCK", ALG_AN},     	/** "CMNSCLOCK"                "задать счетчик MNS" */
	{SYS_LS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 0, 0, 0, 240, 15, "CLSINIT", ALG_AN},        	/** "CLSINIT"                    "запуск LSсистемы" */
	{SYS_LS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 1, 1, 0, 241, 15, "CLSPWRON", ALG_AN},       	/** "CLSPWRON"                     "питание LS ыкл" */
	{SYS_LS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 2, 2, 0, 242, 15, "CLSPWROFF", ALG_AN},      	/** "CLSPWROFF"                   "питание ls выкл" */
	{SYS_LS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 8, 8, 0, 248, 15, "CLSCLOCK", ALG_AN}        	/** "CLSCLOCK"                  "задать счетчик LS" */
};                                                                                                                                          
