//# SpectralFit.cc: Least Squares fitting of spectral elements to spectrum
//# Copyright (C) 2001
//# Associated Universities, Inc. Washington DC, USA.
//#
//# This library is free software; you can redistribute it and/or modify it
//# under the terms of the GNU Library General Public License as published by
//# the Free Software Foundation; either version 2 of the License, or (at your
//# option) any later version.
//#
//# This library is distributed in the hope that it will be useful, but WITHOUT
//# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
//# License for more details.
//#
//# You should have received a copy of the GNU Library General Public License
//# along with this library; if not, write to the Free Software Foundation,
//# Inc., 675 Massachusetts Ave, Cambridge, MA 02139, USA.
//#
//# Correspondence concerning AIPS++ should be addressed as follows:
//#        Internet email: aips2-request@nrao.edu.
//#        Postal address: AIPS++ Project Office
//#                        National Radio Astronomy Observatory
//#                        520 Edgemont Road
//#                        Charlottesville, VA 22903-2475 USA
//#
//# $Id$

//# Includes
#include <trial/Wnbt/SpectralFit.h>
#include <aips/Arrays/Vector.h>
#include <aips/Exceptions/Error.h>
#include <aips/Functionals/SumFunction.h>
#include <aips/Functionals/Gaussian1D.h>
#include <aips/Functionals/Polynomial.h>
#include <trial/Wnbt/SpectralElement.h>
#include <trial/Fitting/NonLinearFitLM.h>
#include <trial/Functionals/FuncWithAutoDerivs.h>

//# Constructors
SpectralFit::SpectralFit() :
  slist_p(0), iter_p(0) {}

SpectralFit::SpectralFit(const SpectralList &in) :
  slist_p(in), iter_p(0) {}

SpectralFit::SpectralFit(const SpectralFit &other) :
  slist_p(other.slist_p), iter_p(other.iter_p) {}

SpectralFit::~SpectralFit() {}

SpectralFit &SpectralFit::operator=(const SpectralFit &other) {
  if (this != &other) {
    slist_p = other.slist_p;
    iter_p = other.iter_p;
  };
  return *this;
}

void SpectralFit::setFitElement(uInt index, const SpectralElement &elem) {
  slist_p.set(elem, index);
  iter_p = 0;
}

void SpectralFit::addFitElement(const SpectralElement &elem) {
  slist_p.add(elem);
  iter_p = 0;
}

void SpectralFit::addFitElement(const SpectralList &elem) {
  slist_p.add(elem);
  iter_p = 0;
}

void SpectralFit::clear() {
  slist_p.clear();
  iter_p = 0;
}

Bool SpectralFit::fit(const Vector<Double> &y,
	 const Vector<Double> &x) {
  return fit(y, x, static_cast<const Vector<Bool> *const>(0));
}

Bool SpectralFit::fit(const Vector<Float> &y,
		      const Vector<Float> &x) {
  return fit(y, x, static_cast<const Vector<Bool> *const>(0));
}

Bool SpectralFit::fit(const Vector<Double> &y,
		      const Vector<Double> &x, const Vector<Bool> &mask) {
  return fit(y, x, &mask);
}

Bool SpectralFit::fit(const Vector<Float> &y,
		      const Vector<Float> &x, const Vector<Bool> &mask) {
  return fit(y, x, &mask);
}

Bool SpectralFit::fit(const Vector<Double> &y,
		      const Vector<Double> &x,
		      const Vector<Bool> *const mask) {
  // The fitter
  NonLinearFitLM<Double> fitter;
  iter_p = 0;
  // The functions to fit
  const Gaussian1D<AutoDiff<Double> > gauss; 
  const Polynomial<AutoDiff<Double> > poly; 
  SumFunction<AutoDiff<Double>,AutoDiff<Double> > func;
  Int npar(0);
  for (uInt i=0; i<slist_p.nelements(); i++) {
    if (slist_p[i].getType() == SpectralElement::GAUSSIAN) {
      func.addFunction(gauss);
      npar += 3;
    } else if (slist_p[i].getType() == SpectralElement::POLYNOMIAL) {
      npar += slist_p[i].getDegree()+1;
      const Polynomial<AutoDiff<Double> > poly(slist_p[i].getDegree());
      func.addFunction(poly);
    };
  };
  FuncWithAutoDerivs<Double,Double> autoFunc(func);
  fitter.setFunction(autoFunc);
  // Initial guess
  Vector<Double> v(npar);
  Vector<Bool> vb(npar);
  // Next one necessary as long as no correct Functionals
  Vector<Double> corsol(npar);
  corsol = 1.0;
  Double fc = 1./sqrt(log(16.0));
  uInt j(0);
  for (uInt i=0; i<slist_p.nelements(); i++) {
    if (slist_p[i].getType() == SpectralElement::GAUSSIAN) {
      vb(j)  = !slist_p[i].fixedAmpl();
      v(j++) = slist_p[i].getAmpl();
      vb(j)  = !slist_p[i].fixedCenter();
      v(j++) = slist_p[i].getCenter();
      vb(j)  = !slist_p[i].fixedFWHM();
      corsol(j) = fc;
      v(j++) = slist_p[i].getFWHM();
    } else if (slist_p[i].getType() == SpectralElement::POLYNOMIAL) {
      for (uInt k=0; k<slist_p[i].getDegree()+1; k++) {
	vb(j)  = !slist_p[i].fixed()(k);
	v(j++) = 0;
      };
    };
  };
  // Force (as interim solution) all values in solution
  fitter.setFullSolution(vb);
  fitter.setFittedFuncParams(v);
  // Max. number of iterations
  fitter.setMaxIter(50+ slist_p.nelements()*10);
  // Convergence criterium
  fitter.setCriteria(0.001);
  // Fit
  Vector<Double> sol;
  Vector<Double> err;
  Vector<Double> sigma(x.nelements());
  sigma = 1.0;
  sol = fitter.fit(x, y, sigma, mask, &corsol);
  // Calculate the errors
  err = fitter.errors();
  // Number of iterations
  iter_p = fitter.currentIteration();
  j = 0;
  Vector<Double> tmp, terr;
  for (uInt i=0; i<slist_p.nelements(); i++) {
    tmp.resize(slist_p[i].getOrder());
    terr.resize(slist_p[i].getOrder());
    for (uInt k=0; k<slist_p[i].getOrder(); k++) {
      if (k==2 && slist_p[i].getType() == SpectralElement::GAUSSIAN) {
	tmp(k) = sol(j) / SpectralElement::SigmaToFWHM;
	terr(k) = err(j++) / SpectralElement::SigmaToFWHM;
      } else {
	tmp(k) = sol(j);
	terr(k) = err(j++);
      };
    };
    slist_p[i].set(tmp);
    slist_p[i].setError(terr);
  };
  return fitter.converged();
}

