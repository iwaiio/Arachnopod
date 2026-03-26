/* -------------------------------------------------------------------------------------------------------------------------------------- */
/* AUTO-GENERATED FILE. Source: BD.xlsx                                                                                                   */
/* Do not edit manually.                                                                                                                  */
/* -------------------------------------------------------------------------------------------------------------------------------------- */

#pragma once                                                                                                                                
#include <stdint.h>                                                                                                                         

#include "sys_list.hpp"                                                                                                                     
#include "type_list.hpp"                                                                                                                    
#include "real_type_list.hpp"                                                                                                               
#include "alg_list.hpp"                                                                                                                     
#include "param_comm_list.hpp"                                                                                                              

#define	TAR_NONE	0                                                                                                                          

struct S_paramtab{                                                                                                                          
	char      	sys_id;             	/** sys_id                                                                                    id системы */
	char      	type;               	/** type                                                                                      тип данных */
	char      	type_real;          	/** type_real                                                                             физический тип */
	char      	sign;               	/** sign                                                                         TYPE_SIGN / TYPE_UNSIGN */
	double    	tar_a;              	/** tar_a                                                                                    тарировка A */
	double    	tar_b;              	/** tar_b                                                                                    тарировка B */
	double    	tar_c;              	/** tar_c                                                                                    тарировка C */
	uint16_t  	msg_offset;         	/** msg_offset                                                           глобальное смещение в сообщении */
	char      	msg_block_offset;   	/** msg_block_offset                                                             смещение в блоке данных */
	char      	msg_block_n;        	/** msg_block_n                                                                       номер блока данных */
	uint16_t  	msg_cs_offset;      	/** msg_cs_offset                                                          глобальное смещение внутри CS */
	char      	msg_cs_block_n;     	/** msg_cs_block_n                                                                 номер блока внутри CS */
	const char*	key;                	/** key                                                                         идентификатор параметра */
	char      	alg;                	/** alg                                                                               алгоритм обработки */
};                                                                                                                                          

