import sys, os , inspect

import pytest
import numpy as np

current_dir= os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
parent_dir= os.path.dirname(current_dir)
sys.path.insert(0,parent_dir)

import Python_files
import Python_files.hawkes_tools as HT
import Python_files.predictor_tools as PT
# import Python_files.hawkes_estimator as HE
# import Python_files.predictor as PE



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


def test_hawkes_pred():

    cascade = np.load("tests/test_cascade.npy")#p, beta = 0.025, 1/3600. alpha, mu = 2.4, 10 m0 = 1000
    res_pred=PT.predictions(np.array([1e-5,2e-6]),cascade,2.016,1)
    assert isinstance(res_pred,tuple)
    assert isinstance(res_pred[0],np.ndarray)
    assert isinstance(res_pred[1],np.floating)
    assert isinstance(res_pred[2],np.floating)

    with pytest.raises(Exception) as execinfo :
        PT.predictions([1e-5,2e-6],cascade,2.016,1)
        assert(str(execinfo.value)==" params must be a np.ndarray")

    with pytest.raises(Exception) as execinfo :
        PT.predictions(np.array(["error p ",2e-6]),cascade,2.016,1)
        assert(str(execinfo.value)==" p must be a int or float")

    with pytest.raises(Exception) as execinfo :
        PT.predictions(np.array([1e-5,"error beta"]),cascade,2.016,1)
        assert(str(execinfo.value)=="beta must be a int or float greater than 0")

    with pytest.raises(Exception) as execinfo :
        PT.predictions(np.array([1e-5,-6]),cascade,2.016,1)
        assert(str(execinfo.value)=="beta must be a int or float greater than 0")

    with pytest.raises(Exception) as execinfo :
        PT.predictions(np.array([1e-5,2e-6]),1,2.016,1)
        assert(str(execinfo.value)==" history must be an np.array with following shape : (n,2)")
    
    with pytest.raises(Exception) as execinfo :
        PT.predictions([1e-5,2e-6],cascade,"alpha error",1)
        assert(str(execinfo.value)==" alpha must be an float or int ")
    
    with pytest.raises(Exception) as execinfo :
        PT.predictions([1e-5,2e-6],cascade,2.016,"mu error")
        assert(str(execinfo.value)==" mu must be an float or int ")