//------------------------------------------------------------------------------
//
//   This file is part of the VAMPIRE open source package under the
//   Free BSD licence (see licence file for details).
//
//   (c) Sam Westmoreland and Richard Evans 2017. All rights reserved.
//
//   Email: sw766@york.ac.uk
//
//------------------------------------------------------------------------------
//

// C++ standard library headers
#include <string>
#include <sstream>

// Vampire headers
#include "anisotropy.hpp"
#include "errors.hpp"
#include "units.hpp"
#include "vio.hpp"

// anisotropy module headers
#include "internal.hpp"

namespace anisotropy{

   //---------------------------------------------------------------------------
   // Function to process input file parameters for anisotropy module
   //---------------------------------------------------------------------------
   bool match_input_parameter(std::string const key, std::string const word, std::string const value, std::string const unit, int const line){

      // Check for valid key, if no match return false
      std::string prefix="anisotropy";
      if(key!=prefix) return false;

      //-------------------------------------------------------------------
      std::string test="surface-anisotropy-threshold";
      if(word==test){
          // test for native keyword
          test="native";
          if(value==test){
             internal::native_neel_anisotropy_threshold = true;
             return EXIT_SUCCESS;
          }
          int sat=atoi(value.c_str());
          // Test for valid range
          vin::check_for_valid_int(sat, word, line, prefix, 0, 1000000000,"input","0 - 1,000,000,000");
          internal::neel_anisotropy_threshold = sat;
          return true;
      }
      //-------------------------------------------------------------------
      test="surface-anisotropy-nearest-neighbour-range";
      if(word==test){
          // Test for valid range
          double r = atof(value.c_str());
          vin::check_for_valid_value(r, word, line, prefix, unit, "length", 0.0, 1.0e9,"input","0.0 - 1,000,000,000");
          internal::nearest_neighbour_distance = r;
          return true;
      }
      //-------------------------------------------------------------------
      test="enable-bulk-neel-anisotropy";
      if(word==test){
          // Enable large threshold to force calculation of Neel anisotropy for all atoms
          internal::neel_anisotropy_threshold = 1000000000;
          return true;
      }
      //-------------------------------------------------------------------
      test="neel-anisotropy-exponential-factor";
      if(word==test){
          // Enable range dependent Neel anisotropy Lij(r) = exp(-F(r-r0)/r0)
          // F should be steepness of function indicating rate of decay with r
          double F = atof(value.c_str());
          vin::check_for_valid_value(F, word, line, prefix, unit, "none", 0.01, 100.0,"input","0.01 - 100");
          internal::neel_exponential_factor = F;
          internal::neel_range_dependent = true;
          return true;
      }
      //-------------------------------------------------------------------
      test="neel-anisotropy-exponential-range";
      if(word==test){
          // Enable range dependent Neel anisotropy Lij(r) = exp(-F(r-r0)/r0)
          // r should be approximately nearest neighbour range ~ 2.5 angstroms
          double r = atof(value.c_str());
          vin::check_for_valid_value(r, word, line, prefix, unit, "length", 0.0001, 1000.0,"input","0.0001 - 1,000");
          internal::neel_exponential_range = r;
          internal::neel_range_dependent = true;
          return true;
      }
      //--------------------------------------------------------------------
      // Keyword not found
      //--------------------------------------------------------------------
      return false;

   }

