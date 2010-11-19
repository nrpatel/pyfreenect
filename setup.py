#!/usr/bin/env python
from distutils.core import setup, Extension
setup(name='pyfreenect',
	version='0.1',
	ext_modules=[Extension('pyfreenect', ['pyfreenect.c'],
	include_dirs=['.','/usr/include/libusb-1.0'],
	library_dirs=['.','/usr/local/lib','/usr/lib'],
	libraries=['libfreenect'],
	)]
	)

