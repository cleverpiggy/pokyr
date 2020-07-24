# pokyr

This is a python library that provides routines for very fast poker hand
comparisons to power hand match ups.

Pure python and c-extention modules are included.  Using the cpoker
module it should be easy to write python code that will, for example,
quickly caluculate expected hand strength squared and opponent cluster
hand strength [(EHS2/OCHS)](http://poker.cs.ualberta.ca/publications/AAMAS13-abstraction.pdf).

The rank lookup sceme uses 13 magic numbers attributed to the [specialK blog](http://specialk-coding.blogspot.com/2010/04/texas-holdem-7-card-evaluator_23.html),
which I used to supplement my own algorithm.  A pure python lite version is
also included that leaves a small memory footprint and is still fast.


### Sample
Cards are represented by intergers 0-51 and groups of cards are contained
in lists.  The pretty_args wrapper allows you to use standard card strings rather than integers.  You can also convert from integers to pretty cards and back with
utils.make_pretty and utils.to_cards.

```python
>>> from poker import cpoker
>>> from poker import utils
>>> #return the evs of a match between any number of hands
>>> enum = utils.pretty_args(cpoker.full_enumeration)
>>> enum(["AsKd", "8c2s"])
[0.6753362720638392, 0.32466372793616083]
>>> enum(["AsKd", "7h 8h", "6c 6d"], "Kc 6h 9h")
[0.021040974529346623, 0.44518272425249167, 0.5337763012181617]
>>> # percentile on river vs all 990 hand combos
>>> utils.pretty_args(cpoker.rivervalue)("As Kd", "Ks Qh Jc 8s 8d")
0.8585858585858586
>>> utils.make_pretty([7, 8, 9])
'Ks Qc Qd'
>>> utils.to_cards('Ks Qc Qd')
[7, 8, 9]
```

### Compile and Install
If you have a c compiler the python dev tools for your system then hopefully this does it.  All you need is python 2 or 3 to run the pure python modules.

```
$ python setup.py install
```
