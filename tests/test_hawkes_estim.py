import sys, os , inspect
import pytest

current_dir= os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
parent_dir= os.path.dirname(current_dir)
sys.path.insert(0,parent_dir)


import Python_files.hawkes_tools as HT
import Python_files.predictor_tools as PT


import numpy as np


def test_hawkes_estim():
    assert isinstance(8.0,float)
    cascade = np.load("tests/test_cascade.npy")#p, beta = 0.025, 1/3600. alpha, mu = 2.4, 10 m0 = 1000
    res_map=HT.compute_MAP(cascade,cascade[-1,0], 2.4, 10)
    print(res_map)
    assert isinstance(res_map[0],np.floating)
    assert isinstance(res_map[1],np.ndarray)
    with pytest.raises(Exception) as execinfo :
        HT.compute_MAP(1,cascade[-1,0], 2.4, 10)
        assert(str(execinfo.value)==" history must be an np.array with following shape : (n,2)")
    with pytest.raises(Exception) as execinfo :
        HT.compute_MAP(cascade,"time error", 2.4, 10)
        assert(str(execinfo.value)==" t must be an float or int greater than 0")

test_hawkes_estim()