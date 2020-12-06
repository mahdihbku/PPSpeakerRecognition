from distutils.core import setup, Extension


ec_elgamal_module = Extension('_ec_elgamal',
                           sources=['ec_elgamal_wrap.c', 'ec_elgamal.c'],
                           libraries=['lcrypto','lssl'],
                           )

setup (name = 'ec_elgamal',
       version = '0.1',
       author      = "SWIG Docs",
       description = """Simple swig ec_elgamal from docs""",
       ext_modules = [ec_elgamal_module],
       py_modules = ["ec_elgamal"],
       )