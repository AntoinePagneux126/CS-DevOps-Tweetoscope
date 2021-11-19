import sys, os , inspect

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
    res_pred=PT.predictions(np.array([1e-5,2e-6]),cascade,2.016,1)
    assert isinstance(res_pred,tuple)
    assert isinstance(res_pred[0],np.ndarray)
    assert isinstance(res_pred[1],np.floating)
    assert isinstance(res_pred[2],np.floating)

test_hawkes_estim()