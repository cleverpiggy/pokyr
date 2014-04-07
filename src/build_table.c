#include "poker_heavy.h"

uint64_t handvalue(uint32_t hand[7]);

//build the rank and flush table using hand_value

typedef struct{
    uint32_t key;
    uint64_t val;
}entry;


entry ranks_entry(uint32_t hand[7]){

    #define WITH_OFFSUIT(c) c * 4 + (offsuit++ % 4)

    static const uint32_t specialks[13] = SPECIALKS;

    int i, offsuit = 0;
    uint32_t offsuit_hand[7], key = 0;

    for (i = 0; i < 7; i++){
        key += specialks[hand[i]];
        offsuit_hand[i] = WITH_OFFSUIT(hand[i]);
    }
    return (entry) {key, handvalue(offsuit_hand)};
}


static int compare(const void *a, const void *b){
    entry *a_ = (entry *) a;
    entry *b_ = (entry *) b;
    if (a_->val < b_->val){
        return -1;
    }
    if (a_->val > b_->val){
        return 1;
    }
    if (a_->key < b_->key){
        return -1;
    }
    if (a_->key > b_->key){
        return 1;
    }
    return 0;
}


//populate arrays with key, value entries sorted by value for
//all canonical hands

void compute_ranks(entry *rankitems){
    uint32_t a=0, i,j,k,l,m,n,o;
    uint32_t hand[7];

    for (i = 0; i < 13; i++){
        hand[0] = i;
        for (j = i; j < 13; j++){
            hand[1] = j;
            for (k = j; k < 13; k++){
                hand[2] = k;
                for (l = k; l < 13; l++){
                    hand[3] = l;
                    for (m = l; m < 13; m++){
                        if (i == j && j == k && k == l && l == m){
                            continue;
                        }
                        hand[4] = m;
                        for ( n = m; n < 13; n ++){
                            if (j == k && k == l && l == m && m == n)
                                continue;
                            hand[5] = n;
                            for ( o = n; o < 13; o ++){
                                if (k == l && l == m && m == n && n == o)
                                    continue;
                                hand[6] = o;
                                rankitems[a++] = ranks_entry(hand);
                                //printf("%lli\n", rankitems[a - 1].val);
                            }
                        }
                    }
                }
            }
        }
    }
    qsort(rankitems, a, sizeof(entry), compare);
}


void compute_flushes(entry *flushitems, entry *sfitems,
                                      const uint16_t flushtable[FLUSH_TABLE_SIZE],
                                      const uint16_t straighttable[FLUSH_TABLE_SIZE]){
    int i;

    for (i = 0; i < FLUSH_TABLE_SIZE; i++){
        if (!flushtable[i])
            continue;
        if (straighttable[i])
            *sfitems++ = (entry) {i, flushtable[i]};
        else
            *flushitems++ = (entry) {i, flushtable[i]};
    }
    qsort(flushitems - NUM_FLUSHES, NUM_FLUSHES, sizeof(entry), compare);
    qsort(sfitems - NUM_SFS, NUM_SFS, sizeof(entry), compare);
}


//populate main flush and rank lookup tables with minimal values
//straighttable is needed for the process
void populate_tables(uint16_t ranktable[RANK_TABLE_SIZE],
                     uint16_t flushtable[FLUSH_TABLE_SIZE],
                     const uint16_t straighttable[FLUSH_TABLE_SIZE]){

    uint16_t newval;
    uint64_t previous_val;
    bool done = false;
    int i, j;

    entry *rankitems = (entry *) malloc(NUM_RANK_COMBOS * sizeof(entry));
    entry *flushitems = (entry *) malloc(NUM_FLUSHES * sizeof(entry));
    entry *sfitems = (entry *) malloc(NUM_SFS * sizeof(entry));

    compute_flushes(flushitems, sfitems, flushtable, straighttable);

    compute_ranks(rankitems);
    newval = 0;
    previous_val = rankitems[0].val;


    for ( i = 0; i < NUM_RANK_COMBOS; i++){
        if (!done && rankitems[i].val > FULL){
            for (j = 0; j < NUM_FLUSHES; j++){
                newval += (flushitems[j].val != previous_val);
                previous_val = flushitems[j].val;
                flushtable[flushitems[j].key] = newval;
            }
            done = true;
        }
        newval += (rankitems[i].val != previous_val);
        previous_val = rankitems[i].val;
        ranktable[rankitems[i].key] = newval;


    }

    for (i = 0; i < NUM_SFS; i++){
        newval += (sfitems[i].val != previous_val);
        previous_val = sfitems[i].val;
        flushtable[sfitems[i].key] = newval;
    }

    free(rankitems);
    free(flushitems);
    free(sfitems);

    for (i =0 ; i < RANK_TABLE_SIZE; i++){

    }
}
