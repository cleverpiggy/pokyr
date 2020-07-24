"""
Poker hand evaluating modules that provide fast enumerations.
Both C Extension module it's pure python analogies are included.
"""
try:
    from setuptools import setup, Extension
except ImportError:
    from distutils.core import setup, Extension
import os


if not os.path.exists(os.path.join("src", "cpokertables.h")):
    from poker import poker_lite
    poker_lite.write_ctables(os.path.join("src", "cpokertables.h"))

sources = [
    'src/build_table.c',
    'src/cpokermod.c',
    'src/deal.c',
    'src/poker_heavy.c',
    'src/poker_lite.c'
]

module = Extension(
    'poker.cpoker',
    sources=sources
)

long_description = "README at https://github.com/cleverpiggy/pokyr"
if os.path.exists("README.md"):
    with open("README.md") as f:
        long_description = f.read()


setup(
    name='pokyr',
    version='0.1.24',
    ext_modules=[module],
    packages=['poker'],
    author='Allen Boyd Cunningham',
    author_email='cleverfoundation@gmail.com',
    url='https://github.com/cleverpiggy/pokyr',
    description='Poker hand evaluating modules that provide fast enumerations.',
    long_description=long_description,
    long_description_content_type='text/markdown',
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
