"""
Poker hand evaluating modules that provide fast enumerations.
Both C Extension module it's pure python analogies are included.
"""

from distutils.core import setup, Extension
import os


if not os.path.exists(os.path.join("src", "cpokertables.h")):
    from poker import poker_lite
    poker_lite.write_ctables(os.path.join("src", "cpokertables.h"))

sources = [os.path.join("src", f) for f in
           os.listdir("src") if f.endswith('.c')]

module = Extension(
    'poker.cpoker',
    sources=sources
)

with open("README.md") as f:
    long_description = f.read()

setup(
    name='pypoker-tools',
    version='0.1',
    ext_modules=[module],
    packages=['poker'],
    author='Allen Boyd Cunningham',
    author_email='cleverfoundation@gmail.com',
    url='https://github.com/cleverpiggy/pypoker-tools',
    description=__doc__,
    long_description=long_description,
    license='GPL',
    classifiers=[
        'Programming Language :: Python',
        'Programming Language :: C',
        'License :: OSI Approved :: GNU General Public License (GPL)',
        'Operating System :: POSIX :: Linux',
        'Operating System :: MacOS :: MacOS X',
        'Intended Audience :: Developers',
        'Topic :: Games/Entertainment'
    ]
)
