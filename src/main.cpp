// Simple mpcd simulation for phase separation
// Romain Mueller (c) 2017

#include "header.hpp"
#include "random.hpp"
#include "tools.hpp"

using namespace std;
namespace opt = boost::program_options;

// ===================================================================
// parameters

// time step
float tau = 2e-2;
// number of boxes in each dimension
vector<int> L;
// total number of boxes
int nboxes;
// number of particles
vector<int> dens = {10};
// number of particles of each type
vector<int> npart;
// total number of particles
int ntot;
// number of types
int ntypes = 1;
// interaction parameters
vector<float> kappa;
// total number of time steps
int nsteps = 100000;
// number of steps between analyses
int ninfo = 100;
// verbosity level
int verbose = 1;
// width of the output
int width = 50;
// intput/output directory
string directory;

// =============================================================================
// components

// single particle
struct particle
{
  // current velocity and position
  vec x, v;
  // particle type
  int t;

  // one step forward
  void stream()
  {
    for(int i=0; i<dim; ++i)
      x[i] = modu(x[i] + tau*v[i], L[i]);
  }
};

// box used for collision operation
struct box
{
  // location of the center of the box
  const vec x;
  // number of particles of each type
  vector<int> n;
  // ptrs to particles
  vector<particle*> particles;
  // mean velocity and noise
  vec vcm, ncm;
  // total ekin
  float ekin;

  box(const vec& x)
    : x(x)
  {
    n.resize(ntypes, 0);
  }

  // the collision operator
  void collision(const vec& shift)
  {
    // compute box properties
    vector<vec> grad (ntypes+1, {{0,0}});
    //vector<vec> grad2(ntypes, {{0,0}});
    vector<int> ngrad(ntypes, 0);
    //int ngrad = 0;
    vcm = ncm = {{ 0, 0 }};
    for(const auto& p : particles)
    {
      // gradient
      const vec d = modu(p->x + shift, L) - x;
      if(d.sq()<.25f)
      {
        grad[p->t] += 12.f*d;
        //grad2[p->t] += 480.f*d*(26.f*d*d + 6.f - 35.f*d.times(d));
        ++ngrad[p->t];
      }

      // noise
      vcm  += p->v;
      p->v  = {{ random_normal(), random_normal() }};
      ncm  += p->v;
    }

    normalize(vcm, particles.size());
    normalize(ncm, particles.size());
    for(int t=0; t<ntypes; ++t)
    {
      normalize(grad[t], ngrad[t]);
      grad[ntypes] += grad[t]; // total grad
    }

    // perform collision
    ekin = 0;
    vec vcm_corr = {{0,0}};
    for(const auto& p : particles)
    {
      p->v += vcm - ncm;

      for(int t=0; t<ntypes; ++t)
        p->v += kappa[p->t]*kappa[t]/n[p->t]/particles.size()
                //*(grad[p->t]-grad[t]-grad[ntypes]*(n[p->t]-n[t])/particles.size());
                *grad[p->t];

      vcm_corr += p->v;
      ekin += p->v.sq()/2;
    }

    // correct for momentum conservation
    normalize(vcm_corr, particles.size());
    for(const auto& p: particles)
      p->v -= vcm_corr - vcm;
  }

  // add particles
  void add(particle* p)
  {
    particles.push_back(p);
    ++n[p->t];
  }

  // empty particle list
  void clear()
  {
    particles.clear();
    fill(begin(n), end(n), 0);
  }
};

// set of boxes
class grid
{
  // all boxes
  vector<box> boxes;
  // the current grid shift
  vec shift;

public:
  grid()
  {
    for(int i=0; i<L[0]; ++i)
      for(int j=0; j<L[1]; ++j)
        boxes.push_back(vec {{i+.5f, j+.5f}});
  }

  void set_shift(const vec& s)
  {
    shift = s;
  }