Bool SpectralFit::fit(const Vector<Float> &y,
		      const Vector<Float> &x,
		      const Vector<Bool> *const mask) {
  // The fitter
  NonLinearFitLM<Float> fitter;
  iter_p = 0;
  // The functions to fit
  const Gaussian1D<AutoDiff<Float> > gauss; 
  const Polynomial<AutoDiff<Float> > poly; 
  SumFunction<AutoDiff<Float>,AutoDiff<Float> > func;
  Int npar(0);
  for (uInt i=0; i<slist_p.nelements(); i++) {
    if (slist_p[i].getType() == SpectralElement::GAUSSIAN) {
      func.addFunction(gauss);
      npar += 3;
    } else if (slist_p[i].getType() == SpectralElement::POLYNOMIAL) {
      npar += slist_p[i].getDegree()+1;
      const Polynomial<AutoDiff<Float> > poly(slist_p[i].getDegree());
      func.addFunction(poly);
    };
  };
  FuncWithAutoDerivs<Float,Float> autoFunc(func);
  fitter.setFunction(autoFunc);
  // Initial guess
  Vector<Float> v(npar);
  Vector<Bool> vb(npar);
  // Next one necessary as long as no correct Functionals
  Vector<Float> corsol(npar);
  corsol = 1.0;
  Float fc = 1./sqrt(log(16.0));
  uInt j(0);
  for (uInt i=0; i<slist_p.nelements(); i++) {
    if (slist_p[i].getType() == SpectralElement::GAUSSIAN) {
      vb(j)  = !slist_p[i].fixedAmpl();
      v(j++) = slist_p[i].getAmpl();
      vb(j)  = !slist_p[i].fixedCenter();
      v(j++) = slist_p[i].getCenter();
      vb(j)  = !slist_p[i].fixedFWHM();
      corsol(j) = fc;
      v(j++) = slist_p[i].getFWHM();
    } else if (slist_p[i].getType() == SpectralElement::POLYNOMIAL) {
      for (uInt k=0; k<slist_p[i].getDegree()+1; k++) {
	vb(j)  = !slist_p[i].fixed()(k);
	v(j++) = 0;
      };
    };
  };
  // Force (as interim solution) all values in solution
  fitter.setFullSolution(vb);
  fitter.setFittedFuncParams(v);
  // Max. number of iterations
  fitter.setMaxIter(50+ slist_p.nelements()*10);
  // Convergence criterium
  fitter.setCriteria(0.001);
  // Fit
  Vector<Float> sol;
  Vector<Double> err;
  Vector<Float> sigma(x.nelements());
  sigma = 1.0;
  sol = fitter.fit(x, y, sigma, mask, &corsol);
  fitter.getErrors(err);
  j = 0;
  Vector<Double> tmp, terr;
  for (uInt i=0; i<slist_p.nelements(); i++) {
    tmp.resize(slist_p[i].getOrder());
    terr.resize(slist_p[i].getOrder());
    for (uInt k=0; k<slist_p[i].getOrder(); k++) {
      if (k==2 && slist_p[i].getType() == SpectralElement::GAUSSIAN) {
	tmp(k) = sol(j) / SpectralElement::SigmaToFWHM;
	terr(k) = err(j++) /  SpectralElement::SigmaToFWHM;
      } else {
	tmp(k) = sol(j);
	terr(k) = err(j++);
      };
    };
    slist_p[i].set(tmp);
    slist_p[i].setError(terr);
  };
  return fitter.converged();
}
