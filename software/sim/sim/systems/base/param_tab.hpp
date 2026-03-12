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

struct S_paramtab{                                                                                                                          
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
	char      	alg;                	/** alg                                                                               алгоритм обработки */
};                                                                                                                                          

struct S_paramtab S_baseparamtab[Param_max]={                                                                                               
	{SYS_PSS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 0, 0, 0, 0, 0, ALG_AN},      	/** "PSSPWR"                                        "PSS вкл/выкл" */
	{SYS_PSS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 1, 1, 0, 1, 0, ALG_AN},      	/** "PSSPWR12"                               "питание контура 12V" */
	{SYS_PSS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 2, 2, 0, 2, 0, ALG_AN},      	/** "PSSPWR6"                                 "питание контура 6V" */
	{SYS_PSS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 3, 3, 0, 3, 0, ALG_AN},      	/** "PSSPWR5"                                 "питание контура 5V" */
	{SYS_PSS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 4, 4, 0, 4, 0, ALG_AN},      	/** "PSSPWR33"                              "питание контура 3.3V" */
	{SYS_PSS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 5, 5, 0, 5, 0, ALG_AN},      	/** "PSSECO"                              "режим экономии энергии" */
	{SYS_PSS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 8, 8, 0, 8, 0, ALG_AN},      	/** "PSSCLOCK"                                       "счетчик PSS" */
	{SYS_PSS, TYPE_AP, TYPE_SIGN, 0, 1, 0, 16, 0, 1, 16, 1, ALG_AN},     	/** "PSSMAXT1"                        "макс температура контура 1" */
	{SYS_PSS, TYPE_AP, TYPE_SIGN, 0, 1, 0, 32, 0, 2, 32, 2, ALG_AN},     	/** "PSSMAXT2"                        "макс температура контура 2" */
	{SYS_PSS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 48, 0, 3, 48, 3, ALG_AN},    	/** "PSSMAXA"                           "макс сила тока A батареи" */
	{SYS_PSS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 56, 8, 3, 56, 3, ALG_AN},    	/** "PSSMAXW"                            "макс мощность W батареи" */
	{SYS_PSS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 64, 0, 4, 64, 4, ALG_AN},    	/** "PSSMINC"                                "мин заряд C батареи" */
	{SYS_PSS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 72, 8, 4, 72, 4, ALG_AN},    	/** "PSSMINV"                           "мин напряжение V батареи" */
	{SYS_PSS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 80, 0, 5, 80, 5, ALG_AN},    	/** "PSSMAXA12"                     "макс сила тока A контура 12V" */
	{SYS_PSS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 88, 8, 5, 88, 5, ALG_AN},    	/** "PSSMAXA6"                       "макс сила тока A контура 6V" */
	{SYS_PSS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 96, 0, 6, 96, 6, ALG_AN},    	/** "PSSMAXA5"                       "макс сила тока A контура 5V" */
	{SYS_PSS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 104, 8, 6, 104, 6, ALG_AN},  	/** "PSSMAXA33"                    "макс сила тока A контура 3.3V" */
	{SYS_PSS, TYPE_AP, TYPE_SIGN, 0, 1, 0, 112, 0, 7, 112, 7, ALG_AN},   	/** "PSST1"                                "температура контура 1" */
	{SYS_PSS, TYPE_AP, TYPE_SIGN, 0, 1, 0, 128, 0, 8, 128, 8, ALG_AN},   	/** "PSST2"                                "температура контура 2" */
	{SYS_PSS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 144, 0, 9, 144, 9, ALG_AN},  	/** "PSSA"                                   "сила тока A батареи" */
	{SYS_PSS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 152, 8, 9, 152, 9, ALG_AN},  	/** "PSSW"                                    "мощность W батареи" */
	{SYS_PSS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 160, 0, 10, 160, 10, ALG_AN},	/** "PSSV"                                  "напряжение V батареи" */
	{SYS_PSS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 168, 8, 10, 168, 10, ALG_AN},	/** "PSSC"                                       "заряд C батареи" */
	{SYS_PSS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 176, 0, 11, 176, 11, ALG_AN},	/** "PSSA12"                               "сила тока контура 12V" */
	{SYS_PSS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 184, 8, 11, 184, 11, ALG_AN},	/** "PSSV12"                              "напряжение контура 12V" */
	{SYS_PSS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 192, 0, 12, 192, 12, ALG_AN},	/** "PSSA6"                                 "сила тока контура 6V" */
	{SYS_PSS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 200, 8, 12, 200, 12, ALG_AN},	/** "PSSV6"                                "напряжение контура 6V" */
	{SYS_PSS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 208, 0, 13, 208, 13, ALG_AN},	/** "PSSA5"                                 "сила тока контура 5V" */
	{SYS_PSS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 216, 8, 13, 216, 13, ALG_AN},	/** "PSSV5"                                "напряжение контура 5V" */
	{SYS_PSS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 224, 0, 14, 224, 14, ALG_AN},	/** "PSSA33"                              "сила тока контура 3.3V" */
	{SYS_PSS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 232, 8, 14, 232, 14, ALG_AN},	/** "PSSV33"                             "напряжение контура 3.3V" */
	{SYS_TCS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 0, 0, 0, 240, 15, ALG_AN},   	/** "TCSPWR"                                        "TCS вкл/выкл" */
	{SYS_TCS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 1, 1, 0, 241, 15, ALG_AN},   	/** "TCSPSS"                                           "опрос PSS" */
	{SYS_TCS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 2, 2, 0, 242, 15, ALG_AN},   	/** "TCSTMS"                                           "опрос TMS" */
	{SYS_TCS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 3, 3, 0, 243, 15, ALG_AN},   	/** "TCSMNS"                                           "опрос MNS" */
	{SYS_TCS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 4, 4, 0, 244, 15, ALG_AN},   	/** "TCSLS"                                             "опрос LS" */
	{SYS_TCS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 8, 8, 0, 248, 15, ALG_AN},   	/** "TCSCLOCK"                                       "счетчик TCS" */
	{SYS_TCS, TYPE_AP, TYPE_UNSIGN, 0, 1, 0, 16, 0, 1, 256, 16, ALG_AN}, 	/** "TCSFRQ"                              "скорость опроса систем" */
	{SYS_TCS, TYPE_AP, TYPE_SIGN, 0, 1, 0, 32, 0, 2, 272, 17, ALG_AN},   	/** "TCST"                                       "температура TCS" */
	{SYS_TMS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 0, 0, 0, 288, 18, ALG_AN},   	/** "TMSPWR"                                        "TMS вкл/выкл" */
	{SYS_TMS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 8, 8, 0, 296, 18, ALG_AN},   	/** "TMSCLOCK"                                       "счетчик TMS" */
	{SYS_TMS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 16, 0, 1, 304, 19, ALG_AC},  	/** "TMSPWRFAN1"                                "питание кулера 1" */
	{SYS_TMS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 24, 8, 1, 312, 19, ALG_AC},  	/** "TMSPWRFAN2"                                "питание кулера 2" */
	{SYS_TMS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 32, 0, 2, 320, 20, ALG_AC},  	/** "TMSPWRFAN3"                                "питание кулера 3" */
	{SYS_TMS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 40, 8, 2, 328, 20, ALG_AC},  	/** "TMSPWRFAN4"                                "питание кулера 4" */
	{SYS_TMS, TYPE_AP, TYPE_SIGN, 0, 1, 0, 48, 0, 3, 336, 21, ALG_AN},   	/** "TMST"                                       "температура TMS" */
	{SYS_MNS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 0, 0, 0, 352, 22, ALG_AN},   	/** "MNSPWR"                                        "TMS вкл/выкл" */
	{SYS_MNS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 8, 8, 0, 360, 22, ALG_AN},   	/** "MNSCLOCK"                                       "счетчик MNS" */
	{SYS_MNS, TYPE_AP, TYPE_SIGN, 0, 1, 0, 16, 0, 1, 368, 23, ALG_AN},   	/** "MNST"                                       "температура MNS" */
	{SYS_LS, TYPE_D, TYPE_UNSIGN, 0, 1, 0, 0, 0, 0, 384, 24, ALG_AN},    	/** "LSPWR"                                          "LS вкл/выкл" */
	{SYS_LS, TYPE_A, TYPE_UNSIGN, 0, 1, 0, 8, 8, 0, 392, 24, ALG_AN},    	/** "LSCLOCK"                                         "счетчик LS" */
	{SYS_LS, TYPE_AP, TYPE_SIGN, 0, 1, 0, 16, 0, 1, 400, 25, ALG_AN}     	/** "LST"                                         "температура LS" */
};                                                                                                                                          
