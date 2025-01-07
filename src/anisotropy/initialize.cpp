//------------------------------------------------------------------------------
//
//   This file is part of the VAMPIRE open source package under the
//   Free BSD licence (see licence file for details).
//
//   (c) Jack Collings (2022), Sam Westmoreland and Richard Evans 2017. All
//   rights reserved.
//
//   Email: sw766@york.ac.uk
//          jbc525@york.ac.uk
//
//------------------------------------------------------------------------------
//

// C++ standard library headers
#include <vector>

// Vampire headers
#include "anisotropy.hpp"
#include "errors.hpp"
#include "vio.hpp"

// anisotropy module headers
#include "internal.hpp"

namespace anisotropy{

   //----------------------------------------------------------------------------
   // Function to take in two anisotropy basis vectors, check orthogonality,
   // normalise them, and generate the last basis vector
   //----------------------------------------------------------------------------

   void set_anisotropy_vectors(  std::vector<double>& v1,
                                 std::vector<double>& v2,
                                 std::vector<double>& v3,
                                 int mat){

      // Set primary axis
      double e1[3] = {v1[0], v1[1], v1[2]};
      // Set secondary axis
      double e2[3] = {v2[0], v2[1], v2[2]};

      // Check that vectors are orthogonal
      double e1dote2 = e1[0] * e2[0] + e1[1] * e2[1] + e1[2] * e2[2];
      if (e1dote2 > 1e-9 || e1dote2 < -1e-9){
         std::cerr << "Anisotropy basis vectors for material " << mat << " are not orthogonal. Exiting." << std::endl;
         zlog << zTs() << "Anisotropy basis vectors for material " << mat << " are not orthogonal. Exiting" << std::endl;
         err::vexit();
      }

      // Set final axis as cross product e1 ^ e2
      double e3[3] = {e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0]};

      // Check for zero vectors
      double mod_e1 = sqrt(e1[0] * e1[0] + e1[1] * e1[1] + e1[2] * e1[2]);
      double mod_e2 = sqrt(e2[0] * e2[0] + e2[1] * e2[1] + e2[2] * e2[2]);
      double mod_e3 = sqrt(e3[0] * e3[0] + e3[1] * e3[1] + e3[2] * e3[2]);

      if (mod_e1 < 1e-9 || mod_e2 < 1e-9 || mod_e3 < 1e-9){
         std::cerr << "Anisotropy basis vectors for material " << mat << " are too small. Exiting." << std::endl;
         zlog << zTs() << "Anisotropy basis vectors for material " << mat << " are too small. Exiting" << std::endl;
         err::vexit();
      }

      // Normalise vectors to unit length
      v1[0] = e1[0] / mod_e1;
      v1[1] = e1[1] / mod_e1;
      v1[2] = e1[2] / mod_e1;

      v2[0] = e2[0] / mod_e2;
      v2[1] = e2[1] / mod_e2;
      v2[2] = e2[2] / mod_e2;

