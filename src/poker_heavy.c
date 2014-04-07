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


#include "poker_heavy.h"

uint16_t Rank_Table[7825760];

//DECK = [r | (s << SUITSHIFT) for r in SPECIALKS for s in (0, 1, 8, 57)]
static const uint32_t Deck[52] = DECK;

// these three come from poker_lite.c which on which
// building the tables for poker_heavy is necessary
extern const int8_t isFlushTable[400];
extern uint16_t Flush_Table[8129];
extern const uint64_t Bits[52];

#define GET_BIT(c) Bits[c]


void printcard(int c){
    static const char ranks[] = "AKQJT98765432";
    static const char suits[] = "cdhs";
    printf("%c%c ", ranks[c >> 2], suits[c % 4]);
}

void printhand(uint32_t *hand, int n){
    int i;
    for (i = 0; i < n; i++){
        printcard(hand[i]);
    }
    printf("\n");
}


static uint16_t dohand(uint32_t c1, uint32_t c2, const partial const *data){

    uint64_t flush;
    int i;

    uint32_t val = data->val + Deck[c1] + Deck[c2];

    if ( isFlushTable[val >> SUITSHIFT] != FAIL){
        flush = GET_BIT(c1) | GET_BIT(c2);
        for ( i = 0; i < 5; i++ ){
            flush += GET_BIT(data->board[i]);
        }
        flush >>= isFlushTable[val >> SUITSHIFT];
        flush &= CARD_MASK;
        return Flush_Table[flush];
    }

    return Rank_Table[val & RANKMASK];
}


int holdem(uint32_t h1[2], uint32_t h2[2], uint32_t board[5]){

    partial data = {0, board};
    int i;

    for (i = 0; i < 5; i++) {
        data.val += Deck[board[i]];
    }

    int v1 = dohand(h1[0], h1[1], &data);
    int v2 = dohand(h2[0], h2[1], &data);
    if (v1 > v2)
        return 0;
    if (v2 > v1)
        return 1;
    return 2;
}


static int dup_check (uint32_t cards[], int num){
    int i, j;

    for (i = 0; i < num; i++){
        for (j = i + 1; j < num; j++){
            if ( cards[i] == cards[j] ){
                return FAIL;
            }
        }
    }
    return 0;
}


struct rivervalue rivervalue (uint32_t hand[2], uint32_t board[5])
//count the number of wins, losses, and ties vs all opponent combinations
{
    uint32_t dead[7], i, j;

    uint64_t my_rank;
    uint64_t his_rank;
    bool deck[52];
    struct rivervalue value = (struct rivervalue) {0, 0};

    partial data = {0, board};
    for (i = 0; i < 5; i++){
        data.val += Deck[board[i]];
    }

    //set deck positions of dead cards to false
    for (i = 52; i--; )
        deck[i] = true;

    for (i = 5; i--; ){
        dead[i] = board[i];
        deck[dead[i]] = false;
    }

    dead[5] = hand[0];
    dead[6] = hand[1];
    if ( dup_check(dead, 7) == FAIL){
        value.wins = FAIL;
        return value;
    }
    deck[hand[0]] = false;
    deck[hand[1]] = false;

    my_rank = dohand(hand[0], hand[1], &data);

    //run the hands, skipping deck[card]=falses
    for (i = 52; --i; ){
        if ( deck[i] ){
            for ( j = i; j--; ){
                if ( deck[j] ){
                    his_rank = dohand(i, j, &data);
                    if (my_rank > his_rank){
                        value.wins ++;
                    }else if (my_rank == his_rank){
                        value.ties ++;
                    }
                }
            }
        }
    }
    return value;
}


