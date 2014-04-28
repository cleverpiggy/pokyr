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


import cpoker
import poker_lite
import poker
import utils


def assert_close(a, b, error=.001):
    if abs(a - b) > error:
        raise AssertionError

def test_full_enumeration():
    f = lambda *hands: cpoker.full_enumeration([utils.to_cards(h) for h in hands])
    map(assert_close, f("3s 2c", "5c 2h"), [0.398847108925, 1 - 0.398847108925])
    map(assert_close, f("8h Kh", "2c 5h"), [0.665651659986, 1 - 0.665651659986])
    map(assert_close, f("2s 3s", "5s Js"), [0.329603271382, 1 - 0.329603271382])
    map(assert_close, f("9c Jc", "Ts Td"), [0.327120651473, 1 - 0.327120651473])
    map(assert_close, f("9d Qh", "3s Ac"), [0.426625762715, 1 - 0.426625762715])
    map(assert_close, f("9c Ac", "Ah Kh"), [0.288065962586, 1 - 0.288065962586])
    map(assert_close, f("8c 4s", "Qh 9d"), [0.333585332978, 1 - 0.333585332978])
    map(assert_close, f("Jc 2s", "7s 2d"), [0.730345779721, 1 - 0.730345779721])
    map(assert_close, f("Ah 3c", "Qs 4d"), [0.609246080135, 1 - 0.609246080135])
    map(assert_close, f("5c 3h", "Qh 4h"), [0.376223789701, 1 - 0.376223789701])

    #multiway
    map(assert_close, f('8c Qd', '9h 9s', '4c 3d'),
        [0.263, 0.584, 0.153])
    map(assert_close, f('9h 4s',  '3s Jc', '4h Ad', '8c 2s'),
        [0.172, 0.269, 0.352, 0.207])
    map(assert_close, f('4s Ad', '5h 4c', 'Jc 9h', '3d Qc', '2s Qh'),
        [0.288, 0.165, 0.328, 0.113, 0.106])


def test_cfull_enumeration_with_boardcards_against_py():
    switch = {3:4, 4:3}
    n = 3
    for __ in range(100):
        n = switch[n]
        cards = utils.deal([2, 2, n])
        hands, board = cards[:2], cards[2]
        map(assert_close, cpoker.full_enumeration(hands, board), poker.full_enumeration(hands, board))

    for __ in range(100):
        n = switch[n]
        cards = utils.deal([2, 2, 2, n])
        hands, board = cards[:3], cards[3]
        map(assert_close, cpoker.full_enumeration(hands, board), poker.full_enumeration(hands, board))

    for __ in range(100):
        n = switch[n]
        cards = utils.deal([2, 2, 2, 2, 2, n])
        hands, board = cards[:5], cards[5]
        map(assert_close, cpoker.full_enumeration(hands, board), poker.full_enumeration(hands, board))