  void bucket(particle* p)
  {
    // construct index from position
    int index = 0;
    for(int i=0; i<dim; ++i)
      index = L[i]*index + int(modu(p->x[i]+shift[i], L[i]));

    // add to corresponding box
    boxes[index].add(p);
  }

  void clear() { for(auto& b : boxes) b.clear(); }
  void collision() { for(auto& b : boxes) b.collision(shift); }

  // iterators over the boxes
  vector<box>::iterator begin() { return boxes.begin(); }
  vector<box>::iterator end() { return boxes.end(); }
  vector<box>::const_iterator begin() const { return boxes.begin(); }
  vector<box>::const_iterator end() const { return boxes.end(); }
};

// =============================================================================
// input/output

// Declare and parse all program options
void parse_options(int ac, char **av)
{
  // we use strings to retreive the arrays
  string kget, dget, Lget, gget;

  // options allowed only in the command line
  opt::options_description generic("Generic options");
  generic.add_options()
    ("help,h", "produce help message")
    ("directory", opt::value<string>(&directory), "input/output directory")
    ("verbose", opt::value<int>(&verbose)->implicit_value(2), "verbosity level (0, 1, 2, default=1)");

  // options allowed only in the config file
  opt::options_description config("Configuration options");
  config.add_options()
    ("L", opt::value<string>(&Lget), "number of boxes")
    ("dens", opt::value<string>(&dget), "density of particles")
    ("ntypes", opt::value<int>(&ntypes), "number of different types")
    ("nsteps", opt::value<int>(&nsteps), "total number of time steps")
    ("ninfo", opt::value<int>(&ninfo), "number of time steps between two analyses")
    ("tau", opt::value<float>(&tau), "time step")
    ("kappa", opt::value<string>(&kget), "interaction parameters");

  // command line options
  opt::options_description cmdline_options;
  cmdline_options.add(generic);
  opt::options_description config_file_options;
  config_file_options.add(config);

  // first unnamed argument is the input directory
  opt::positional_options_description p;
  p.add("directory", 1);

  // the variables map
  opt::variables_map vm;

  // parse first the cmd line
  opt::store(
      opt::command_line_parser(ac, av)
      .options(cmdline_options)
      .positional(p)
      .run(), vm);
  opt::notify(vm);

  // print help msg
  if(vm.count("help"))
  {
    cout << cmdline_options << endl;
    exit(0);
  }

  // parse input file (values are not erased, such that cmd line args are 'stronger')
  if(!directory.empty())
  {
    if(directory.back()!='/') directory += '/';
    const string inputname = directory+"parameters";
    std::fstream file(inputname.c_str(), std::fstream::in);
    if(!file.good())
      throw inline_str("error while opening runcard file ", inputname);
    opt::store(opt::parse_config_file(file, config_file_options), vm);
    opt::notify(vm);
  }
  else throw inline_str("please specify an input/output directory");

  // get system size
  L = get_ints_from_string(Lget);
  if(L.size()!=dim) throw inline_str("wrong format for system size");

  // total number of boxes
  nboxes = accumulate(begin(L), end(L), 1, multiplies<int>());

  // get number of particles array
  dens = get_ints_from_string(dget);
  if(dens.size()!=size_t(ntypes)) throw inline_str("wrong number of densities");

  // get interaction params
  kappa = get_floats_from_string(kget);
  if(kappa.size()!=size_t(ntypes)) throw inline_str("wrong number of interaction parameters");

  // total number of particles of each specie
  npart.reserve(ntypes);
  for(int t=0; t<ntypes; ++t)
    npart.push_back(nboxes*dens[t]);
  // total number of particles
  ntot = accumulate(begin(npart), end(npart), 0.f);

  /*
  // dirty conversion to interaction matrix...
  auto values = get_floats_from_string(Mget);
  // ... check
  if(values.size()!=ntypes*ntypes)
    throw inline_str("wrong number of elements in interaction matrix");
  // ... store
  M.resize(ntypes);
  for(int i=0; i<ntypes; ++i)
    for(int j=0; j<ntypes; ++j)
      M[i].push_back(values[ntypes*i + j]);
  */

  // print the simulation parameters
  if(verbose)
  {
    cout << "Run parameters" << endl;
    cout << string(width, '=') << endl;
    print_vm(vm, width);
  }
}

