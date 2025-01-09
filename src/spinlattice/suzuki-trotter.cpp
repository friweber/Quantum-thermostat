//------------------------------------------------------------------------------
//
//   This file is part of the VAMPIRE open source package under the
//   Free BSD licence (see licence file for details).
//
//   (c) Mara Strungaru 2022. All rights reserved.
//
//   Email: mara.strungaru@york.ac.uk
//
//   implementation based on the paper Phys. Rev. B 103, 024429, (2021) M.Strungaru, M.O.A. Ellis et al
//------------------------------------------------------------------------------
//

// C++ standard library headers
#include <iostream>
#include <cmath>
#include <vector>

// Vampire headers
#include "atoms.hpp"
#include "create.hpp"
#include "errors.hpp"
#include "material.hpp"
#include "random.hpp"
#include "sim.hpp"
#include "sld.hpp"

//sld module headers M Strungaru
#include "internal.hpp"

namespace sld{
   void compute_forces_fields(const int start_index, // first atom for exchange interactions to be calculated
               const int end_index,
               const std::vector<int>& neighbour_list_start_index,
               const std::vector<int>& neighbour_list_end_index,
               const std::vector<int>& type_array, // type for atom
               const std::vector<int>& neighbour_list_array, // list of interactions between atom
               const std::vector<double>& x0_coord_array, // coord vectors for atoms
               const std::vector<double>& y0_coord_array,
               const std::vector<double>& z0_coord_array,
               std::vector<double>& x_coord_array, // coord vectors for atoms
               std::vector<double>& y_coord_array,
               std::vector<double>& z_coord_array,
               std::vector<double>& forces_array_x, //  vectors for forces
               std::vector<double>& forces_array_y,
               std::vector<double>& forces_array_z){

         return;
               }

     void stats_sld(){
      const int num_atoms=atoms::num_atoms;

      std::fill(sld::internal::fields_array_x.begin(), sld::internal::fields_array_x.end(), 0.0);
      std::fill(sld::internal::fields_array_y.begin(), sld::internal::fields_array_y.end(), 0.0);
      std::fill(sld::internal::fields_array_z.begin(), sld::internal::fields_array_z.end(), 0.0);

      sld::compute_fields(0, // first atom for exchange interactions to be calculated
                        num_atoms, // last +1 atom to be calculated
                        atoms::neighbour_list_start_index,
                        atoms::neighbour_list_end_index,
                        atoms::type_array, // type for atom
                        atoms::neighbour_list_array, // list of interactions between atoms
                        atoms::x_coord_array,
                        atoms::y_coord_array,
                        atoms::z_coord_array,
                        atoms::x_spin_array,
                        atoms::y_spin_array,
                        atoms::z_spin_array,
                        sld::internal::forces_array_x,
                        sld::internal::forces_array_y,
                        sld::internal::forces_array_z,
                        sld::internal::fields_array_x,
                        sld::internal::fields_array_y,
                        sld::internal::fields_array_z);



      sld::J_eff= sld::compute_effective_J(0,atoms::num_atoms,
                  sld::internal::sumJ);
//
      sld::C_eff= sld::compute_effective_C(0,atoms::num_atoms,
                  sld::internal::sumC);



   }



