/* -------------------------------------------------------------------------------------------------------------------------------------- */
/* AUTO-GENERATED FILE. Source: BD.xlsx/Algorithms                                                                                        */
/* Do not edit manually.                                                                                                                  */
/* -------------------------------------------------------------------------------------------------------------------------------------- */

#pragma once                                                                                                                                

#include "alg_list.hpp"                                                                                                                     

struct S_algtab{                                                                                                                            
	const char*	key;                	/** key                                                                         идентификатор алгоритма */
	const char*	encode_expr;        	/** encode_expr                                                                          формула encode */
	const char*	decode_expr;        	/** decode_expr                                                                          формула decode */
	const char*	condition_expr;     	/** condition_expr                                                                 условие применимости */
};                                                                                                                                          

inline S_algtab S_basealgtab[ALG_max]={                                                                                                     
	{"NONE", "", "", ""},                                      	/** NONE                                                            reserved */
	{"AN", "q = x", "x = q", ""},                              	/** "AN"                                                             "x = q" */
	{"AC", "q = x - C", "x = q + C", ""},                      	/** "AC"                                                         "x = q + C" */
	{"ABC", "q = (x -C) / B", "x = B*q + C", "B != 0"},        	/** "ABC"                                                      "x = B*q + C" */
	{"AAC", "q = sqrt((x - C) / A)", "x = A*q^2 + C", "A != 0"}	/** "AAC"                                                    "x = A*q^2 + C" */
};                                                                                                                                          
