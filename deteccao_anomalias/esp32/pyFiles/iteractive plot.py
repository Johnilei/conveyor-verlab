import pandas as pd
import plotly.express as px
import numpy as np
from scipy import integrate
from scipy import signal
from scipy.fft import fft, fftfreq
from scipy.interpolate import interp1d
import plotly.graph_objects as go
from plotly.subplots import make_subplots

PATH_DIR = "C:/Users/romeu/OneDrive/Área de Trabalho/Trabalho/SIF-DI241794-SISTEMA-INERCIAL-COM-HADRWARE-FLEX-VEL/192.168.1.111_20250704_034011.csv" 
df = pd.read_csv(PATH_DIR)
PATH_DIR = "C:/Users/romeu/OneDrive/Área de Trabalho/Trabalho/SIF-DI241794-SISTEMA-INERCIAL-COM-HADRWARE-FLEX-VEL/192.168.1.110_20250612_162017.csv" 
df2 = pd.read_csv(PATH_DIR)
sensor_dest = ["192.168.1.111", "", ""]

if any(sensor in PATH_DIR for sensor in sensor_dest):
    df = df.rename(columns={
        'x_data': 'x_data',
        'y_data': 'x_gyro',
        'z_data': 'y_data',
        'x_gyro': 'y_gyro',
        'y_gyro': 'z_data',
        'z_gyro': 'z_gyro'
        # temp continua igual
    })

print(df.head())
# Parâmetros de conversão
RANGE_ACC = 16  # RANGE em G
RANGE_GYRO = 2000  # RANGE em dps
CONVERSION_FACTOR_ACC = ((2 * RANGE_ACC) / (2**15)) * 9.81  # Fator de conversão para m/s²
CONVERSION_FACTOR_GYRO = ((2 * RANGE_GYRO) / (2**15))  # Fator de conversão para °/s
DT = 0.02
BELT_SPEED = 6  # m/s
BELT_LENGTH = 1000  # m

# Parâmetros filtro passa-baixa
LOWPASS_CUTOFF_ACC = 5
LOWPASS_CUTOFF_GYRO = 5
FILTER_ORDER_ACC = 4
FILTER_ORDER_GYRO = 4

def filter_signal(data, cutoff=3, order=4, dt=DT):
    fs = 1 / dt
    sos = signal.butter(order, cutoff, btype='lowpass', fs=fs, output='sos')
    return signal.sosfilt(sos, data)

def animate_acceleration_trajectory(df, step=1):
    # Dados convertidos
    acc_x = np.array(df['x_data'], dtype=float) * CONVERSION_FACTOR_ACC
    acc_y = np.array(df['y_data'], dtype=float) * CONVERSION_FACTOR_ACC
    acc_z = np.array(df['z_data'], dtype=float) * CONVERSION_FACTOR_ACC

    acc_x_filtered = filter_signal(acc_z)
    acc_z_filtered = filter_signal(acc_y)
    acc_y_filtered = filter_signal(acc_x)

    indices = range(0, len(acc_x), step)
    line_length_scale = 0.2  # fator que representa o "tempo" de cada deslocamento

    # Ponto inicial
    x_traj = [0]
    y_traj = [0]
    z_traj = [0]

    frames = []

    for i in indices:
                # vetor atual
        vx = acc_x_filtered[i]
        vy = acc_y_filtered[i]
        vz = acc_z_filtered[i]

        norm = np.linalg.norm([vx, vy, vz])
        if norm == 0:
            norm = 1  # evita divisão por zero

        # deslocamento constante (norm = 1)
        dx = (vx / norm) * line_length_scale
        dy = (vy / norm) * line_length_scale
        dz = (vz / norm) * line_length_scale

        # novo ponto da trajetória
        x_traj.append(x_traj[-1] + dx)
        y_traj.append(y_traj[-1] + dy)
        z_traj.append(z_traj[-1] + dz)

        frames.append(go.Frame(
            data=[
                go.Cone(
                    x=[x_traj[-1]], y=[y_traj[-1]], z=[z_traj[-1]],
                    u=[vx], v=[vy], w=[vz],
                    anchor="tail",
                    sizemode="absolute", sizeref=1.5,
                    colorscale='Blues',
                    showscale=False
                ),
                go.Scatter3d(
                    x=x_traj,
                    y=y_traj,
                    z=z_traj,
                    mode='lines',
                    line=dict(color='orange', width=4),
                    showlegend=False
                )
            ],
            name=str(i)
        ))


    # Inicial
    ux0, uy0, uz0 = acc_x_filtered[0], acc_y_filtered[0], acc_z_filtered[0]
    initial_cone = go.Cone(
        x=[0], y=[0], z=[0],
        u=[ux0], v=[uy0], w=[uz0],
        anchor="tail",
        sizemode="absolute", sizeref=1.5,
        colorscale='Blues',
        showscale=False
    )
    initial_line = go.Scatter3d(
        x=[0],
        y=[0],
        z=[0],
        mode='lines',
        line=dict(color='orange', width=4),
        showlegend=False
    )

    layout=go.Layout(
    title="Trajetória acumulada da aceleração (como deslocamento)",
    scene=dict(
        xaxis=dict(range=[-500, 500], title="X"),
        yaxis=dict(range=[-500, 500], title="Y"),
        zaxis=dict(range=[-500, 500], title="Z"),
        aspectmode='cube'
    ),
    template='plotly_dark',
    width=700,
    height=700,
    updatemenus=[dict(
        type="buttons",
        showactive=False,
        y=1,
        x=1.2,
        xanchor="right",
        yanchor="top",
        buttons=[
            dict(label="▶️ Play", method="animate", args=[None, {"frame": {"duration": 1, "redraw": True}, "fromcurrent": True}]),
            dict(label="⏸ Pause", method="animate", args=[[None], {"frame": {"duration": 0, "redraw": False}, "mode": "immediate"}]),
            dict(label="🔄 Reset", method="animate", args=[None, {"frame": {"duration": 0, "redraw": True}, "mode": "immediate", "fromcurrent": False}])
        ]
    )]
)

    fig = go.Figure(
        data=[initial_cone, initial_line],
        frames=frames,
        layout=layout
    )

    fig.show()
animate_acceleration_trajectory(df, step=30)