inline S_paramtab S_baseparamtab[Param_max]={                                                                                               
	{SYS_PSS, TYPE_D, RTYPE_BOOL, TYPE_UNSIGN, 0, 1, 0, 0, 0, 0, 0, 0, "PSSPWR", ALG_AN},           	/** "PSSPWR"             "PSS вкл/выкл" */
	{SYS_PSS, TYPE_D, RTYPE_BOOL, TYPE_UNSIGN, 0, 1, 0, 1, 1, 0, 1, 0, "PSSPWR12", ALG_AN},         	/** "PSSPWR12"    "питание контура 12V" */
	{SYS_PSS, TYPE_D, RTYPE_BOOL, TYPE_UNSIGN, 0, 1, 0, 2, 2, 0, 2, 0, "PSSPWR6", ALG_AN},          	/** "PSSPWR6"      "питание контура 6V" */
	{SYS_PSS, TYPE_D, RTYPE_BOOL, TYPE_UNSIGN, 0, 1, 0, 3, 3, 0, 3, 0, "PSSPWR5", ALG_AN},          	/** "PSSPWR5"      "питание контура 5V" */
	{SYS_PSS, TYPE_D, RTYPE_BOOL, TYPE_UNSIGN, 0, 1, 0, 4, 4, 0, 4, 0, "PSSPWR33", ALG_AN},         	/** "PSSPWR33"   "питание контура 3.3V" */
	{SYS_PSS, TYPE_D, RTYPE_BOOL, TYPE_UNSIGN, 0, 1, 0, 5, 5, 0, 5, 0, "PSSECO", ALG_AN},           	/** "PSSECO"   "режим экономии энергии" */
	{SYS_PSS, TYPE_A, RTYPE_INT8, TYPE_UNSIGN, 0, 1, 0, 8, 8, 0, 8, 0, "PSSCLOCK", ALG_AN},         	/** "PSSCLOCK"            "счетчик PSS" */
	{SYS_PSS, TYPE_AP, RTYPE_FLOAT32, TYPE_SIGN, 0, 1, 0, 16, 0, 1, 16, 1, "PSSMAXT1", ALG_ABC},    	/** "PSSMAXT1" "макс температура контур */
	{SYS_PSS, TYPE_AP, RTYPE_FLOAT32, TYPE_SIGN, 0, 1, 0, 32, 0, 2, 32, 2, "PSSMAXT2", ALG_ABC},    	/** "PSSMAXT2" "макс температура контур */
	{SYS_PSS, TYPE_A, RTYPE_FLOAT32, TYPE_UNSIGN, 0, 1, 0, 48, 0, 3, 48, 3, "PSSMAXA", ALG_ABC},    	/** "PSSMAXA" "макс сила тока A батареи */
	{SYS_PSS, TYPE_A, RTYPE_FLOAT32, TYPE_UNSIGN, 0, 1, 0, 56, 8, 3, 56, 3, "PSSMAXW", ALG_ABC},    	/** "PSSMAXW" "макс мощность W батареи" */
	{SYS_PSS, TYPE_A, RTYPE_INT8, TYPE_UNSIGN, 0, 1, 0, 64, 0, 4, 64, 4, "PSSMINC", ALG_AN},        	/** "PSSMINC"     "мин заряд C батареи" */
	{SYS_PSS, TYPE_A, RTYPE_FLOAT32, TYPE_UNSIGN, 0, 1, 0, 72, 8, 4, 72, 4, "PSSMINV", ALG_ABC},    	/** "PSSMINV" "мин напряжение V батареи */
	{SYS_PSS, TYPE_A, RTYPE_FLOAT32, TYPE_UNSIGN, 0, 1, 0, 80, 0, 5, 80, 5, "PSSMAXA12", ALG_ABC},  	/** "PSSMAXA12" "макс сила тока A конту */
	{SYS_PSS, TYPE_A, RTYPE_FLOAT32, TYPE_UNSIGN, 0, 1, 0, 88, 8, 5, 88, 5, "PSSMAXA6", ALG_ABC},   	/** "PSSMAXA6" "макс сила тока A контур */
	{SYS_PSS, TYPE_A, RTYPE_FLOAT32, TYPE_UNSIGN, 0, 1, 0, 96, 0, 6, 96, 6, "PSSMAXA5", ALG_ABC},   	/** "PSSMAXA5" "макс сила тока A контур */
	{SYS_PSS, TYPE_A, RTYPE_FLOAT32, TYPE_UNSIGN, 0, 1, 0, 104, 8, 6, 104, 6, "PSSMAXA33", ALG_ABC},	/** "PSSMAXA33" "макс сила тока A конту */
	{SYS_PSS, TYPE_AP, RTYPE_FLOAT32, TYPE_SIGN, 0, 1, 0, 112, 0, 7, 112, 7, "PSST1", ALG_ABC},     	/** "PSST1"     "температура контура 1" */
	{SYS_PSS, TYPE_AP, RTYPE_FLOAT32, TYPE_SIGN, 0, 1, 0, 128, 0, 8, 128, 8, "PSST2", ALG_ABC},     	/** "PSST2"     "температура контура 2" */
	{SYS_PSS, TYPE_A, RTYPE_FLOAT32, TYPE_UNSIGN, 0, 1, 0, 144, 0, 9, 144, 9, "PSSA", ALG_ABC},     	/** "PSSA"        "сила тока A батареи" */
	{SYS_PSS, TYPE_A, RTYPE_FLOAT32, TYPE_UNSIGN, 0, 1, 0, 152, 8, 9, 152, 9, "PSSW", ALG_ABC},     	/** "PSSW"         "мощность W батареи" */
	{SYS_PSS, TYPE_A, RTYPE_FLOAT32, TYPE_UNSIGN, 0, 1, 0, 160, 0, 10, 160, 10, "PSSV", ALG_ABC},   	/** "PSSV"       "напряжение V батареи" */
	{SYS_PSS, TYPE_A, RTYPE_INT8, TYPE_UNSIGN, 0, 1, 0, 168, 8, 10, 168, 10, "PSSC", ALG_AN},       	/** "PSSC"            "заряд C батареи" */
	{SYS_PSS, TYPE_A, RTYPE_FLOAT32, TYPE_UNSIGN, 0, 1, 0, 176, 0, 11, 176, 11, "PSSA12", ALG_ABC}, 	/** "PSSA12"    "сила тока контура 12V" */
	{SYS_PSS, TYPE_A, RTYPE_FLOAT32, TYPE_UNSIGN, 0, 1, 0, 184, 8, 11, 184, 11, "PSSV12", ALG_ABC}, 	/** "PSSV12"   "напряжение контура 12V" */
	{SYS_PSS, TYPE_A, RTYPE_FLOAT32, TYPE_UNSIGN, 0, 1, 0, 192, 0, 12, 192, 12, "PSSA6", ALG_ABC},  	/** "PSSA6"      "сила тока контура 6V" */
	{SYS_PSS, TYPE_A, RTYPE_FLOAT32, TYPE_UNSIGN, 0, 1, 0, 200, 8, 12, 200, 12, "PSSV6", ALG_ABC},  	/** "PSSV6"     "напряжение контура 6V" */
	{SYS_PSS, TYPE_A, RTYPE_FLOAT32, TYPE_UNSIGN, 0, 1, 0, 208, 0, 13, 208, 13, "PSSA5", ALG_ABC},  	/** "PSSA5"      "сила тока контура 5V" */
	{SYS_PSS, TYPE_A, RTYPE_FLOAT32, TYPE_UNSIGN, 0, 1, 0, 216, 8, 13, 216, 13, "PSSV5", ALG_ABC},  	/** "PSSV5"     "напряжение контура 5V" */
	{SYS_PSS, TYPE_A, RTYPE_FLOAT32, TYPE_UNSIGN, 0, 1, 0, 224, 0, 14, 224, 14, "PSSA33", ALG_ABC}, 	/** "PSSA33"   "сила тока контура 3.3V" */
	{SYS_PSS, TYPE_A, RTYPE_FLOAT32, TYPE_UNSIGN, 0, 1, 0, 232, 8, 14, 232, 14, "PSSV33", ALG_ABC}, 	/** "PSSV33"  "напряжение контура 3.3V" */
	{SYS_TCS, TYPE_D, RTYPE_BOOL, TYPE_UNSIGN, 0, 1, 0, 0, 0, 0, 240, 15, "TCSPWR", ALG_AN},        	/** "TCSPWR"             "TCS вкл/выкл" */
	{SYS_TCS, TYPE_D, RTYPE_BOOL, TYPE_UNSIGN, 0, 1, 0, 1, 1, 0, 241, 15, "TCSPSS", ALG_AN},        	/** "TCSPSS"                "опрос PSS" */
	{SYS_TCS, TYPE_D, RTYPE_BOOL, TYPE_UNSIGN, 0, 1, 0, 2, 2, 0, 242, 15, "TCSTMS", ALG_AN},        	/** "TCSTMS"                "опрос TMS" */
	{SYS_TCS, TYPE_D, RTYPE_BOOL, TYPE_UNSIGN, 0, 1, 0, 3, 3, 0, 243, 15, "TCSMNS", ALG_AN},        	/** "TCSMNS"                "опрос MNS" */
	{SYS_TCS, TYPE_D, RTYPE_BOOL, TYPE_UNSIGN, 0, 1, 0, 4, 4, 0, 244, 15, "TCSLS", ALG_AN},         	/** "TCSLS"                  "опрос LS" */
	{SYS_TCS, TYPE_A, RTYPE_INT8, TYPE_UNSIGN, 0, 1, 0, 8, 8, 0, 248, 15, "TCSCLOCK", ALG_AN},      	/** "TCSCLOCK"            "счетчик TCS" */
	{SYS_TCS, TYPE_AP, RTYPE_INT16, TYPE_UNSIGN, 0, 1, 0, 16, 0, 1, 256, 16, "TCSFRQ", ALG_AN},     	/** "TCSFRQ"   "скорость опроса систем" */
	{SYS_TCS, TYPE_AP, RTYPE_FLOAT32, TYPE_SIGN, 0, 1, 0, 32, 0, 2, 272, 17, "TCST", ALG_ABC},      	/** "TCST"            "температура TCS" */
	{SYS_TMS, TYPE_D, RTYPE_BOOL, TYPE_UNSIGN, 0, 1, 0, 0, 0, 0, 288, 18, "TMSPWR", ALG_AN},        	/** "TMSPWR"             "TMS вкл/выкл" */
	{SYS_TMS, TYPE_A, RTYPE_INT8, TYPE_UNSIGN, 0, 1, 0, 8, 8, 0, 296, 18, "TMSCLOCK", ALG_AN},      	/** "TMSCLOCK"            "счетчик TMS" */
	{SYS_TMS, TYPE_A, RTYPE_INT8, TYPE_UNSIGN, 0, 1, 0, 16, 0, 1, 304, 19, "TMSPWRFAN1", ALG_AC},   	/** "TMSPWRFAN1"     "питание кулера 1" */
	{SYS_TMS, TYPE_A, RTYPE_INT8, TYPE_UNSIGN, 0, 1, 0, 24, 8, 1, 312, 19, "TMSPWRFAN2", ALG_AC},   	/** "TMSPWRFAN2"     "питание кулера 2" */
	{SYS_TMS, TYPE_A, RTYPE_INT8, TYPE_UNSIGN, 0, 1, 0, 32, 0, 2, 320, 20, "TMSPWRFAN3", ALG_AC},   	/** "TMSPWRFAN3"     "питание кулера 3" */
	{SYS_TMS, TYPE_A, RTYPE_INT8, TYPE_UNSIGN, 0, 1, 0, 40, 8, 2, 328, 20, "TMSPWRFAN4", ALG_AC},   	/** "TMSPWRFAN4"     "питание кулера 4" */
	{SYS_TMS, TYPE_AP, RTYPE_FLOAT32, TYPE_SIGN, 0, 1, 0, 48, 0, 3, 336, 21, "TMST", ALG_ABC},      	/** "TMST"            "температура TMS" */
	{SYS_MNS, TYPE_D, RTYPE_BOOL, TYPE_UNSIGN, 0, 1, 0, 0, 0, 0, 352, 22, "MNSPWR", ALG_AN},        	/** "MNSPWR"             "TMS вкл/выкл" */
	{SYS_MNS, TYPE_A, RTYPE_INT8, TYPE_UNSIGN, 0, 1, 0, 8, 8, 0, 360, 22, "MNSCLOCK", ALG_AN},      	/** "MNSCLOCK"            "счетчик MNS" */
	{SYS_MNS, TYPE_AP, RTYPE_FLOAT32, TYPE_SIGN, 0, 1, 0, 16, 0, 1, 368, 23, "MNST", ALG_ABC},      	/** "MNST"            "температура MNS" */
	{SYS_LS, TYPE_D, RTYPE_BOOL, TYPE_UNSIGN, 0, 1, 0, 0, 0, 0, 384, 24, "LSPWR", ALG_AN},          	/** "LSPWR"               "LS вкл/выкл" */
	{SYS_LS, TYPE_A, RTYPE_INT8, TYPE_UNSIGN, 0, 1, 0, 8, 8, 0, 392, 24, "LSCLOCK", ALG_AN},        	/** "LSCLOCK"              "счетчик LS" */
	{SYS_LS, TYPE_AP, RTYPE_FLOAT32, TYPE_SIGN, 0, 1, 0, 16, 0, 1, 400, 25, "LST", ALG_ABC}         	/** "LST"              "температура LS" */
};                                                                                                                                          
