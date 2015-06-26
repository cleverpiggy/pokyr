// Copyright 2013 Allen Boyd Cunningham

// This file is part of pokyr.

//     pokyr is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//     pokyr is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.

//     You should have received a copy of the GNU General Public License
//     along with pokyr.  If not, see <http://www.gnu.org/licenses/>.


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


static uint16_t dohand(uint32_t c1, uint32_t c2, const partial *data){

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


int holdem2p(uint32_t h1[2], uint32_t h2[2], uint32_t board[5]){

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


int multi_holdem(uint32_t hands[MAX_HANDS][2], int n, uint32_t board[5], int winners_buf[]){

    //Return the number of players tied for the win (usually one)
    //assign indices in hands[] of the winners to winners[]
    //    from the beginning to nwinners

    partial data = {0, board};
    int i, val, ties = 0, best = -1;

    for (i = 0; i < 5; i++) {
        data.val += Deck[board[i]];
    }

    for (i = 0; i < n; i++){
        val = dohand(hands[i][0], hands[i][1], &data);
        if (val > best){
            winners_buf[(ties = 0)] = i;
            best = val;
        }
        else if (val == best){
            winners_buf[++ties] = i;
        }
    }
    return ties + 1;
}


int set_dead(void *cards1_, int n1, void *cards2_, int n2, bool dead[52]){
    //assign true to all positions in deck that are listed in cards1
    //or cards2.  otherwise false.
    //Return FAIL if there were duplicate cards
    int i;
    uint32_t *cards1 = (uint32_t*) cards1_;
    uint32_t *cards2 = (uint32_t*) cards2_;

    for (i = 52; i--;)
        dead[i] = false;

    for (i = 0; i < n1; i++){
        if (dead[cards1[i]]){ //duplicate card
            return FAIL;
        }
        dead[cards1[i]] = true;
    }
    for (i = 0; i < n2; i++){
        if (dead[cards2[i]]){ //duplicate card
            return FAIL;
        }
        dead[cards2[i]] = true;
    }
    return SUCCESS;
}


struct rivervalue rivervalue (uint32_t hand[2], uint32_t board[5])
//count the number of wins, losses, and ties vs all opponent combinations
{
    uint32_t i, j;

    uint16_t my_rank;
    uint16_t his_rank;
    bool dead[52];
    struct rivervalue value = (struct rivervalue) {0, 0};

    partial data = {0, board};
    for (i = 0; i < 5; i++){
        data.val += Deck[board[i]];
    }

    if (set_dead(hand, 2, board, 5, dead) == FAIL){
        value.wins = FAIL;
        return value;
    }

    my_rank = dohand(hand[0], hand[1], &data);

    //run the hands, skipping deck[card]=falses
    for (i = 52; --i; ){
        if ( !dead[i] ){
            for ( j = i; j--; ){
                if ( !dead[j] ){
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
//super ugly optimized for two hands preflop
double enum2p(uint32_t h1[2], uint32_t h2[2]){
    bool dead[52];

    uint32_t i, j, k, l, m;
    uint32_t results[3] = {0, 0, 0};

    const uint64_t flush1 = GET_BIT(h1[0]) + GET_BIT(h1[1]);
    const uint64_t flush2 = GET_BIT(h2[0]) + GET_BIT(h2[1]);
    const uint32_t h1val = Deck[h1[0]] + Deck[h1[1]];
    const uint32_t h2val = Deck[h2[0]] + Deck[h2[1]];
    uint32_t vals[5];

    uint32_t temp1, temp2;
    uint64_t tempflush1, tempflush2;

    if (set_dead(h1, 2, h2, 2, dead) == FAIL)
        return FAIL;

    for (i = 52; --i;){
        if (dead[i]) continue;
        //board[0] = i;
        vals[0] = Deck[i];

        for (j = i; j--;){
            if (dead[j]) continue;
            //board[1] = j;
            vals[1] = vals[0] + Deck[j];

            for (k = j; k--;){
                if (dead[k]) continue;
                //board[2] = k;
                vals[2] = vals[1] + Deck[k];

                for (l = k; l--;){
                    if (dead[l]) continue;
                    //board[3] = l;
                    vals[3] = vals[2] + Deck[l];

                    for (m = l; m--;){
                        if (dead[m]) continue;
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


int monte_carlo(uint32_t hands[MAX_HANDS][2], int nhands, int nruns, double results[]){
    //no board because full_enumeration should be fast enough

    int deal(uint32_t [], int);
    int initdeck(bool [52]);

    int i, n, nwinners, winners[MAX_HANDS];
    uint32_t board[5];
    bool dead[52];

    if (set_dead(hands, nhands * 2, NULL, 0, dead) == FAIL)
        return FAIL;
    if (initdeck(dead) == FAIL)
        return FAIL;

    for (i = 0; i < nhands; results[i++] = 0.0);

    for (i = 0; i < nruns; i++){
        deal(board, 5);
        nwinners = multi_holdem(hands, nhands, board, winners);
        if (nwinners == 1){
            results[winners[0]] += 1.0;
        }
        else{
            for (n = nwinners - 1; n >= 0; n--){
                results[winners[n]] += 1.0 / nwinners;
             }
        }
    }

    for (i = 0; i < nhands; i++){
        results[i] /= nruns;
    }
    return SUCCESS;
}


int full_enumeration(uint32_t hands[MAX_HANDS][2], int nhands, uint32_t board[5], int nboard, double results[]){
    //hands ->array of two card hands with no duplicates
    //results -> buffer to hold the results, ev of each hand
    //nhands -> number of hands
    //board -> populated by up to 4 cards with room for 5
    //nboard -> between 0 and 4

    bool dead[52];

    uint32_t i, j, k, l, m;
    int n, nwinners, nrunnouts = 0, winners[MAX_HANDS];

    if (set_dead(hands, nhands * 2, board, nboard, dead) == FAIL)
        return FAIL;

    for (i = 0; i < nhands; results[i++] = 0.0);

    //a solution for incorporating variable number of board cards
    //without changing the preflop code much

    #define CRUNCH nwinners = multi_holdem(hands, nhands, board, winners); \
                   nrunnouts++; \
                   if (nwinners == 1) \
                       results[winners[0]] += 1.0; \
                   else{ \
                       for (n = nwinners - 1; n >= 0; n--){ \
                           results[winners[n]] += 1.0 / nwinners; \
                        } \
                   }

    #define CRUNCH_IF(n_) if (nboard == n_){ \
                              CRUNCH \
                              continue; \
                          }

    for (i = 52; --i;){
        if (dead[i]) continue;
        board[4] = i;
        CRUNCH_IF(4)
        for (j = i; j--;){
            if (dead[j]) continue;
            board[3] = j;
            CRUNCH_IF(3)
            for (k = j; k--;){
                if (dead[k]) continue;
                board[2] = k;
                CRUNCH_IF(2)
                for (l = k; l--;){
                    if (dead[l]) continue;
                    board[1] = l;
                    CRUNCH_IF(1)
                    for (m = l; m--;){
                        if (dead[m]) continue;
                        board[0] = m;
                        CRUNCH
                    }
                }
            }
        }
    }
    //we end up skipping card 0 if only running out out rivers
    if (nboard == 4 && !dead[0]){
        board[4] = 0;
        CRUNCH
    }

    for (i = 0; i < nhands; i++){
        results[i] /= nrunnouts;
    }
    return SUCCESS;
}


int river_distribution (uint32_t hand[2], uint32_t board[5], int chart[], dictEntry *dict)
{
    int deal(uint32_t [], int);
    int initdeck(bool [52]);

    uint32_t i, j;

    uint64_t my_rank;
    uint64_t his_rank;
    bool dead[52];

    int dict_i = 0;

    partial data = {0, board};
    for (i = 0; i < 5; i++){
        data.val += Deck[board[i]];
    }

    if (set_dead(hand, 2, board, 5, dead) == FAIL)
        return FAIL;

    my_rank = dohand(hand[0], hand[1], &data);

    //run the hands, skipping dead cards
    for (i = 0; i < 52; ++i ){
        if ( !dead[i] ){
            for ( j = i + 1; j < 52; ++j, ++dict_i ){
                if ( !dead[j] ){
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

int ehs(uint32_t hand[2], uint32_t board[5], int nboard, int iter_board, int iter_opp, double result[2]) {
    int deal(uint32_t [], int);
    int initdeck(bool [52]);
    int resetdeck(bool [52]);
    int i, j;
    int winner, wins[3];
    uint32_t temp[5];
    bool dead[52];
    uint32_t opp[2];
    double eq;
    int k;

    if (set_dead(hand, 2, board, nboard, dead) == FAIL) return FAIL;
    if (initdeck(dead) == FAIL) return FAIL;
    result[0] = 0.0;
    result[1] = 0.0;
    for (i = 0; i < iter_board; i++){
	for (j = 0; j < 3; j++) wins[j] = 0;

	deal(temp, 5 - nboard + 2); 
    
	for (j = 0; j < 5 - nboard; j++) board[j + nboard] = temp[j];

	// remove the new cards dealt
	if (set_dead(hand, 2, board, 5, dead) == FAIL) return FAIL;
	if (resetdeck(dead) == FAIL) return FAIL;

	for (j = 0; j < iter_opp; j++){
	    deal(opp, 2);
	    winner = holdem2p(hand, opp, board);
	    wins[winner] += 1;
	}

	// add them back  
	if (set_dead(hand, 2, board, nboard, dead) == FAIL) return FAIL;
	if (resetdeck(dead) == FAIL) return FAIL;


	eq = (wins[0] + .5 * wins[2]) / iter_opp;
	result[0] += eq;
	result[1] += eq * eq;
    }
    result[0] /= iter_board;
    result[1] /= iter_board;

    return SUCCESS;
}