   int suzuki_trotter(){
      const int num_atoms=atoms::num_atoms;
      double cay_dt=-mp::dt/4.0;//-dt4*consts::gyro - mp::dt contains gamma;
      double dt2=0.5*mp::dt_SI*1e12;



      //vectors for thermal noise spin plus lattice
      std::vector <double> Hx_th(atoms::x_spin_array.size());
   	  std::vector <double> Hy_th(atoms::x_spin_array.size());
   	  std::vector <double> Hz_th(atoms::x_spin_array.size());

      generate (Hx_th.begin(),Hx_th.end(), mtrandom::gaussian);
      generate (Hy_th.begin(),Hy_th.end(), mtrandom::gaussian);
      generate (Hz_th.begin(),Hz_th.end(), mtrandom::gaussian);

      //vectors for thermal forces
      std::vector <double> Fx_th(atoms::x_spin_array.size());
      std::vector <double> Fy_th(atoms::x_spin_array.size());
      std::vector <double> Fz_th(atoms::x_spin_array.size());

      generate (Fx_th.begin(),Fx_th.end(), mtrandom::gaussian);
      generate (Fy_th.begin(),Fy_th.end(), mtrandom::gaussian);
      generate (Fz_th.begin(),Fz_th.end(), mtrandom::gaussian);


      std::fill(sld::internal::fields_array_x.begin(), sld::internal::fields_array_x.end(), 0.0);
      std::fill(sld::internal::fields_array_y.begin(), sld::internal::fields_array_y.end(), 0.0);
      std::fill(sld::internal::fields_array_z.begin(), sld::internal::fields_array_z.end(), 0.0);


      /*for(int atom=0;atom<=num_atoms-1;atom++){

         if ( (atom==555)) {
         std::cout<<std::setprecision(15)<<std::endl;

         std::cout <<" b i=atom="<<atom<<"\txyz "<<atoms::x_coord_array[atom]<<"\t"<<atoms::y_coord_array[atom]<<"\t"<<atoms::z_coord_array[atom]<<std::endl;
         std::cout <<" b i=atom="<<atom<<"\tvel "<<atoms::x_velo_array[atom]<<"\t"<<atoms::y_velo_array[atom]<<"\t"<<atoms::z_velo_array[atom]<<std::endl;


         }}*/

      for(int atom=0;atom<=num_atoms-1;atom++){




      sld::compute_fields(atom, // first atom for exchange interactions to be calculated
                        atom+1, // last +1 atom to be calculated
                        atoms::neighbour_list_start_index,
                        atoms::neighbour_list_end_index,
                        atoms::type_array, // type for atom
                        atoms::neighbour_list_array, // list of interactions between atoms
                        atoms::x_coord_array,
                        atoms::y_coord_array,
                        atoms::z_coord_array,
                        atoms::x_spin_array,
                        atoms::y_spin_array,
                        atoms::z_spin_array,
                        sld::internal::forces_array_x,
                        sld::internal::forces_array_y,
                        sld::internal::forces_array_z,
                        sld::internal::fields_array_x,
                        sld::internal::fields_array_y,
                        sld::internal::fields_array_z);

      sld::internal::add_spin_noise(atom,
                  atom+1,
                  mp::dt_SI*1e12,
                  atoms::type_array, // type for atom
                  atoms::x_spin_array,
                  atoms::y_spin_array,
                  atoms::z_spin_array,
                  sld::internal::fields_array_x,
                  sld::internal::fields_array_y,
                  sld::internal::fields_array_z,
                  Hx_th, //  vectors for fields
                  Hy_th,
                  Hz_th);


      sld::internal::cayley_update(atom,
                  atom+1,
                  cay_dt,
                  atoms::x_spin_array,
                  atoms::y_spin_array,
                  atoms::z_spin_array,
                  sld::internal::fields_array_x,
                  sld::internal::fields_array_y,
                  sld::internal::fields_array_z);

      }


      std::fill(sld::internal::fields_array_x.begin(), sld::internal::fields_array_x.end(), 0.0);
      std::fill(sld::internal::fields_array_y.begin(), sld::internal::fields_array_y.end(), 0.0);
      std::fill(sld::internal::fields_array_z.begin(), sld::internal::fields_array_z.end(), 0.0);

      for(int atom=num_atoms-1;atom>=0;atom--){


         sld::compute_fields(atom, // first atom for exchange interactions to be calculated
                           atom+1, // last +1 atom to be calculated
                           atoms::neighbour_list_start_index,
                           atoms::neighbour_list_end_index,
                           atoms::type_array, // type for atom
                           atoms::neighbour_list_array, // list of interactions between atoms
                           atoms::x_coord_array,
                           atoms::y_coord_array,
                           atoms::z_coord_array,
                           atoms::x_spin_array,
                           atoms::y_spin_array,
                           atoms::z_spin_array,
                           sld::internal::forces_array_x,
                           sld::internal::forces_array_y,
                           sld::internal::forces_array_z,
                           sld::internal::fields_array_x,
                           sld::internal::fields_array_y,
                           sld::internal::fields_array_z);


         sld::internal::add_spin_noise(atom,
                     atom+1,
                     mp::dt_SI*1e12,
                     atoms::type_array, // type for atom
                     atoms::x_spin_array,
                     atoms::y_spin_array,
                     atoms::z_spin_array,
                     sld::internal::fields_array_x,
                     sld::internal::fields_array_y,
                     sld::internal::fields_array_z,
                     Hx_th, //  vectors for fields
                     Hy_th,
                     Hz_th);

         sld::internal::cayley_update(atom,
                     atom+1,
                     cay_dt,
                     atoms::x_spin_array,
                     atoms::y_spin_array,
                     atoms::z_spin_array,
                     sld::internal::fields_array_x,
                     sld::internal::fields_array_y,
                     sld::internal::fields_array_z);

      }




      //forces are set to 0 for computation
      std::fill(sld::internal::forces_array_x.begin(), sld::internal::forces_array_x.end(), 0.0);
      std::fill(sld::internal::forces_array_y.begin(), sld::internal::forces_array_y.end(), 0.0);
      std::fill(sld::internal::forces_array_z.begin(), sld::internal::forces_array_z.end(), 0.0);



      sld::compute_fields(0, // first atom for exchange interactions to be calculated
                        num_atoms, // last +1 atom to be calculated
                        atoms::neighbour_list_start_index,
                        atoms::neighbour_list_end_index,
                        atoms::type_array, // type for atom
                        atoms::neighbour_list_array, // list of interactions between atoms
                        atoms::x_coord_array,
                        atoms::y_coord_array,
                        atoms::z_coord_array,
                        atoms::x_spin_array,
                        atoms::y_spin_array,
                        atoms::z_spin_array,
                        sld::internal::forces_array_x,
                        sld::internal::forces_array_y,
                        sld::internal::forces_array_z,
                        sld::internal::fields_array_x,
                        sld::internal::fields_array_y,
                        sld::internal::fields_array_z);

      sld::compute_forces(0, // first atom for exchange interactions to be calculated
                        num_atoms, // last +1 atom to be calculated
                        atoms::neighbour_list_start_index,
                        atoms::neighbour_list_end_index,
                        atoms::type_array, // type for atom
                        atoms::neighbour_list_array, // list of interactions between atoms
                        sld::internal::x0_coord_array, // list of isotropic exchange constants
                        sld::internal::y0_coord_array, // list of vectorial exchange constants
                        sld::internal::z0_coord_array, // list of tensorial exchange constants
                        atoms::x_coord_array,
                        atoms::y_coord_array,
                        atoms::z_coord_array,
                        sld::internal::forces_array_x,
                        sld::internal::forces_array_y,
                        sld::internal::forces_array_z,
                        sld::internal::potential_eng);


     //update position, Velocity
      for(int atom=0;atom<num_atoms;atom++){

      const unsigned int imat = atoms::type_array[atom];
      double dt2_m=0.5*mp::dt_SI*1e12/sld::internal::mp[imat].mass.get();
      double f_eta=1.0-0.5*sld::internal::mp[imat].damp_lat.get()*mp::dt_SI*1e12;
      double velo_noise=sld::internal::mp[imat].F_th_sigma.get()*sqrt(sim::temperature);

       //if during equilibration:
       if (sim::time < sim::equilibration_time) {
              f_eta=1.0-0.5*sld::internal::mp[imat].eq_damp_lat.get()*mp::dt_SI*1e12;
              velo_noise=sld::internal::mp[imat].F_th_sigma_eq.get()*sqrt(sim::temperature);
       }

             atoms::x_velo_array[atom] =  f_eta*atoms::x_velo_array[atom]+ dt2_m * sld::internal::forces_array_x[atom]+dt2*velo_noise*Fx_th[atom];
             atoms::y_velo_array[atom] =  f_eta*atoms::y_velo_array[atom]+ dt2_m * sld::internal::forces_array_y[atom]+dt2*velo_noise*Fy_th[atom];
             atoms::z_velo_array[atom] =  f_eta*atoms::z_velo_array[atom]+ dt2_m * sld::internal::forces_array_z[atom]+dt2*velo_noise*Fz_th[atom];

             atoms::x_coord_array[atom] +=  mp::dt_SI*1e12 * atoms::x_velo_array[atom];
             atoms::y_coord_array[atom] +=  mp::dt_SI*1e12 * atoms::y_velo_array[atom];
             atoms::z_coord_array[atom] +=  mp::dt_SI*1e12 * atoms::z_velo_array[atom];

       }




       //reset forces to 0 for v integration
        std::fill(sld::internal::forces_array_x.begin(), sld::internal::forces_array_x.end(), 0.0);
        std::fill(sld::internal::forces_array_y.begin(), sld::internal::forces_array_y.end(), 0.0);
        std::fill(sld::internal::forces_array_z.begin(), sld::internal::forces_array_z.end(), 0.0);


       sld::compute_fields(0, // first atom for exchange interactions to be calculated
                          num_atoms, // last +1 atom to be calculated
                          atoms::neighbour_list_start_index,
                          atoms::neighbour_list_end_index,
                          atoms::type_array, // type for atom
                          atoms::neighbour_list_array, // list of interactions between atoms
                          atoms::x_coord_array,
                          atoms::y_coord_array,
                          atoms::z_coord_array,
                          atoms::x_spin_array,
                          atoms::y_spin_array,
                          atoms::z_spin_array,
                          sld::internal::forces_array_x,
                          sld::internal::forces_array_y,
                          sld::internal::forces_array_z,
                          sld::internal::fields_array_x,
                          sld::internal::fields_array_y,
                          sld::internal::fields_array_z);


        sld::compute_forces(0, // first atom for exchange interactions to be calculated
                          num_atoms, // last +1 atom to be calculated
                          atoms::neighbour_list_start_index,
                          atoms::neighbour_list_end_index,
                          atoms::type_array, // type for atom
                          atoms::neighbour_list_array, // list of interactions between atoms
                          sld::internal::x0_coord_array, // list of isotropic exchange constants
                          sld::internal::y0_coord_array, // list of vectorial exchange constants
                          sld::internal::z0_coord_array, // list of tensorial exchange constants
                          atoms::x_coord_array,
                          atoms::y_coord_array,
                          atoms::z_coord_array,
                          sld::internal::forces_array_x,
                          sld::internal::forces_array_y,
                          sld::internal::forces_array_z,
                          sld::internal::potential_eng);


      for(int atom=0;atom<num_atoms;atom++){

        const unsigned int imat = atoms::type_array[atom];
        double dt2_m=0.5*mp::dt_SI*1e12/sld::internal::mp[imat].mass.get();
        double f_eta=1.0-0.5*sld::internal::mp[imat].damp_lat.get()*mp::dt_SI*1e12;
        double velo_noise=sld::internal::mp[imat].F_th_sigma.get()*sqrt(sim::temperature);


          //if during equilibration:
          if (sim::time < sim::equilibration_time) {
                f_eta=1.0-0.5*sld::internal::mp[imat].eq_damp_lat.get()*mp::dt_SI*1e12;
                velo_noise=sld::internal::mp[imat].F_th_sigma_eq.get()*sqrt(sim::temperature);
          }


         atoms::x_velo_array[atom] =  f_eta*atoms::x_velo_array[atom] + dt2_m * sld::internal::forces_array_x[atom]+dt2*velo_noise*Fx_th[atom];
         atoms::y_velo_array[atom] =  f_eta*atoms::y_velo_array[atom] + dt2_m * sld::internal::forces_array_y[atom]+dt2*velo_noise*Fy_th[atom];
         atoms::z_velo_array[atom] =  f_eta*atoms::z_velo_array[atom] + dt2_m * sld::internal::forces_array_z[atom]+dt2*velo_noise*Fz_th[atom];


      }


  std::fill(sld::internal::fields_array_x.begin(), sld::internal::fields_array_x.end(), 0.0);
  std::fill(sld::internal::fields_array_y.begin(), sld::internal::fields_array_y.end(), 0.0);
  std::fill(sld::internal::fields_array_z.begin(), sld::internal::fields_array_z.end(), 0.0);


   for(int atom=0;atom<=num_atoms-1;atom++){


   sld::compute_fields(atom, // first atom for exchange interactions to be calculated
                     atom+1, // last +1 atom to be calculated
                     atoms::neighbour_list_start_index,
                     atoms::neighbour_list_end_index,
                     atoms::type_array, // type for atom
                     atoms::neighbour_list_array, // list of interactions between atoms
                     atoms::x_coord_array,
                     atoms::y_coord_array,
                     atoms::z_coord_array,
                     atoms::x_spin_array,
                     atoms::y_spin_array,
                     atoms::z_spin_array,
                     sld::internal::forces_array_x,
                     sld::internal::forces_array_y,
                     sld::internal::forces_array_z,
                     sld::internal::fields_array_x,
                     sld::internal::fields_array_y,
                     sld::internal::fields_array_z);

   sld::internal::add_spin_noise(atom,
               atom+1,
               mp::dt_SI*1e12,
               atoms::type_array, // type for atom
               atoms::x_spin_array,
               atoms::y_spin_array,
               atoms::z_spin_array,
               sld::internal::fields_array_x,
               sld::internal::fields_array_y,
               sld::internal::fields_array_z,
               Hx_th, //  vectors for fields
               Hy_th,
               Hz_th);

   sld::internal::cayley_update(atom,
               atom+1,
               cay_dt,
               atoms::x_spin_array,
               atoms::y_spin_array,
               atoms::z_spin_array,
               sld::internal::fields_array_x,
               sld::internal::fields_array_y,
               sld::internal::fields_array_z);


   }

   std::fill(sld::internal::fields_array_x.begin(), sld::internal::fields_array_x.end(), 0.0);
   std::fill(sld::internal::fields_array_y.begin(), sld::internal::fields_array_y.end(), 0.0);
   std::fill(sld::internal::fields_array_z.begin(), sld::internal::fields_array_z.end(), 0.0);

   for(int atom=num_atoms-1;atom>=0;atom--){

      sld::compute_fields(atom, // first atom for exchange interactions to be calculated
                        atom+1, // last +1 atom to be calculated
                        atoms::neighbour_list_start_index,
                        atoms::neighbour_list_end_index,
                        atoms::type_array, // type for atom
                        atoms::neighbour_list_array, // list of interactions between atoms
                        atoms::x_coord_array,
                        atoms::y_coord_array,
                        atoms::z_coord_array,
                        atoms::x_spin_array,
                        atoms::y_spin_array,
                        atoms::z_spin_array,
                        sld::internal::forces_array_x,
                        sld::internal::forces_array_y,
                        sld::internal::forces_array_z,
                        sld::internal::fields_array_x,
                        sld::internal::fields_array_y,
                        sld::internal::fields_array_z);

      sld::internal::add_spin_noise(atom,
                  atom+1,
                  mp::dt_SI*1e12,
                  atoms::type_array, // type for atom
                  atoms::x_spin_array,
                  atoms::y_spin_array,
                  atoms::z_spin_array,
                  sld::internal::fields_array_x,
                  sld::internal::fields_array_y,
                  sld::internal::fields_array_z,
                  Hx_th, //  vectors for fields
                  Hy_th,
                  Hz_th);

      sld::internal::cayley_update(atom,
                  atom+1,
                  cay_dt,
                  atoms::x_spin_array,
                  atoms::y_spin_array,
                  atoms::z_spin_array,
                  sld::internal::fields_array_x,
                  sld::internal::fields_array_y,
                  sld::internal::fields_array_z);

   }

    /*

      std::fill(sld::internal::fields_array_x.begin(), sld::internal::fields_array_x.end(), 0.0);
      std::fill(sld::internal::fields_array_y.begin(), sld::internal::fields_array_y.end(), 0.0);
      std::fill(sld::internal::fields_array_z.begin(), sld::internal::fields_array_z.end(), 0.0);

      sld::compute_fields(0, // first atom for exchange interactions to be calculated
                        num_atoms, // last +1 atom to be calculated
                        atoms::neighbour_list_start_index,
                        atoms::neighbour_list_end_index,
                        atoms::type_array, // type for atom
                        atoms::neighbour_list_array, // list of interactions between atoms
                        atoms::x_coord_array,
                        atoms::y_coord_array,
                        atoms::z_coord_array,
                        atoms::x_spin_array,
                        atoms::y_spin_array,
                        atoms::z_spin_array,
                        sld::internal::forces_array_x,
                        sld::internal::forces_array_y,
                        sld::internal::forces_array_z,
                        sld::internal::fields_array_x,
                        sld::internal::fields_array_y,
                        sld::internal::fields_array_z);

      sld::spin_temperature= sld::compute_spin_temperature(0,atoms::num_atoms,
                  atoms::type_array, // type for atom
                  atoms::x_spin_array,
                  atoms::y_spin_array,
                  atoms::z_spin_array,
                  sld::internal::fields_array_x,
                  sld::internal::fields_array_y,
                  sld::internal::fields_array_z,
                  mp::mu_s_array);
      //
      sld::lattice_temperature= sld::compute_lattice_temperature(0,atoms::num_atoms,
                  atoms::type_array,
                  atoms::x_velo_array,
                  atoms::y_velo_array,
                  atoms::z_velo_array);*/


               /*   for(int atom=0;atom<=num_atoms-1;atom++){

                     if ( (atom==555)) {
                     std::cout<<std::setprecision(15)<<std::endl;

                     std::cout <<" end i=atom="<<atom<<"\txyz "<<atoms::x_coord_array[atom]<<"\t"<<atoms::y_coord_array[atom]<<"\t"<<atoms::z_coord_array[atom]<<std::endl;
                     std::cout <<" end i=atom="<<atom<<"\tvel "<<atoms::x_velo_array[atom]<<"\t"<<atoms::y_velo_array[atom]<<"\t"<<atoms::z_velo_array[atom]<<std::endl;


                     }}*/


      return EXIT_SUCCESS;
  }

namespace internal{

void cayley_update(const int start_index,
            const int end_index,
            double dt,
            std::vector<double>& x_spin_array, // coord vectors for atoms
            std::vector<double>& y_spin_array,
            std::vector<double>& z_spin_array,
            std::vector<double>& fields_array_x, //  vectors for fields
            std::vector<double>& fields_array_y,
            std::vector<double>& fields_array_z){

      for( int i = start_index; i<end_index; i++)
      {
          double Sx = x_spin_array[i];
          double Sy = y_spin_array[i];
          double Sz = z_spin_array[i];

          double Ax = fields_array_x[i] * dt;
          double Ay = fields_array_y[i] * dt;
          double Az = fields_array_z[i] * dt;

          double AS = Ax*Sx + Ay*Sy + Az*Sz;
          double A2 = Ax * Ax + Ay* Ay + Az * Az;

          double AxSx = Ay * Sz - Az * Sy;
          double AxSy = Az * Sx - Ax * Sz;
          double AxSz = Ax * Sy - Ay * Sx;

          double factor = 1.0 / (1.0 + 0.25 * A2);

          x_spin_array[i] = (Sx * ( 1.0 - 0.25 * A2) + AxSx + 0.5 * Ax * AS) * factor;
          y_spin_array[i] = (Sy * ( 1.0 - 0.25 * A2) + AxSy + 0.5 * Ay * AS) * factor;
          z_spin_array[i] = (Sz * ( 1.0 - 0.25 * A2) + AxSz + 0.5 * Az * AS) * factor;
      }

  return;
}

void add_spin_noise(const int start_index,
            const int end_index,
            double dt,
            const std::vector<int>& type_array, // type for atom
            const std::vector<double>& x_spin_array, // coord vectors for atoms
            const std::vector<double>& y_spin_array,
            const std::vector<double>& z_spin_array,
            std::vector<double>& fields_array_x, //  vectors for fields
            std::vector<double>& fields_array_y,
            std::vector<double>& fields_array_z,
            std::vector<double>& Hx_th, //  vectors for fields
            std::vector<double>& Hy_th,
            std::vector<double>& Hz_th){


     for( int i = start_index; i<end_index; i++)

    {
        const unsigned int imat = atoms::type_array[i];

        double lambda=mp::material[imat].alpha;
        double spin_noise=mp::material[imat].H_th_sigma*sqrt(sim::temperature);


        //if during equilibration:
        if (sim::time < sim::equilibration_time) {
        lambda=mp::material[imat].alpha_eq;
        spin_noise=mp::material[imat].H_th_sigma_eq*sqrt(sim::temperature);
        }



        double Sx = x_spin_array[i];
        double Sy = y_spin_array[i];
        double Sz = z_spin_array[i];

        double Fx = fields_array_x[i] + spin_noise * Hx_th[i];
        double Fy = fields_array_y[i] + spin_noise * Hy_th[i];
        double Fz = fields_array_z[i] + spin_noise * Hz_th[i];

        double FxSx = Fy * Sz - Fz * Sy;
        double FxSy = Fz * Sx - Fx * Sz;
        double FxSz = Fx * Sy - Fy * Sx;

        double inv_l2 = 1.0 / (1.0 + lambda*lambda);

        fields_array_x[i] = (Fx + lambda * FxSx) * inv_l2;
        fields_array_y[i] = (Fy + lambda * FxSy) * inv_l2;
        fields_array_z[i] = (Fz + lambda * FxSz) * inv_l2;
    }

return;
}//end of add_spin_noise


} // end of internal namespace

} // end of sld namespace
