""" this is the class version of the previous code"""

import numpy as np 
import scipy.optimize as optim

class Cascade : 
    def __init__(self,json):
    ##############################################
    ###########   Given Attributes  ##############
    ##############################################

        self._json=json.value
        self._key = json.value["key"]
        self._history=json.value["tweets"]
        self._t=json.value["T_obs"]

    ##############################################
    #####  Constants given by Mishra et al  ######
    ##############################################


        self._mu=1
        self._alpha=2.016

    ##############################################
    ############    Default constants ############
    ##############################################


        self._p = 0 # default value
        self._beta= 0 #default value
        self._N=1 #default value 



    #################################
    ##########  Accessors   #########
    #################################

    def Get_p(self):
        return self._p
    def Get_beta(self):
        return self._beta
    def Get_N(self):
        return self._N
    def Get_history(self):
        return self._history
    def Get_T(self):
        return self._t
    def Get_alpha(self):
        return self._alpha
    def Get_mu(self):
        return self._mu


    #################################
    ##########    Setters    ########
    #################################
    def Set_p(self,value):
        self._p=value
    def Set_beta(self,value):
        self._beta=value
    def Set_N(self,value):
        self._N=value
    def Set_history(self,value):
        self._history=value
    def Set_T(self,value):
        self._t=value

    #################################
    ##########  Methods   ###########
    #################################


    def loglikelihood(self):
        """
        Returns the loglikelihood of a Hawkes process with exponential kernel
        computed with a linear time complexity
            
        params   -- parameter tuple (p,beta) of the Hawkes process
        history  -- (n,2) numpy array containing marked time points (t_i,m_i)  
        t        -- current time (i.e end of observation window)
        """
        
        p,beta,history,t = self.Get_p(),self.Get_beta(),self.Get_history(),self.Get_T()
    
        
        if p <= 0 or p >= 1 or beta <= 0.: return -np.inf

        n = len(history)
        tis = history[:,0]
        mis = history[:,1]
        
        LL = (n-1) * np.log(p * beta)
        logA = -np.inf
        prev_ti, prev_mi = history[0]
        
        i = 0
        for ti,mi in history[1:]:
            if(prev_mi + np.exp(logA) <= 0):
                print("Bad value", prev_mi + np.exp(logA))
            
            logA = np.log(prev_mi + np.exp(logA)) - beta * (ti - prev_ti)
            LL += logA
            prev_ti,prev_mi = ti,mi
            i += 1
            
        logA = np.log(prev_mi + np.exp(logA)) - beta * (t - prev_ti)
        LL -= p * (np.sum(mis) - np.exp(logA))

        return LL
        
    def compute_MAP(self,
                prior_params = [ 0.02, 0.0002, 0.01, 0.001, -0.1],
                max_n_star = 1, display=False):
        """
        Returns the pair of the estimated logdensity of a posteriori and parameters (as a numpy array)

        history      -- (n,2) numpy array containing marked time points (t_i,m_i)  
        t            -- current time (i.e end of observation window)
        alpha        -- power parameter of the power-law mark distribution
        mu           -- min value parameter of the power-law mark distribution
        prior_params -- list (mu_p, mu_beta, sig_p, sig_beta, corr) of hyper parameters of the prior
                    -- where:
                    --   mu_p:     is the prior mean value of p
                    --   mu_beta:  is the prior mean value of beta
                    --   sig_p:    is the prior standard deviation of p
                    --   sig_beta: is the prior standard deviation of beta
                    --   corr:     is the correlation coefficient between p and beta
        max_n_star   -- maximum authorized value of the branching factor (defines the upper bound of p)
        display      -- verbose flag to display optimization iterations (see 'disp' options of optim.optimize)
        """
        history,t = self.Get_history(),self.Get_T()
        alpha,mu=self.Get_alpha(),self.Get_mu()
        # Compute prior moments
        mu_p, mu_beta, sig_p, sig_beta, corr = prior_params
        sample_mean = np.array([mu_p, mu_beta])
        cov_p_beta = corr * sig_p * sig_beta
        Q = np.array([[sig_p ** 2, cov_p_beta], [cov_p_beta, sig_beta **2]])
        
        # Apply method of moments
        cov_prior = np.log(Q / sample_mean.reshape((-1,1)) / sample_mean.reshape((1,-1)) + 1)
        mean_prior = np.log(sample_mean) - np.diag(cov_prior) / 2.

        # Compute the covariance inverse (precision matrix) once for all
        inv_cov_prior = np.asmatrix(cov_prior).I

        # Define the target function to minimize as minus the log of the a posteriori density    
        def target(params):
            log_params = np.log(params)
            
            if np.any(np.isnan(log_params)):
                return np.inf
            else:
                dparams = np.asmatrix(log_params - mean_prior)
                prior_term = float(- 1/2 * dparams * inv_cov_prior * dparams.T)
                self.Set_beta(params[1])
                self.Set_p(params[0])
                logLL = self.loglikelihood()
                return - (prior_term + logLL)
        
        EM = mu * (alpha - 1) / (alpha - 2)
        eps = 1.E-8

        # Set realistic bounds on p and beta
        p_min, p_max       = eps, max_n_star/EM - eps
        beta_min, beta_max = 1/(3600. * 24 * 10), 1/(60. * 1)
        
        # Define the bounds on p (first column) and beta (second column)
        bounds = optim.Bounds(
            np.array([p_min, beta_min]),
            np.array([p_max, beta_max])
        )
        
        # Run the optimization
        res = optim.minimize(
            target, sample_mean,
            method='Powell',
            bounds=bounds,
            options={'xtol': 1e-8, 'disp': display}
        )
        # Returns the loglikelihood and found parameters
        return(-res.fun, res.x) ## res.x contains p et beta
