"""
This code is aimed to provide tools for prediction process. 
"""

import numpy as np



def predictions(params, history, alpha, mu, T = None):
    """
    Returns the expected total numbers of points for a set of time points
    
    params   -- parameter tuple (p,beta) of the Hawkes process
    history  -- (n,2) numpy array containing marked time points (t_i,m_i)  
    alpha    -- power parameter of the power-law mark distribution
    mu       -- min value parameter of the power-law mark distribution
    T        -- 1D-array of times (i.e ends of observation window)
    """
    if T : 
        if not isinstance(T, (float, int) ) or T < 0:
            raise Exception(" T must be an float or int greater than 0")

    if not isinstance(params, np.ndarray) :
            raise Exception(" params must be a np.ndarray")
    
    if not isinstance(params[0], np.floating) : 
            raise Exception ("p must be a int or float")

    if not isinstance(params[0], np.floating)  or params[1]<0: 
            raise Exception ("beta must be a int or float greater than 0")

    if not isinstance(history, np.ndarray) or history.shape[1]!=2 : 
            raise Exception(" history must be an np.array with following shape : (n,2)")
    
    if not isinstance(alpha, (int,float)): 
            raise Exception(" alpha must be an float or int ")

    if not isinstance(mu, (int,float)): 
            raise Exception(" mu must be an float or int ")
    p,beta = params
    
    tis = history[:,0]
    if T is None:
        T = np.linspace(60,tis[-1],1000)

    N = np.zeros((len(T),2))
    N[:,0] = T
    
    EM = mu * (alpha - 1) / (alpha - 2)
    n_star = p * EM
    if n_star >= 1:
        raise Exception(f"Branching factor {n_star:.2f} greater than one")

    Si, ti_prev, i = 0., 0., 0
    
    for j,t in enumerate(T):
        for (ti,mi) in history[i:]:
            if ti >= t:
                break
            else:
                Si = Si * np.exp(-beta * (ti - ti_prev)) + mi
                ti_prev = ti
                i += 1

        n = i + 1
        G1 = p * Si * np.exp(-beta * (t - ti_prev))
        N[j,1] = n + G1 / (1. - n_star)
    return N,n_star,G1