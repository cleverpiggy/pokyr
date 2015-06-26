# Copyright 2013 Allen Boyd Cunningham

# This file is part of pokyr.

#     pokyr is free software: you can redistribute it and/or modify
#     it under the terms of the GNU General Public License as published by
#     the Free Software Foundation, either version 3 of the License, or
#     (at your option) any later version.

#     pokyr is distributed in the hope that it will be useful,
#     but WITHOUT ANY WARRANTY; without even the implied warranty of
#     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#     GNU General Public License for more details.

#     You should have received a copy of the GNU General Public License
#     along with pokyr.  If not, see <http://www.gnu.org/licenses/>.


"""
This module provides functions for comparing seven card
poker hands and holdem hands.  Note that several lookup
tables are immediately build on import to increase performance
by eliminating the dot notation of classes.

It includes a 30 MB lookup table which allows approximately
a 4 times speed increase for holdem2p() over poker_lite.holdem2p().

The hash scheme for the lookup table was inspired by the specialK hand
evaluator blog:
http://specialk-coding.blogspot.com/2010/04/texas-holdem-7-card-evaluator_23.html
The table is populated by the poker_lite.handvalue function.
"""

import itertools
import poker_lite
import utils


from poker_lite import _IS_FLUSH, _BITS, CARD_MASK

_SPECIALKS = (0, 1, 5, 22, 98, 453, 2031, 8698, 22854, 83661, 262349, 636345, 1479181)
_SUITSHIFT = 23
_RANKMASK = 0x7fffff
_DECK = [_r | (_s << _SUITSHIFT) for _r in _SPECIALKS for _s in (0, 1, 8, 57)]


def _ranks_combos():
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


def _build_ranktable():
    #Returns a dict.
    ranktable = {}
    offsuits = (i % 4 for i in xrange(1000000))

    for hand in _ranks_combos():
        key = sum(_SPECIALKS[i] for i in hand)
        offhand = [r * 4 + next(offsuits) for r in hand]
        val = poker_lite.handvalue(offhand)
        ranktable[key] = val

    return ranktable


def _build_suittable():
    #Returns a dict.
    flushtable = {}
    for hand, value in enumerate(poker_lite._FLUSH_TABLE):
        if not value: continue
        flushtable[hand] = poker_lite.SF | value if poker_lite._STRAIGHT_TABLE[hand] else poker_lite.FLUSH | value
    return flushtable


_FLUSH_TABLE = _build_suittable()
_RANK_TABLE = _build_ranktable()


def handvalue(hand, val=0, computed_cards=[]):
    """Return a value of a seven card hand which can be
    compared to the handvalue value of any other hand to
    see if it is better worse or equal.

    Only supply the hand.  The other kwargs are for internal
    use and efficiency"""
    deck = _DECK
    for c in hand:
        val += deck[c]

    if _IS_FLUSH[(val >> _SUITSHIFT)] != -1:
        flush = 0
        for c in hand + computed_cards:
            flush += _BITS[c]
        flush >>= _IS_FLUSH[val >> _SUITSHIFT]
        flush &= CARD_MASK
        return _FLUSH_TABLE[flush]

    return _RANK_TABLE[val & _RANKMASK]


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


