# -*- coding: utf-8 -*-
"""
Spyder Editor

This is a temporary script file.
"""
import numpy as np
import scipy as sci
import matplotlib as plt

# variables to be changed
ki = 1 # integral coefficient
kp = 1 # proportional coefficient
kd = 1 # derivitive coefficient

slip_ratio_d = 1 # desired slip ratio
radius = 1 # radius of car tyre

# sensor values
velocity = [] # remains as m/s 
revolutions = [] # this needs to be converted into rad/s

"""
this code needs to be able to store error values so that integral can work?
"""

def slip_ratio_a():
    # this is the actual wheel slip calculated using above variables
    """
    make sure to double check the units of the velocity sensor and
    the revolutions sensors
    """
    for i in revolutions:
        revs_omega = []
        revs_omega = (revrevolutions*2*np.pi)/60
        wsa = []
        wsa = (revs_omega*radius)/velocity
    return wsa # returns an array of slip ratios

def proportional():
    # this is the error between real and expected wheel slips
    slip_a = slip_ratio_a() # call actual slip ratio
    for i in slip_a:
        error = []
        error = i - slip_ratio_d
    return error # returns an array of error values

def integral():
    # this is the sum of the errors over time
    """
    how would we code this?
    is it possible to sum from beginning to a certain variable in the array?
    """
    error = proportional() # call array or errors
    for i in error:
         sum(i [0:i]), "cant integrate as not a function?"
    return None

def derivitive():
    # calculates the difference between two slip ratios
    error = proportional() # call array of errors
    for i in error:
        der = []
        if i == 1: # when i is 1
            der = error[i] # as "0 - value"
        else:
            der = error[i]-error[i-1]
    return der

def PID():
    return sum(kp*proportional() + ki*integral() + kd*derivitive())


"""
CHECK LIST:
    
once pid equation is cmplete we need to think about torque requests. this will
likely be a slimple if statement saying "if slip ratio is too high, reduce 
torque"

"""
