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


// void printcard(int c){
//     static const char ranks[] = "AKQJT98765432";
//     static const char suits[] = "cdhs";
//     printf("%c%c ", ranks[c >> 2], suits[c % 4]);
// }

// void printhand(uint32_t *hand, int n){
//     int i;
//     for (i = 0; i < n; i++){
//         printcard(hand[i]);
//     }
//     printf("\n");
// }


// #define GET_INDEX(c1, c2) c1 * 52 - c1 * (c1 + 1) / 2 + c2 - c1 - 1


// int main(int argc, char const *argv[])
// {
//     uint32_t cards[2];
//     int results[1326];
//     int i, temp;

//     bool dead[52];
//     for (i = 0; i < 52; i ++) dead[i] = i % 2;


//     if (initdeck(NULL) == FAIL)
//         return FAIL;

//     // for (i = 0; i < DeckSize; i++){
//     //     printf ("%i\n", Deck[i]);
//     // }
//     // return 0;
//     for (i = 0; i < 1326; i++) results[i] = 0;

//     for (i = 0; i < 10000000; i++){
//         deal(cards, 2);

//         //sort the cards
//         if (cards[0] > cards[1]){
//             temp = cards[0];
//             cards[0] = cards[1];
//             cards[1] = temp;
//         }
//         results[GET_INDEX(cards[0], cards[1])] ++;
//     }

//     for (i = 0; i < 1326; i++){
//         printf("%i\n", results[i]);
//     }

//     return 0;
// }

