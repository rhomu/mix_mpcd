#
# simple plotting
#

import re
import sys
import struct
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as ani

# ------------------------------------------------------------------------------

# check args and open param file
if len(sys.argv)<2:
    print "Error: please provide a output directory."
    exit(1)

outdir = sys.argv[1]
dat = open(outdir + '/parameters').read()

# load params of the form 'name = value'
def load(name, dat):
    m = re.search(r'\s+{}\s+=\s+(.*)'.format(name), dat)
    return m.group(1)

L      = eval(load('L', dat))
dens   = eval(load('dens', dat))
ntypes = int(load('ntypes', dat))
nsteps = int(load('nsteps', dat))
ninfo  = int(load('ninfo', dat))

cmap = 'viridis'
if ntypes==3:
    cmap = 'jet'

# ------------------------------------------------------------------------------
# plot

def get_density(frame, t = -1):
    suffix = '' if t==-1 else '.{}'.format(t)
    fn = '{0}/frame{1}.density{2}.dat'.format(outdir ,str(frame*ninfo), suffix)
    dat = open(fn, mode='rw').read()
    arr = np.array(struct.unpack('i'*(len(dat)//4), dat))
    return arr.reshape((L[0], L[1]))

def plot_frame(frame):
    fig.clf()
    plt.subplots_adjust(wspace=.5)

    d = np.zeros(L)
    h = np.zeros(L)
    for i in range(ntypes):
        dens = get_density(frame, t=i)
        d += dens
        h += dens.astype(float)*(i+.5)

    h = (h - .5)/(ntypes - .5)

    plt.subplot(1, 2, 1)
    cax = plt.imshow(h/d, origin='lower', cmap=cmap, vmin=0, vmax=1)
    #cbar = fig.colorbar(cax, fraction=0.046, pad=0.04)

    plt.subplot(1, 2, 2)
    cax = plt.imshow(d, origin='lower')
    cbar = fig.colorbar(cax, fraction=0.046, pad=0.04)

fig = plt.figure(figsize=(12,6))
an = ani.FuncAnimation(fig, plot_frame,
                       frames = np.arange(0, nsteps//ninfo),
                       interval = 100, blit = False)
plt.show(); exit(0)
writer = ani.writers['ffmpeg'](fps=10, bitrate=1800)
an.save('movie.mp4', writer=writer)
