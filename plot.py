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

dens   = eval(load('dens', dat))
ntypes = int(load('ntypes', dat))
nboxes = int(load('nboxes', dat))
nsteps = int(load('nsteps', dat))
ninfo  = int(load('ninfo', dat))

# ------------------------------------------------------------------------------
# plot

def get_density(frame, t = -1):
    suffix = '' if t==-1 else '.{}'.format(t)
    fn = '{0}/frame{1}.density{2}.dat'.format(outdir ,str(frame*ninfo), suffix)
    dat = open(fn, mode='rw').read()
    arr = np.array(struct.unpack('i'*(len(dat)//4), dat))
    return arr.reshape((nboxes, nboxes))


def plot_frame(frame):
    fig.clf()
    dens0 = get_density(frame, t=0)
    dens1 = get_density(frame, t=1)
    phi = (dens0 - dens1).astype(float)/(dens0 + dens1)
    cax = plt.imshow(phi, origin='lower', vmin=-1, vmax=1)
    #cax = plt.imshow(dens1, origin='lower')
    cbar = fig.colorbar(cax)

fig = plt.figure()
an = ani.FuncAnimation(fig, plot_frame,
                       frames = np.arange(0, nsteps//ninfo),
                       interval = 200, blit = False)
plt.show()