# Copyright 2013 Allen Boyd Cunningham

# This file is part of pypoker-tools.

#     pypoker_tools is free software: you can redistribute it and/or modify
#     it under the terms of the GNU General Public License as published by
#     the Free Software Foundation, either version 3 of the License, or
#     (at your option) any later version.

#     pypoker_tools is distributed in the hope that it will be useful,
#     but WITHOUT ANY WARRANTY; without even the implied warranty of
#     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#     GNU General Public License for more details.

#     You should have received a copy of the GNU General Public License
#     along with pypoker_tools.  If not, see <http://www.gnu.org/licenses/>.


"""
This module provides functions for comparing seven card
poker hands and holdem hands.  Note that several lookup
tables are immediately build on import to increase performance
by eliminating the dot notation of classes.
"""

from itertools import combinations, combinations_with_replacement

#A card rank is is represented as one bit in a 13 bit field.
#handvalue() assembles the ranks of the cards according to the
#following 3 schemes:
#for straights: 13bits with all ranks of hand set
#for flushes: 52bits -- 13 for each suit with ranks of that suit set
#for pairs: 52bits:
#   -quads13bits-trips13bits-pairs13bits-singles13bits-

#_phase2() is seperate to allow caching handvalue calculations
#(ie. in do_board() and do_hand()) and does the following:
#check flush table for each of four flushes
#check straight table
#turn off bits of unused cards
#attach prefix representing hand type (two pair, flush, etc.)

#ultimately an integer is returned which can be compared
#to find the winning hand


#one of the first 13 bits set for each rank
_RANKS = [1 << _n for _n in reversed(range(13)) for __ in range(4)]

#one of 52 bits set for each cards
_BITS = [_r << _i % 4 * 13 for _i, _r in enumerate(_RANKS)]

_SUITS = [0, 1, 8, 57] * 13

#the hand type goes on the most significant bits
SF = 8 << 52
QUADS = 7 << 52
FULL = 6 << 52
FLUSH = 5 << 52
STRAIGHT = 4 << 52
TRIPS = 3 << 52
TWOPAIR = 2 << 52
PAIR = 1 << 52

CARD_MASK = 0x1fff

MIN_PAIR = 1 << 13
MIN_TRIPS = 1 << 26
MIN_QUADS = 1 << 39


def _build_straighttable():
    # Return a table to determine if a group of ranks make a straight.
    # The value is zero for not a straight, otherwise a 1-10 based on rank.
    BIGGEST_HAND = sum(_RANKS[:28:4])
    RANKS = _RANKS[::4]
    indexes = [range(i, i+5) for i in range(9)] + [[9, 10, 11, 12, 0]]
    setcards = set(range(13))
    #the extras are the set of card - straight cards + one higher
    extras = [setcards - set(i + [i[0] - 1]) for i in indexes]
    straights = [sum(RANKS[i] for i in s) for s in indexes]
    table = [0] * (BIGGEST_HAND + 1)
    for i, s in enumerate(straights):
        value = 11 - i # the smallest straights are on the right
        assert value > 0
        table[s] = value
        for e in extras[i]:
            straight6 = s | RANKS[e]
            table[straight6] = value
        for a, b in combinations(extras[i], 2):
            straight7 = s | RANKS[a] | RANKS[b]
            table[straight7] = value
    return table


def _build_flushtable(st=None, lbt=None):
    # Return a table to determine flushes.

    # The indices are thirteen bit sets of cards of a single suit.
    # If a number contains 5 or more distinct ranks,
    # it gets a nonszero value mapped to flush strength.
    # Straight flushes are included with possibly a seperate set of values.
    BIGGEST_HAND = sum(_RANKS[:28:4])
    RANKS = _RANKS[::4]
    if st is None:
        st = _build_straighttable()
    if lbt is None:
        lbt = _build_lowbittable()
    table = [0] * (BIGGEST_HAND + 1)
    for i in (5, 6, 7):
        for hand in combinations(RANKS, i):
            index = sum(hand)
            if st[index]:
                v = st[index]
            else:
                v = index
                for __ in range(i - 5):
                    v ^= lbt[v]
            table[index] = v
    return table