   //---------------------------------------------------------------------------
   // Function to process material parameters
   //---------------------------------------------------------------------------
   bool match_material_parameter(std::string const word, std::string const value, std::string const unit, int const line, int const super_index, const int sub_index, const int max_materials){

      // add prefix string
      std::string prefix="material:";

      // Check for empty material parameter array and resize to avoid segmentation fault
      if(internal::mp.size() == 0){
         internal::mp.resize(max_materials);
      }

      //------------------------------------------------------------
      // Check for material properties
      //------------------------------------------------------------

      // Single-ion anisotropy energy in terms of orthogonal tesseral harmonics
      // A simple orthogonal version is given, as well as an orthonormal version

      // Second order uniaxial anisotropy constant (negative of standard K_1)
      std::string test  = "2-0-order-anisotropy-constant";
      std::string test2 = "second-order-uniaxial-anisotropy-constant";
      std::string test3 = "uniaxial-anisotropy-constant"; // legacy form (deprecated but probably never obsoleted)
      if( (word == test) || (word == test2) || (word == test3) )
      {

         double ku2 = atof(value.c_str());
         vin::check_for_valid_value(ku2, word, line, prefix, unit, "energy", -1e-17, 1e-17,"material"," < +/- 1.0e-17 J/atom");
         internal::mp[super_index].ku2 = ku2;
         internal::enable_uniaxial_second_order = true; // Switch on second order tensor calculation for all spins
         return true;

      }

      // Second order theta uniaxial anisotropy constant (orthonormal)
      test = "2-0-orthonormal-tesseral-harmonic-anisotropy-constant";
      if ( word == test )
      {

         double ku2 = - atof( value.c_str() ) * ( 3.0 / 4.0 ) * sqrt( 5.0 / M_PI );
         vin::check_for_valid_value(ku2, word, line, prefix, unit, "energy", -1e-17, 1e-17,"material"," < +/- 1.0e-17 J/atom");
         internal::mp[super_index].ku2 = ku2;
         internal::enable_uniaxial_second_order = true; // Switch on second order tensor calculation for all spins
         return true;

      }

      // Second order theta first order phi anisotropy constant
      test  = "2-1-order-anisotropy-constant";
      test2 = "second-order-theta-first-order-phi-anisotropy-constant";
      if( (word == test) || (word == test2) )
      {

         double k2r1 = atof(value.c_str());
         vin::check_for_valid_value(k2r1, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k2r1 = k2r1;
         internal::enable_rotational_2_1_order = true;
         return true;

      }

      // Second order theta first order phi anisotropy constant (orthonormal)
      test = "2-1-orthonormal-tesseral-harmonic-anisotropy-constant";
      if ( word == test )
      {

         double k2r1 = - atof( value.c_str() ) * ( 0.25 ) * sqrt( 15.0 / M_PI );
         vin::check_for_valid_value(k2r1, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k2r1 = k2r1;
         internal::enable_rotational_2_1_order = true;
         return true;

      }

      // Second order theta first order phi odd anisotropy constant
      test  = "2--1-order-anisotropy-constant";
      test2 = "second-order-theta-first-order-phi-odd-anisotropy-constant";
      if( (word == test) || (word == test2) )
      {

         double k2r1_odd = atof(value.c_str());
         vin::check_for_valid_value(k2r1_odd, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k2r1_odd = k2r1_odd;
         internal::enable_rotational_2_1_order_odd = true;
         return true;

      }

      // Second order theta first order phi anisotropy constant (orthonormal)
      test = "2--1-orthonormal-tesseral-harmonic-anisotropy-constant";
      if( word == test )
      {

         double k2r1_odd = - atof( value.c_str() ) * ( 0.25 ) * sqrt( 15.0 / M_PI );
         vin::check_for_valid_value(k2r1_odd, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k2r1_odd = k2r1_odd;
         internal::enable_rotational_2_1_order_odd = true;
         return true;

      }

      // Second order theta second order phi anisotropy constant
      test  = "2-2-order-anisotropy-constant";
      test2 = "second-order-theta-second-order-phi-anisotropy-constant";
      test3 = "second-order-rotational-anisotropy-constant";
      if( (word == test) || (word == test2) || (word == test3) )
      {

         double k2r2 = atof(value.c_str());
         vin::check_for_valid_value(k2r2, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k2r2 = k2r2;
         internal::enable_rotational_2_2_order = true;
         return true;

      }

      // Second order theta second order phi anisotropy constant (orthonormal)
      test = "2-2-orthonormal-tesseral-harmonic-anisotropy-constant";
      if( word == test )
      {

         double k2r2 = - atof( value.c_str() ) * 0.25 * sqrt( 15.0 / M_PI );
         vin::check_for_valid_value(k2r2, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k2r2 = k2r2;
         internal::enable_rotational_2_2_order = true;
         return true;

      }

      // Second order theta second order phi odd anisotropy constant
      test  = "2--2-order-anisotropy-constant";
      test2 = "second-order-theta-second-order-phi-odd-anisotropy-constant";
      test3 = "second-order-odd-rotational-anisotropy-constant";
      if( (word == test) || (word == test2) || (word == test3) )
      {

         double k2r2_odd = atof(value.c_str());
         vin::check_for_valid_value(k2r2_odd, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k2r2_odd = k2r2_odd;
         internal::enable_rotational_2_2_order_odd = true;
         return true;

      }

      // Second order theta second order phi odd anisotropy constant (orthonormal)
      test = "2--2-orthonormal-tesseral-harmonic-anisotropy-constant";
      if( word == test )
      {

         double k2r2_odd = - atof( value.c_str() ) * 0.25 * sqrt( 15.0 / M_PI );
         vin::check_for_valid_value(k2r2_odd, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k2r2_odd = k2r2_odd;
         internal::enable_rotational_2_2_order_odd = true;
         return true;

      }

      // Fourth order uniaxial anisotropy constant
      test  = "4-0-order-anisotropy-constant";
      test2 = "fourth-order-uniaxial-anisotropy-constant";
      if( (word == test) || (word == test2) )
      {

         double ku4 = atof( value.c_str() );
         vin::check_for_valid_value(ku4, word, line, prefix, unit, "energy", -1e-17, 1e-17,"material"," < +/- 1.0e-17 J/atom");
         internal::mp[super_index].ku4 = ku4;
         internal::enable_uniaxial_fourth_order = true; // Switch on second order tensor calculation for all spins (from spherical harmonics)
         return true;

      }

      // Fourth order uniaxial anisotropy constant (orthonormal)
      test = "4-0-orthonormal-tesseral-harmonic-anisotropy-constant";
      if( word == test )
      {

         double ku4 = - atof( value.c_str() ) * ( 3.0 * 35.0 / 16.0 ) * sqrt( 1.0 / M_PI );
         vin::check_for_valid_value(ku4, word, line, prefix, unit, "energy", -1e-17, 1e-17,"material"," < +/- 1.0e-17 J/atom");
         internal::mp[super_index].ku4 = ku4;
         internal::enable_uniaxial_fourth_order = true; // Switch on second order tensor calculation for all spins (from spherical harmonics)
         return true;

      }

      // Fourth order theta first order phi anisotropy constant
      test = "4-1-order-anisotropy-constant";
      test2 = "fourth-order-theta-first-order-phi-anisotropy-constant";
      if( (word == test) || (word == test2) )
      {

         double k4r1 = atof(value.c_str());
         vin::check_for_valid_value(k4r1, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k4r1 = k4r1;
         internal::enable_rotational_4_1_order = true;
         return true;

      }

      // Fourth order theta first order phi anisotropy constant (orthonormal)
      test = "4-1-orthonormal-tesseral-harmonic-anisotropy-constant";
      if( word == test )
      {

         double k4r1 = - atof( value.c_str() ) * ( 3.0 * 7.0 / 8.0 ) * sqrt( 10.0 / M_PI );
         vin::check_for_valid_value(k4r1, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k4r1 = k4r1;
         internal::enable_rotational_4_1_order = true;
         return true;

      }

      // Fourth order theta first order phi odd anisotropy constant
      test = "4--1-order-anisotropy-constant";
      test2 = "fourth-order-theta-first-order-phi-odd-anisotropy-constant";
      if( (word == test) || (word == test2) )
      {

         double k4r1_odd = atof(value.c_str());
         vin::check_for_valid_value(k4r1_odd, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k4r1_odd = k4r1_odd;
         internal::enable_rotational_4_1_order_odd = true;
         return true;

      }

      // Fourth order theta first order phi odd anisotropy constant (orthonormal)
      test = "4--1-orthonormal-tesseral-harmonic-anisotropy-constant";
      if( word == test )
      {

         double k4r1_odd = - atof( value.c_str() ) * ( 3.0 * 7.0 / 8.0 ) * sqrt( 10.0 / M_PI );
         vin::check_for_valid_value(k4r1_odd, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k4r1_odd = k4r1_odd;
         internal::enable_rotational_4_1_order_odd = true;
         return true;

      }

      // Fourth order theta second order phi anisotropy constant
      test = "4-2-order-anisotropy-constant";
      test2 = "fourth-order-theta-second-order-phi-anisotropy-constant";
      if( (word == test) || (word == test2) )
      {

         double k4r2 = atof(value.c_str());
         vin::check_for_valid_value(k4r2, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k4r2 = k4r2;
         internal::enable_rotational_4_2_order = true;
         return true;

      }

      // Fourth order theta second order phi anisotropy constant (orthonormal)
      test = "4-2-orthonormal-tesseral-harmonic-anisotropy-constant";
      if( word == test )
      {

         double k4r2 = - atof( value.c_str() ) * ( 3.0 * 7.0 ) * sqrt( 5.0 / M_PI );
         vin::check_for_valid_value(k4r2, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k4r2 = k4r2;
         internal::enable_rotational_4_2_order = true;
         return true;

      }

      // Fourth order theta second order phi odd anisotropy constant
      test = "4--2-order-anisotropy-constant";
      test2 = "fourth-order-theta-second-order-phi-odd-anisotropy-constant";
      if( (word == test) || (word == test2) )
      {

         double k4r2_odd = atof(value.c_str());
         vin::check_for_valid_value(k4r2_odd, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k4r2_odd = k4r2_odd;
         internal::enable_rotational_4_2_order_odd = true;
         return true;

      }

      // Fourth order theta second order phi odd anisotropy constant (orthonormal)
      test = "4--2-orthonormal-tesseral-harmonic-anisotropy-constant";
      if( word == test )
      {

         double k4r2_odd = - atof( value.c_str() ) * ( 3.0 * 7.0 ) * sqrt( 5.0 / M_PI );
         vin::check_for_valid_value(k4r2_odd, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k4r2_odd = k4r2_odd;
         internal::enable_rotational_4_2_order_odd = true;
         return true;

      }

      // Fourth order theta third order phi anisotropy constant
      test = "4-3-order-anisotropy-constant";
      test2 = "fourth-order-theta-third-order-phi-anisotropy-constant";
      if( (word == test) || (word == test2) )
      {

         double k4r3 = atof(value.c_str());
         vin::check_for_valid_value(k4r3, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k4r3 = k4r3;
         internal::enable_rotational_4_3_order = true;
         return true;

      }

      // Fourth order theta third order phi anisotropy constant (orthonormal)
      test = "4-3-orthonormal-tesseral-harmonic-anisotropy-constant";
      if( word == test )
      {

         double k4r3 = - atof( value.c_str() ) * ( 3.0 / 8.0 ) * sqrt( 70.0 / M_PI );
         vin::check_for_valid_value(k4r3, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k4r3 = k4r3;
         internal::enable_rotational_4_3_order = true;
         return true;

      }

      // Fourth order theta third order phi odd anisotropy constant
      test = "4--3-order-anisotropy-constant";
      test2 = "fourth-order-theta-third-order-phi-odd-anisotropy-constant";
      if( (word == test) || (word == test2) )
      {

         double k4r3_odd = atof(value.c_str());
         vin::check_for_valid_value(k4r3_odd, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k4r3_odd = k4r3_odd;
         internal::enable_rotational_4_3_order_odd = true;
         return true;

      }

      // Fourth order theta third order phi odd anisotropy constant (orthonormal)
      test = "4--3-orthonormal-tesseral-harmonic-anisotropy-constant";
      if( word == test )
      {

         double k4r3_odd = - atof( value.c_str() ) * ( 3.0 / 8.0 ) * sqrt( 70.0 / M_PI );
         vin::check_for_valid_value(k4r3_odd, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k4r3_odd = k4r3_odd;
         internal::enable_rotational_4_3_order_odd = true;
         return true;

      }

      // Fourth order theta fourth order phi rotational anisotropy constant
      test = "4-4-order-anisotropy-constant";
      test2 = "fourth-order-theta-fourth-order-phi-anisotropy-constant";
      test3 = "fourth-order-rotational-anisotropy-constant";
      if( (word == test) || (word == test2) || (word == test3) )
      {

         double k4r4 = atof(value.c_str());
         vin::check_for_valid_value(k4r4, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k4r4 = k4r4;
         internal::enable_rotational_4_4_order = true;
         return true;

      }

      // Fourth order theta fourth order phi rotational anisotropy constant (orthonormal)
      test = "4-4-orthonormal-tesseral-harmonic-anisotropy-constant";
      if( word == test )
      {

         double k4r4 = - atof( value.c_str() ) * ( 3.0 / 16.0 ) * sqrt( 35.0 / M_PI );
         vin::check_for_valid_value(k4r4, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k4r4 = k4r4;
         internal::enable_rotational_4_4_order = true;
         return true;

      }

      // Fourth order theta fourth order phi odd anisotropy constant
      test = "4--4-order-anisotropy-constant";
      test2 = "fourth-order-theta-fourth-order-phi-odd-anisotropy-constant";
      test3 = "fourth-order-odd-rotational-anisotropy-constant";
      if( (word == test) || (word == test2) || (word == test3) )
      {

         double k4r4_odd = atof(value.c_str());
         vin::check_for_valid_value(k4r4_odd, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k4r4_odd = k4r4_odd;
         internal::enable_rotational_4_4_order_odd = true;
         return true;

      }

      // Fourth order theta fourth order phi odd anisotropy constant (orthonormal)
      test = "4--4-orthonormal-tesseral-harmonic-anisotropy-constant";
      if( word == test )
      {

         double k4r4_odd = atof( value.c_str() ) * ( 3.0 / 16.0 ) * sqrt( 35.0 / M_PI );
         vin::check_for_valid_value(k4r4_odd, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k4r4_odd = k4r4_odd;
         internal::enable_rotational_4_4_order_odd = true;
         return true;

      }

      // Sixth order uniaxial anisotropy constant
      test = "6-0-order-anisotropy-constant";
      test2 = "sixth-order-uniaxial-anisotropy-constant";
      if( (word == test) || (word == test2) )
      {

         double ku6 = atof(value.c_str());
         vin::check_for_valid_value(ku6, word, line, prefix, unit, "energy", -1e-17, 1e-17,"material"," < +/- 1.0e-17 J/atom");
         internal::mp[super_index].ku6 = ku6;
         internal::enable_uniaxial_sixth_order = true; // Switch on second order tensor calculation for all spins (from spherical harmonics)
         return true;

      }

      // Sixth order uniaxial anisotropy constant (orthonormal)
      test = "6-0-orthonormal-tesseral-harmonic-anisotropy-constant";
      if ( word == test )
      {

         double ku6 = - atof( value.c_str() ) * ( 231.0 / 32.0 ) * sqrt( 13.0 / M_PI );
         vin::check_for_valid_value(ku6, word, line, prefix, unit, "energy", -1e-17, 1e-17,"material"," < +/- 1.0e-17 J/atom");
         internal::mp[super_index].ku6 = ku6;
         internal::enable_uniaxial_sixth_order = true; // Switch on second order tensor calculation for all spins (from spherical harmonics)
         return true;

      }

      // Sixth order theta first order phi anisotropy constant
      test = "6-1-order-anisotropy-constant";
      test2 = "sixth-order-theta-first-order-phi-anisotropy-constant";
      if( (word == test) || (word == test2) )
      {

         double k6r1 = atof(value.c_str());
         vin::check_for_valid_value(k6r1, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k6r1 = k6r1;
         internal::enable_rotational_6_1_order = true;
         return true;

      }

      // Sixth order theta first order phi anisotropy constant (orthonormal)
      test = "6-1-orthonormal-tesseral-harmonic-anisotropy-constant";
      if( word == test )
      {

         double k6r1 = - atof( value.c_str() ) * ( 33.0 / 16.0 ) * sqrt( 273.0 / M_PI );
         vin::check_for_valid_value(k6r1, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k6r1 = k6r1;
         internal::enable_rotational_6_1_order = true;
         return true;

      }

      // Sixth order theta first order phi odd anisotropy constant
      test = "6--1-order-anisotropy-constant";
      test2 = "sixth-order-theta-first-order-phi-odd-anisotropy-constant";
      if( (word == test) || (word == test2) )
      {

         double k6r1_odd = atof(value.c_str());
         vin::check_for_valid_value(k6r1_odd, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k6r1_odd = k6r1_odd;
         internal::enable_rotational_6_1_order_odd = true;
         return true;

      }

      // Sixth order theta first order phi odd anisotropy constant (orthonormal)
      test = "6--1-orthonormal-tesseral-harmonic-anisotropy-constant";
      if( (word == test) )
      {

         double k6r1_odd = - atof( value.c_str() ) * ( 33.0 / 16.0 ) * sqrt( 273.0 / M_PI );
         vin::check_for_valid_value(k6r1_odd, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k6r1_odd = k6r1_odd;
         internal::enable_rotational_6_1_order_odd = true;
         return true;

      }

      // Sixth order theta second order phi anisotropy constant
      test = "6-2-order-anisotropy-constant";
      test2 = "sixth-order-theta-second-order-phi-anisotropy-constant";
      if( (word == test) || (word == test2) )
      {

         double k6r2 = atof( value.c_str() );
         vin::check_for_valid_value(k6r2, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k6r2 = k6r2;
         internal::enable_rotational_6_2_order = true;
         return true;

      }

      // Sixth order theta second order phi anisotropy constant (orthonormal)
      test = "6-2-orthonormal-tesseral-harmonic-anisotropy-constant";
      if( word == test )
      {

         double k6r2 = - atof( value.c_str() ) * ( 33.0 / 64.0 ) * sqrt( 2730.0 / M_PI );
         vin::check_for_valid_value(k6r2, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k6r2 = k6r2;
         internal::enable_rotational_6_2_order = true;
         return true;

      }

      // Sixth order theta second order phi odd anisotropy constant
      test = "6--2-order-anisotropy-constant";
      test2 = "sixth-order-theta-second-order-phi-odd-anisotropy-constant";
      if( (word == test) || (word == test2) )
      {

         double k6r2_odd = atof( value.c_str() );
         vin::check_for_valid_value(k6r2_odd, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k6r2_odd = k6r2_odd;
         internal::enable_rotational_6_2_order_odd = true;
         return true;

      }

      // Sixth order theta second order phi odd anisotropy constant (orthonormal)
      test = "6--2-orthonormal-tesseral-harmonic-anisotropy-constant";
      if( word == test )
      {

         double k6r2_odd = - atof( value.c_str() ) * ( 33.0 / 64.0 ) * sqrt( 2730.0 / M_PI );
         vin::check_for_valid_value(k6r2_odd, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k6r2_odd = k6r2_odd;
         internal::enable_rotational_6_2_order_odd = true;
         return true;

      }

      // Sixth order theta third order phi anisotropy constant
      test = "6-3-order-anisotropy-constant";
      test2 = "sixth-order-theta-third-order-phi-anisotropy-constant";
      if( (word == test) || (word == test2) )
      {

         double k6r3 = atof( value.c_str() );
         vin::check_for_valid_value(k6r3, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k6r3 = k6r3;
         internal::enable_rotational_6_3_order = true;
         return true;

      }

      // Sixth order theta third order phi anisotropy constant
      test = "6-3-orthonormal-tesseral-harmonic-anisotropy-constant";
      if( word == test )
      {

         double k6r3 = - atof( value.c_str() ) * ( 11.0 / 32.0 ) * sqrt( 2730.0 / M_PI );
         vin::check_for_valid_value(k6r3, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k6r3 = k6r3;
         internal::enable_rotational_6_3_order = true;
         return true;

      }

      // Sixth order theta third order phi odd anisotropy constant
      test = "6--3-order-anisotropy-constant";
      test2 = "sixth-order-theta-third-order-phi-odd-anisotropy-constant";
      if( (word == test) || (word == test2) )
      {

         double k6r3_odd = atof( value.c_str() );
         vin::check_for_valid_value(k6r3_odd, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k6r3_odd = k6r3_odd;
         internal::enable_rotational_6_3_order_odd = true;
         return true;

      }

      // Sixth order theta third order phi odd anisotropy constant (orthonormal)
      test = "6--3-orthonormal-tesseral-harmonic-anisotropy-constant";
      if( word == test )
      {

         double k6r3_odd = - atof( value.c_str() ) * ( 11.0 / 32.0 ) * sqrt( 2730.0 / M_PI );
         vin::check_for_valid_value(k6r3_odd, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k6r3_odd = k6r3_odd;
         internal::enable_rotational_6_3_order_odd = true;
         return true;

      }

      // Sixth order theta fourth order phi anisotropy constant
      test = "6-4-order-anisotropy-constant";
      test2 = "sixth-order-theta-fourth-order-phi-anisotropy-constant";
      if( (word == test) || (word == test2) )
      {

         double k6r4 = atof(value.c_str());
         vin::check_for_valid_value(k6r4, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k6r4 = k6r4;
         internal::enable_rotational_6_4_order = true;
         return true;

      }

      // Sixth order theta fourth order phi anisotropy constant (orthonormal)
      test = "6-4-orthonormal-tesseral-harmonic-anisotropy-constant";
      if( word == test )
      {

         double k6r4 = - atof( value.c_str() ) * ( 3.0 * 11.0 / 32.0 ) * sqrt( 91.0 / M_PI );
         vin::check_for_valid_value(k6r4, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k6r4 = k6r4;
         internal::enable_rotational_6_4_order = true;
         return true;

      }

      // Sixth order theta fourth order phi odd anisotropy constant
      test = "6--4-order-anisotropy-constant";
      test2 = "sixth-order-theta-fourth-order-phi-odd-anisotropy-constant";
      if( (word == test) || (word == test2) )
      {

         double k6r4_odd = atof( value.c_str() );
         vin::check_for_valid_value(k6r4_odd, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k6r4_odd = k6r4_odd;
         internal::enable_rotational_6_4_order_odd = true;
         return true;

      }

      // Sixth order theta fourth order phi odd anisotropy constant (orthonormal)
      test = "6--4-orthonormal-tesseral-harmonic-anisotropy-constant";
      if( word == test )
      {

         double k6r4_odd = - atof( value.c_str() ) * ( 3.0 * 11.0 / 32.0 ) * sqrt( 91.0 / M_PI );
         vin::check_for_valid_value(k6r4_odd, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k6r4_odd = k6r4_odd;
         internal::enable_rotational_6_4_order_odd = true;
         return true;

      }

      // Sixth order theta fifth order phi anisotropy constant
      test = "6-5-order-anisotropy-constant";
      test2 = "sixth-order-theta-fifth-order-phi-anisotropy-constant";
      if( (word == test) || (word == test2) )
      {

         double k6r5 = atof( value.c_str() );
         vin::check_for_valid_value(k6r5, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k6r5 = k6r5;
         internal::enable_rotational_6_5_order = true;
         return true;

      }

      // Sixth order theta fifth order phi anisotropy constant (orthonormal)
      test = "6-5-orthonormal-tesseral-harmonic-anisotropy-constant";
      if( word == test )
      {

         double k6r5 = - atof( value.c_str() ) * ( 3.0 / 32.0 ) * sqrt( 2002.0 / M_PI );
         vin::check_for_valid_value(k6r5, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k6r5 = k6r5;
         internal::enable_rotational_6_5_order = true;
         return true;

      }

      // Sxith order theta fifth order phi odd anisotropy constant
      test = "6--5-order-anisotropy-constant";
      test2 = "sixth-order-theta-fifth-order-phi-odd-anisotropy-constant";
      if( (word == test) || (word == test2) )
      {

         double k6r5_odd = atof( value.c_str() );
         vin::check_for_valid_value(k6r5_odd, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k6r5_odd = k6r5_odd;
         internal::enable_rotational_6_5_order_odd = true;
         return true;

      }

      // Sxith order theta fifth order phi odd anisotropy constant (orthonormal)
      test = "6--5-orthonormal-tesseral-harmonic-anisotropy-constant";
      if( word == test )
      {

         double k6r5_odd = - atof( value.c_str() ) * ( 3.0 / 32.0 ) * sqrt( 2002.0 / M_PI );
         vin::check_for_valid_value(k6r5_odd, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k6r5_odd = k6r5_odd;
         internal::enable_rotational_6_5_order_odd = true;
         return true;

      }

      // Sixth order theta sixth order phi anisotropy constant
      test = "6-6-order-anisotropy-constant";
      test2 = "sixth-order-theta-sixth-order-phi-anisotropy-constant";
      test3 = "sixth-order-rotational-anisotropy-constant";
      if( (word == test) || (word == test2) || (word == test3) )
      {

         double k6r6 = atof( value.c_str() );
         vin::check_for_valid_value(k6r6, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k6r6 = k6r6;
         internal::enable_rotational_6_6_order = true;
         return true;

      }

      // Sixth order theta sixth order phi anisotropy constant (orthonormal)
      test = "6-6-orthonormal-tesseral-harmonic-anisotropy-constant";
      if( word == test )
      {

         double k6r6 = - atof( value.c_str() ) * ( 1.0 / 64.0 ) * sqrt( 6006.0 / M_PI );
         vin::check_for_valid_value(k6r6, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k6r6 = k6r6;
         internal::enable_rotational_6_6_order = true;
         return true;

      }

      // Sixth order theta sixth order phi odd anisotropy constant
      test = "6--6-order-anisotropy-constant";
      test2 = "sixth-order-theta-sixth-order-phi-odd-anisotropy-constant";
      test3 = "sixth-order-odd-rotational-anisotropy-constant";
      if( (word == test) || (word == test2) || (word == test3) )
      {

         double k6r6_odd = atof( value.c_str() );
         vin::check_for_valid_value(k6r6_odd, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k6r6_odd = k6r6_odd;
         internal::enable_rotational_6_6_order_odd = true;
         return true;

      }

      // Sixth order theta sixth order phi odd anisotropy constant (orthonormal)
      test = "6--6-orthonormal-tesseral-harmonic-anisotropy-constant";
      if( word == test )
      {

         double k6r6_odd = - atof( value.c_str() ) * ( 1.0 / 64.0 ) * sqrt( 6006.0 / M_PI );
         vin::check_for_valid_value(k6r6_odd, word, line, prefix, unit, "energy", -1e-17, 1e-17, "material", " < +/-1.0e-17 J/atom");
         internal::mp[super_index].k6r6_odd = k6r6_odd;
         internal::enable_rotational_6_6_order_odd = true;
         return true;

      }

      //------------------------------------------------------------
      // Triaxial anisotropy in second and fourth order
      test="second-order-triaxial-anisotropy-vector";
      if(word == test){
         std::vector<double> u(3);
         // read values from string
         u = vin::doubles_from_string(value);
         //std::cout << u[0] << '\t' << u[1] << "\t" << u[2] << std::endl;
         // check for sane input and normalise if necessary
         vin::check_for_valid_vector(u, word, line, prefix, unit, "anisotropy", -1e-10, 1e-10,"material"," < +/- 1.0e-10");
         // Copy sanitised unit vector to material
         internal::ku_triaxial_vector_x[super_index] = u[0];
         internal::ku_triaxial_vector_y[super_index] = u[1];
         internal::ku_triaxial_vector_z[super_index] = u[2];
         internal::enable_triaxial_anisotropy = true;
         return true;

      }
      test="fourth-order-triaxial-anisotropy-vector";
      if(word == test){
         std::vector<double> u(3);
         // read values from string
         u = vin::doubles_from_string(value);
         //std::cout << u[0] << '\t' << u[1] << "\t" << u[2] << std::endl;
         // check for sane input and normalise if necessary
         vin::check_for_valid_vector(u, word, line, prefix, unit, "anisotropy", -1e-10, 1e-10,"material"," < +/- 1.0e-10");
         // Copy sanitised unit vector to material
         internal::ku4_triaxial_vector_x[super_index] = u[0];
         internal::ku4_triaxial_vector_y[super_index] = u[1];
         internal::ku4_triaxial_vector_z[super_index] = u[2];
         internal::enable_triaxial_fourth_order = true;
         return true;

      }

      test="second-order-triaxial-basis-vector-1";
      if(word == test){
         std::vector<double> u(3);
         // read values from string
         u = vin::doubles_from_string(value);
         //std::cout << u[0] << '\t' << u[1] << "\t" << u[2] << std::endl;
         // check for sane input and normalise if necessary
         vin::check_for_valid_unit_vector(u, word, line, prefix, "material");
         // Copy sanitised unit vector to material
         internal::ku_triaxial_basis1x[super_index] = u[0];
         internal::ku_triaxial_basis1y[super_index] = u[1];
         internal::ku_triaxial_basis1z[super_index] = u[2];
         internal::triaxial_second_order_fixed_basis[super_index] = false;
      //   std::cout << u[0] << '\t' << u[1] << '\t' << u[2] << "\t" << super_index <<  std::endl;
      //   std::cin.get();
         return true;

      }

      test="second-order-triaxial-basis-vector-2";
      if(word == test){
         std::vector<double> u(3);
         // read values from string
         u = vin::doubles_from_string(value);
         //std::cout << u[0] << '\t' << u[1] << "\t" << u[2] << std::endl;
         // check for sane input and normalise if necessary
        vin::check_for_valid_unit_vector(u, word, line, prefix, "material");
         // Copy sanitised unit vector to material
         internal::ku_triaxial_basis2x[super_index] = u[0];
         internal::ku_triaxial_basis2y[super_index] = u[1];
         internal::ku_triaxial_basis2z[super_index] = u[2];
         internal::triaxial_second_order_fixed_basis[super_index] = false;
         return true;

      }

      test="second-order-triaxial-basis-vector-3";
      if(word == test){
         std::vector<double> u(3);
         // read values from string
         u = vin::doubles_from_string(value);
         //std::cout << u[0] << '\t' << u[1] << "\t" << u[2] << std::endl;
         // check for sane input and normalise if necessary
         vin::check_for_valid_unit_vector(u, word, line, prefix, "material");
         // Copy sanitised unit vector to material
         internal::ku_triaxial_basis3x[super_index] = u[0];
         internal::ku_triaxial_basis3y[super_index] = u[1];
         internal::ku_triaxial_basis3z[super_index] = u[2];
         internal::triaxial_second_order_fixed_basis[super_index] = false;
         return true;

      }

      test="fourth-order-triaxial-basis-vector-1";
      if(word == test){
         std::vector<double> u(3);
         // read values from string
         u = vin::doubles_from_string(value);
         //std::cout << u[0] << '\t' << u[1] << "\t" << u[2] << std::endl;
         // check for sane input and normalise if necessary
         vin::check_for_valid_unit_vector(u, word, line, prefix, "material");
         // Copy sanitised unit vector to material
         internal::ku4_triaxial_basis1x[super_index] = u[0];
         internal::ku4_triaxial_basis1y[super_index] = u[1];
         internal::ku4_triaxial_basis1z[super_index] = u[2];
         internal::triaxial_fourth_order_fixed_basis[super_index] = false;
      //   std::cout << u[0] << '\t' << u[1] << '\t' << u[2] << "\t" << super_index <<  std::endl;
      //   std::cin.get();
         return true;

      }

      test="fourth-order-triaxial-basis-vector-2";
      if(word == test){
         std::vector<double> u(3);
         // read values from string
         u = vin::doubles_from_string(value);
         //std::cout << u[0] << '\t' << u[1] << "\t" << u[2] << std::endl;
         // check for sane input and normalise if necessary
         vin::check_for_valid_unit_vector(u, word, line, prefix, "material");
         // Copy sanitised unit vector to material
         internal::ku4_triaxial_basis2x[super_index] = u[0];
         internal::ku4_triaxial_basis2y[super_index] = u[1];
         internal::ku4_triaxial_basis2z[super_index] = u[2];
         internal::triaxial_fourth_order_fixed_basis[super_index] = false;
         return true;

      }

      test="fourth-order-triaxial-basis-vector-3";
      if(word == test){
         std::vector<double> u(3);
         // read values from string
         u = vin::doubles_from_string(value);
         //std::cout << u[0] << '\t' << u[1] << "\t" << u[2] << std::endl;
         // check for sane input and normalise if necessary
         vin::check_for_valid_unit_vector(u, word, line, prefix, "material");
         // Copy sanitised unit vector to material
         internal::ku4_triaxial_basis3x[super_index] = u[0];
         internal::ku4_triaxial_basis3y[super_index] = u[1];
         internal::ku4_triaxial_basis3z[super_index] = u[2];
         internal::triaxial_fourth_order_fixed_basis[super_index] = false;
         return true;

      }


      //------------------------------------------------------------
      //Implementation of biaxial fourth-order anisotropy (simple version)
      test = "fourth-order-biaxial-anisotropy-constant";
      if( word == test ){
         double ku4 = atof(value.c_str());
         vin::check_for_valid_value(ku4, word, line, prefix, unit, "energy", -1e-17, 1e-17,"material"," < +/- 1.0e-17 J/atom");
         internal::mp[super_index].ku4 = ku4;
         internal::enable_biaxial_fourth_order_simple = true;
         return true;
      }
      
      //Minimal orthogonality
      test = "fourth-order-cubic-anisotropy-constant"; // new form (preferred)
      test2  = "cubic-anisotropy-constant"; // legacy form (deprecated but probably never obsoleted)
      if( (word == test) || (word == test2) ){
         double kc4 = atof(value.c_str());
         // Test for valid range
         vin::check_for_valid_value(kc4, word, line, prefix, unit, "energy", -1e-17, 1e-17,"material"," < +/- 1.0e-17 J/atom");
         internal::mp[super_index].kc4 = kc4;
         if(internal::enable_cubic_fourth_order_rotation == false) internal::enable_cubic_fourth_order = true; // Switch on second order tensor calculation for all spins (from spherical harmonics)
         return true;
      }
      //------------------------------------------------------------
      //Minimal orthogonality
      test = "sixth-order-cubic-anisotropy-constant";
      if( word == test ){
         double kc6 = atof(value.c_str());
         // Test for valid range
         vin::check_for_valid_value(kc6, word, line, prefix, unit, "energy", -1e-17, 1e-17,"material"," < +/- 1.0e-17 J/atom");
         internal::mp[super_index].kc6 = kc6;
         internal::enable_cubic_sixth_order = true; // Switch on second order tensor calculation for all spins (from spherical harmonics)
         return true;
      }
      //------------------------------------------------------------
      test="neel-anisotropy-constant"; // new and preferred form
      test2="surface-anisotropy-constant"; // deprecated but never obsolete form
      if( (word == test) || (word == test2) ){
         double kij = atof(value.c_str());
         vin::check_for_valid_value(kij, word, line, prefix, unit, "energy", -1e-17, 1e-17,"material"," < +/- 1.0e17");
         internal::mp[super_index].kij[sub_index] = kij;
         internal::enable_neel_anisotropy = true;
         return true;
      }
      //------------------------------------------------------------
      test = "lattice-anisotropy-constant";
      if( word == test ){
         double kl = atof(value.c_str());
         // Test for valid range
         vin::check_for_valid_value(kl, word, line, prefix, unit, "energy", -1.0e-17, 1.0e17,"material","-1e17 - 1e17 J/atom");
         internal::mp[super_index].k_lattice = kl;
         internal::enable_lattice_anisotropy = true;
         return true;
      }
      //------------------------------------------------------------
      test="lattice-anisotropy-file";
      if(word==test){

         // Open lattice file
         std::stringstream latt_file;
         latt_file.str( vin::get_string(value.c_str(), "material", line) );

         // specify number of points to be read
         int num_pts=0;

         // Read in number of temperature points
         latt_file >> num_pts;

         // Check for valid number of points
         if(num_pts<=1){
            std::cerr << "Error in lattice-anisotropy-file " << value.c_str() << " on line " << line << " of material file. The first number must be an integer greater than 1. Exiting." << std::endl;
            zlog << zTs() << "Error in lattice-anisotropy-file " << value.c_str() << " on line " << line << " of material file. The first number must be an integer greater than 1. Exiting." << std::endl;
            return false;
         }

         // Loop over all lines in file
         for(int c=0;c<num_pts;c++){

            // temporary variables
            double T;
            double k;

            // Read in points and add them to material
            latt_file >> T >> k;

            // Check for premature end of file
            if(latt_file.eof()){
               std::cerr << "Error in lattice anisotropy-file " << value.c_str() << " on line " << line << " of material file. End of file reached before reading all values. Exiting" << std::endl;
               zlog << zTs() << "Error in lattice anisotropy-file " << value.c_str() << " on line " << line << " of material file. End of file reached before reading all values. Exiting" << std::endl;
               return false;
            }
            internal::mp[super_index].lattice_anisotropy.add_point(T,k);
         }

         return true;

      }
      //------------------------------------------------------------
      test = "uniaxial-anisotropy-direction";
      if(word == test){
         // set up test comparisons
         test="random";
         test2="random-grain";
         // test for random anisotropy directions
         if( value == test ){
            internal::mp[super_index].random_anisotropy = true;
            internal::mp[super_index].random_grain_anisotropy = false;
            internal::enable_random_anisotropy = true; // enable initialisation of random anisotropy
         }
         // test for random grain anisotropy
         else if( value == test2 ){
            internal::mp[super_index].random_anisotropy = false;
            internal::mp[super_index].random_grain_anisotropy = true;
            internal::enable_random_anisotropy = true;
         }
         // set easy axis unit vector for material
         else{
            // temporary storage container
            std::vector<double> u(3);
            // read values from string
            u = vin::doubles_from_string(value);
            // check for sane input and normalise if necessary
            vin::check_for_valid_unit_vector(u, word, line, prefix, "material");
            // Copy sanitised unit vector to material
            internal::mp[super_index].ku_vector = u;
         }
         return true;
      }
      //------------------------------------------------------------
      test = "rotational-anisotropy-direction";
      if(word == test){
         // temporary storage container
         std::vector<double> r(3);
         // read values from string
         r = vin::doubles_from_string(value);
         // check for sane input and normalise if necessary
         vin::check_for_valid_unit_vector(r, word, line, prefix, "material");
         // copy sanitised unit vector to material
         internal::mp[super_index].kr_vector = r;
         return true;

      }
      //--------------------------------------
      // Direction 1
      //--------------------------------------
      test = "cubic-anisotropy-direction-1";
      if(word == test){
         // temporary storage container
         std::vector<double> u(3);
         // read values from string
         u = vin::doubles_from_string(value);
         // check for sane input and normalise if necessary
         vin::check_for_valid_unit_vector(u, word, line, prefix, "material");
         // Copy sanitised unit vector to material
         internal::mp[super_index].kc_vector1 = u;
         // enable rotated anisotropy and disable normal anisotropy
         internal::enable_cubic_fourth_order_rotation = true;
         internal::enable_cubic_fourth_order = false;
         return true;
      }
      //--------------------------------------
      // Direction 2
      //--------------------------------------
      test = "cubic-anisotropy-direction-2";
      if(word == test){
         // temporary storage container
         std::vector<double> u(3);
         // read values from string
         u = vin::doubles_from_string(value);
         // check for sane input and normalise if necessary
         vin::check_for_valid_unit_vector(u, word, line, prefix, "material");
         // Copy sanitised unit vector to material
         internal::mp[super_index].kc_vector2 = u;
         // enable rotated anisotropy and disable normal anisotropy
         internal::enable_cubic_fourth_order_rotation = true;
         internal::enable_cubic_fourth_order = false;
         return true;
      }
      //------------------------------------------------------------
      /*test="uniaxial-anisotropy-tensor";
      if(word==test){
         std::vector<double> t;
         // read values from string
         t = vin::doubles_from_string(value);
         // check size is nine elements
         if( t.size() != 9 ){
            terminaltextcolor(RED);
            std::cerr << "Error in input file - material[" << super_index << "]:uniaxial-anisotropy-tensor must have nine values." << std::endl;
            terminaltextcolor(WHITE);
            zlog << zTs() << "Error in input file - material[" << super_index << "]:uniaxial-anisotropy-tensor must have nine values." << std::endl;
            return false;
         }
         string unit_type = "energy";
         // if no unit given, assume internal
         if( unit.size() != 0 ){
            units::convert(unit, t, unit_type);
         }
         string str = "energy";
         if( unit_type == str ){
            // Copy anisotropy vector to material
            internal::mp[super_index].ku_tensor = t;
            internal::enable_second_order_tensor = true; // Switch on second order tensor calculation for all spins
            return true;
         }
         else{
            terminaltextcolor(RED);
            std::cerr << "Error - unit type \'" << unit_type << "\' is invalid for parameter \'material:" << word << "\'"<< std::endl;
            terminaltextcolor(WHITE);
            err::vexit();
         }
      }*/
      //------------------------------------------------------------
      /*test="cubic-anisotropy-tensor";
      if(word==test){
         std::vector<double> t;
         // read values from string
         t = vin::doubles_from_string(value);
         // check size is nine elements
         if( t.size() != 9 ){
            terminaltextcolor(RED);
            std::cerr << "Error in input file - material[" << super_index << "]:uniaxial-anisotropy-tensor must have nine values." << std::endl;
            terminaltextcolor(WHITE);
            zlog << zTs() << "Error in input file - material[" << super_index << "]:uniaxial-anisotropy-tensor must have nine values." << std::endl;
            return false;
         }
         string unit_type = "energy";
         // if no unit given, assume internal
         if( unit.size() != 0 ){
            units::convert(unit, t, unit_type);
         }
         string str = "energy";
         if( unit_type == str ){
            // Copy anisotropy vector to material
            internal::mp[super_index].kc_tensor = t;
            internal::enable_fourth_order_tensor = true; // Switch on fourth order tensor calculation for all spins
            return true;
         }
         else{
            terminaltextcolor(RED);
            std::cerr << "Error - unit type \'" << unit_type << "\' is invalid for parameter \'material:" << word << "\'"<< std::endl;
            terminaltextcolor(WHITE);
            err::vexit();
         }
      }*/
      //--------------------------------------------------------------------
      // Keyword not found
      //--------------------------------------------------------------------
      return false;

   }

} // end of anisotropy namespace
