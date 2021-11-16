# -*- coding: utf-8 -*-
"""
Spyder Editor

This is a temporary script file.
"""

import numpy as np
import sympy as sym
import matplotlib.pyplot as plt
from scipy import interpolate
from time import sleep

x = np.linspace(0, 1, 11)
y = [0, 0.78, 0.74, 0.69, 0.64, 0.59, 0.54, 0.5, 0.45, 0.42, 0.38]

spline = interpolate.InterpolatedUnivariateSpline(x, y)




cg_x, cg_y, w_b, mass, mu, w_r, g, a_x, ts, W_Izz = sym.symbols("Cg_x Cg_y W_b m mu W_r g a_x t_s W_{I_{zz}}")
time_step_s = 0.001
wheel_base_m = 1.5 #W_b
cg_y_m = 0.6 #Cg_y
cg_x_m = 1 #From front wheels Cg_x
driven_torque_Nm = 300 #T_m
mass_kg = 217.5
mu_static = spline(1)
mu_dynamic = 0.7
wheel_radius_m = 7 * 2.54 * 0.01
time = 0
distance = 0 #End when distance = 75
acceleration = 0
velocity = 0
W_Izz = 0.11


#CONVENTION: Down Positive

#Basic definitions
ts = time_step_s
#Vehicle Speed and Wheel Speeds
#V_w is equivalent vehicle speed derived from angular velocity of driven wheels
Vc = 0
Vw_radps = 0
times = []
tractive_forces = []
R_rs = []
W_accs = []

#Forces
R_f = sym.symbols("R_f")
R_r, T_m, Fx = sym.symbols("R_r T_m Fx")

def rear_reaction_expr():
    #Balance moments about Cg

    moments = cg_x * R_f + (cg_y - w_r) * Fx - (w_b - cg_x) * R_r
    R_f_expr = sym.solve(moments, R_f)[0]

    #Balance vertical forces
    vertical = mass * g - R_f_expr - R_r
    R_r_expr = sym.solve(vertical, R_r)[0]

    #Moments about 
    return R_r_expr

Rr_sym = rear_reaction_expr()
lam_Rr = sym.utilities.lambdify([cg_x, cg_y, w_r, w_b, g, mass, Fx], Rr_sym, "numpy")

def rear_reaction(df):
    g = 9.81
    R_r = (cg_x_m*g*mass_kg + cg_y_m*df - wheel_radius_m*df) / wheel_base_m
    return R_r

R_r = lam_Rr(cg_x_m, cg_y_m, wheel_radius_m, wheel_base_m, 9.81, mass_kg, 0)
R_rs.append(R_r)
tractive_force = 0
i = 0
while distance < 75:
    i += 1
    traction_max = mu_static * R_r
    F_x = driven_torque_Nm / wheel_radius_m
    #LOOPING BEGINS HERE
    if traction_max < F_x:
        tractive_force = traction_max 
    else:
        tractive_force = F_x
        print("not traction limited after time = %f, i = %d" % (time, i))

    times.append(time)
    tractive_forces.append(tractive_force)

    R_r = lam_Rr(cg_x_m, cg_y_m, wheel_radius_m, wheel_base_m, 9.81, mass_kg, tractive_force)
    R_rs.append(R_r)
    Ac = tractive_force / mass_kg
    Vc += Ac * ts
    distance += (Vc * ts)
    W_acc = (driven_torque_Nm - tractive_force * wheel_radius_m) / W_Izz
    Vw_radps += W_acc * ts
    Vw = Vw_radps * wheel_radius_m
    time += ts
    slip_ratio = (Vw-Vc)/(Vw)
    if slip_ratio < 0:
        slip_ratio = 0.0001
    mu_static = spline(slip_ratio)
    W_accs.append(Vc)
    print(Vw)
    
print("Time: %f" % time)

plt.plot(times, W_accs)
