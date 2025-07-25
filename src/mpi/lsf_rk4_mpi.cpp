//------------------------------------------------------------------------------
//
//   This file is part of the VAMPIRE open source package under the
//   Free BSD licence (see licence file for details).
//
//   (c) David R Papp 2024. All rights reserved.
//
//------------------------------------------------------------------------------
//

#ifdef MPICF
#include "atoms.hpp"
#include "errors.hpp"
#include "exchange.hpp"
#include "material.hpp"
#include "sim.hpp"
#include "constants.hpp"
#include "random.hpp"
#include "lsf_rk4.hpp"
#include "vio.hpp"
#include "../simulate/internal.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdio.h>
#include <random>
#include <fstream>

int calculate_spin_fields(const int,const int);
int calculate_external_fields(const int,const int);

namespace sim{

// LSF magnetic field function
void calculate_lsf_rk4_field(const int start_index, const int end_index){

   // Check calling of routine if error checking is activated
	if(err::check==true){std::cout << "calculate_lsf_magnetic_field has been called" << std::endl;}

   // LSF-RK4 Hamiltonian calculation
   for(int atom=start_index;atom<end_index;atom++){

      const int imaterial = atoms::type_array[atom];

      const double sx = atoms::x_spin_array[atom];
      const double sy = atoms::y_spin_array[atom];
      const double sz = atoms::z_spin_array[atom];

      const double imu_S = -1.0/mp::material[imaterial].mu_s_SI;

      const double L2 = 2.0*sim::internal::lsf_second_order_coefficient[imaterial]*imu_S;
      const double L4 = 4.0*sim::internal::lsf_fourth_order_coefficient[imaterial]*imu_S;
      const double L6 = 6.0*sim::internal::lsf_sixth_order_coefficient[imaterial]*imu_S;

   	const double ss2 = sx*sx + sy*sy + sz*sz;

      LSF_RK4_arrays::x_lsf_array[atom] = L2*sx + L4*sx*ss2 + L6*sx*ss2*ss2;
      LSF_RK4_arrays::y_lsf_array[atom] = L2*sy + L4*sy*ss2 + L6*sy*ss2*ss2;
      LSF_RK4_arrays::z_lsf_array[atom] = L2*sz + L4*sz*ss2 + L6*sz*ss2*ss2;

   }

}

int LSF_RK4_mpi(){
	//======================================================
	// Subroutine to perform a single LSF integration step
	//======================================================


	//----------------------------------------------------------
	// check calling of routine if error checking is activated
	//----------------------------------------------------------
	if(err::check==true){std::cout << "LSF_RK4-mpi has been called" << std::endl;}

	using namespace LSF_RK4_arrays;

	// Check for initialisation of LSF integration arrays
	if(LSF_RK4_set==false) sim::LSF_RK4_init();

	//----------------------------------------
	// Local variables for system generation
	//----------------------------------------
	//const int num_atoms = atoms::num_atoms;
	const int pre_comm_si = 0;
	const int pre_comm_ei = vmpi::num_core_atoms;
	const int post_comm_si = vmpi::num_core_atoms;
	const int post_comm_ei = vmpi::num_core_atoms+vmpi::num_bdry_atoms;

	double xyz[3];		/// Local Delta Spin Components
   const double kB = 1.3806503e-23; // Boltzmann constant

		//----------------------------------------
		// Initiate halo swap
		//----------------------------------------
		vmpi::mpi_init_halo_swap();

      //----------------------------------------
		// Calculate fields (core)
		//----------------------------------------

		calculate_spin_fields(pre_comm_si,pre_comm_ei);
      calculate_lsf_rk4_field(pre_comm_si,pre_comm_ei);
		calculate_external_fields(pre_comm_si,pre_comm_ei);

		//----------------------------------------
		// Store initial spin positions (all)
		//----------------------------------------

		for(int atom=pre_comm_si;atom<post_comm_ei;atom++){
			x_initial_spin_array[atom] = atoms::x_spin_array[atom];
			y_initial_spin_array[atom] = atoms::y_spin_array[atom];
			z_initial_spin_array[atom] = atoms::z_spin_array[atom];
		}

      //----------------------------------------
		// Generate random temperature fluctuations
		//----------------------------------------
      double sigma = ( sqrt( ( 2.0 * kB * sim::temperature * mp::gamma_SI) / ( mp::dt_SI ) ) );
      generate (tx.begin()+pre_comm_si,tx.begin()+pre_comm_ei, mtrandom::gaussian);
      generate (ty.begin()+pre_comm_si,ty.begin()+pre_comm_ei, mtrandom::gaussian);
      generate (tz.begin()+pre_comm_si,tz.begin()+pre_comm_ei, mtrandom::gaussian);

		//----------------------------------------
		// Calculate K1 (Core)
		//----------------------------------------

		for(int atom=pre_comm_si;atom<pre_comm_ei;atom++){

			const int imaterial=atoms::type_array[atom];
			const double alpha = mp::material[imaterial].alpha;
         const double mu = mp::material[imaterial].mu_s_SI;

			// Store local spin in S and local field in H
		   const double S[3] = {atoms::x_spin_array[atom],atoms::y_spin_array[atom],atoms::z_spin_array[atom]};

		   const double H[3] = {atoms::x_total_spin_field_array[atom]+atoms::x_total_external_field_array[atom]+LSF_RK4_arrays::x_lsf_array[atom],
								 		atoms::y_total_spin_field_array[atom]+atoms::y_total_external_field_array[atom]+LSF_RK4_arrays::y_lsf_array[atom],
								 		atoms::z_total_spin_field_array[atom]+atoms::z_total_external_field_array[atom]+LSF_RK4_arrays::z_lsf_array[atom]};

			// Calculate Delta S
         xyz[0]=(-mp::gamma_SI*(S[1]*H[2]-S[2]*H[1]))+(mp::gamma_SI*alpha*H[0])+(tx[atom]*(sigma * sqrt(alpha / mu)));
         xyz[1]=(-mp::gamma_SI*(S[2]*H[0]-S[0]*H[2]))+(mp::gamma_SI*alpha*H[1])+(ty[atom]*(sigma * sqrt(alpha / mu)));
         xyz[2]=(-mp::gamma_SI*(S[0]*H[1]-S[1]*H[0]))+(mp::gamma_SI*alpha*H[2])+(tz[atom]*(sigma * sqrt(alpha / mu)));

			// k1 = \Delta S * \Delta t
         x_k1_array[atom] = xyz[0] * mp::dt_SI;
         y_k1_array[atom] = xyz[1] * mp::dt_SI;
         z_k1_array[atom] = xyz[2] * mp::dt_SI;

         // Write new spin vectors
         atoms::x_spin_array[atom] += 0.5*x_k1_array[atom];
         atoms::y_spin_array[atom] += 0.5*y_k1_array[atom];
			atoms::z_spin_array[atom] += 0.5*z_k1_array[atom];
		}

		//----------------------------------------
		// Complete halo swap
		//----------------------------------------
		vmpi::mpi_complete_halo_swap();

		//----------------------------------------
		// Calculate fields (boundary)
		//----------------------------------------

		calculate_spin_fields(post_comm_si,post_comm_ei);
      calculate_lsf_rk4_field(post_comm_si,post_comm_ei);
		calculate_external_fields(post_comm_si,post_comm_ei);

      //----------------------------------------
		// Generate random temperature fluctuations
		//----------------------------------------
      	generate (tx.begin()+post_comm_si,tx.begin()+post_comm_ei, mtrandom::gaussian);
      	generate (ty.begin()+post_comm_si,ty.begin()+post_comm_ei, mtrandom::gaussian);
      	generate (tz.begin()+post_comm_si,tz.begin()+post_comm_ei, mtrandom::gaussian);

		//----------------------------------------
		// Calculate K1 (boundary)
		//----------------------------------------

		for(int atom=post_comm_si;atom<post_comm_ei;atom++){

			const int imaterial=atoms::type_array[atom];
			const double alpha = mp::material[imaterial].alpha;
         const double mu = mp::material[imaterial].mu_s_SI;

			// Store local spin in S and local field in H
		   const double S[3] = {atoms::x_spin_array[atom],atoms::y_spin_array[atom],atoms::z_spin_array[atom]};

		   const double H[3] = {atoms::x_total_spin_field_array[atom]+atoms::x_total_external_field_array[atom]+LSF_RK4_arrays::x_lsf_array[atom],
								 		atoms::y_total_spin_field_array[atom]+atoms::y_total_external_field_array[atom]+LSF_RK4_arrays::y_lsf_array[atom],
								 		atoms::z_total_spin_field_array[atom]+atoms::z_total_external_field_array[atom]+LSF_RK4_arrays::z_lsf_array[atom]};

         // Calculate Delta S
         xyz[0]=(-mp::gamma_SI*(S[1]*H[2]-S[2]*H[1]))+(mp::gamma_SI*alpha*H[0])+(tx[atom]*(sigma * sqrt(alpha / mu)));
         xyz[1]=(-mp::gamma_SI*(S[2]*H[0]-S[0]*H[2]))+(mp::gamma_SI*alpha*H[1])+(ty[atom]*(sigma * sqrt(alpha / mu)));
         xyz[2]=(-mp::gamma_SI*(S[0]*H[1]-S[1]*H[0]))+(mp::gamma_SI*alpha*H[2])+(tz[atom]*(sigma * sqrt(alpha / mu)));

			// k1 = \Delta S * \Delta t
         x_k1_array[atom] = xyz[0] * mp::dt_SI;
         y_k1_array[atom] = xyz[1] * mp::dt_SI;
         z_k1_array[atom] = xyz[2] * mp::dt_SI;

         // Write new spin vectors
         atoms::x_spin_array[atom] += 0.5*x_k1_array[atom];
         atoms::y_spin_array[atom] += 0.5*y_k1_array[atom];
			atoms::z_spin_array[atom] += 0.5*z_k1_array[atom];
		}

		//------------------------------------------
		// Initiate second halo swap
		//------------------------------------------
		vmpi::mpi_init_halo_swap();

		//------------------------------------------
		// Recalculate spin dependent fields (core)
		//------------------------------------------

		calculate_spin_fields(pre_comm_si,pre_comm_ei);
      calculate_lsf_rk4_field(pre_comm_si,pre_comm_ei);

		//----------------------------------------
		// Calculate K2 (core)
		//----------------------------------------

		for(int atom=pre_comm_si;atom<pre_comm_ei;atom++){

			const int imaterial=atoms::type_array[atom];
         const double alpha = mp::material[imaterial].alpha;
         const double mu = mp::material[imaterial].mu_s_SI;

			// Store local spin in S and local field in H
         const double S[3] = {atoms::x_spin_array[atom],atoms::y_spin_array[atom],atoms::z_spin_array[atom]};

         const double H[3] = {atoms::x_total_spin_field_array[atom]+atoms::x_total_external_field_array[atom]+LSF_RK4_arrays::x_lsf_array[atom],
                              atoms::y_total_spin_field_array[atom]+atoms::y_total_external_field_array[atom]+LSF_RK4_arrays::y_lsf_array[atom],
                              atoms::z_total_spin_field_array[atom]+atoms::z_total_external_field_array[atom]+LSF_RK4_arrays::z_lsf_array[atom]};

         // Calculate Delta S
         xyz[0]=(-mp::gamma_SI*(S[1]*H[2]-S[2]*H[1]))+(mp::gamma_SI*alpha*H[0])+(tx[atom]*(sigma * sqrt(alpha / mu)));
         xyz[1]=(-mp::gamma_SI*(S[2]*H[0]-S[0]*H[2]))+(mp::gamma_SI*alpha*H[1])+(ty[atom]*(sigma * sqrt(alpha / mu)));
         xyz[2]=(-mp::gamma_SI*(S[0]*H[1]-S[1]*H[0]))+(mp::gamma_SI*alpha*H[2])+(tz[atom]*(sigma * sqrt(alpha / mu)));

         // k2
         x_k2_array[atom] = xyz[0] * mp::dt_SI;
         y_k2_array[atom] = xyz[1] * mp::dt_SI;
         z_k2_array[atom] = xyz[2] * mp::dt_SI;

         // Write new spin vectors
         atoms::x_spin_array[atom] += 0.5*x_k2_array[atom];
         atoms::y_spin_array[atom] += 0.5*y_k2_array[atom];
			atoms::z_spin_array[atom] += 0.5*z_k2_array[atom];
		}

		//------------------------------------------
		// Complete second halo swap
		//------------------------------------------
		vmpi::mpi_complete_halo_swap();

		//------------------------------------------
		// Recalculate spin dependent fields (boundary)
		//------------------------------------------

		calculate_spin_fields(post_comm_si,post_comm_ei);
      calculate_lsf_rk4_field(post_comm_si,post_comm_ei);

		//----------------------------------------
		// Calculate K2 (boundary)
		//----------------------------------------

		for(int atom=post_comm_si;atom<post_comm_ei;atom++){

			const int imaterial=atoms::type_array[atom];
         const double alpha = mp::material[imaterial].alpha;
         const double mu = mp::material[imaterial].mu_s_SI;

			// Store local spin in S and local field in H
         const double S[3] = {atoms::x_spin_array[atom],atoms::y_spin_array[atom],atoms::z_spin_array[atom]};

         const double H[3] = {atoms::x_total_spin_field_array[atom]+atoms::x_total_external_field_array[atom]+LSF_RK4_arrays::x_lsf_array[atom],
                              atoms::y_total_spin_field_array[atom]+atoms::y_total_external_field_array[atom]+LSF_RK4_arrays::y_lsf_array[atom],
                              atoms::z_total_spin_field_array[atom]+atoms::z_total_external_field_array[atom]+LSF_RK4_arrays::z_lsf_array[atom]};

         // Calculate Delta S
         xyz[0]=(-mp::gamma_SI*(S[1]*H[2]-S[2]*H[1]))+(mp::gamma_SI*alpha*H[0])+(tx[atom]*(sigma * sqrt(alpha / mu)));
         xyz[1]=(-mp::gamma_SI*(S[2]*H[0]-S[0]*H[2]))+(mp::gamma_SI*alpha*H[1])+(ty[atom]*(sigma * sqrt(alpha / mu)));
         xyz[2]=(-mp::gamma_SI*(S[0]*H[1]-S[1]*H[0]))+(mp::gamma_SI*alpha*H[2])+(tz[atom]*(sigma * sqrt(alpha / mu)));

         // k2
         x_k2_array[atom] = xyz[0] * mp::dt_SI;
         y_k2_array[atom] = xyz[1] * mp::dt_SI;
         z_k2_array[atom] = xyz[2] * mp::dt_SI;

         // Write new spin vectors
         atoms::x_spin_array[atom] += 0.5*x_k2_array[atom];
         atoms::y_spin_array[atom] += 0.5*y_k2_array[atom];
			atoms::z_spin_array[atom] += 0.5*z_k2_array[atom];
		}

		//------------------------------------------
		// Initiate third halo swap
		//------------------------------------------
		vmpi::mpi_init_halo_swap();

		//----------------------------------------
		// Calculate K3 (core)
		//----------------------------------------

		for(int atom=pre_comm_si;atom<pre_comm_ei;atom++){

			const int imaterial=atoms::type_array[atom];
         const double alpha = mp::material[imaterial].alpha;
         const double mu = mp::material[imaterial].mu_s_SI;

			// Store local spin in S and local field in H
         const double S[3] = {atoms::x_spin_array[atom],atoms::y_spin_array[atom],atoms::z_spin_array[atom]};

         const double H[3] = {atoms::x_total_spin_field_array[atom]+atoms::x_total_external_field_array[atom]+LSF_RK4_arrays::x_lsf_array[atom],
                              atoms::y_total_spin_field_array[atom]+atoms::y_total_external_field_array[atom]+LSF_RK4_arrays::y_lsf_array[atom],
                              atoms::z_total_spin_field_array[atom]+atoms::z_total_external_field_array[atom]+LSF_RK4_arrays::z_lsf_array[atom]};

         // Calculate Delta S
         xyz[0]=(-mp::gamma_SI*(S[1]*H[2]-S[2]*H[1]))+(mp::gamma_SI*alpha*H[0])+(tx[atom]*(sigma * sqrt(alpha / mu)));
         xyz[1]=(-mp::gamma_SI*(S[2]*H[0]-S[0]*H[2]))+(mp::gamma_SI*alpha*H[1])+(ty[atom]*(sigma * sqrt(alpha / mu)));
         xyz[2]=(-mp::gamma_SI*(S[0]*H[1]-S[1]*H[0]))+(mp::gamma_SI*alpha*H[2])+(tz[atom]*(sigma * sqrt(alpha / mu)));

         // k3
         x_k3_array[atom] = xyz[0] * mp::dt_SI;
      	y_k3_array[atom] = xyz[1] * mp::dt_SI;
         z_k3_array[atom] = xyz[2] * mp::dt_SI;

         // Write new spin vectors
         atoms::x_spin_array[atom] += x_k3_array[atom];
         atoms::y_spin_array[atom] += y_k3_array[atom];
			atoms::z_spin_array[atom] += z_k3_array[atom];
		}

		//------------------------------------------
		// Complete third halo swap
		//------------------------------------------
		vmpi::mpi_complete_halo_swap();

		//----------------------------------------
		// Calculate K3 (boundary)
		//----------------------------------------

		for(int atom=post_comm_si;atom<post_comm_ei;atom++){

			const int imaterial=atoms::type_array[atom];
         const double alpha = mp::material[imaterial].alpha;
         const double mu = mp::material[imaterial].mu_s_SI;

			// Store local spin in S and local field in H
         const double S[3] = {atoms::x_spin_array[atom],atoms::y_spin_array[atom],atoms::z_spin_array[atom]};

         const double H[3] = {atoms::x_total_spin_field_array[atom]+atoms::x_total_external_field_array[atom]+LSF_RK4_arrays::x_lsf_array[atom],
                              atoms::y_total_spin_field_array[atom]+atoms::y_total_external_field_array[atom]+LSF_RK4_arrays::y_lsf_array[atom],
                              atoms::z_total_spin_field_array[atom]+atoms::z_total_external_field_array[atom]+LSF_RK4_arrays::z_lsf_array[atom]};

         // Calculate Delta S
         xyz[0]=(-mp::gamma_SI*(S[1]*H[2]-S[2]*H[1]))+(mp::gamma_SI*alpha*H[0])+(tx[atom]*(sigma * sqrt(alpha / mu)));
         xyz[1]=(-mp::gamma_SI*(S[2]*H[0]-S[0]*H[2]))+(mp::gamma_SI*alpha*H[1])+(ty[atom]*(sigma * sqrt(alpha / mu)));
         xyz[2]=(-mp::gamma_SI*(S[0]*H[1]-S[1]*H[0]))+(mp::gamma_SI*alpha*H[2])+(tz[atom]*(sigma * sqrt(alpha / mu)));

         // k3
         x_k3_array[atom] = xyz[0] * mp::dt_SI;
         y_k3_array[atom] = xyz[1] * mp::dt_SI;
         z_k3_array[atom] = xyz[2] * mp::dt_SI;

         // Write new spin vectors
         atoms::x_spin_array[atom] += x_k3_array[atom];
         atoms::y_spin_array[atom] += y_k3_array[atom];
			atoms::z_spin_array[atom] += z_k3_array[atom];
		}

		//------------------------------------------
		// Initiate fourth halo swap
		//------------------------------------------
		vmpi::mpi_init_halo_swap();

		//------------------------------------------
		// Recalculate spin dependent fields (core)
		//------------------------------------------

		calculate_spin_fields(pre_comm_si,pre_comm_ei);
      calculate_lsf_rk4_field(pre_comm_si,pre_comm_ei);

		//----------------------------------------
		// Calculate K4 (core)
		//----------------------------------------

		for(int atom=pre_comm_si;atom<pre_comm_ei;atom++){

			const int imaterial=atoms::type_array[atom];
         const double alpha = mp::material[imaterial].alpha;
         const double mu = mp::material[imaterial].mu_s_SI;

			// Store local spin in S and local field in H
         const double S[3] = {atoms::x_spin_array[atom],atoms::y_spin_array[atom],atoms::z_spin_array[atom]};

         const double H[3] = {atoms::x_total_spin_field_array[atom]+atoms::x_total_external_field_array[atom]+LSF_RK4_arrays::x_lsf_array[atom],
                              atoms::y_total_spin_field_array[atom]+atoms::y_total_external_field_array[atom]+LSF_RK4_arrays::y_lsf_array[atom],
                              atoms::z_total_spin_field_array[atom]+atoms::z_total_external_field_array[atom]+LSF_RK4_arrays::z_lsf_array[atom]};

         // Calculate Delta S
         xyz[0]=(-mp::gamma_SI*(S[1]*H[2]-S[2]*H[1]))+(mp::gamma_SI*alpha*H[0])+(tx[atom]*(sigma * sqrt(alpha / mu)));
         xyz[1]=(-mp::gamma_SI*(S[2]*H[0]-S[0]*H[2]))+(mp::gamma_SI*alpha*H[1])+(ty[atom]*(sigma * sqrt(alpha / mu)));
         xyz[2]=(-mp::gamma_SI*(S[0]*H[1]-S[1]*H[0]))+(mp::gamma_SI*alpha*H[2])+(tz[atom]*(sigma * sqrt(alpha / mu)));

         // k4
         x_k4_array[atom] = xyz[0] * mp::dt_SI;
         y_k4_array[atom] = xyz[1] * mp::dt_SI;
         z_k4_array[atom] = xyz[2] * mp::dt_SI;
		}

		//------------------------------------------
		// Complete fourth halo swap
		//------------------------------------------
		vmpi::mpi_complete_halo_swap();

		//------------------------------------------
		// Recalculate spin dependent fields (boundary)
		//------------------------------------------

		calculate_spin_fields(post_comm_si,post_comm_ei);
      calculate_lsf_rk4_field(post_comm_si,post_comm_ei);

		//----------------------------------------
		// Calculate K4 (boundary)
		//----------------------------------------

		for(int atom=post_comm_si;atom<post_comm_ei;atom++){

			const int imaterial=atoms::type_array[atom];
         const double alpha = mp::material[imaterial].alpha;
         const double mu = mp::material[imaterial].mu_s_SI;

			// Store local spin in S and local field in H
         const double S[3] = {atoms::x_spin_array[atom],atoms::y_spin_array[atom],atoms::z_spin_array[atom]};

         const double H[3] = {atoms::x_total_spin_field_array[atom]+atoms::x_total_external_field_array[atom]+LSF_RK4_arrays::x_lsf_array[atom],
                              atoms::y_total_spin_field_array[atom]+atoms::y_total_external_field_array[atom]+LSF_RK4_arrays::y_lsf_array[atom],
                              atoms::z_total_spin_field_array[atom]+atoms::z_total_external_field_array[atom]+LSF_RK4_arrays::z_lsf_array[atom]};

         // Calculate Delta S
         xyz[0]=(-mp::gamma_SI*(S[1]*H[2]-S[2]*H[1]))+(mp::gamma_SI*alpha*H[0])+(tx[atom]*(sigma * sqrt(alpha / mu)));
         xyz[1]=(-mp::gamma_SI*(S[2]*H[0]-S[0]*H[2]))+(mp::gamma_SI*alpha*H[1])+(ty[atom]*(sigma * sqrt(alpha / mu)));
         xyz[2]=(-mp::gamma_SI*(S[0]*H[1]-S[1]*H[0]))+(mp::gamma_SI*alpha*H[2])+(tz[atom]*(sigma * sqrt(alpha / mu)));

         // k4
         x_k4_array[atom] = xyz[0] * mp::dt_SI;
         y_k4_array[atom] = xyz[1] * mp::dt_SI;
         z_k4_array[atom] = xyz[2] * mp::dt_SI;
		}

		//----------------------------------------
		// Calculate RK4 Step
		//----------------------------------------

		for(int atom=pre_comm_si;atom<post_comm_ei;atom++){
			atoms::x_spin_array[atom] = x_initial_spin_array[atom] + (1.0/6.0)*(x_k1_array[atom] + (2.0*x_k2_array[atom]) + (2.0*x_k3_array[atom]) + x_k4_array[atom]);
			atoms::y_spin_array[atom] = y_initial_spin_array[atom] + (1.0/6.0)*(y_k1_array[atom] + (2.0*y_k2_array[atom]) + (2.0*y_k3_array[atom]) + y_k4_array[atom]);
			atoms::z_spin_array[atom] = z_initial_spin_array[atom] + (1.0/6.0)*(z_k1_array[atom] + (2.0*z_k2_array[atom]) + (2.0*z_k3_array[atom]) + z_k4_array[atom]);
		}

	// Swap timers compute -> wait
	vmpi::TotalComputeTime+=vmpi::SwapTimer(vmpi::ComputeTime, vmpi::WaitTime);

	// Wait for other processors
	vmpi::barrier();

	// Swap timers wait -> compute
	vmpi::TotalWaitTime+=vmpi::SwapTimer(vmpi::WaitTime, vmpi::ComputeTime);

   return EXIT_SUCCESS;
}

} // end of namespace sim
#endif