// write the current state
void write_frame(int t,
                 const grid& boxes,
                 const vector<particle>& particles)
{
  // helper to construct file names
  const auto fname = [&](const string& s) {
    return inline_str(directory, "/frame", t, ".", s, ".dat");
  };

  // open files
  vector<ofstream> files(3 + ntypes);
  files[0].open(fname("density"), ios::out | ios::binary);
  files[1].open(fname("velocity"), ios::out | ios::binary);
  files[2].open(fname("energy"), ios::out | ios::binary);
  for(size_t i=3; i<files.size(); ++i)
    files[i].open(fname(inline_str("density.", i-3)), ios::out | ios::binary);
  for(auto& f : files) if(not f.good())
    throw inline_str("unable to open files for writing");

  // write data
  for(auto& b : boxes)
  {
    write_binary(files[0], b.particles.size());
    write_binary(files[1], b.vcm);
    write_binary(files[2], b.ekin);
    for(int i=0; i<ntypes; ++i) write_binary(files[3+i], b.n[i]);
  }

  // close all files
  for(auto& f : files) f.close();

  // write particles
  /*
  {
    ofstream file(fname("particles"), ios::out | ios::binary);

    for(auto& p : particles)
      file << p.x[0] << p.x[1] << p.v[0] << p.v[1] << p.t;

    file.close();
  }
  */
}

// =============================================================================
// simulation

void simulate()
{
  // ---------------------------------------------------------------------------
  // init

  // create the particles at random
  vector<particle> particles;
  particles.reserve(ntot);
  for(int t=0; t<ntypes; ++t)
    for(int i=0; i<L[0]; ++i)
      for(int j=0; j<L[1]; ++j)
        for(int k=0; k<dens[t]; ++k)
          particles.push_back({
            {{ random_real(float(i), float(i+1)),
               random_real(float(j), float(j+1)) }},
            {{0,0}},
            t});

  // the grid
  grid boxes;

  // ---------------------------------------------------------------------------
  // the algo

  for(int time=0; time<=nsteps; ++time)
  {
    // the random shift
    boxes.set_shift(vec {{ random_real(), random_real() }});

    // simulate particles
    for(auto& p : particles)
    {
      // stream
      p.stream();
      // bucket
      boxes.bucket(&p);
    }

    // store step
    if(time%ninfo == 0)
    {
      // print status
      cout << "t = " << time <<  " / " << nsteps << endl;

      // collision
      boxes.collision();
      // rebucket with zero shift before storing
      boxes.clear();
      boxes.set_shift({{0,0}});
      for(auto& p : particles)
        boxes.bucket(&p);
      // store and clear
      write_frame(time, boxes, particles);
      boxes.clear();
    }
    // normal step
    else
    {
      boxes.collision();
      boxes.clear();
    }
  }
}

// =============================================================================

int main(int argc, char *argv[])
{
  cout << "MPCD : Simple MPCD simulation for phase separation" << endl
       << "       Romain Mueller (c) 2017" << endl
       << string(width, '=') << endl;

  try
  {
    // ========================================
    // Initialization

    // parse options (may throw)
    parse_options(argc, argv);

    if(verbose) cout << endl << "Initialization"
                     << endl << string(width, '=')
                     << endl;

    init_random();

    // ========================================
    // Running

    if(verbose) cout << endl << "Run" << endl << string(width, '=') << endl;

    // do the job
    simulate();
  }
  // error messages
  catch(const string& s) {
    cerr << argv[0] << ": " << s << endl;
    return 1;
  }
  // all the rest (mainly from boost)
  catch(const exception& e) {
    cerr << argv[0] << ": " << e.what() << endl;
    return 1;
  }

  return 0;
}
