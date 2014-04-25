#ifndef POKER_HEAVY_DOT_H
#define POKER_HEAVY_DOT_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>


#define RANK_TABLE_SIZE 7825760
#define FLUSH_TABLE_SIZE (0x7f << 6) + 1
#define NUM_FLUSHES 4421
#define NUM_SFS 298
#define NUM_RANK_COMBOS 49205

#define SPECIALKS {0, 1, 5, 22, 98, 453, 2031, 8698, 22854, 83661, 262349, 636345, 1479181}
//DECK = [r | (s << SUITSHIFT) for r in SPECIALKS for s in (0, 1, 8, 57)]
#define DECK {0,8388608,67108864,478150656,1,8388609,67108865,478150657,5,8388613,67108869,478150661,22,8388630,67108886,478150678,98,8388706,67108962,478150754,453,8389061,67109317,478151109,2031,8390639,67110895,478152687,8698,8397306,67117562,478159354,22854,8411462,67131718,478173510,83661,8472269,67192525,478234317,262349,8650957,67371213,478413005,636345,9024953,67745209,478787001,1479181,9867789,68588045,479629837}

#define FULL (uint64_t) 6 << 52

#define SUITSHIFT 23
#define RANKMASK 0x7fffff
#define CARD_MASK (uint64_t) 0x1fff

#define NUM_STARTING_HANDS 1326
#define MAX_HANDS 22

#define FAIL -1
#define SUCCESS 1

#define GET_RANK(c) (1 << (c >> 2))
#define GET_SUIT(c) ((c % 4) * 13)


struct rivervalue{
    int ties;
    int wins;
};

typedef struct{
    uint32_t val;
    uint32_t *board;
} partial;


typedef
struct{
    int hand[2];
    int value;
}dictEntry;


int holdem2p(uint32_t h1[2], uint32_t h2[2], uint32_t board[5]);
int multi_holdem(uint32_t [MAX_HANDS][2], int, uint32_t [5], int []);
uint64_t handvalue(uint32_t hand[7]);
struct rivervalue rivervalue (uint32_t hand[2], uint32_t board[5]);
double enum2p(uint32_t h1[2], uint32_t h2[2]);
int full_enumeration(uint32_t [MAX_HANDS][2], double [], int );
int river_distribution (uint32_t hand[2], uint32_t board[5], int chart[], dictEntry *dict);
void populate_tables(uint16_t ranktable[RANK_TABLE_SIZE],
                     uint16_t flushtable[FLUSH_TABLE_SIZE],
                     const uint16_t straighttable[FLUSH_TABLE_SIZE]);

#endif
