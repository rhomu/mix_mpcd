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
double T = 1e-4;
// number of boxes in each dimension
int nboxes = 100;
// number of particles
vector<int> dens = {10};
// total number of particles
int ntot;
// number of types
int ntypes = 1;
// interaction matrix
vector<vector<double>> M;
// total number of time steps
int nsteps = 10000;
// number of steps between analyses
int ninfo = 1000;
// verbosity level
int verbose = 1;
// width of the output
int width = 50;
// intput/output directory
string directory;
// number of threads
int nthreads = 0;
// external force
vec f({ 0, 0 });

// =============================================================================
// components

// get box index from vector with positive coordinates
int get_box(const vec& v, int n)
{
  int index = 0;
  for(const auto& c : v)
    index = n*index + int(c*n);
  return index;
}

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
      x = collapse(x+v);
  }
};

struct box
{
  // location of the center of the box and size
  const vec x;
  // number of particles of each type
  vector<int> n;
  // total number of particles
  int ntot;
  // ptrs to particles
  vector<particle*> particles;
  // mean velocity and noise
  vec vcm;
  // total ekin
  double ekin;

  box(const vec& x)
    : x(x)
  {
    n.resize(ntypes);
  }

  // the collision operator
  void collision()
  {
    // total number of particles
    if(ntot==0) return;

    // compute box properties
    vector<vec> grad(ntypes, {{0,0}}),
                ncm(ntypes, {{0,0}}),
                tcm(ntypes, {{0,0}});
    vcm = {{ 0, 0 }};
    for(auto p : particles)
    {
      // gradient
      const vec q = p->x - x;
      grad[p->t] += 30.*q*(8.*q*q + 6.*q.times(q) - 3.);
      tcm[p->t]  += p->v;
      vcm        += p->v;
      p->v        = random_vec(normal_distribution<>(0., T));
      ncm[p->t]  += p->v;
    }
    vcm /= ntot;

    // perform collision
    ekin = 0;
    for(auto& p : particles)
    {
      p->v += vcm - ncm[p->t]/n[p->t];
      for(int t=0; t<ntypes; ++t)
        if(n[t]>0) p->v += M[p->t][t]*grad[t]/n[t]/n[p->t];
      ekin += p->v.sq()/2.;
    }
  }

  // analysis function
  void analyse()
  {
    //ekin = 0;
    //for(auto p : particles)
    //  ekin += p->v.sq();
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


// =============================================================================
// input/output

// Declare and parse all program options
void parse_options(int ac, char **av)
{
  // we use strings to retreive the arrays
  string Mget, dget;

  // options allowed only in the command line
  opt::options_description generic("Generic options");
  generic.add_options()
    ("help,h", "produce help message")
    ("directory", opt::value<string>(&directory), "input/output directory")
    ("verbose", opt::value<int>(&verbose)->implicit_value(2), "verbosity level (0, 1, 2, default=1)")
    ("thread,t", opt::value<int>(&nthreads)->implicit_value(1), "number of threads (0=no multithreading, 1=OpenMP default, >1=your favorite number)");

  // options allowed only in the config file
  opt::options_description config("Configuration options");
  config.add_options()
    ("nboxes", opt::value<int>(&nboxes), "number of boxes (both x and y)")
    ("dens", opt::value<string>(&dget), "density of particles")
    ("ntypes", opt::value<int>(&ntypes), "number of different types")
    ("nsteps", opt::value<int>(&nsteps), "total number of time steps")
    ("ninfo", opt::value<int>(&ninfo), "number of time steps between two analyses")
    ("T", opt::value<double>(&T), "temperature")
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

  // get number of particles array
  dens = get_ints_from_string(dget);
  if(dens.size()!=ntypes) throw inline_str("wrong number of densities");
  ntot = pow(nboxes, dim)*accumulate(begin(dens), end(dens), 0.);

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
                 const vector<box>& boxes,
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
    for(int i=0; i<nboxes; ++i)
      for(int j=0; j<nboxes; ++j)
        for(int k=0; k<dens[t]; ++k)
          particles.push_back({
            {{ random_real(double(i)/nboxes, double(i+1)/nboxes),
               random_real(double(j)/nboxes, double(j+1)/nboxes) }},
            {{0,0}},
            t});

  // the boxes
  vector<box> boxes;
  for(int i=0; i<nboxes; ++i)
    for(int j=0; j<nboxes; ++j)
      boxes.push_back(vec {{(i+.5)/nboxes, (j+.5)/nboxes}});

  // ---------------------------------------------------------------------------
  // the algo

  for(int t=0; t<=nsteps; ++t)
  {
    // the random shift
    vec a = random_vec(0, 1./nboxes);
    // ok that is dirty...
    /*if(t%ninfo == 0)*/ a = {{0,0}};

    // simulate particles
    //#pragma omp parallel for num_threads(nthreads) if(nthreads)
    for(int i=0; i<particles.size(); ++i)
    {
      // stream
      particles[i].stream();
      // bucket particles
      const int index = get_box(collapse(particles[i].x+a), nboxes);
      boxes[index].add(&particles[i]);
    }

    // store step
    if(t%ninfo == 0)
    {
      // print status
      cout << "t = " << t <<  " / " << nsteps << endl;

      // collision and store
      //#pragma omp parallel for num_threads(nthreads) if(nthreads)
      for(auto b=boxes.begin(); b<boxes.end(); ++b)
      {
        b->collision();
        //b->analyse();
      }

      // serialize
      write_frame(t, boxes, particles);

      //#pragma omp parallel for num_threads(nthreads) if(nthreads)
      for(auto b=boxes.begin(); b<boxes.end(); ++b)
        b->clear();
    }
    // normal step
    else
    {
      //#pragma omp parallel for num_threads(nthreads) if(nthreads)
      for(auto b=boxes.begin(); b<boxes.end(); ++b)
      {
        b->collision();
        b->clear();
      }
    }
  }
}

// =============================================================================
// system

// Somewhow omp_get_num_threads() is not working properly with gcc...
void set_nthreads()
{
  // if nthreads is 1 we use the default number of threads from OpenMP
  if(nthreads == 1)
  {
    // count the number of OpenMP threads
    int count = 0;
    //#pragma omp parallel
    {
      //#pragma omp atomic
      ++count;
    }
    nthreads = count;
  }
}

// compute derived parameters etc
void initialize()
{
}

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

    // parameters init
    if(verbose) cout << "system initialisation ...";
    initialize();
    if(verbose) cout << " done" << endl;

    // multi-threading
    if(nthreads)
    {
      // print some friendly message
      if(verbose) cout << "multi-threading ... ";
      set_nthreads();
      if(verbose) cout << nthreads << " active threads" << endl;
    }

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