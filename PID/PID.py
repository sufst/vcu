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

slip_ratio_d = 0.1 # desired slip ratio
radius = 1 # radius of car tyre

# sensor values
velocity = [] # remains as m/s 
revolutions = [] # this needs to be converted into rad/s
errors = []
integral_e = 0
derivative_e = 0
proportional_e = 0
time_step = 0.01
time = 0
pids = []
"""
this code needs to be able to store error values so that integral can work?
"""

def slip_ratio_a(Vc, Vw):
    # this is the actual wheel slip calculated using above variables
    """
    make sure to double check the units of the velocity sensor and
    the revolutions sensors
    """
    slip_ratio = (Vw - Vc) / Vw
    
    return slip_ratio # returns an array of slip ratios 

def proportional():
    global proportional_e
    # this is the error between real and expected wheel slips
    slip_a = slip_ratio_a() # call actual slip ratio
    error = slip_a - slip_ratio_d
    proportional = error

def integral(tp):
    global integral_e
    # this is the sum of the errors over time
    """
    how would we code this?
    is it possible to sum from beginning to a certain variable in the array?
    """
    area = (errors[-2] + errors[-1]) * time_step / 2
    integral_e += area
    

def derivative(tp):
    # calculates the difference between two slip ratios
    derivative_e = (errors[-2] - errors[-1]) / (time_step)


prev_time = time()

while 1:
    proportional()
    integral()
    derivative()
    pid = kp * proportional_e + ki * integral_e + kd * derivative_e
    pids.append(pid)



"""
CHECK LIST:
    
once pid equation is complete we need to think about torque requests. this will
likely be a slimple if statement saying "if slip ratio is too high, reduce 
torque"

"""
