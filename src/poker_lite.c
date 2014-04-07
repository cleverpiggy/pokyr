// Copyright 2013 Allen Boyd Cunningham

// This file is part of pypoker-tools.

//     pypoker_tools is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//     pypoker_tools is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.

//     You should have received a copy of the GNU General Public License
//     along with pypoker_tools.  If not, see <http://www.gnu.org/licenses/>.


#include <stdint.h>
#include "cpokertables.h"

static uint64_t phase2(uint64_t val);

typedef struct{
    uint64_t val;
    uint64_t flush;
    uint_fast16_t straight;
    uint_fast16_t isflush;
}partial;


#define RANK_SHIFT 52
#define CARD_MASK (uint64_t) 0x1fff
#define FAIL -1

//expanation for Suits and Ranks tables
//#define GET_RANK(c) (1 << (c >> 2))
//#define GET_SUIT(c) ((c % 4) * 13)

#define GET_RANK(c) Ranks[c]
#define GET_SUIT(c) Suits[c]
#define GET_BIT(c) Bits[c]


#define SF (uint64_t) 8 << RANK_SHIFT
#define QUADS (uint64_t) 7 << RANK_SHIFT
#define FULL (uint64_t) 6 << RANK_SHIFT
#define FLUSH (uint64_t) 5 << RANK_SHIFT
#define STRAIGHT (uint64_t) 4 << RANK_SHIFT
#define TRIPS (uint64_t) 3 << RANK_SHIFT
#define TWOPAIR (uint64_t) 2 << RANK_SHIFT
#define PAIR (uint64_t) 1 << RANK_SHIFT


#define MIN_PAIR (uint64_t) 1 << 13
#define MIN_TRIPS (uint64_t) 1 << 26
#define MIN_QUADS (uint64_t) 1 << 39


//one of each 13 bits set for each rank with aces high
static const uint64_t Ranks[52] = {4096, 4096, 4096, 4096, 2048, 2048, 2048, 2048, 1024, 1024, 1024, 1024, 512, 512, 512, 512, 256, 256, 256, 256, 128, 128, 128, 128, 64, 64, 64, 64, 32, 32, 32, 32, 16, 16, 16, 16, 8, 8, 8, 8, 4, 4, 4, 4, 2, 2, 2, 2, 1, 1, 1, 1};

static const uint_fast16_t Suits[52] = {0, 1, 8, 57, 0, 1, 8, 57, 0, 1, 8, 57, 0, 1, 8, 57, 0, 1, 8, 57, 0, 1, 8, 57, 0, 1, 8, 57, 0, 1, 8, 57, 0, 1, 8, 57, 0, 1, 8, 57, 0, 1, 8, 57, 0, 1, 8, 57, 0, 1, 8, 57};

const uint16_t Straight_Table[8129] = STRAIGHT_TABLE;

const int8_t isFlushTable[400] = ISFLUSH;

//one of 52 bits set for each card in deck
const uint64_t Bits[52] = {4096ULL, 33554432ULL, 274877906944ULL, 2251799813685248ULL, 2048ULL, 16777216ULL, 137438953472ULL, 1125899906842624ULL, 1024ULL, 8388608ULL, 68719476736ULL, 562949953421312ULL, 512ULL, 4194304ULL, 34359738368ULL, 281474976710656ULL, 256ULL, 2097152ULL, 17179869184ULL, 140737488355328ULL, 128ULL, 1048576ULL, 8589934592ULL, 70368744177664ULL, 64ULL, 524288ULL, 4294967296ULL, 35184372088832ULL, 32ULL, 262144ULL, 2147483648ULL, 17592186044416ULL, 16ULL, 131072ULL, 1073741824ULL, 8796093022208ULL, 8ULL, 65536ULL, 536870912ULL, 4398046511104ULL, 4ULL, 32768ULL, 268435456ULL, 2199023255552ULL, 2ULL, 16384ULL, 134217728ULL, 1099511627776ULL, 1ULL, 8192ULL, 67108864ULL, 549755813888ULL};


//this will be updated in poker heavy, but will still be usable here
uint16_t Flush_Table[8129] = FLUSH_TABLE;


uint64_t handvalue(uint32_t hand[7]){
    uint64_t rank;
    uint32_t i;
    uint64_t val = 0;
    uint_fast16_t straight = 0;
    uint64_t flush = 0;
    uint_fast16_t isflush = 0;

    for (i = 0; i < 7; i++){
        isflush += GET_SUIT(hand[i]);
    }

    if (isFlushTable[isflush] != FAIL){

        for (i = 0; i < 7; i++)
            flush |= GET_BIT(hand[i]);

        flush >>= isFlushTable[isflush];
        flush &= CARD_MASK;
        return ( (Straight_Table[flush]) ? SF : FLUSH ) | Flush_Table[flush];
    }

    for (i = 0; i < 7; i++){
        rank = GET_RANK(hand[i]);
        straight |= rank;
        while (rank & val){
            rank <<= 13;
        }
        val |= rank;
    }

    if (Straight_Table[straight]){
        return STRAIGHT | Straight_Table[straight];
    }

    //reduce the less paired status of the more paired cards
    return phase2( val ^ (val >> 13) );
}



