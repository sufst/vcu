import numpy as np
import sympy as sym
import matplotlib.pyplot as plt
from scipy import interpolate
import csv


x = np.linspace(0, 1, 11)
y = [0, 0.78, 0.74, 0.69, 0.64, 0.59, 0.54, 0.5, 0.45, 0.42, 0.38]

spline = interpolate.InterpolatedUnivariateSpline(x, y)

#COORDINATE CONVENTION: CG is at (0, 0) x positive is towards rear of car
# y positive is up

#AERO
with open("aero-coords.csv") as csvfile:
    aero_coords = list(csv.reader(csvfile))

Fw_x, Rw_x, Uf_x, Sw_x = sym.symbols("Fw_x Rw_x Uf_x Sw_x")
Fw_y, Rw_y, Uf_y, Sw_y = sym.symbols("Fw_y Rw_y Uf_y Sw_y")
fw_x, rw_x, uf_x, sw_x = aero_coords[1][1:-1]
fw_y, rw_y, uf_y, sw_y = aero_coords[2][1:-1]
aero_coords_x = np.array([float(val) for val in aero_coords[1][1:-1]])
aero_coords_y = np.array([float(val) for val in aero_coords[2][1:-1]])

with open("aero-forces.csv") as csvfile:
    aero_forces = list(csv.reader(csvfile))

sCls = np.array([float(val) for val in aero_forces[1][0:4]])
sCds = np.array([float(val) for val in aero_forces[1][5:]])
sCd = float(aero_forces[1][4])


#CONSTANTS
w_b, mu, w_r, ts, W_Izz = sym.symbols("W_b mu W_r t_s W_{I_{zz}}")
time_step_s = 0.001
wheel_base_m = 1.5 #W_b
cg_y = 0.4 #Cg_y
cg_x = 1 #From front wheels Cg_x
driven_torque_Nm = 300 #T_m
mass_kg = 217.5
mu = spline(1)
mu_dynamic = 0.7
wheel_radius_m = 7 * 2.54 * 0.01
W_Izz = 0.11
g = -9.81
aero_const = 0.5 * 1.225

#FORCES - Need updating to real values
R_f_x = - 1.0
R_r_x = 0.5
Fx_y = - (cg_y - wheel_base_m)

#ITERATION
time = 0
distance = 0 #End when distance = 75
acceleration = 0
velocity = 0




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
    moments = R_f * R_f_x + R_r * R_r_x + Fx * Fx_y + Vc ** 2 * aero_const * (np.sum(aero_coords_x * sCls) + np.sum(aero_coords_y * sCds))
    #moments = cg_x * R_f + (cg_y - w_r) * Fx - (w_b - cg_x) * R_r
    R_f_expr = sym.solve(moments, R_f)[0]

    #Balance vertical forces
    vertical = mass_kg * g - R_f_expr - R_r
    R_r_expr = sym.solve(vertical, R_r)[0]

    #Moments about 
    return R_r_expr

Rr_sym = rear_reaction_expr()
lam_Rr = sym.utilities.lambdify([Fx], Rr_sym, "numpy")


R_r = lam_Rr(0)
R_rs.append(R_r)
tractive_force = 0
i = 0
while distance < 75:
    i += 1
    traction_max = abs(mu * R_r)
    F_x = driven_torque_Nm / wheel_radius_m
    #LOOPING BEGINS HERE
    if traction_max < F_x:
        tractive_force = traction_max 
    else:
        tractive_force = F_x
        print("not traction limited after time = %f, i = %d" % (time, i))

    
    times.append(time)
    tractive_forces.append(tractive_force)

    #Account for drag
    tractive_force -= aero_const * sCd * Vc ** 2
    R_r = lam_Rr(tractive_force)
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
    mu = spline(slip_ratio)
    if mu <= 0:
        print("Failed because mu was 0 or negative")
        break
    W_accs.append(Vc)
    print(Vw)
    
print("Time: %f" % time)

plt.plot(times, W_accs)