def holdem2p(h1, h2, board):
    """Return an integer representing the winner of two
    holdem hands and a board.
    0 -> h1 wins
    1 -> h2 wins
    2 -> tie"""
    deck = _DECK
    val = 0
    for c in board:
        val += deck[c]

    c1, c2 = h1
    v1 = val + deck[c1] + deck[c2]
    if _IS_FLUSH[(v1 >> _SUITSHIFT)] != -1:
        flush = _BITS[c1] + _BITS[c2]
        for c in board:
            flush += _BITS[c]
        flush >>= _IS_FLUSH[v1 >> _SUITSHIFT]
        flush &= CARD_MASK
        v1 = _FLUSH_TABLE[flush]

    else:
        v1 = _RANK_TABLE[v1 & _RANKMASK]

    c1, c2 = h2
    v2 = val + deck[c1] + deck[c2]
    if _IS_FLUSH[(v2 >> _SUITSHIFT)] != -1:
        flush = _BITS[c1] + _BITS[c2]
        for c in board:
            flush += _BITS[c]
        flush >>= _IS_FLUSH[v2 >> _SUITSHIFT]
        flush &= CARD_MASK
        v2 = _FLUSH_TABLE[flush]

    else:
        v2 = _RANK_TABLE[v2 & _RANKMASK]

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

    deck=_DECK
    boardval = 0
    for c in board:
        boardval += deck[c]

    results = []
    best = 0
    for i, h in enumerate(hands):
        v = handvalue(h, boardval, board)
        if v > best:
            results = [i]
            best = v
        elif v == best:
            results.append(i)
    return results


def monte_carlo(hands, trials=100000):
    """
    Return ev of each player.

    hands -> list of 2 - 22 hands
    trials -> the approximate number of simulations to run
    """
    nplayers = len(hands)
    wins = [0 for __ in range(nplayers)]

    nboards = (52 - nplayers * 2) / 5
    scheme = [5] * nboards
    deck = utils.Deck(sum(hands, []))

    for t in xrange(trials / nboards):
        for b in deck.deal(scheme):
            winners = multi_holdem(hands, b)
            nwinners = len(winners)
            for w in winners:
                wins[w] += 1.0 / nwinners
    trials = trials / nboards * nboards
    return [w / trials for w in wins]


def full_enumeration(hands, board=[]):
    """
    Return ev of each player.

    hands -> list of two card hands
    board -> any # of cards 0-5.
    """
    nplayers = len(hands)
    if nplayers == 2:
        return enum2p(hands[0], hands[1], board)
    if nplayers == 1:
        return [1.0]
    wins = [0 for __ in range(nplayers)]
    trials = 0
    dead = sum(hands, board)
    deck = utils.Deck(dead)
    needed_cards = 5 - len(board)
    for cards in itertools.combinations(deck, needed_cards):

        winners = multi_holdem(hands, board + list(cards))
        nwinners = len(winners)
        trials += 1
        for w in winners:
            wins[w] += 1.0 / nwinners

    return [w / trials for w in wins]


def enum2p(h1, h2, board=[]):
    """
    Return ev of each player.

    hands -> list of two card hands
    board -> any # of cards 0-5.
    """
    wins = [0,0,0]
    dead = h1 + h2 + board
    deck = utils.Deck(dead)
    needed_cards = 5 - len(board)
    for cards in itertools.combinations(deck, needed_cards):

        winners = holdem2p(h1, h2, board + list(cards))
        wins[winners] += 1

    ev1 = (wins[0] + .5 * wins[2]) / sum(wins)
    return [ev1, 1.0 - ev1]

def ehs(hand, board=[], iter_board=1000, iter_opp=100):
    """Returns expected hand strength and expected hand strength squared
    of player.

    hand -> two card hand
    board -> any # of cards 0-5.
    """
    needed_cards = 5 - len(board)
    ehs = 0
    ehs2 = 0
    deck = utils.Deck(dead=hand+board)
    for b in xrange(iter_board):
        wins = [0, 0, 0]
        temp = board + deck.deal(needed_cards)
        newdeck = utils.Deck(dead=hand+temp)
        for t in xrange(iter_opp):
            h2 = newdeck.deal(2)
            winners = holdem2p(hand, h2, temp)
            wins[winners]+= 1
        eq = (wins[0] + .5 * wins[2]) / iter_opp
        ehs += eq
        ehs2 += eq * eq
    ehs /= iter_board
    ehs2 /= iter_board
    print "Total: {0}, EHS: {1}, EHS2: {2}".format(iter_board*iter_opp, ehs, ehs2)
    return ehs, ehs2