      v3[0] = e3[0] / mod_e3;
      v3[1] = e3[1] / mod_e3;
      v3[2] = e3[2] / mod_e3;

   }

   //----------------------------------------------------------------------------
   // Function to initialize anisotropy module
   //----------------------------------------------------------------------------
   void initialize (const unsigned int   num_atoms, // number of atoms
                    std::vector<int>&    atom_material_array, // atoms::atom_type_array
                    std::vector<double>& mu_s_array // array of magnetic moments
                   ){


      //---------------------------------------------------------------------
      // get number of materials for simulation
      //---------------------------------------------------------------------
      int init_num_materials = internal::mp.size();

      // if no anisotropy constants initialised, then make sure anisotropy array is the correct size
      if(init_num_materials == 0) internal::mp.resize(mu_s_array.size());

      // set actual number of materials
      const int num_materials = internal::mp.size();

      // output informative message
      zlog << zTs() << "Initialising data structures for anisotropy calculation for " << num_materials << " materials" << std::endl;

      // check for prior initialisation
      if (internal::initialised){
         zlog << zTs() << "Warning: Anisotropy calculation already initialised. Continuing." << std::endl;
         return;
      }

      //---------------------------------------------------------------------
      // Unroll inverse mu_S array for materials to convert Joules to Tesla
      //---------------------------------------------------------------------
      const double mu_B = 9.27400915e-24; // Bohr magneton
      std::vector <double> inverse_mu_s(num_materials); // array storing inverse spin moment in J/T
      for(int m = 0; m < num_materials; m++) inverse_mu_s[m] = 1.0 / ( mu_s_array[m] * mu_B );

      //---------------------------------------------------------------------
      // Unroll material constants into arrays
      //---------------------------------------------------------------------
      // Second order uniaxial
      if(internal::enable_uniaxial_second_order){
         internal::ku2.resize(num_materials);
         for(int m = 0; m < num_materials; m++) internal::ku2[m] = internal::mp[m].ku2 * inverse_mu_s[m];
      }
      // Second order theta first order phi rotational
      if(internal::enable_rotational_2_1_order){
         internal::k2r1.resize(num_materials);
         for(int m = 0; m < num_materials; m++) internal::k2r1[m] = internal::mp[m].k2r1 * inverse_mu_s[m];
      }
      // Second order theta first order phi odd rotational
      if(internal::enable_rotational_2_1_order_odd){
         internal::k2r1_odd.resize(num_materials);
         for(int m = 0; m < num_materials; m++) internal::k2r1_odd[m] = internal::mp[m].k2r1_odd * inverse_mu_s[m];
      }
      // Second order theta second order phi rotational
      if(internal::enable_rotational_2_2_order){
         internal::k2r2.resize(num_materials);
         for(int m = 0; m < num_materials; ++m) internal::k2r2[m] = internal::mp[m].k2r2 * inverse_mu_s[m];
      }
      // Second order theta second order phi odd rotational
      if(internal::enable_rotational_2_2_order_odd){
         internal::k2r2_odd.resize(num_materials);
         for(int m = 0; m < num_materials; ++m) internal::k2r2_odd[m] = internal::mp[m].k2r2_odd * inverse_mu_s[m];
      }
      // Fourth order uniaxial
      if(internal::enable_uniaxial_fourth_order){
         internal::ku4.resize(num_materials);
         for(int m = 0; m < num_materials; m++) internal::ku4[m] = internal::mp[m].ku4 * inverse_mu_s[m];
      }
      // Fourth order theta first order phi rotational
      if(internal::enable_rotational_4_1_order){
         internal::k4r1.resize(num_materials);
         for(int m = 0; m < num_materials; ++m) internal::k4r1[m] = internal::mp[m].k4r1 * inverse_mu_s[m];
      }
      // Fourth order theta first order phi odd rotational
      if(internal::enable_rotational_4_1_order_odd){
         internal::k4r1_odd.resize(num_materials);
         for(int m = 0; m < num_materials; ++m) internal::k4r1_odd[m] = internal::mp[m].k4r1_odd * inverse_mu_s[m];
      }
      // Fourth order theta second order phi rotational
      if(internal::enable_rotational_4_2_order){
         internal::k4r2.resize(num_materials);
         for(int m = 0; m < num_materials; ++m) internal::k4r2[m] = internal::mp[m].k4r2 * inverse_mu_s[m];
      }
      // Fourth order theta second order phi odd rotational
      if(internal::enable_rotational_4_2_order_odd){
         internal::k4r2_odd.resize(num_materials);
         for(int m = 0; m < num_materials; ++m) internal::k4r2_odd[m] = internal::mp[m].k4r2_odd * inverse_mu_s[m];
      }
      // Fourth order theta third order phi rotational
      if(internal::enable_rotational_4_3_order){
         internal::k4r3.resize(num_materials);
         for(int m = 0; m < num_materials; ++m) internal::k4r3[m] = internal::mp[m].k4r3 * inverse_mu_s[m];
      }
      // Fourth order theta third order phi odd rotational
      if(internal::enable_rotational_4_3_order_odd){
         internal::k4r3_odd.resize(num_materials);
         for(int m = 0; m < num_materials; ++m) internal::k4r3_odd[m] = internal::mp[m].k4r3_odd * inverse_mu_s[m];
      }
      // Fourth order theta fourth order phi rotational
      if(internal::enable_rotational_4_4_order){
         internal::k4r4.resize(num_materials);
         for(int m = 0; m < num_materials; ++m) internal::k4r4[m] = internal::mp[m].k4r4 * inverse_mu_s[m];
      }
      // Fourth order theta fourth order phi odd rotational
      if(internal::enable_rotational_4_4_order_odd){
         internal::k4r4_odd.resize(num_materials);
         for(int m = 0; m < num_materials; ++m) internal::k4r4_odd[m] = internal::mp[m].k4r4_odd * inverse_mu_s[m];
      }
      // Sixth order uniaxial
      if(internal::enable_uniaxial_sixth_order){
         internal::ku6.resize(num_materials);
         for(int m = 0; m < num_materials; m++) internal::ku6[m] = internal::mp[m].ku6 * inverse_mu_s[m];
      }
      // Sixth order theta first order phi rotational
      if(internal::enable_rotational_6_1_order){
         internal::k6r1.resize(num_materials);
         for(int m = 0; m < num_materials; ++m) internal::k6r1[m] = internal::mp[m].k6r1 * inverse_mu_s[m];
      }
      // Sixth order theta first order phi odd rotational
      if(internal::enable_rotational_6_1_order_odd){
         internal::k6r1_odd.resize(num_materials);
         for(int m = 0; m < num_materials; ++m) internal::k6r1_odd[m] = internal::mp[m].k6r1_odd * inverse_mu_s[m];
      }
      // Sixth order theta second order phi rotational
      if(internal::enable_rotational_6_2_order){
         internal::k6r2.resize(num_materials);
         for(int m = 0; m < num_materials; ++m) internal::k6r2[m] = internal::mp[m].k6r2 * inverse_mu_s[m];
      }
      // Sixth order theta second order phi odd rotational
      if(internal::enable_rotational_6_2_order_odd){
         internal::k6r2_odd.resize(num_materials);
         for(int m = 0; m < num_materials; ++m) internal::k6r2_odd[m] = internal::mp[m].k6r2_odd * inverse_mu_s[m];
      }
      // Sixth order theta third order phi rotational
      if(internal::enable_rotational_6_3_order){
         internal::k6r3.resize(num_materials);
         for(int m = 0; m < num_materials; ++m) internal::k6r3[m] = internal::mp[m].k6r3 * inverse_mu_s[m];
      }
      // Sixth order theta third order phi odd rotational
      if(internal::enable_rotational_6_3_order_odd){
         internal::k6r3_odd.resize(num_materials);
         for(int m = 0; m < num_materials; ++m) internal::k6r3_odd[m] = internal::mp[m].k6r3_odd * inverse_mu_s[m];
      }
      // Sixth order theta fourth order phi rotational
      if(internal::enable_rotational_6_4_order){
         internal::k6r4.resize(num_materials);
         for(int m = 0; m < num_materials; ++m) internal::k6r4[m] = internal::mp[m].k6r4 * inverse_mu_s[m];
      }
      // Sixth order theta fourth order phi odd rotational
      if(internal::enable_rotational_6_4_order_odd){
         internal::k6r4_odd.resize(num_materials);
         for(int m = 0; m < num_materials; ++m) internal::k6r4_odd[m] = internal::mp[m].k6r4_odd * inverse_mu_s[m];
      }
      // Sixth order theta fifth order phi rotational
      if(internal::enable_rotational_6_5_order){
         internal::k6r5.resize(num_materials);
         for(int m = 0; m < num_materials; ++m) internal::k6r5[m] = internal::mp[m].k6r5 * inverse_mu_s[m];
      }
      // Sixth order theta fifth order phi odd rotational
      if(internal::enable_rotational_6_5_order_odd){
         internal::k6r5_odd.resize(num_materials);
         for(int m = 0; m < num_materials; ++m) internal::k6r5_odd[m] = internal::mp[m].k6r5_odd * inverse_mu_s[m];
      }
      // Sixth order theta sixth order phi rotational
      if(internal::enable_rotational_6_6_order){
         internal::k6r6.resize(num_materials);
         for(int m = 0; m < num_materials; ++m) internal::k6r6[m] = internal::mp[m].k6r6 * inverse_mu_s[m];
      }
      // Sixth order theta sixth order phi odd rotational
      if(internal::enable_rotational_6_6_order_odd){
         internal::k6r6_odd.resize(num_materials);
         for(int m = 0; m < num_materials; ++m) internal::k6r6_odd[m] = internal::mp[m].k6r6_odd * inverse_mu_s[m];
      }
      // Fourth order biaxial (simple version)
      if(internal::enable_biaxial_fourth_order_simple){
         internal::ku4.resize(num_materials);
         for(int m = 0; m < num_materials; m++) internal::ku4[m] = internal::mp[m].ku4 * inverse_mu_s[m];
      }
      // Fourth order cubic
      if(internal::enable_cubic_fourth_order || internal::enable_cubic_fourth_order_rotation){
         internal::kc4.resize(num_materials);
         for(int m = 0; m < num_materials; m++) internal::kc4[m] = internal::mp[m].kc4 * inverse_mu_s[m];
      }
      // Sixth order cubic
      if(internal::enable_cubic_sixth_order){
         internal::kc6.resize(num_materials);
         for(int m = 0; m < num_materials; m++) internal::kc6[m] = internal::mp[m].kc6 * inverse_mu_s[m];
      }

      //------------------------------------------------------------------------
      // 4th order triaxial
      //------------------------------------------------------------------------
      if(internal::enable_triaxial_fourth_order){

         internal::ku4_triaxial_vector_x.resize(num_materials);
         internal::ku4_triaxial_vector_y.resize(num_materials);
         internal::ku4_triaxial_vector_z.resize(num_materials);

         for(int m = 0; m < num_materials; m++) {
           internal::ku4_triaxial_vector_x[m] = internal::ku4_triaxial_vector_x[m] * inverse_mu_s[m];
           internal::ku4_triaxial_vector_y[m] = internal::ku4_triaxial_vector_y[m] * inverse_mu_s[m];
           internal::ku4_triaxial_vector_z[m] = internal::ku4_triaxial_vector_z[m] * inverse_mu_s[m];
         }

         //check orthogonality for fourth order basis sets
         for (int mat = 0; mat < num_materials; mat ++ ){
            if (!internal::triaxial_fourth_order_fixed_basis[mat]){
               double e1[3] = {internal::ku4_triaxial_basis1x[mat],internal::ku4_triaxial_basis1y[mat],internal::ku4_triaxial_basis1z[mat]};
               double e2[3] = {internal::ku4_triaxial_basis2x[mat],internal::ku4_triaxial_basis2y[mat],internal::ku4_triaxial_basis2z[mat]};
               double e3[3] = {internal::ku4_triaxial_basis3x[mat],internal::ku4_triaxial_basis3y[mat],internal::ku4_triaxial_basis3z[mat]};
               double cross[3] = {0,0,0};
               double onedottwo, onedotthree;

               //if basis 1 has been set in the material file
               if (e1[0] != 0 || e1[1] != 0 || e1[2] != 0){
                  //if basis 2 has been set in the material file
                  if (e2[0] != 0 || e2[1] != 0 || e2[2] != 0){
                     //are basis 1 and 2 orthogonal (1.2 = 0?)
                     onedottwo = e1[0]*e2[0] + e1[1]*e2[1] + e1[2]*e2[2];
                     if (onedottwo < 0.05){
                        //if so work out the 3rd orthoogonal basis set
                        cross[0] = e1[1] * e2[2] - e1[2] * e2[1];
                        cross[1] = e1[2] * e2[0] - e1[0] * e2[2];
                        cross[2] = e1[0] * e2[1] - e1[1] * e2[0];
                        //was the third already set in the input file?
                        if ((e3[0] != 0 || e3[1] != 0 || e3[2] != 0)){
                           //std::cout << "SET" << "\t" << e3[0] << '\t' << e3[1] << '\t' << e3[2] << std::endl;
                           //does it equal the one calcuated if not set to be the orthogonal basis set and print to logfile that i ahve done that
                           if ((cross[0] - e3[0] < 0.05 && cross[1] - e3[1] < 0.05 && cross[2] - e3[2] < 0.05)){}
                           else{
                              e3[0] = cross[0];
                              e3[1] = cross[1];
                              e3[2] = cross[2];
                              std::cerr << "Basis 3 is not orthogonal to basis 1,2 in material " << mat << " changing basis 3 to: (" <<  cross[0] << "," <<cross[1] << "," <<cross[2] << ")" << std::endl;
                              zlog << zTs() << "Basis 3 is not orthogonal to basis 1,2 in material " << mat << " changing basis 3 to: (" <<  cross[0] << "," <<cross[1] << "," <<cross[2] << ")" << std::endl;
                           }
                        }
                        else {
                           e3[0] = cross[0];
                           e3[1] = cross[1];
                           e3[2] = cross[2];
                           //std::cerr << "Basis 3 for material  " << mat << " is set to: (" <<  cross[0] << "," <<cross[1] << "," <<cross[2] << ")" << std::endl;
                           zlog << zTs() << "Basis 3 for material  " << mat << " is set to: (" <<  cross[0] << "," <<cross[1] << "," <<cross[2] << ")" << std::endl;
                        }
                     }
                     else {
                        std::cerr << "Basis 1,2 are not orthogonal for material:" << mat << std::endl;
                        zlog << zTs() << "Basis 1,2 are not orthogonal for material:" << mat << std::endl;
                        err::vexit();
                     }

                  }
                  else if ((e3[0] != 0 || e3[1] != 0 || e3[2] != 0)){
                     onedotthree = e1[0]*e3[0] + e1[1]*e3[1] + e1[2]*e3[2];
                     if (onedotthree < 0.05){
                        //if so work out the 3rd orthoogonal basis set
                        cross[0] = e1[1] * e3[2] - e1[2] * e3[1];
                        cross[1] = e1[2] * e3[0] - e1[0] * e3[2];
                        cross[2] = e1[0] * e3[1] - e1[1] * e3[0];
                        e2[0] = cross[0];
                        e2[1] = cross[1];
                        e2[2] = cross[2];
                        //std::cerr << "Basis 2 for material  " << mat << " is set to: (" <<  cross[0] << "," <<cross[1] << "," <<cross[2] << ")" << std::endl;
                        zlog << zTs() << "Basis 2 for material  " << mat << " is set to: (" <<  cross[0] << "," <<cross[1] << "," <<cross[2] << ")" << std::endl;

                     }
                     else {
                        std::cerr << "Basis 1,3 are not orthogonal for material:" << mat << std::endl;
                        zlog << zTs() << "Basis 1,3 are not orthogonal for material:" << mat << std::endl;
                        err::vexit();
                     }

                  }
                  else {
                     std::cerr << "Only one basis vector set for material:" << mat << " Please specify another basis vector" << std::endl;
                     zlog << zTs() << "Only one basis vector set for material:" << mat << " Please specify another basis vector" << std::endl;
                     err::vexit();
                  }
               }

            }
            else{
               internal::ku4_triaxial_basis1x[mat] = 1;
               internal::ku4_triaxial_basis1y[mat] = 0;
               internal::ku4_triaxial_basis1z[mat] = 0;
               internal::ku4_triaxial_basis2x[mat] = 0;
               internal::ku4_triaxial_basis2y[mat] = 1;
               internal::ku4_triaxial_basis2z[mat] = 0;
               internal::ku4_triaxial_basis3x[mat] = 0;
               internal::ku4_triaxial_basis3y[mat] = 0;
               internal::ku4_triaxial_basis3z[mat] = 1;

            }

         }

         //---------------------------------------------------------------------
         // override which version of triaxial anisotropy is needed
         //---------------------------------------------------------------------
         bool fixed_basis_fourth_order = true;
         for (int mat = 0; mat < num_materials; mat ++ ){
            if (internal::triaxial_fourth_order_fixed_basis[mat] == false){
               fixed_basis_fourth_order = false;
            }
         }
         // if any material requires rotated basis set, then use fancy function
         if(fixed_basis_fourth_order == false){
            internal::enable_triaxial_fourth_order = false;
            internal::enable_triaxial_fourth_order_rotated = true;
         }

      }

      //------------------------------------------------------------------------
      // Second order triaxial
      //------------------------------------------------------------------------
      if(internal::enable_triaxial_anisotropy){

         internal::ku_triaxial_vector_x.resize(num_materials);
         internal::ku_triaxial_vector_y.resize(num_materials);
         internal::ku_triaxial_vector_z.resize(num_materials);

         for(int m = 0; m < num_materials; m++) {
            internal::ku_triaxial_vector_x[m] = internal::ku_triaxial_vector_x[m] * inverse_mu_s[m];
            internal::ku_triaxial_vector_y[m] = internal::ku_triaxial_vector_y[m] * inverse_mu_s[m];
            internal::ku_triaxial_vector_z[m] = internal::ku_triaxial_vector_z[m] * inverse_mu_s[m];
         }

         //check orthogonality for second order basis sets
         for (int mat = 0; mat < num_materials; mat ++ ){
            if (!internal::triaxial_second_order_fixed_basis[mat]){
               double e1[3] = {internal::ku_triaxial_basis1x[mat],internal::ku_triaxial_basis1y[mat],internal::ku_triaxial_basis1z[mat]};
               double e2[3] = {internal::ku_triaxial_basis2x[mat],internal::ku_triaxial_basis2y[mat],internal::ku_triaxial_basis2z[mat]};
               double e3[3] = {internal::ku_triaxial_basis3x[mat],internal::ku_triaxial_basis3y[mat],internal::ku_triaxial_basis3z[mat]};
               double cross[3] = {0,0,0};
               double onedottwo, onedotthree;

               //if basis 1 has been set in the material file
               if (e1[0] != 0 || e1[1] != 0 || e1[2] != 0){
                  //if basis 2 has been set in the material file
                  if (e2[0] != 0 || e2[1] != 0 || e2[2] != 0){
                     //are basis 1 and 2 orthogonal (1.2 = 0?)
                     onedottwo = e1[0]*e2[0] + e1[1]*e2[1] + e1[2]*e2[2];
                     if (onedottwo < 0.05){
                        //if so work out the 3rd orthoogonal basis set
                        cross[0] = e1[1] * e2[2] - e1[2] * e2[1];
                        cross[1] = e1[2] * e2[0] - e1[0] * e2[2];
                        cross[2] = e1[0] * e2[1] - e1[1] * e2[0];
                        //was the third already set in the input file?
                        if ((e3[0] != 0 || e3[1] != 0 || e3[2] != 0)){
                           //std::cout << "SET" << "\t" << e3[0] << '\t' << e3[1] << '\t' << e3[2] << std::endl;
                           //does it equal the one calcuated if not set to be the orthogonal basis set and print to logfile that i ahve done that
                           if ((cross[0] - e3[0] < 0.05 && cross[1] - e3[1] < 0.05 && cross[2] - e3[2] < 0.05)){}
                           else{
                              e3[0] = cross[0];
                              e3[1] = cross[1];
                              e3[2] = cross[2];
                              std::cerr << "Basis 3 is not orthogonal to basis 1,2 in material " << mat << " changing basis 3 to: (" <<  cross[0] << "," <<cross[1] << "," <<cross[2] << ")" << std::endl;
                              zlog << zTs() << "Basis 3 is not orthogonal to basis 1,2 in material " << mat << " changing basis 3 to: (" <<  cross[0] << "," <<cross[1] << "," <<cross[2] << ")" << std::endl;
                           }
                        }
                        else {
                           e3[0] = cross[0];
                           e3[1] = cross[1];
                           e3[2] = cross[2];
                           //std::cerr << "Basis 3 for material  " << mat << " is set to: (" <<  cross[0] << "," <<cross[1] << "," <<cross[2] << ")" << std::endl;
                           zlog << zTs() << "Basis 3 for material  " << mat << " is set to: (" <<  cross[0] << "," <<cross[1] << "," <<cross[2] << ")" << std::endl;
                        }
                     }
                     else {
                        std::cerr << "Basis 1,2 are not orthogonal for material:" << mat << std::endl;
                        zlog << zTs() << "Basis 1,2 are not orthogonal for material:" << mat << std::endl;
                        err::vexit();
                     }

                  }
                  else if ((e3[0] != 0 || e3[1] != 0 || e3[2] != 0)){
                     onedotthree = e1[0]*e3[0] + e1[1]*e3[1] + e1[2]*e3[2];
                     if (onedotthree < 0.05){
                        //if so work out the 3rd orthoogonal basis set
                        cross[0] = e1[1] * e3[2] - e1[2] * e3[1];
                        cross[1] = e1[2] * e3[0] - e1[0] * e3[2];
                        cross[2] = e1[0] * e3[1] - e1[1] * e3[0];
                        e2[0] = cross[0];
                        e2[1] = cross[1];
                        e2[2] = cross[2];
                        std::cerr << "Basis 2 for material  " << mat << " is set to: (" <<  cross[0] << "," <<cross[1] << "," <<cross[2] << ")" << std::endl;
                        zlog << zTs() << "Basis 2 for material  " << mat << " is set to: (" <<  cross[0] << "," <<cross[1] << "," <<cross[2] << ")" << std::endl;

                     }
                     else {
                        std::cerr << "Basis 1,3 are not orthogonal for material:" << mat << std::endl;
                        zlog << zTs() << "Basis 1,3 are not orthogonal for material:" << mat << std::endl;
                        err::vexit();
                     }

                  }
                  else {
                     std::cerr << "Only one basis vector set for material:" << mat << " Please specify another basis vector" << std::endl;
                     zlog << zTs() << "Only one basis vector set for material:" << mat << " Please specify another basis vector" << std::endl;
                     err::vexit();
                  }
               }

            }
            else{
               internal::ku_triaxial_basis1x[mat] = 1;
               internal::ku_triaxial_basis1y[mat] = 0;
               internal::ku_triaxial_basis1z[mat] = 0;
               internal::ku_triaxial_basis2x[mat] = 0;
               internal::ku_triaxial_basis2y[mat] = 1;
               internal::ku_triaxial_basis2z[mat] = 0;
               internal::ku_triaxial_basis3x[mat] = 0;
               internal::ku_triaxial_basis3y[mat] = 0;
               internal::ku_triaxial_basis3z[mat] = 1;

            }
         }

         //---------------------------------------------------------------------
         // override which version of triaxial anisotropy is needed
         //---------------------------------------------------------------------
         bool fixed_basis_second_order = true;
         for (int mat = 0; mat < num_materials; mat ++ ){
            if (!internal::triaxial_second_order_fixed_basis[mat]){
               fixed_basis_second_order = false;
            }
         }
         // if any material requires rotated basis set, then use fancy function
         if(fixed_basis_second_order == false){
            internal::enable_triaxial_anisotropy = false;
            internal::enable_triaxial_anisotropy_rotated = true;
         }

      }

      //---------------------------------------------------------------------
      // Initialise anisotropy basis axes for each material
      //---------------------------------------------------------------------

      internal::ku_vector.resize(num_materials);
      internal::kr_vector.resize(num_materials);
      internal::kl_vector.resize(num_materials);

      // Loop through all materials
      for(int m = 0; m < num_materials; ++m){

         // Vectors defining the anisotropy basis directions

         // Check orthogonality, set orthonormality and generate the last anisotropy vector
         set_anisotropy_vectors(internal::mp[m].ku_vector, internal::mp[m].kr_vector, internal::mp[m].kl_vector, m);

         // Uniaxial easy/hard [0,0,1] (z) axis
         internal::ku_vector[m].x = internal::mp[m].ku_vector[0];
         internal::ku_vector[m].y = internal::mp[m].ku_vector[1];
         internal::ku_vector[m].z = internal::mp[m].ku_vector[2];

         // Rotational [1,0,0] (x) axis
         internal::kr_vector[m].x = internal::mp[m].kr_vector[0];
         internal::kr_vector[m].y = internal::mp[m].kr_vector[1];
         internal::kr_vector[m].z = internal::mp[m].kr_vector[2];

         // Last axis [0,1,0] (y) axis
         internal::kl_vector[m].x = internal::mp[m].kl_vector[0];
         internal::kl_vector[m].y = internal::mp[m].kl_vector[1];
         internal::kl_vector[m].z = internal::mp[m].kl_vector[2];

      }

      //---------------------------------------------------------------------
      // Initialise cubic anisotropy basis axes for each material
      //---------------------------------------------------------------------

      for(int mat = 0; mat < num_materials; ++mat){

         // Check orthogonality, set orthonormality, and generate the last anisotropy vector
         set_anisotropy_vectors(internal::mp[mat].kc_vector1, internal::mp[mat].kc_vector2, internal::mp[mat].kc_vector3, mat);

      }

      //---------------------------------------------------------------------
      // initialise lattice anisotropy for each material
      //---------------------------------------------------------------------
      if(internal::enable_lattice_anisotropy){


         // arrays for storing unrolled parameters for lattice anisotropy
         internal::klattice.resize(num_materials);
         internal::klattice_array.resize(num_materials); // anisoptropy constant

         // loop over all materials and set up lattice anisotropy constants
         for(int m = 0; m < num_materials; m++){

            // save anisotropy constant to unrolled array in units of tesla
            internal::klattice[m] = internal::mp[m].k_lattice * inverse_mu_s[m];

            // set up interpolation between temperature points
            internal::mp[m].lattice_anisotropy.set_interpolation_table();

            // output interpolated data to file
            //internal::mp[m].lattice_anisotropy.output_interpolated_function(mat);

         }

      }

      //---------------------------------------------------------------------
      // set flag after initialization
      //---------------------------------------------------------------------
      internal::initialised = true;

      return;
   }

} // end of anisotropy namespace