def test_holdem():
    def multi(h1, h2, board):
        r = cpoker.multi_holdem([h1, h2], board)
        if r != poker.multi_holdem([h1, h2], board):
            for h in [h1, h2, board]:
                print utils.make_pretty(h)
            print r
            print poker.multi_holdem([h1, h2], board)
            assert False
        return r[0] if len(r) == 1 else 2

    funcs = [poker_lite.holdem2p,
             lambda h1, h2, b:poker_lite.compare(h1 + b, h2 + b),
             cpoker.holdem2p,
             poker.holdem2p,
             multi]

    for f in funcs:
        f = utils.pretty_args(f)
        assert f('Td 3d', 'Ac As', 'Ks 8h 4h 6d Qh') == 1
        assert f('Td 5h', 'Ad 2s', 'Ah 4h Kd 5c 4c') == 1
        assert f('2s Td', '7c 8c', '7h Ah 3c 2h 2d') == 0
        assert f('Jc 4h', '3h 8d', '9d 5d Js Tc 7s') == 1
        assert f('Ts 7c', 'Tc 2h', '6s Ad Ac 6h Kc') == 2
        assert f('5d 7d', '8d Kd', '3h 5s 3c 5h Qh') == 0
        assert f('7s 4h', 'Ts Js', '4d 6h 6c Qs 6d') == 0
        assert f('7s 6c', '2h Jd', '8c Ks 3h 7c Ts') == 0
        assert f('Kd 3d', 'Jh 3c', '6d Kh 2s Th 6c') == 0
        assert f('2s 4s', '9d Th', 'Kc Ks Qs Ah 3c') == 1
        assert f('Kh Th', 'Qd 4d', 'Qh Js 5c 2s 2d') == 1
        assert f('8s Td', 'Jc Qs', '4h 8d 9s 9h Qd') == 1
        assert f('7c Js', 'Kd 6s', '3h Ad 4h 2c Tc') == 1
        assert f('6c Qh', 'Jh Qs', '9c 9s 4h Ts Jc') == 1
        assert f('2h 3c', 'Tc Kd', '4c 4d Kh 5s 4s') == 1
        assert f('Ts 4d', 'Ks Ad', '2h 3s 4s 7s Jd') == 0
        assert f('4s Qc', '8c Qd', '9h 9s 4c 3d Js') == 0
        assert f('3c 2c', '5s 2d', '3h 4h 8s Ad 2h') == 1
        assert f('7c Jh', 'Ah 6c', '8s 9s Qd 3c 5d') == 1
        assert f('Qc Ts', '9h 5d', '5h Jd 5c Js 3c') == 1
        assert f('Tc 5h', '6s Jd', 'Ah 7d 4h 9s 4s') == 1
        assert f('4c 9h', '9c 7h', 'Ac 9d Jd 8h 5c') == 2
        assert f('4d Ah', 'Qs 8s', '9c 3s 2d Ad Ts') == 0
        assert f('Th 3d', 'Ad 3c', 'Jh 7s 5s Ah 2c') == 1
        assert f('9h 4s', '3s Jc', '4h Ad 8c 2s 9c') == 0
        assert f('7d Js', 'Td 8s', '6h 6s Kc 7h 4c') == 0
        assert f('2d 6d', 'Qd 9d', 'Ad 2s 6h Qs 7h') == 0
        assert f('Qs Ts', 'Th 5d', '9d 2c Qc 3h Ah') == 0
        assert f('5h 8d', '4d Jc', 'Ac Js Jd 4c 2s') == 1
        assert f('4s Ad', '5h 4c', 'Jc 9h 3d Qc 2s') == 0

    #do some more for good measure
    for __ in xrange(1000):
        cards = utils.deal()
        if len(set([f(*cards) for f in funcs])) != 1:
            print [f(*cards) for f in funcs]
            assert False


def test_multi_holdem():

    for hands, board, result in [
        (['5d 6h', '5h Kh', '8h 8s'], 'Ah Ks Qc 6s 8c', [2]),
        (['9h 5s', '7d 7c', '6d 2d'], '2s Kh 7s 5h 8h', [1]),
        (['Jd Tc', 'Ts Th', 'Td Ac'], '9h 3c Jc Qs Ad', [2]),
        (['Qh 2c', 'Kd 6s', '8s Js'], '4s Kh Ah Ks Jc', [1]),
        (['5h 8c', 'Tc 4c', '3h As'], '7d Ah 8d 3s 9c', [2]),
        (['4h Js', '3h Jc', '4d 5s'], 'As Ks Kh 8d Ah', [0, 1]),
        (['2c 6d', '5h Kd', '7d 5c'], 'Ah 6s 9s Td 6h', [0]),
        (['Jd 4h', 'Kh 8h', '9d 5h'], 'Jh Qd Ad Qh Ks', [1]),
        (['Qc 7d', 'Th Jc', 'As 8c'], 'Qh 5d 7h 9h Jd', [0]),
        (['4d Kc', 'Qc 4h', '5s Jd'], 'Ks 9s 5c Qd 2c', [0]),
        (['6h Ks', '9c 9s', '3h 2c'], '5s Td 4c 3c Kh', [0]),
        (['9h 7d', '8h 4h', '5d 3h'], 'Kc Jd Ad 5s 8c', [1]),
        (['Ts Js', '8h 7h', '7s 3h'], 'As Jh 2c Ac 3c', [0]),
        (['Ah Ts', '2s 7d', 'Kh Js'], '7c 3s 3d Td 7s', [1]),
        (['9d 5h', 'Kh 6h', '8d 8c'], 'Ks 4d 7h Qd Th', [1]),
        (['7c Js', 'Ah Kc', 'Jc 6c'], '2d Qh 4h 5d 2c', [1]),
        (['3d 8d', 'Ts 4d', 'Ah 5s'], '3c 9d As 6s Jd', [2]),
        (['4s 6c', '6s Qs', '9c 3d'], 'Th Ac 2d 8h 2h', [1]),
        (['Qd Ah', 'Jc 9h', '6c 7c'], '3s Tc 6s 4d 4c', [2]),
        (['Ad 6s', '3c 2d', '9d 8s'], '6d Th 3h Ah 9h', [0]),
        (['Ad 6s', '3c Jd', '9d 8s', 'As Jc'], '6d Th 7h 8s 9h', [1, 3]),
        (['Ad 6s', '3c 2d', '9d 8s', 'As Ac'], '6d Th 7h 8s 9h', [0, 1, 2, 3]),
        (['Ad 6s', 'Js 2d', 'Jd 8s', 'Qs Jc', '8c 8d'], '6d Th 7h 8s 9h', [3])
    ]:
        assert (cpoker.multi_holdem([utils.to_cards(h) for h in hands], utils.to_cards(board)) ==
        poker.multi_holdem([utils.to_cards(h) for h in hands], utils.to_cards(board)) ==
        result)