static uint64_t phase2(uint64_t val){

    static const unsigned short lowbits[] = LOWBITS;
    #define SINGLE_RANK(field)  ((field) == lowbits[(field)])

    uint64_t pairs, trips, kickers;

    if (val < MIN_PAIR){
        val ^= lowbits[val];
        val ^= lowbits[val];
        return val;
    }

    if (val < MIN_TRIPS){
        pairs = val >> 13;
        if (SINGLE_RANK(pairs)){//just a pair
            //get rid of the two lowest cards
            kickers = val & CARD_MASK;
            val ^= lowbits[kickers];
            kickers ^= lowbits[kickers];
            val ^= lowbits[kickers];
            return PAIR | val;
       }

       if ( SINGLE_RANK( pairs ^ lowbits[pairs] ) ){//just two pair
            kickers = val & CARD_MASK;
            val ^= lowbits[kickers];
            kickers ^= lowbits[kickers];
            val ^= lowbits[kickers];
            return TWOPAIR | val;
       }
       //three pair
        pairs = lowbits[pairs];
        //take the bad pair rank from pairs and add it to singles
        val ^= (pairs << 13);
        val |= pairs;
        kickers = val & CARD_MASK;
        val ^= lowbits[kickers];
        return TWOPAIR | val;
    }

    if (val < MIN_QUADS){
        trips = val >> 26;
       if ( !SINGLE_RANK(trips) ){//two sets of trips
           //move low trips to pair position
           val |= ((uint64_t)lowbits[trips] << 13);
           val ^= ((uint64_t)lowbits[trips] << 26);
           kickers = val & CARD_MASK;
           val ^= lowbits[kickers];
           return FULL | val;
       }
       pairs = (val >> 13) & CARD_MASK;
       if ( SINGLE_RANK(pairs) ){ //full
           kickers = val & CARD_MASK;
           val ^= lowbits[kickers];
           kickers ^= lowbits[kickers];
           val ^= lowbits[kickers];
           return FULL | val;
       }
       //trips and two pair
       if (pairs){
           val ^= ((uint64_t)lowbits[pairs] << 13);
           return FULL | val;
       }
       //trips
       kickers = val & CARD_MASK;
       val ^= lowbits[kickers];
       kickers ^= lowbits[kickers];
       val ^= lowbits[kickers];
       return TRIPS | val;
    }
    //quads, remove the other three cards, find the highest, add to ones column
    kickers = (val & CARD_MASK) | ((val >> 13) & CARD_MASK) | ((val >> 26) & CARD_MASK);
    //get the kickers down to 1
    while (lowbits[kickers] != kickers)
        kickers ^= lowbits[kickers];
    val &= (CARD_MASK << 39);
    val |= kickers;
    return QUADS | val;
    }


static uint64_t dohand(uint32_t c1, uint32_t c2, const partial const *data){

    uint64_t r1, r2;
    uint64_t flush;
    uint64_t val = data->val;
    uint_fast16_t straight = data->straight;
    const uint_fast16_t isflush = data->isflush + GET_SUIT(c1) + GET_SUIT(c2);

    if ( isFlushTable[isflush] != FAIL){
        flush = data->flush | GET_BIT(c1) | GET_BIT(c2);
        flush >>= isFlushTable[isflush];
        flush &= CARD_MASK;
        return ( (Straight_Table[flush]) ? SF : FLUSH ) | Flush_Table[flush];
    }

    r1 = GET_RANK(c1);
    r2 = GET_RANK(c2);

    straight |= r1 | r2;

    if (Straight_Table[straight]){
        return STRAIGHT | Straight_Table[straight];
    }

    while (r1 & val){
        r1 <<= 13;
    }
    val |= r1;

    while (r2 & val){
        r2 <<= 13;
    }
    val |= r2;

    //reduce the less paired status of the more paired cards
    return phase2( val ^ (val >> 13) );
}


static partial doboard(uint32_t hand[5]){
    uint64_t r;
    uint32_t i;
    partial data = {0,0,0,0};

    for (i = 0; i < 5; i++){
        r = GET_RANK(hand[i]);
        data.straight |= r;
        data.isflush += GET_SUIT(hand[i]);
        data.flush |= GET_BIT(hand[i]);
        while (r & data.val){
            r <<= 13;
        }
        data.val |= r;
    }
    return data;
}


int holdem_lite(uint32_t h1[2], uint32_t h2[2], uint32_t board[5]){
    const partial data = doboard(board);
    uint64_t v1 = dohand(h1[0], h1[1], &data);
    uint64_t v2 = dohand(h2[0], h2[1], &data);
    if (v1 > v2)
        return 0;
    if (v2 > v1)
        return 1;
    return 2;
}
