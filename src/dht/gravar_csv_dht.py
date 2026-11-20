#!/usr/bin/env python3
"""
CP2 - Grava clipes rotulados do DHT11 (temperatura + umidade) em CSV para o Edge Impulse.

Uso:
  pip install pyserial

  # 1) ambiente normal (sem isqueiro) - grave estes primeiro
  python gravar_csv_dht.py --porta COM3 --label normal --clips 20 --duracao 30

  # 2) calor: acenda o isqueiro a ~5-10 cm do sensor DEPOIS do ENTER.
  #    --pre 8 mostra as leituras ao vivo por 8 s (nao salva) para a temperatura subir;
  #    depois grava 30 s com o isqueiro ainda aceso. --descanso espera o sensor esfriar.
  python gravar_csv_dht.py --porta COM3 --label calor --clips 20 --duracao 30 --pre 8 --descanso 60

Gera dados/normal.01.csv, dados/calor.01.csv, ... (o Edge Impulse usa o texto antes do
primeiro ponto como rotulo). Feche o Monitor Serial da Arduino IDE antes de rodar.
"""
import argparse
import csv
import os
import re
import time

import serial


def proximo_indice(pasta, label):
    padrao = re.compile(rf"^{re.escape(label)}\.(\d+)\.csv$")
    nums = [int(m.group(1)) for n in os.listdir(pasta) if (m := padrao.match(n))]
    return max(nums, default=0) + 1


def ler_linha(ser):
    """Retorna (ms, temp, umid) ou None se a linha for invalida."""
    partes = ser.readline().decode(errors="ignore").strip().split(",")
    if len(partes) != 3:
        return None
    try:
        return int(partes[0]), float(partes[1]), float(partes[2])
    except ValueError:
        return None


def pre_aquecimento(ser, segundos):
    """Mostra leituras ao vivo (sem salvar) para o operador ver a temperatura subir."""
    fim = time.time() + segundos
    while time.time() < fim:
        r = ler_linha(ser)
        if r:
            print(f"    ao vivo: T={r[1]:.1f} C  H={r[2]:.0f} %")


def gravar_clipe(ser, duracao):
    ser.reset_input_buffer()
    linhas, t0 = [], None
    fim = time.time() + duracao
    while time.time() < fim:
        r = ler_linha(ser)
        if not r:
            continue
        ms, t, h = r
        if t0 is None:
            t0 = ms
        linhas.append((ms - t0, t, h))
    return linhas


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--porta", required=True, help="ex.: COM3 ou /dev/ttyUSB0")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--label", required=True, help="classe: normal | calor")
    ap.add_argument("--clips", type=int, default=10)
    ap.add_argument("--duracao", type=float, default=30.0, help="segundos gravados por clipe")
    ap.add_argument("--pre", type=float, default=0.0, help="segundos de pre-aquecimento (nao gravados)")
    ap.add_argument("--descanso", type=float, default=0.0, help="espera apos cada clipe (sensor esfriar)")
    ap.add_argument("--pasta", default="dados")
    args = ap.parse_args()

    os.makedirs(args.pasta, exist_ok=True)
    ser = serial.Serial(args.porta, args.baud, timeout=2)
    time.sleep(2)  # o ESP32 reinicia ao abrir a serial

    esperado = int(args.duracao)  # 1 Hz
    for i in range(args.clips):
        input(f"[{i + 1}/{args.clips}] classe '{args.label}' - ENTER para comecar... ")
        if args.pre > 0:
            print(f"  pre-aquecimento de {args.pre:.0f}s (nao salvo)...")
            pre_aquecimento(ser, args.pre)
        print(f"  gravando {args.duracao:.0f}s...")
        linhas = gravar_clipe(ser, args.duracao)
        if len(linhas) < esperado * 0.9:
            print(f"  aviso: so {len(linhas)} amostras (esperado ~{esperado}). Clipe descartado.")
        else:
            idx = proximo_indice(args.pasta, args.label)
            caminho = os.path.join(args.pasta, f"{args.label}.{idx:02d}.csv")
            with open(caminho, "w", newline="") as f:
                w = csv.writer(f)
                w.writerow(["timestamp", "temperatura", "umidade"])
                w.writerows(linhas)
            ts = [l[1] for l in linhas]
            hs = [l[2] for l in linhas]
            print(f"  salvo {caminho}: {len(linhas)} amostras | "
                  f"T {min(ts):.1f}-{max(ts):.1f} C | H {min(hs):.0f}-{max(hs):.0f} %")
        if args.descanso > 0 and i < args.clips - 1:
            print(f"  esfriando {args.descanso:.0f}s (apague o isqueiro)...")
            time.sleep(args.descanso)

    ser.close()


if __name__ == "__main__":
    main()
