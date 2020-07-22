#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#define FAIL -1
#define SUCCESS 1


static uint32_t Deck[52];
static int DeckSize = 0;
static char randstate[256];


int initdeck(bool dead[52]){
    //dead is true at positions of cards that are dead
    uint32_t card;
    int i = 0;
    for (card = 0; card < 52; card++){
        if (!dead || !dead[card])
            Deck[i++] = card;
    }
    DeckSize = i;
    if (!initstate((unsigned int)time(NULL), randstate, 256))
        return FAIL;
    return SUCCESS;
}


int deal(uint32_t cards[], int n){
    int i, r;
    int decksize = DeckSize;
    int last_index = decksize - 1;
    uint32_t *deck = Deck;

    if (decksize < n)
        return FAIL;

    for (i = 0; i < n; i++, last_index--){
         r = random() * (decksize --) / RAND_MAX;
         cards[i] = deck[r];
         deck[r] = deck[last_index];
         deck[last_index] = cards[i];
    }
    return SUCCESS;
}
