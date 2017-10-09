#define DEBUG 1

#include <iostream>
//#include <cmath>
#include "Hubbard2D.hpp"

#ifdef USE_SPECTRA
#include <MatOp/SparseGenMatProd.h>
#include <SymEigsSolver.h>
#endif

#include <boost/random.hpp>
#include <boost/limits.hpp>
#include <ietl/interface/eigen3.h>
#include <ietl/vectorspace.h>
#include <ietl/lanczos.h>

int main(int argc, const char * argv[]) {
  // insert code here...
  std::cout << "Hello, World!\n";
  
  int Lx=4,Ly=4; //dimensions of lattice
  int ne=2; //# of electrons
  int U=4;
  HubbardModel2D H(4,4,ne,ne,1,U);
  std::cout << "2D "<<Lx<<"x"<<Ly<<" Hubbard model with "
  << ne <<" up/down electrons and U/t="<<U<<std::endl;
  H.makeBasis();
  std::cout <<"Made Basis: " << H.getBasis()->size()<< std::endl;

  H.buildHubbard2D();
  std::cout <<"Built Matrix" << std::endl;
  SpMat Hmat = *(H.getH());
  std::cout << "# of non-zero elements:"<<(H.getH())->nonZeros() << std::endl;
  H.getH()->makeCompressed();

  //construct test states
  lattice_t psi1;
  int i;
  for(i=0;i<ne;i++)
    psi1[i].flip();

  for(i=0;i<ne;i++)
    psi1[(i+BASISSIZE/2)%(BASISSIZE)].flip();

  lattice_t psi2;
  i=0;

  for(i=0;i<ne;i++)
    psi2[i].flip();

  for(i=0;i<ne;i++)
    psi2[(i+ne+BASISSIZE/2)%(BASISSIZE)].flip();

  //std::cout<<psi1<<std::endl;
  //std::cout<<psi2<<std::endl;

  std::string double_occ="|";
  for(i=0;i<ne;i++)
    double_occ+="↑↓|";
  std::string not_occ="|";
  for(i=0;i<ne;i++)
    not_occ+="↑|";
  for(i=0;i<ne;i++)
    not_occ+="↓|";


  //Diag with specrta
#ifdef USE_SPECTRA
  // Construct matrix operation object using the wrapper class SparseGenMatProd
  using wrapper_t =Spectra::SparseGenMatProd<double,Eigen::RowMajor,long>;
  wrapper_t op(Hmat);
  
  // Construct eigen solver object, requesting the smallest eigenvalue
  Spectra::SymEigsSolver< double, Spectra::SMALLEST_ALGE , wrapper_t > eigs(&op, 1, 6);
  // Initialize and compute
  eigs.init();
  int nconv = eigs.compute();
  
#ifdef DEBUG
  if(eigs.info() != Spectra::SUCCESSFUL)
      std::cout <<"Warning something failed " <<nconv << std::endl;
#endif

  std::cout<< "E0="<<eigs.eigenvalues()[0]<<std::endl;

  std::cout<<double_occ<<" :"<<eigs.eigenvectors()(H.getState(psi1),0)<<std::endl; //psi1
  std::cout<<not_occ<<" :"<<eigs.eigenvectors()(H.getState(psi2),0)<<std::endl; //psi2
#else
  typedef boost::lagged_fibonacci607 Gen;
  Gen mygen;
  mygen.seed(0);
  
  typedef Eigen::SparseMatrix<double,Eigen::RowMajor,long> Matrix;
  typedef Eigen::VectorXd Vector;
  typedef ietl::vectorspace<Vector> Vecspace;
   
   
   
  // Creation of an iteration object:
  int max_iter = 10*H.getBasis()->size();
  double rel_tol = 500*std::numeric_limits<double>::epsilon();
  double abs_tol = std::pow(std::numeric_limits<double>::epsilon(),2./3);
  int n_lowest_eigenval = 1;
  std::vector<double> eigen;
  std::vector<double> err;
  std::vector<int> multiplicity;

  std::vector<double> groundStates;

  Vecspace vec(Hmat.cols());
  ietl::lanczos<Matrix,Vecspace> lanczos(Hmat,vec);
  ietl::lanczos_iteration_nlowest<double>
  iter(max_iter, n_lowest_eigenval, rel_tol, abs_tol);
  std::cout << "lanczos" << std::endl;
  try{
    lanczos.calculate_eigenvalues(iter,mygen);
    eigen = lanczos.eigenvalues();
    err = lanczos.errors();
    multiplicity = lanczos.multiplicities();
    std::cout<<"number of iterations: "<<iter.iterations()<<"\n";
    groundStates.push_back(eigen[0]);
  }
  catch (std::runtime_error& e) {
    std::cout << e.what() << "\n";
  }
  std::cout << "#        eigenvalue            error         multiplicity\n";
  for (int i=0;i<10;++i)
    std::cout << i << "\t" << eigen[i] << "\t" << err[i] << "\t"
    << multiplicity[i] << "\n";

  // call of eigenvectors function follows:
  std::cout << "\nEigen vectors computations for the lowest eigenvalue:\n\n";
  std::vector<double>::iterator start = eigen.begin();
  std::vector<double>::iterator end = eigen.begin()+1;
  std::vector<Vector> eigenvectors; // for storing the eigen vectors.
  ietl::Info<double> info; // (m1, m2, ma, eigenvalue, residualm, status).

  try {
    lanczos.eigenvectors(start,end,std::back_inserter(eigenvectors),info,mygen,max_iter);
  }
  catch (std::runtime_error& e) {
    std::cout << e.what() << "\n";
  }

  for(int i=0;i<10;i++){
    std::cout << eigenvectors[0](i)<<std::endl;
  }
  std::cout << " Information about the eigenvector computations:\n\n";
  for(int i = 0; i < info.size(); i++) {
    std::cout << " m1(" << i+1 << "): " << info.m1(i) << ", m2(" << i+1 << "): "
    << info.m2(i) << ", ma(" << i+1 << "): " << info.ma(i) << " eigenvalue("
    << i+1 << "): " << info.eigenvalue(i) << " residual(" << i+1 << "): "
    << info.residual(i) << " error_info(" << i+1 << "): "
    << info.error_info(i) << "\n\n";
  }

  std::cout<<double_occ<<" :"<<eigenvectors[0](H.getState(psi1))<<std::endl; //psi1
  std::cout<<not_occ<<" :"<<eigenvectors[0](H.getState(psi2))<<std::endl; //psi2

#endif

  return 0;
}
