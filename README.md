pypoker-tools
=============

This is a python library that provides routines for very fast poker hand
comparisons to power hand match ups.

Pure python and c-extention modules are included.  Using the cpoker
module it should be easy to write python code that will, for example,
quickly caluculate expected hand strength squared and opponent cluster 
hand strength [(EHS2/OCHS)](http://poker.cs.ualberta.ca/publications/AAMAS13-abstraction.pdf).

Be aware that the main modules use a lookup table of over 30mb, which in
the case of the pure python version can take up to a second to build. The
rank lookup sceme uses 13 magic numbers attributed to the [specialK blog](http://specialk-coding.blogspot.com/2010/04/texas-holdem-7-card-evaluator_23.html),
which I used to supplement my own algorithm.  A pure python lite version is
also include that leaves a small memory footprint and is still fast.  A good
use case might be where you can't use third party modules and have
limited resources, such as on a free google-app-engine site.


Sample
------
Cards are represented by intergers 0-51 and groups of cards are contained
in lists.  A Card class is provided for convenience.  The pretty_args
wrapper allows you to use standard card strings rather than Card objects
or integers.

```python
>>> from poker import cpoker
>>> from poker.utils import pretty_args
>>> #return the evs of a match between any number of hands
>>> pretty_args(cpoker.full_enumeration)("AsKd", "8c2s")
[0.6753362720638392, 0.32466372793616083]
>>> # percentile on river vs all 990 hand combos
>>> pretty_args(cpoker.rivervalue)("As Kd", "Ks Qh Jc 8s 8d")
0.8585858585858586
```

Compile and Install
-------
OSX -> Xcode should provide you with everything you need.
Linux -> You just need python-dev and gcc and you should be good to go.
```
$python setup.py install
```
There is an issue with later versions of osx and Xcode 5.1.
If you get the error:
```
clang: error: unknown argument: '-mno-fused-madd' [-Wunused-command-line-argument-hard-error-in-future]
```
you can try running this before setup.py:
```
$export CFLAGS=-Qunused-arguments
```

Apple will hopefully fix this problem in a future release.