def find_lowbit(n):
    """Return the lowest set index in a 13 bit hand"""
    for i in xrange(13):
        if n & (1 << i):
            return i

def _build_lowbittable():
    #this contains the bit itself
    NO_HAND = 0xff
    return [NO_HAND] + [1 << find_lowbit(n) for n in xrange(1, 1 << 13)]


def _build_isflush():
    # Populate with the numbers to right shift for the flush value.
    # -1 for not a flush.
    suits = [0, 1, 8, 57]
    table = [-1] * (57*7 + 1)
    for i, s in enumerate(suits):
        flush = s * 5
        for c in combinations_with_replacement(suits, 2):
            table[flush + sum(c)] = i * 13
    return table


_LOWBITS = _build_lowbittable()
_STRAIGHT_TABLE = _build_straighttable()
_FLUSH_TABLE = _build_flushtable()
_IS_FLUSH = _build_isflush()


def handvalue(hand):
    """Return a value of a seven card hand which can be
    compared to the handvalue value of any other hand to
    see if it is better worse or equal.

    Only supply the hand.  The other kwargs are for internal
    use and efficiency"""
    ranks=_RANKS
    suits=_SUITS
    f = 0
    for c in hand:
        f += suits[c]

    if _IS_FLUSH[f] != -1:
        flush = 0
        for c in hand:
            flush += _BITS[c]
        flush >>= _IS_FLUSH[f]
        flush &= CARD_MASK
        return SF | _FLUSH_TABLE[flush] if _STRAIGHT_TABLE[flush] else FLUSH | _FLUSH_TABLE[flush]

    val = 0
    straight = 0
    for c in hand:
        r = ranks[c]
        straight |= r
        while r & val:
            r <<= 13
        val |= r

    if _STRAIGHT_TABLE[straight]:
        return STRAIGHT | _STRAIGHT_TABLE[straight]

    #reduce the less paired status of the more paired cards for _phase2
    return _phase2(val ^ (val >> 13))

_rsb = zip(_RANKS,_SUITS,_BITS)

def _doboard(board):
    """Return the part of the hand evaluating function after
    processing just the board.

    If you want to calculate the values of many two card hands
    on one board, use this result along with the different hands
    in the _dohand function"""
    rsb = _rsb
    val = 0
    flush = 0
    straight = 0
    isflush = 0

    for c in board:
        r, s, b = rsb[c]
        straight |= r
        flush |= b
        isflush += s
        while r & val:
            r <<= 13
        val |= r
    return val, flush, straight, isflush


def _dohand(hand, board_info):
    """Return a hand value.
    hand -> two card holdem hand
    board_info -> the return value of the _doboard function"""
    ranks = _RANKS
    suits = _SUITS
    val, flush, straight, isflush = board_info

    c1, c2 = hand

    isflush += suits[c1] + suits[c2]
    if _IS_FLUSH[isflush] != -1:
        flush += _BITS[c1] + _BITS[c2]
        flush >>= _IS_FLUSH[isflush]
        flush &= CARD_MASK
        return SF | _FLUSH_TABLE[flush] if _STRAIGHT_TABLE[flush] else FLUSH | _FLUSH_TABLE[flush]

    straight |= ranks[c1] | ranks[c2]
    if _STRAIGHT_TABLE[straight]:
        return STRAIGHT | _STRAIGHT_TABLE[straight]

    r = ranks[c1]
    while r & val:
        r <<= 13
    val |= r

    r = ranks[c2]
    while r & val:
        r <<= 13
    val |= r

    #reduce the less paired status of the more paired cards for _phase2
    return _phase2(val ^ (val >> 13))


def holdem2p(h1, h2, board):
    """Return an integer representing the winner of two
    holdem hands and a board.
    0 -> h1 wins
    1 -> h2 wins
    2 -> tie"""
    board_info = _doboard(board)
    v1 = _dohand(h1, board_info)
    v2 = _dohand(h2, board_info)
    if v1 > v2:
        return 0
    if v2 > v1:
        return 1
    return 2



_schemes2p = [[0], [1], [0,1]]

