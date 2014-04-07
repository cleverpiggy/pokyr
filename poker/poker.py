"""
This module provides functions for comparing seven card
poker hands and holdem hands.  Note that several lookup
tables are immediately build on import to increase performance
by eliminating the dot notation of classes.

It includes a 30 MB lookup table which allows approximately
a 4 times speed increase for holdem() over poker_lite.holdem().

The hash scheme for the lookup table was inspired by the specialK hand
evaluator blog:
http://specialk-coding.blogspot.com/2010/04/texas-holdem-7-card-evaluator_23.html
The table is populated by the poker_lite.handvalue function.
"""

import poker_lite
from poker_lite import IS_FLUSH, BITS, CARD_MASK


SPECIALKS = (0, 1, 5, 22, 98, 453, 2031, 8698, 22854, 83661, 262349, 636345, 1479181)
SUITSHIFT = 23
RANKMASK = 0x7fffff
DECK = [r | (s << SUITSHIFT) for r in SPECIALKS for s in (0, 1, 8, 57)]


def ranks_combos():
    #Yield each possible suitless hand.
    for i in xrange(13):
        for j in xrange(i, 13):
            for k in xrange(j, 13):
                for l in xrange(k, 13):
                    for m in xrange(l, 13):
                        if i == j == k == l == m:
                            continue
                        for n in xrange(m, 13):
                            if j == k == l == m == n:
                                continue
                            for o in xrange(n, 13):
                                if k == l == m == n == o:
                                    continue
                                yield i, j, k, l, m, n, o


def build_ranktable():
    #Returns a dict.
    ranktable = {}
    offsuits = (i % 4 for i in xrange(1000000))

    for hand in ranks_combos():
        key = sum(SPECIALKS[i] for i in hand)
        offhand = [r * 4 + next(offsuits) for r in hand]
        val = poker_lite.handvalue(offhand)
        ranktable[key] = val

    return ranktable


def build_suittable():
    #Returns a dict.
    flushtable = {}
    for hand, value in enumerate(poker_lite.FLUSH_TABLE):
        if not value: continue
        flushtable[hand] = poker_lite.SF | value if poker_lite.STRAIGHT_TABLE[hand] else poker_lite.FLUSH | value
    return flushtable


FLUSH_TABLE = build_suittable()
RANK_TABLE = build_ranktable()


def handvalue(hand, deck=DECK):
    """Return a value of a seven card hand which can be
    compared to the handvalue value of any other hand to
    see if it is better worse or equal.

    Only supply the hand.  The other kwargs are for internal
    use and efficiency"""
    val = 0
    for c in hand:
        val += deck[c]

    if IS_FLUSH[(val >> SUITSHIFT)] != -1:
        flush = 0
        for c in hand:
            flush += BITS[c]
        flush >>= IS_FLUSH[val >> SUITSHIFT]
        flush &= CARD_MASK
        return FLUSH_TABLE[flush]

    return RANK_TABLE[val & RANKMASK]


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


def holdem(h1, h2, board, deck=DECK):
    """Return an integer representing the winner of two
    holdem hands and a board.
    0 -> h1 wins
    1 -> h2 wins
    2 -> tie"""
    val = 0
    for c in board:
        val += deck[c]

    c1, c2 = h1
    v1 = val + deck[c1] + deck[c2]
    if IS_FLUSH[(v1 >> SUITSHIFT)] != -1:
        flush = BITS[c1] + BITS[c2]
        for c in board:
            flush += BITS[c]
        flush >>= IS_FLUSH[v1 >> SUITSHIFT]
        flush &= CARD_MASK
        v1 = FLUSH_TABLE[flush]

    else:
        v1 = RANK_TABLE[v1 & RANKMASK]

    c1, c2 = h2
    v2 = val + deck[c1] + deck[c2]
    if IS_FLUSH[(v2 >> SUITSHIFT)] != -1:
        flush = BITS[c1] + BITS[c2]
        for c in board:
            flush += BITS[c]
        flush >>= IS_FLUSH[v2 >> SUITSHIFT]
        flush &= CARD_MASK
        v2 = FLUSH_TABLE[flush]

    else:
        v2 = RANK_TABLE[v2 & RANKMASK]

    if v1 > v2:
        return 0
    if v2 > v1:
        return 1
    return 2


def test():
    from general_store import utils
    #print type(FLUSH_TABLE)
    for i in xrange(100000):
        holes1, holes2, board = utils.deal()
        if poker_lite.holdem(holes1, holes2, board) != holdem(holes1, holes2, board):
            print "made it to ", i
            print utils.make_pretty(holes1)
            print utils.make_pretty(holes2)
            print utils.make_pretty(board)
            print "old:", ["first wins", "second wins", "tie"][poker_lite.holdem(holes1, holes2, board)]
            print "new:", ["first wins", "second wins", "tie"][holdem(holes1, holes2, board)]
            assert False


if __name__ == "__main__":
    test()
