"""Deterministic original synth effects, with no third-party samples."""
import math
import random
import struct
import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parent / 'GeneratedAudio'
ROOT.mkdir(exist_ok=True)
RATE = 22050
rng = random.Random(1701)

def write(name, duration, sample):
    with wave.open(str(ROOT / f'{name}.wav'), 'wb') as out:
        out.setparams((1, 2, RATE, 0, 'NONE', 'not compressed'))
        values = []
        for i in range(int(duration * RATE)):
            t = i / RATE
            value = max(-1, min(1, sample(t, duration)))
            values.append(struct.pack('<h', int(value * 25000)))
        out.writeframes(b''.join(values))

write('Impact', .65, lambda t,d: (math.sin(2*math.pi*(65*t-22*t*t))*.65 + rng.uniform(-1,1)*.3)*math.exp(-t*9))
write('Lance', .55, lambda t,d: (math.sin(2*math.pi*(380*t+900*t*t))*.3+rng.uniform(-1,1)*.15)*math.sin(math.pi*t/d)**2)
write('Alarm', .9, lambda t,d: math.sin(2*math.pi*440*t)*.22*(.5+.5*math.sin(2*math.pi*7*t))*math.sin(math.pi*t/d))
write('Breach', 1.1, lambda t,d: (math.sin(2*math.pi*(140*t-50*t*t))*.45+rng.uniform(-1,1)*.15)*math.exp(-t*3))
write('Launch', 1.8, lambda t,d: (math.sin(2*math.pi*110*t)+math.sin(2*math.pi*165*t)+math.sin(2*math.pi*220*t))*.13*math.sin(math.pi*t/d))
write('Breath', 4.5, lambda t,d: rng.uniform(-1,1)*.11*max(0,math.sin(2*math.pi*t/d))**2 + math.sin(2*math.pi*42*t)*.035*math.sin(math.pi*t/d))
write('Rumble', 5, lambda t,d: (math.sin(2*math.pi*38*t)*.22+math.sin(2*math.pi*57*t)*.08+rng.uniform(-1,1)*.025)*math.sin(math.pi*t/d))
write('Heartbeat', 3, lambda t,d: math.sin(2*math.pi*52*t)*.28*(math.exp(-((t%1.0)/.09)**2)+.65*math.exp(-(((t%1.0)-.22)/.07)**2)))
write('Roar', 2, lambda t,d: (math.sin(2*math.pi*(90*t-18*t*t))*.23+math.sin(2*math.pi*131*t)*.12+rng.uniform(-1,1)*.16)*math.sin(math.pi*t/d)**.7)
print('Generated nine original sound effects.')
