This is a python library that provides routines for very fast
poker hand comparisons to power simulations.

Pure python and c-extention modules are included.  Be aware that
the main modules use a lookup table of over 30mb, which in the case
of the pure python version can take up to a second to build. The rank
lookup sceme includes 13 magic numbers found on the specialK blog,
http://specialk-coding.blogspot.com/2010/04/texas-holdem-7-card-evaluator_23.html
which I used to supplement my own algorithm.  A pure python lite version is
also include that leaves a small memory footprint and is still fast.

Cards are represented by intergers 0-51 and groups of cards are contained
in lists.  A Card class is provided for convenience.  The pretty_args
wrapper allows you to use standard card strings rather than Card objects
or integers.

>>> import cpoker
>>> from utils import pretty_args
>>> rivervalue = pretty_args(cpoker.rivervalue)
>>> preflop_match = pretty_args(cpoker.preflop_match)
>>> preflop_match("AsKd", "8c2s")
0.6753362720638392
>>> # percentile on river vs all 990 hand combos
>>> rivervalue("As Kd", "Ks Qh Jc 8s 8d")
0.8585858585858586
