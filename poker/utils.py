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


import random
import re
from functools import update_wrapper


class Card(int):
    """
    You can initialize a new card by string "As", by integer from
    0 - 51, or by rank{0..12} and suit{0..3}.
    """

    _strings = [r + s for r in "AKQJT98765432" for s in 'cdhs']
    _numbers = {s:i for i, s in enumerate(_strings)}

    def __new__(cls, c, suit=None):
        if isinstance(c, tuple):
            return int.__new__(cls, c[0] * 4 + c[1])
        if suit is not None:
            return int.__new__(cls, c * 4 + suit)
        card = cls._numbers.get(c, c)
        assert card % 52 == card
        return int.__new__(cls, card)

    def __str__(self):
        return self._strings[self]

    def __repr__(self):
        return '<Card {}>'.format(str(self))

    def __iter__(self):
        return self.generator()

    def __getitem__(self, item):
        return (self >> 2, self % 4)[item]

    def generator(self):
        yield self >> 2
        yield self % 4

    @property
    def rank(self):
        return self >> 2

    @property
    def suit(self):
        return self % 4


class Deck(list):

    _deck = set(Card(c) for c in range(52))

    def __init__(self, dead=None):
        deck = self._deck if dead is None else self._deck - set(dead)
        super(Deck, self).__init__(deck)

    def __str__(self):
        return ' '.join(str(c) for c in self)

    def __repr__(self):
        return str(self)

    def deal(self, hands=None, pretty=False):
        """
        Return a list of hands according to 'hands' dimensions
        provided.  Default is a hand, hand, board holdem scheme.

        hands -> scalar or sequence
        pretty -> flag for pretty cards
        >>> [len(h) for h in Deck().deal()]
        [2, 2, 5]
        >>> [len(h) for h in Deck().deal((7, 7))]
        [7, 7]
        """
        convert = make_pretty if pretty else lambda x: x
        if hands is None:
            cards = random.sample(self, 9)
            bb_hand, button_hand, board = cards[:2], cards[2:4], cards[4:]
            return convert(bb_hand), convert(button_hand), convert(board)

        if isinstance(hands, int):
            return convert(random.sample(self, hands))

        cards = random.sample(self, sum(hands))
        i = 0
        result = []
        for n in hands:
            result.append(convert(cards[i: i + n]))
            i += n
        return result


deal = Deck().deal


def split_cards(cards):
    """
    Return list of each card string in spaceless cards.
    >>> split_cards("AsKsQc")
    ['As', 'Ks', 'Qc']
    >>> split_cards("As Ks Qs")
    ['As', 'Ks', 'Qs']
    >>> split_cards("AKQ")
    ['A', 'K', 'Q']
    >>> split_cards("As")
    ['As']
    """
    return re.findall("[2-9AKQJT][cdhs]?", cards)


def make_pretty(hand, sort=False):
    """
    Hand is integers or Card objects.
    >>> print(make_pretty([Card("As"), Card("Ks"), Card("Qs"), Card("Js"), Card("Ts")]))
    As Ks Qs Js Ts
    >>> print(make_pretty([0, 1, 2, 3]))
    Ac Ad Ah As
    """
    if isinstance(hand, int):
        hand = [hand]
    elif isinstance(hand, str):
        hand = split_cards(hand)
    sort_ = sorted if sort else (lambda x, reverse=0: x)
    return ' '.join(str(Card(c)) for c in sort_(hand, reverse=True))


def to_cards(hand):
    """
    Return a list of cards from a string such as 'As Jc 9d 9h 4d'
    >>> to_cards("Ac Ad Ah As")
    [0, 1, 2, 3]
    >>> to_cards("AcAdAhAs")
    [0, 1, 2, 3]
    >>> to_cards("Ac")
    [0]
    """
    if isinstance(hand, int):
        return [Card(hand)]
    if isinstance(hand, str):
        hand_ = split_cards(hand)
        if len(''.join(hand_)) != len(hand.replace(' ', '')):
            raise TypeError("%s is not a proper card string" % hand)
    # we got a sequence.  I'll make to work if you can call card on each object
    elif hasattr(hand, '__len__'):
        hand_ = hand
    else:
        raise TypeError("Cannot coerce object to Cards")
    return [Card(c) for c in hand_]



def _convert(args, kwargs):
    #helper for pretty_args
    #returns a list and a dictionary with card strings converted to cards
    """
    >>> pa = _convert(("As Ks", "I have the As Ks", 3.14), {})
    >>> pa == ([to_cards("As Ks"), "I have the As Ks", 3.14], {})
    True
    >>> pa = _convert(("bottybot", "As", .123), {})
    >>> pa == (["bottybot", to_cards("As"), .123], {})
    True
    >>> pa = _convert(("bottybot", ["As", "Kd Jd"], .123), {})
    >>> pa == (["bottybot", [to_cards("As"), to_cards("Kd Jd")], .123], {})
    True
    >>> pa = _convert(["bottybot", ["As", "Kd Jd"]], {"c":["Js", "4d 3d"]})
    >>> pa == (["bottybot", [to_cards("As"), to_cards("Kd Jd")]], {"c":[to_cards("Js"), to_cards("4d 3d")]})
    True
    """
    ugly_args = []
    for a in args:
        u_arg = a
        if isinstance(a, str):
            try:
                u_arg = to_cards(a)
            except TypeError:
                u_arg = a
        if isinstance(a, list):
            u_arg = _convert(a, {})[0]
        ugly_args.append(u_arg)

    for k, v in kwargs.items():
        u_arg = v
        if isinstance(v, str):
            try:
                u_arg = to_cards(v)
            except TypeError:
                u_arg = v
        if isinstance(v, list):
            u_arg = _convert(v, {})[0]
        kwargs[k] = u_arg
    return ugly_args, kwargs


def pretty_args(ugly_func):
    """
    Return a function that can use pretty cards or lists
    of pretty cards as args.
    """

    def func(*args, **kwargs):
        ugly_args, kwargs = _convert(args, kwargs)
        return ugly_func(*ugly_args, **kwargs)

    update_wrapper(func, ugly_func)
    return func


if __name__ == "__main__":
    import doctest
    doctest.testmod()
