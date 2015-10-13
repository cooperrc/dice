// @HEADER
// ************************************************************************
//
//               Digital Image Correlation Engine (DICe)
//                 Copyright (2014) Sandia Corporation
//
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact:
//              Dan Turner   (danielzturner@gmail.com)
//
// ************************************************************************
// @HEADER

/*! \file  DICe_TestCorrelateParams.cpp
    \brief Test of various parameters that can be set for a correlation.
    We try to hit all combinations below for a square subset
*/

#include <DICe.h>
#include <DICe_Schema.h>

#include <Teuchos_RCP.hpp>
#include <Teuchos_oblackholestream.hpp>
#include <Teuchos_ParameterList.hpp>
#include <Teuchos_GlobalMPISession.hpp>

#include <iostream>
#include <iomanip>
#include <cstdio>

#include <cassert>

using namespace DICe;

int main(int argc, char *argv[]) {

  // initialize kokkos
  Kokkos::initialize(argc, argv);

  int_t iprint     = argc - 1;
  // for serial, the global MPI session is a no-op, but in parallel
  // ensures that MPI_Init is called (needed by the schema)
  Teuchos::GlobalMPISession mpi_session(&argc, &argv, NULL);
  Teuchos::RCP<std::ostream> outStream;
  Teuchos::oblackholestream bhs; // outputs nothing
  if (iprint > 0)
    outStream = Teuchos::rcp(&std::cout, false);
  else
    outStream = Teuchos::rcp(&bhs, false);
  int_t errorFlag  = 0;
  scalar_t errtol  = 1.0E-2;

  *outStream << "--- Begin test ---" << std::endl;

  // initialization

  std::string fileRef("./images/TestSubsetConstructionRef.tif");
  std::string fileDef("./images/TestSubsetConstructionNormalRotDisp.tif");

  // exact solution
  // note: deformation vectors are a subset of the schema's data vector so DICe_DEFORMATION_SIZE != DICe::MAX_FIELD_NAME
  // the first DICe_DEFORMATION_SIZE values of the schema's data vector are the deformation values
  std::vector<scalar_t>defExact(DICE_DEFORMATION_SIZE,0.0);
  defExact[DICe::NORMAL_STRAIN_X] = 0.121;
  defExact[DICe::NORMAL_STRAIN_Y] = 0.121;
  defExact[DICe::ROTATION_Z] = 0.262;
  defExact[DICe::DISPLACEMENT_X] = 9.8;
  defExact[DICe::DISPLACEMENT_Y] = -7.62;

  *outStream << "testing square subset param combinations" << std::endl;
  const int_t subset_size = 21;
  // solution parameters
  Teuchos::RCP<Teuchos::ParameterList> params = rcp(new Teuchos::ParameterList());
  // enable all the shape functions (even if not needed)
  params->set(DICe::enable_translation,true);
  params->set(DICe::enable_rotation,true);
  params->set(DICe::enable_normal_strain,true);
  params->set(DICe::enable_shear_strain,true);
  Teuchos::RCP<DICe::Schema> schemaSquare = Teuchos::rcp(new DICe::Schema(fileRef,fileDef,params));
  schemaSquare->initialize(1,subset_size); // only one correlation point for this

  // create the column titles
  std::vector<std::string> colTitles;
  colTitles.push_back("Status");
  colTitles.push_back("Rout");
  colTitles.push_back("Interp");
  colTitles.push_back("Init");
  colTitles.push_back("Opt");
  colTitles.push_back("Ux");
  colTitles.push_back("Uy");
  colTitles.push_back("Rz");
  colTitles.push_back("Nx");
  colTitles.push_back("Ny");
  colTitles.push_back("Sxy");
  colTitles.push_back("Sigma");
  colTitles.push_back("Gamma");
  colTitles.push_back("Flag");
  for(int_t title=0;title<colTitles.size();++title){
    if((title>11&&title<15)||title>15)
      *outStream << std::setw(12) << colTitles[title];
    else
      *outStream << std::setw(7) << colTitles[title];
  }
  *outStream << std::endl;

  std::vector<DICe::Optimization_Method> opt_methods;
  opt_methods.push_back(DICe::SIMPLEX);
  opt_methods.push_back(DICe::SIMPLEX_THEN_GRADIENT_BASED);
  opt_methods.push_back(DICe::GRADIENT_BASED);
  opt_methods.push_back(DICe::GRADIENT_BASED_THEN_SIMPLEX);

  std::vector<DICe::Interpolation_Method> interp_methods;
  interp_methods.push_back(DICe::BILINEAR);
  interp_methods.push_back(DICe::KEYS_FOURTH);

  // BEGIN PARAMS LOOP

  for(int_t opt_method=0;opt_method<opt_methods.size();++opt_method){
    for(int_t interp_method=0;interp_method<interp_methods.size();++interp_method){
      // set the parameters
      // change the default values
      params->set(DICe::correlation_routine,DICe::GENERIC_ROUTINE);
      params->set(DICe::interpolation_method,interp_methods[interp_method]);
      params->set(DICe::initialization_method,DICe::USE_FIELD_VALUES);
      params->set(DICe::optimization_method,opt_methods[opt_method]);
      schemaSquare->set_params(params);

      // reset the initial guess
      schemaSquare->field_value(0,DICe::COORDINATE_X) = 30;
      schemaSquare->field_value(0,DICe::COORDINATE_Y) = 30;
      schemaSquare->field_value(0,DICe::SIGMA) = 0.0;
      schemaSquare->field_value(0,DICe::GAMMA) = 0.0;
      schemaSquare->field_value(0,DICe::STATUS_FLAG) = 0;
      schemaSquare->field_value(0,DICe::DISPLACEMENT_X) = defExact[DICe::DISPLACEMENT_X] - 0.25;
      schemaSquare->field_value(0,DICe::DISPLACEMENT_Y) = defExact[DICe::DISPLACEMENT_Y] - 0.25;
      schemaSquare->field_value(0,DICe::ROTATION_Z)     = defExact[DICe::ROTATION_Z]     - 0.05;
      schemaSquare->field_value(0,DICe::NORMAL_STRAIN_X) = 0.0;
      schemaSquare->field_value(0,DICe::NORMAL_STRAIN_Y) = 0.0;
      schemaSquare->field_value(0,DICe::SHEAR_STRAIN_XY) = 0.0;

      schemaSquare->execute_correlation();

      // check the solution to ensure the error is small:
      bool valueError = false;
      if(std::abs(schemaSquare->field_value(0,DICe::DISPLACEMENT_X) - defExact[DICe::DISPLACEMENT_X]) > errtol) valueError = true;
      if(std::abs(schemaSquare->field_value(0,DICe::DISPLACEMENT_Y) - defExact[DICe::DISPLACEMENT_Y]) > errtol) valueError = true;
      if(std::abs(schemaSquare->field_value(0,DICe::ROTATION_Z) - defExact[DICe::ROTATION_Z]) > errtol) valueError = true;
      if(std::abs(schemaSquare->field_value(0,DICe::NORMAL_STRAIN_X) - defExact[DICe::NORMAL_STRAIN_X]) > errtol) valueError = true;
      if(std::abs(schemaSquare->field_value(0,DICe::NORMAL_STRAIN_Y) - defExact[DICe::NORMAL_STRAIN_Y]) > errtol) valueError = true;
      if(std::abs(schemaSquare->field_value(0,DICe::SHEAR_STRAIN_XY) - defExact[DICe::SHEAR_STRAIN_XY]) > errtol) valueError = true;
      if(std::abs(schemaSquare->field_value(0,DICe::STATUS_FLAG) - DICe::INITIALIZE_USING_PREVIOUS_FRAME_SUCCESSFUL) > errtol) valueError = true;
      if(std::abs(schemaSquare->field_value(0,DICe::SIGMA) + 1) < errtol) valueError = true; // check that sigma != -1

      std::string statusStr = valueError ? "FAIL": "PASS";
      if(valueError) errorFlag++;

      // print the results in easy to read table format
      std::ios::fmtflags f( outStream->flags() ); // get the state of the cout flags
      *outStream << std::setw(7)  << statusStr;
      *outStream << std::setw(7)  << schemaSquare->correlation_routine();
      *outStream << std::setw(7)  << schemaSquare->interpolation_method();
      *outStream << std::setw(7)  << schemaSquare->initialization_method();
      *outStream << std::setw(7)  << schemaSquare->optimization_method();
      *outStream << std::setw(7)  << std::setprecision(3) << schemaSquare->field_value(0,DICe::DISPLACEMENT_X);
      *outStream << std::setw(7)  << std::setprecision(3) << schemaSquare->field_value(0,DICe::DISPLACEMENT_Y);
      *outStream << std::setw(7)  << std::setprecision(3) << schemaSquare->field_value(0,DICe::ROTATION_Z);
      *outStream << std::setw(7)  << std::setprecision(3) << schemaSquare->field_value(0,DICe::NORMAL_STRAIN_X);
      *outStream << std::setw(7)  << std::setprecision(3) << schemaSquare->field_value(0,DICe::NORMAL_STRAIN_Y);
      *outStream << std::setw(12) << std::setprecision(2) << schemaSquare->field_value(0,DICe::SHEAR_STRAIN_XY);
      *outStream << std::setw(12) << std::setprecision(3) << std::setiosflags(std::ios::scientific) <<  schemaSquare->field_value(0,DICe::SIGMA);
      *outStream << std::setw(12) <<  schemaSquare->field_value(0,DICe::GAMMA);
      *outStream << std::setw(7)  << std::setiosflags(std::ios_base::fixed) << schemaSquare->field_value(0,DICe::STATUS_FLAG);
      *outStream << std::endl;
      outStream->flags(f); // reset the cout flags to the original state
    }
  }
  // END PARAMS LOOP

  *outStream << "--- End test ---" << std::endl;

  // finalize kokkos
  Kokkos::finalize();

  if (errorFlag != 0)
    std::cout << "End Result: TEST FAILED\n";
  else
    std::cout << "End Result: TEST PASSED\n";

  return 0;

}

