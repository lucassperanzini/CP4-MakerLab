#!/usr/bin/env python3
"""
Gera CSVs SINTETICOS no mesmo formato do gravar_csv_dht.py, so para testar o
pipeline do Edge Impulse (upload -> treino -> biblioteca) antes de ter dados reais.

Parametros baseados no seu ambiente:
  normal: T ~ 25.5 C (ruido pequeno), H entre 50 e 55 %
  calor : T sobe para 28-29 C e H cai para ~45 % (isqueiro perto do sensor)

ATENCAO: dados sinteticos NAO substituem a coleta real. O trabalho precisa ter dados
medidos pelo grupo; use este gerador apenas para validar o fluxo. Os arquivos saem com
o prefixo 'sint_' no nome do arquivo para nao se misturarem com os reais.

Uso:  python gerar_dados_sinteticos.py --pasta dados_sinteticos --clips 25
"""
import argparse
import csv
import os

import numpy as np


def clipe_normal(rng, n):
    t_base = rng.normal(25.5, 0.2)
    h_base = rng.uniform(50.5, 54.5)
    t = t_base + np.cumsum(rng.normal(0, 0.03, n)) + rng.normal(0, 0.12, n)
    h = h_base + np.cumsum(rng.normal(0, 0.08, n)) + rng.normal(0, 0.5, n)
    return np.round(t, 1), np.round(np.clip(h, 49, 56), 0)


def clipe_calor(rng, n):
    t_base = rng.normal(25.5, 0.2)
    h_base = rng.uniform(50.5, 54.5)
    t_alvo = rng.uniform(28.0, 29.2)
    h_alvo = rng.uniform(44.0, 46.5)
    tau = rng.uniform(3.0, 6.0)                 # constante de tempo da subida (s)
    # 40% dos clipes incluem a subida desde o inicio; 60% ja comecam aquecidos
    inicio = rng.uniform(0, 4) if rng.random() < 0.4 else -rng.uniform(10, 20)
    s = np.arange(n) - inicio
    frac = np.where(s > 0, 1 - np.exp(-s / tau), 1 - np.exp(10 / tau))
    frac = np.clip(frac, 0, 1)
    t = t_base + (t_alvo - t_base) * frac + rng.normal(0, 0.15, n)
    h = h_base + (h_alvo - h_base) * frac + rng.normal(0, 0.6, n)
    return np.round(t, 1), np.round(np.clip(h, 40, 56), 0)


def salvar(caminho, t, h):
    with open(caminho, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["timestamp", "temperatura", "umidade"])
        for i, (tt, hh) in enumerate(zip(t, h)):
            w.writerow([i * 1000, f"{tt:.1f}", f"{hh:.1f}"])


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--pasta", default="dados_sinteticos")
    ap.add_argument("--clips", type=int, default=25, help="clipes por classe")
    ap.add_argument("--duracao", type=int, default=30, help="segundos por clipe (1 Hz)")
    ap.add_argument("--seed", type=int, default=42)
    args = ap.parse_args()

    rng = np.random.default_rng(args.seed)
    os.makedirs(args.pasta, exist_ok=True)
    for i in range(1, args.clips + 1):
        salvar(os.path.join(args.pasta, f"normal.sint_{i:02d}.csv"), *clipe_normal(rng, args.duracao))
        salvar(os.path.join(args.pasta, f"calor.sint_{i:02d}.csv"), *clipe_calor(rng, args.duracao))
    print(f"{2 * args.clips} arquivos em {args.pasta}/ ({args.clips} normal + {args.clips} calor)")


if __name__ == "__main__":
    main()
