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
# Deterministic city ambience and mechanical impacts; no external recordings.
def wind(t, d):
    return sum(math.sin(2*math.pi*f*t + f*.3) / (i+1) for i, f in enumerate([23, 37, 59, 83, 127, 179, 263, 389])) * .045 * (.7+.3*math.sin(2*math.pi*t/d))

write('CityWind', 6, wind)
write('CityHum', 4, lambda t,d: (math.sin(2*math.pi*48*t)*.18+math.sin(2*math.pi*96*t)*.035+math.sin(2*math.pi*144*t)*.012)*(.8+.2*math.cos(2*math.pi*t/d)))
write('Collapse', 2.8, lambda t,d: (math.sin(2*math.pi*(52*t-5*t*t))*.37+rng.uniform(-1,1)*.20+math.sin(2*math.pi*133*t)*.06)*min(1,t*25)*math.exp(-t*1.4)*(1-t/d))
write('Hydraulics', 2.6, lambda t,d: (math.sin(2*math.pi*(155*t+12*t*t))*.15+math.sin(2*math.pi*78*t)*.12+rng.uniform(-1,1)*.045)*math.sin(math.pi*t/d))
write('MechStep', .85, lambda t,d: (math.sin(2*math.pi*(62*t-18*t*t))*.5+rng.uniform(-1,1)*.1)*min(1,t*180)*math.exp(-t*8)*(1-t/d))
# Destruction and district ambience. Appended after the earlier effects so their random streams are unchanged.
grains = sorted((rng.uniform(0, 1.05), rng.uniform(.25, 1), rng.uniform(40, 140)) for _ in range(70))
def crumble(t, d):
    value = math.sin(2*math.pi*(48*t-9*t*t))*.3*math.exp(-t*5)
    for start, gain, decay in grains:
        age = t-start
        if 0 <= age < .06:
            value += rng.uniform(-1, 1)*gain*.34*math.exp(-age*decay)
    return value*(1-t/d)

write('Crumble', 1.3, crumble)
wash = [0.0]
def waves(t, d):
    swell = math.sin(math.pi*t/d*2)**2
    wash[0] += (rng.uniform(-1, 1)-wash[0])*.045
    return (wash[0]*1.9*(.25+.75*swell) + math.sin(2*math.pi*31*t)*.03*swell)*.8

write('Waves', 8, waves)
def cicadas(t, d):
    pulse = max(0.0, math.sin(2*math.pi*7.5*t))**3*(.6+.4*math.sin(2*math.pi*t/d))
    carrier = math.sin(2*math.pi*4200*t)*.5+math.sin(2*math.pi*5300*t+1.3)*.35+rng.uniform(-1, 1)*.22
    return carrier*pulse*(.55+.45*math.sin(2*math.pi*55*t))*.2

write('Cicadas', 4, cicadas)
print('Generated seventeen original sound effects and ambience loops.')
