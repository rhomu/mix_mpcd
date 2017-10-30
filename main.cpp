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
double tau = 2e-2;
// number of boxes in each dimension
vector<int> L;
// number of particles
vector<int> dens = {10};
// total number of particles
int ntot;
// number of types
int ntypes = 1;
// interaction matrix
vector<vector<double>> M;
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
// external force
vec f({ 0, 0 });

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
  // total number of particles
  int ntot = 0;
  // ptrs to particles
  vector<particle*> particles;
  // mean velocity
  vec vcm;
  // total ekin
  double ekin;

  box(const vec& x)
    : x(x)
  {
    n.resize(ntypes, 0);
  }

  // the collision operator
  void collision(const vec& shift)
  {
    // total number of particles
    if(ntot==0) return;

    // compute box properties
    vector<vec> ncm(ntypes, {{0,0}}),
                tcm(ntypes, {{0,0}}),
                grad {{0,0}};
    vcm = {{ 0, 0 }};
    for(auto& p : particles)
    {
      // gradient
      const vec d = modu(p->x + shift, L) - x;
      if(d.sq()<1.) grad[p->t] += 12.*d;
      tcm[p->t]  += p->v;
      vcm        += p->v;
      p->v        = random_vec(normal_distribution<>(0., 1.));
      ncm[p->t]  += p->v;
    }

    vcm /= ntot;

    // perform collision
    ekin = 0;
    for(auto& p : particles)
    {
      p->v += (tcm[p->t] - ncm[p->t])/n[p->t];
      for(int t=0; t<ntypes; ++t)
        p->v += M[p->t][t]*2*n[t]/pow(n[p->t]+n[t], 2)*(grad[p->t] - grad[t]);
      ekin += p->v.sq()/2.;
    }
  }

  // add particles
  void add(particle* p)
  {
    particles.push_back(p);
    ++n[p->t];
    ++ntot;
  }

  // empty particle list
  void clear()
  {
    particles.clear();
    ntot = 0;
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
        boxes.push_back(vec {{i+.5, j+.5}});
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
  string Mget, dget, Lget;

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
    ("tau", opt::value<double>(&tau), "time step")
    ("M", opt::value<string>(&Mget), "interaction matrix");

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

  // get number of particles array
  dens = get_ints_from_string(dget);
  if(dens.size()!=ntypes) throw inline_str("wrong number of densities");
  ntot = accumulate(begin(L), end(L), 1., std::multiplies<double>())*
         accumulate(begin(dens), end(dens), 0.);

  // dirty conversion to interaction matrix...
  auto values = get_doubles_from_string(Mget);
  // ... check
  if(values.size()!=ntypes*ntypes)
    throw inline_str("wrong number of elements in interaction matrix");
  // ... store
  M.resize(ntypes);
  for(int i=0; i<ntypes; ++i)
    for(int j=0; j<ntypes; ++j)
      M[i].push_back(values[ntypes*i + j]);

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
  for(int i=3; i<files.size(); ++i)
    files[i].open(fname(inline_str("density.", i-3)), ios::out | ios::binary);
  for(auto& f : files) if(not f.good())
    throw inline_str("unable to open files for writing");

  // write data
  for(auto& b : boxes)
  {
    write_binary(files[0], &b.ntot);
    write_binary(files[1], &b.vcm);
    write_binary(files[2], &b.ekin);
    for(int i=0; i<ntypes; ++i) write_binary(files[3+i], &b.n[i]);
  }

  // close all files
  for(auto& f : files) f.close();

  return;

  // write particles
  {
    ofstream file(fname("particles"), ios::out | ios::binary);

    for(auto& p : particles)
      file << p.x[0] << p.x[1] << p.v[0] << p.v[1] << p.t;

    file.close();
  }
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
            {{ random_real(double(i), double(i+1)),
               random_real(double(j), double(j+1)) }},
            {{0,0}},
            t});

  // the grid
  grid boxes;

  // ---------------------------------------------------------------------------
  // the algo

  for(int t=0; t<=nsteps; ++t)
  {
    // the random shift
    boxes.set_shift(random_vec(0, 1));

    // simulate particles
    for(auto& p : particles)
    {
      // stream
      p.stream();
      // bucket particles
      boxes.bucket(&p);
    }

    // store step
    if(t%ninfo == 0)
    {
      // print status
      cout << "t = " << t <<  " / " << nsteps << endl;

      // collision
      boxes.collision();
      // rebucket with zero shift before storing
      boxes.clear();
      boxes.set_shift({{0,0}});
      for(auto& p : particles)
        boxes.bucket(&p);
      // store and clear
      write_frame(t, boxes, particles);
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