//return the win% of h1
double preflop_match(uint32_t h1[2], uint32_t h2[2]){
    bool deck[52];

    uint32_t i, j, k, l, m, dead[4];
    uint32_t results[3] = {0, 0, 0};

    const uint64_t flush1 = GET_BIT(h1[0]) + GET_BIT(h1[1]);
    const uint64_t flush2 = GET_BIT(h2[0]) + GET_BIT(h2[1]);
    const uint32_t h1val = Deck[h1[0]] + Deck[h1[1]];
    const uint32_t h2val = Deck[h2[0]] + Deck[h2[1]];
    uint32_t vals[5];

    uint32_t temp1, temp2;
    uint64_t tempflush1, tempflush2;


    //set deck positions of dead cards to false
    for (i = 52; i--; )
        deck[i] = true;

    dead[0] = h1[0];
    dead[1] = h1[1];
    dead[2] = h2[0];
    dead[3] = h2[1];

    if ( dup_check(dead, 4) == FAIL){
        return FAIL;
    }

    deck[h1[0]] = false;
    deck[h1[1]] = false;
    deck[h2[0]] = false;
    deck[h2[1]] = false;

    for (i = 52; --i;){
        if (!deck[i]) continue;
        //board[0] = i;
        vals[0] = Deck[i];

        for (j = i; j--;){
            if (!deck[j]) continue;
            //board[1] = j;
            vals[1] = vals[0] + Deck[j];

            for (k = j; k--;){
                if (!deck[k]) continue;
                //board[2] = k;
                vals[2] = vals[1] + Deck[k];

                for (l = k; l--;){
                    if (!deck[l]) continue;
                    //board[3] = l;
                    vals[3] = vals[2] + Deck[l];

                    for (m = l; m--;){
                        if (!deck[m]) continue;
                        //board[4] = m;
                        vals[4] = vals[3] + Deck[m];
                        temp1 = vals[4] + h1val;
                        temp2 = vals[4] + h2val;


                        if ( isFlushTable[(temp1 >> SUITSHIFT)] != FAIL ){

                            tempflush1 = flush1 + GET_BIT(i) + GET_BIT(j) + GET_BIT(k) + GET_BIT(l) + GET_BIT(m);

                            tempflush1 >>= isFlushTable[temp1 >> SUITSHIFT];
                            tempflush1 &= CARD_MASK;
                            temp1 = Flush_Table[tempflush1];

                        }
                        else{
                            temp1 = Rank_Table[temp1 & RANKMASK];
                        }


                        if ( isFlushTable[(temp2 >> SUITSHIFT)] != FAIL ){

                            tempflush2 = flush2 + GET_BIT(i) + GET_BIT(j) + GET_BIT(k) + GET_BIT(l) + GET_BIT(m);

                            tempflush2 >>= isFlushTable[temp2 >> SUITSHIFT];
                            tempflush2 &= CARD_MASK;
                            temp2 = Flush_Table[tempflush2];

                        }
                        else{
                            temp2 = Rank_Table[temp2 & RANKMASK];
                        }


                        if (temp1 > temp2){
                            results[0] ++;
                        }
                        else if (temp1 < temp2){
                            results[1] ++;
                        }
                        else{
                            results[2] ++;
                        }
                    }
                }
            }
        }
    }
    return (results[0] + 0.5 * (double) results[2]) / (results[0] + results[1]+ results[2]);
}

//more sane version of above, 10 -15% slower

// //return the win% of h1
// double preflop_match(uint32_t h1[2], uint32_t h2[2]){
//     bool deck[52];

//     uint32_t i, j, k, l, m, board[5], dead[4];
//     uint32_t results[3] = {0, 0, 0};

//     //set deck positions of dead cards to false
//     for (i = 52; i--; )
//         deck[i] = true;

//     dead[0] = h1[0];
//     dead[1] = h1[1];
//     dead[2] = h2[0];
//     dead[3] = h2[1];

//     if ( dup_check(dead, 4) == FAIL){
//         return FAIL;
//     }

//     deck[h1[0]] = false;
//     deck[h1[1]] = false;
//     deck[h2[0]] = false;
//     deck[h2[1]] = false;

//     for (i = 52; --i;){
//         if (!deck[i]) continue;
//         board[0] = i;
//         for (j = i; j--;){
//             if (!deck[j]) continue;
//             board[1] = j;
//             for (k = j; k--;){
//                 if (!deck[k]) continue;
//                 board[2] = k;
//                 for (l = k; l--;){
//                     if (!deck[l]) continue;
//                     board[3] = l;
//                     for (m = l; m--;){
//                         if (!deck[m]) continue;
//                         board[4] = m;
//                         results[holdem(h1, h2, board)]++;
//                     }
//                 }
//             }
//         }
//     }
//     return (results[0] + 0.5 * (double) results[2]) / (results[0] + results[1]+ results[2]);
// }


int river_distribution (uint32_t hand[2], uint32_t board[5], long chart[], dictEntry *dict)
{
    uint32_t dead[7], i, j;

    uint64_t my_rank;
    uint64_t his_rank;
    bool deck[52];

    int dict_i = 0;

    partial data = {0, board};
    for (i = 0; i < 5; i++){
        data.val += Deck[board[i]];
    }

    //set deck positions of dead cards to false
    for (i = 0; i < 52; ++i )
        deck[i] = true;

    for (i = 5; i--; ){
        dead[i] = board[i];
        deck[dead[i]] = false;
    }

    dead[5] = hand[0];
    dead[6] = hand[1];
    if ( dup_check(dead, 7) == FAIL){
        return FAIL;
    }
    deck[hand[0]] = false;
    deck[hand[1]] = false;

    my_rank = dohand(hand[0], hand[1], &data);

    //run the hands, skipping deck[card]=falses
    for (i = 0; i < 52; ++i ){
        if ( deck[i] ){
            for ( j = i + 1; j < 52; ++j, ++dict_i ){
                if ( deck[j] ){
                    if (dict_i >= NUM_STARTING_HANDS){
                        return FAIL;
                    }
                    his_rank = dohand(i, j, &data);
                    if (my_rank > his_rank)
                        chart[dict[dict_i].value] += 2;
                    if (my_rank == his_rank)
                        chart[dict[dict_i].value] += 1;
                }
            }
        }
        else{
            dict_i += 52 - i - 1;
        }
    }
    return 0;
}