def test_crivervalue():
    f = utils.pretty_args(cpoker.rivervalue)
    assert_close(f('2d 6d', 'Tc Ts 8s 6c 5s'), 0.620202020202)
    assert_close(f('Jd Jc', '5s 7s Td 7d 4h'), 0.85202020202)
    assert_close(f('5c 5s', '2d Ac 9c 2h Jd'), 0.529797979798)
    assert_close(f('9s 7h', '8s 4c Ah 6d 3c'), 0.0409090909091)
    assert_close(f('Ts Qs', 'Tc Ad Ks 3s 5d'), 0.679797979798)
    assert_close(f('Ts Jh', 'Kh 2d 2h Ah 3h'), 0.926262626263)
    assert_close(f('Qc 5c', 'Jc 6h 7h Ts 2h'), 0.135353535354)
    assert_close(f('Kd Qh', '7h 5s 6s 4c Jd'), 0.125757575758)
    assert_close(f('Qh 8d', '4s 8h 8c Th 9d'), 0.927777777778)
    assert_close(f('9h 2d', '4s 9s 8h 6h 7h'), 0.49595959596)
    assert_close(f('Td 5s', 'Th 4s 8h 5c 2s'), 0.932323232323)
    assert_close(f('Kc 4h', '7c 3s 8c 5h 6d'), 0.769191919192)
    assert_close(f('2d 3c', 'Jc 4s 6h Kd 4d'), 0.0166666666667)
    assert_close(f('Td Kc', '6c Qh Js 7c Ks'), 0.866666666667)
    assert_close(f('Qs 5h', '8c 3h Qd Ts Qh'), 0.939898989899)
    assert_close(f('Kh 5h', '8h 2h 4s Ks Tc'), 0.850505050505)
    assert_close(f('3c 3d', 'Td 8s Ts 8h 4h'), 0.0166666666667)
    assert_close(f('Kh Td', 'Ah Th 3d 5c 9s'), 0.779797979798)
    assert_close(f('9c 6s', 'Qh 9d Th 7c Qd'), 0.670707070707)
    assert_close(f('Ac 3d', '6h 5d 7h 9h 7s'), 0.29797979798)
    assert_close(f('2c Th', 'Jd 7s Ah 3h 5h'), 0.119696969697)
    assert_close(f('9h Ks', '8h 4c 7c 3d 6h'), 0.15)
    assert_close(f('5h Jh', '2c 4c 9d 7h 5d'), 0.605050505051)
    assert_close(f('6s Ts', '5d 6d Ad Td Kh'), 0.574747474747)
    assert_close(f('As 3h', 'Qs 3s Qd Jc Ah'), 0.845454545455)
    assert_close(f('Tc Qc', '3c 4h 4d Ac 5s'), 0.259090909091)
    assert_close(f('5s 3d', '5d 6d 7c Jc 9c'), 0.307575757576)
    assert_close(f('Jd 3s', '8h 9s 2s 3c 4h'), 0.521212121212)
    assert_close(f('Ac Qc', '4d 9d 4h 5h 4c'), 0.638888888889)
    assert_close(f('3c 9c', 'Ac 7s Ah Qc As'), 0.257070707071)

def main():
    for name, f in globals().items():
        if name.startswith('test'):
            f()


if __name__ == '__main__':
    main()