def multi_holdem(hands, board):
    """Return a list indices representing hands that win or tie."""

    if len(hands) == 2: #them holdem2p is optimal
        return _schemes2p[holdem2p(hands[0], hands[1], board)]

    board_info = _doboard(board)

    results = []
    best = 0
    for i, h in enumerate(hands):
        v = _dohand(h, board_info)
        if v > best:
            results = [i]
            best = v
        elif v == best:
            results.append(i)
    return results


def _phase2(val):
    
    # This is a helper function for the handvalue functions.
    # It goes through the hand, turning off the bits of the
    # unused cards to produce the best five.  Then it attaches
    # the hand type code (Two pair, flush, etc.) to the most
    # significant bit.
    lowbits = _LOWBITS

    if val < MIN_PAIR:
        val ^= lowbits[val]
        val ^= lowbits[val]
        return val

    if val < MIN_TRIPS:
        pairs = val >> 13
        if pairs == lowbits[pairs]:#just a pair
            #get rid of the two lowest cards
            kickers = val & CARD_MASK
            val ^= lowbits[kickers]
            kickers ^= lowbits[kickers]
            val ^= lowbits[kickers]
            return PAIR | val

        elif pairs ^ (lowbits[pairs]) == lowbits[pairs ^ (lowbits[pairs])]:# just two pair
            kickers = val & CARD_MASK
            val ^= lowbits[kickers]
            kickers ^= lowbits[kickers]
            val ^= lowbits[kickers]
            return TWOPAIR | val
        else: #three pair
            badpair = lowbits[pairs]
            #take the bad pair rank from pairs and add it to singles
            val ^= (badpair << 13)
            val |= badpair
            kickers = val & CARD_MASK
            val ^= lowbits[kickers]
            return TWOPAIR | val

    if val < MIN_QUADS:
        trips = val >> 26
        if trips != lowbits[trips]:#two sets of trips
            #move low trips to pair position
            val |= (lowbits[trips] << 13)
            val ^= (lowbits[trips] << 26)
            kickers = val & CARD_MASK
            val ^= lowbits[kickers]
            return FULL | val
        pairs = (val >> 13) & CARD_MASK
        if pairs == lowbits[pairs]:
            kickers = val & CARD_MASK
            val ^= lowbits[kickers]
            kickers ^= lowbits[kickers]
            val ^= lowbits[kickers]
            return FULL | val
        #trips and two pair
        if pairs:
            val ^= (lowbits[pairs] << 13)
            return FULL | val
        #trips
        kickers = val & CARD_MASK
        val ^= lowbits[kickers]
        kickers ^= lowbits[kickers]
        val ^= lowbits[kickers]
        return TRIPS | val
    #quads, remove the other three cards, find the highest, add to ones column
    kickers = (val & CARD_MASK) | ((val >> 13) & CARD_MASK) | ((val >> 26) & CARD_MASK)
    #get the kickers down to 1
    while lowbits[kickers] != kickers:
        kickers ^= lowbits[kickers]
    val &= (CARD_MASK << 39)
    val |= kickers
    return QUADS | val


def compare(h1, h2):
    """Return an integer representing the winner of two
    seven card hands.
    0 -> h1 wins
    1 -> h2 wins
    2 -> tie"""
    r1 = handvalue(h1)
    r2 = handvalue(h2)
    if r1 > r2:
        return 0
    elif r2 > r1:
        return 1
    else:
        return 2


def arraystr(l):
    return str(l)[1:-1].replace(' ', '').join("{}") + '\n'

def write_ctables(name="cpokertables.h"):
    """Write the cpokertables.h header file."""
    tables = [
        "#define FLUSH_TABLE " + arraystr(_FLUSH_TABLE),
        "#define STRAIGHT_TABLE " + arraystr(_STRAIGHT_TABLE),
        "#define LOWBITS " + arraystr(_LOWBITS),
        "#define ISFLUSH " + arraystr(_IS_FLUSH)
    ]
    with open(name, 'w') as f:
        f.write("\n".join(tables))


if __name__ == "__main__":
    import sys
    if len(sys.argv) > 1:
        write_ctables(sys.argv[1])

