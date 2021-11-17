import sys, os , inspect

current_dir= os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
parent_dir= os.path.dirname(current_dir)
sys.path.insert(0,parent_dir)


import Python_files.hawkes_tools as HT


import numpy as np


def test_hawkes_estim():
    assert isinstance(8.0,float)
    cascade = np.load("tests/test_cascade.npy")#p, beta = 0.025, 1/3600. alpha, mu = 2.4, 10 m0 = 1000
    res=HT.compute_MAP(cascade,cascade[-1,0], 2.4, 10)
    assert isinstance(res[0],np.floating)
    assert isinstance(res[1],np.ndarray)

test_hawkes_estim()
