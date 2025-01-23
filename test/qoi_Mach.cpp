#include <iostream>
#include <ctime>
#include <cstdlib>
#include <vector>
#include <cmath>
#include <bitset>
#include <numeric>
#include "utils.hpp"
#include "qoi_utils.hpp"
#include "MDR/Reconstructor/Reconstructor.hpp"
#include "nomask_Synthesizer4GE.hpp"
#include "PDR/Reconstructor/Reconstructor.hpp"
#define Dummy 0
#define SZ3 1
#define PMGARD 2
using namespace MDR;

using T = float;
std::vector<float> P_ori;
std::vector<float> D_ori;
std::vector<float> Vx_ori;
std::vector<float> Vy_ori;
std::vector<float> Vz_ori;
float * P_dec = NULL;
float * D_dec = NULL;
float * Vx_dec = NULL;
float * Vy_dec = NULL;
float * Vz_dec = NULL;
float * Mach_ori = NULL;
std::vector<float> error_Mach;
std::vector<float> error_est_Mach;
int iter = 0;

template<class T>
bool halfing_error_Mach_uniform(const T * Vx, const T * Vy, const T * Vz, const T * P, const T * D, size_t n, const std::vector<unsigned char>& mask, const T tau, std::vector<T>& ebs){
	T eb_Vx = ebs[0];
	T eb_Vy = ebs[1];
	T eb_Vz = ebs[2];
	T eb_P = ebs[3];
	T eb_D = ebs[4];
	T R = 287.1;
	T gamma = 1.4;
	T mi = 3.5;
	T mu_r = 1.716e-5;
	T T_r = 273.15;
	T S = 110.4;
	T c_1 = 1.0 / R;
	T c_2 = sqrt(gamma * R);
	int C7i[8] = {1, 7, 21, 35, 35, 21, 7, 1};
	T max_value = 0;
	int max_index = 0;
    T max_Vx = 0;
    T max_Vy = 0;
    T max_Vz = 0;
    T max_P = 0;
    T max_D = 0;
    T max_e_VTOT_2 = 0;
    T max_VTOT_2 = 0;
    T max_e_T = 0;
    T max_T = 0;
    T max_C = 0;
    T max_e_C = 0;
    T max_e_Mach = 0;
    T max_Mach = 0;
	int n_variable = ebs.size();
	for(int i=0; i<n; i++){
		T e_V_TOT_2 = 0;
		if(mask[i]) e_V_TOT_2 = compute_bound_x_square(Vx[i], eb_Vx) + compute_bound_x_square(Vy[i], eb_Vy) + compute_bound_x_square(Vz[i], eb_Vz);
		T V_TOT_2 = Vx[i]*Vx[i] + Vy[i]*Vy[i] + Vz[i]*Vz[i];
		// error of total velocity
		T e_V_TOT = 0;
		if(mask[i]) e_V_TOT = compute_bound_square_root_x(V_TOT_2, e_V_TOT_2);
		T V_TOT = sqrt(V_TOT_2);
		// print_error("V_TOT", V_TOT, V_TOT_ori[i], e_V_TOT);
		// error of temperature
		T e_T = c_1 * compute_bound_division(P[i], D[i], eb_P, eb_D);
		T Temp = P[i] / (D[i] * R);
		// print_error("T", Temp, Temp_ori[i], e_T);
		// error of C
		T e_C = c_2*compute_bound_square_root_x(Temp, e_T);
		T C = c_2 * sqrt(Temp);
		// print_error("C", C, C_ori[i], e_C);
		T e_Mach = compute_bound_division(V_TOT, C, e_V_TOT, e_C);
		T Mach = V_TOT / C;
		// print_error("Mach", Mach, Mach_ori[i], e_Mach);
		error_est_Mach[i] = e_Mach;
		error_Mach[i] = Mach - Mach_ori[i];
		if(max_value < error_est_Mach[i]){
			max_value = error_est_Mach[i];
			max_index = i;
            max_Vx = Vx[i];
            max_Vy = Vy[i];
            max_Vz = Vz[i];
            max_P = P[i];
            max_D = D[i];
            max_e_VTOT_2 = e_V_TOT_2;
            max_VTOT_2 = V_TOT_2;
            max_e_T = e_T;
            max_T = Temp;
            max_C = C;
            max_e_C = e_C;
            max_e_Mach = e_Mach;
            max_Mach = Mach;
		}
	}
	std::cout << names[3] << ": max estimated error = " << max_value << ", index = " << max_index << std::endl;
    std::cout << "e_Mach = " << max_e_Mach << " Mach = " << max_Mach << std::endl;
    std::cout << "e_C = " << max_e_C << " C = " << max_C << std::endl;
    std::cout << "e_VTOT_2 = " << max_e_VTOT_2 << ", VTOT_2 = " << max_VTOT_2 << ", Vx = " << max_Vx << ", Vy = " << max_Vy << ", Vz = " << max_Vz << std::endl;
    std::cout << "e_T = " << max_e_T << " T = " << max_T << ", P = " << max_P << ", D = " << max_D << std::endl;
	// estimate error bound based on maximal errors
	if(max_value > tau){
		auto i = max_index;
		T estimate_error = max_value;
		T eb_Vx = ebs[0];
		T eb_Vy = ebs[1];
		T eb_Vz = ebs[2];
		T eb_P = ebs[3];
		T eb_D = ebs[4];
		while(estimate_error > tau){
    		std::cout << "uniform decrease\n";
			eb_Vx = eb_Vx / 1.5;
			eb_Vy = eb_Vy / 1.5;
			eb_Vz = eb_Vz / 1.5; 
			eb_P = eb_P / 1.5;
			eb_D = eb_D / 1.5;
			T e_V_TOT_2 = 0;
			if(mask[i]) e_V_TOT_2 = compute_bound_x_square(Vx[i], eb_Vx) + compute_bound_x_square(Vy[i], eb_Vy) + compute_bound_x_square(Vz[i], eb_Vz);
			T V_TOT_2 = Vx[i]*Vx[i] + Vy[i]*Vy[i] + Vz[i]*Vz[i];
			T e_V_TOT = 0;
			if(mask[i]) e_V_TOT = compute_bound_square_root_x(V_TOT_2, e_V_TOT_2);
			T V_TOT = sqrt(V_TOT_2);
			T e_T = c_1 * compute_bound_division(P[i], D[i], eb_P, eb_D);
			T Temp = P[i] / (D[i] * R);
			T e_C = c_2*compute_bound_square_root_x(Temp, e_T);
			T C = c_2 * sqrt(Temp);
			estimate_error = compute_bound_division(V_TOT, C, e_V_TOT, e_C);
            if (ebs[0] / eb_Vx > 10) break;
		}
		ebs[0] = eb_Vx;
		ebs[1] = eb_Vy;
		ebs[2] = eb_Vz;
		ebs[3] = eb_P;
		ebs[4] = eb_D;
		return false;
	}
	return true;
}

template<class T>
bool halfing_error_Mach_uniform(const T * Vx, const T * Vy, const T * Vz, const T * P, const T * D, size_t n, const std::vector<unsigned char>& mask, const T tau, std::vector<T>& ebs, std::vector<std::vector<int>> weights){
	T eb_Vx = ebs[0];
	T eb_Vy = ebs[1];
	T eb_Vz = ebs[2];
	T eb_P = ebs[3];
	T eb_D = ebs[4];
	T R = 287.1;
	T gamma = 1.4;
	T mi = 3.5;
	T mu_r = 1.716e-5;
	T T_r = 273.15;
	T S = 110.4;
	T c_1 = 1.0 / R;
	T c_2 = sqrt(gamma * R);
	int C7i[8] = {1, 7, 21, 35, 35, 21, 7, 1};
	T max_value = 0;
	int max_index = 0;
    T max_Vx = 0;
    T max_Vy = 0;
    T max_Vz = 0;
    T max_P = 0;
    T max_D = 0;
    T max_e_VTOT_2 = 0;
    T max_VTOT_2 = 0;
    T max_e_T = 0;
    T max_T = 0;
    T max_C = 0;
    T max_e_C = 0;
    T max_e_Mach = 0;
    T max_Mach = 0;
    int max_weight_Vx = 0;
    int max_weight_Vy = 0;
    int max_weight_Vz = 0;
    int max_weight_P = 0;
    int max_weight_D = 0;
	int n_variable = ebs.size();
	for(int i=0; i<n; i++){
		T e_V_TOT_2 = 0;
		if(mask[i]) e_V_TOT_2 = compute_bound_x_square(Vx[i], eb_Vx / static_cast<T>(std::pow(2.0, weights[0][i]))) + compute_bound_x_square(Vy[i], eb_Vy / static_cast<T>(std::pow(2.0, weights[1][i]))) + compute_bound_x_square(Vz[i], eb_Vz / static_cast<T>(std::pow(2.0, weights[2][i])));
		T V_TOT_2 = Vx[i]*Vx[i] + Vy[i]*Vy[i] + Vz[i]*Vz[i];
		// error of total velocity
		T e_V_TOT = 0;
		if(mask[i]) e_V_TOT = compute_bound_square_root_x(V_TOT_2, e_V_TOT_2);
		T V_TOT = sqrt(V_TOT_2);
		// print_error("V_TOT", V_TOT, V_TOT_ori[i], e_V_TOT);
		// error of temperature
		T e_T = c_1 * compute_bound_division(P[i], D[i], eb_P / static_cast<T>(std::pow(2.0, weights[3][i])), eb_D / static_cast<T>(std::pow(2.0, weights[4][i])));
		T Temp = P[i] / (D[i] * R);
		// print_error("T", Temp, Temp_ori[i], e_T);
		// error of C
		T e_C = c_2*compute_bound_square_root_x(Temp, e_T);
		T C = c_2 * sqrt(Temp);
		// print_error("C", C, C_ori[i], e_C);
		T e_Mach = compute_bound_division(V_TOT, C, e_V_TOT, e_C);
		T Mach = V_TOT / C;
		// print_error("Mach", Mach, Mach_ori[i], e_Mach);
		error_est_Mach[i] = e_Mach;
		error_Mach[i] = Mach - Mach_ori[i];
		if(max_value < error_est_Mach[i]){
			max_value = error_est_Mach[i];
			max_index = i;
            max_Vx = Vx[i];
            max_Vy = Vy[i];
            max_Vz = Vz[i];
            max_P = P[i];
            max_D = D[i];
            max_e_VTOT_2 = e_V_TOT_2;
            max_VTOT_2 = V_TOT_2;
            max_e_T = e_T;
            max_T = Temp;
            max_C = C;
            max_e_C = e_C;
            max_e_Mach = e_Mach;
            max_Mach = Mach;
            max_weight_Vx = weights[0][i];
            max_weight_Vy = weights[1][i];
            max_weight_Vz = weights[2][i];
            max_weight_P = weights[3][i];
            max_weight_D = weights[4][i];
		}
	}
	std::cout << names[3] << ": max estimated error = " << max_value << ", index = " << max_index << std::endl;
    std::cout << "e_Mach = " << max_e_Mach << " Mach = " << max_Mach << std::endl;
    std::cout << "e_C = " << max_e_C << " C = " << max_C << std::endl;
    std::cout << "e_VTOT_2 = " << max_e_VTOT_2 << ", VTOT_2 = " << max_VTOT_2 << ", Vx = " << max_Vx << ", Vy = " << max_Vy << ", Vz = " << max_Vz << std::endl;
    std::cout << "max_weight_Vx = " << max_weight_Vx << ", max_weight_Vy = " << max_weight_Vy << ", max_weight_Vz = " << max_weight_Vz << std::endl;
    std::cout << "e_T = " << max_e_T << " T = " << max_T << ", P = " << max_P << ", D = " << max_D << std::endl;
    std::cout << "max_weight_P = " << max_weight_P << ", max_weight_D = " << max_weight_D << std::endl;
	// estimate error bound based on maximal errors
	if(max_value > tau){
		auto i = max_index;
		T estimate_error = max_value;
		T eb_Vx = ebs[0];
		T eb_Vy = ebs[1];
		T eb_Vz = ebs[2];
		T eb_P = ebs[3];
		T eb_D = ebs[4];
		while(estimate_error > tau){
    		std::cout << "uniform decrease\n";
			eb_Vx = eb_Vx / 1.5;
			eb_Vy = eb_Vy / 1.5;
			eb_Vz = eb_Vz / 1.5; 
			eb_P = eb_P / 1.5;
			eb_D = eb_D / 1.5;
			T e_V_TOT_2 = 0;
			if(mask[i]) e_V_TOT_2 = compute_bound_x_square(Vx[i], eb_Vx / static_cast<T>(std::pow(2.0, weights[0][i]))) + compute_bound_x_square(Vy[i], eb_Vy / static_cast<T>(std::pow(2.0, weights[1][i]))) + compute_bound_x_square(Vz[i], eb_Vz / static_cast<T>(std::pow(2.0, weights[2][i])));
			T V_TOT_2 = Vx[i]*Vx[i] + Vy[i]*Vy[i] + Vz[i]*Vz[i];
			T e_V_TOT = 0;
			if(mask[i]) e_V_TOT = compute_bound_square_root_x(V_TOT_2, e_V_TOT_2);
			T V_TOT = sqrt(V_TOT_2);
			T e_T = c_1 * compute_bound_division(P[i], D[i], eb_P / static_cast<T>(std::pow(2.0, weights[3][i])), eb_D / static_cast<T>(std::pow(2.0, weights[4][i])));
			T Temp = P[i] / (D[i] * R);
			T e_C = c_2*compute_bound_square_root_x(Temp, e_T);
			T C = c_2 * sqrt(Temp);
			estimate_error = compute_bound_division(V_TOT, C, e_V_TOT, e_C);
            if (ebs[0] / eb_Vx > 10) break;
		}
		ebs[0] = eb_Vx;
		ebs[1] = eb_Vy;
		ebs[2] = eb_Vz;
		ebs[3] = eb_P;
		ebs[4] = eb_D;
		return false;
	}
	return true;
}

template<class T>
std::vector<size_t> retrieve_Mach_Dummy(std::string rdata_file_prefix, T tau, std::vector<T> ebs, size_t num_elements, const std::vector<unsigned char>& mask, int weighted, T & max_act_error, T & max_est_error, size_t & weight_file_size){
    int max_iter = 10;
    bool tolerance_met = false;
    int n_variable = ebs.size();
    std::cout << "n_variable = " << n_variable << std::endl; 
    std::vector<std::vector<T>> reconstructed_vars(n_variable, std::vector<T>(num_elements));
    std::vector<size_t> total_retrieved_size(n_variable, 0);
    if(!weighted){
        std::vector<PDR::ApproximationBasedReconstructor<T, PDR::DummyApproximator<T>, MDR::NegaBinaryBPEncoder<T, uint32_t>, AdaptiveLevelCompressor, SignExcludeGreedyBasedSizeInterpreter<MDR::MaxErrorEstimatorHB<T>>, MaxErrorEstimatorHB<T>, ConcatLevelFileRetriever>> reconstructors;
        for(int i=0; i<n_variable; i++){
            std::string rdir_prefix = rdata_file_prefix + varlist[i];
            std::string metadata_file = rdir_prefix + "_refactored/metadata.bin";
            std::vector<std::string> files;
            int num_levels = 1;
            for(int i=0; i<num_levels; i++){
                std::string filename = rdir_prefix + "_refactored/level_" + std::to_string(i) + ".bin";
                files.push_back(filename);
            }
            auto approximator = PDR::DummyApproximator<T>();
            auto encoder = NegaBinaryBPEncoder<T, uint32_t>();
            auto compressor = AdaptiveLevelCompressor(64);
            auto estimator = MaxErrorEstimatorHB<T>();
            auto interpreter = SignExcludeGreedyBasedSizeInterpreter<MaxErrorEstimatorHB<T>>(estimator);
            auto retriever = ConcatLevelFileRetriever(metadata_file, files);
            reconstructors.push_back(generateBPReconstructor<T>(approximator, encoder, compressor, estimator, interpreter, retriever));
            reconstructors.back().load_metadata();
        }
        while((!tolerance_met) && (iter < max_iter)){
            iter ++;
            for(int i=0; i<n_variable; i++){
                auto reconstructed_data = reconstructors[i].progressive_reconstruct(ebs[i], -1);
                total_retrieved_size[i] = reconstructors[i].get_retrieved_size();
                if(i < 3){
                    // reconstruct with mask
                    int index = 0;
                    for(int j=0; j<num_elements; j++){
                        if(mask[j]){
                            reconstructed_vars[i][j] = reconstructed_data[index ++];
                        }
                        else{
                            reconstructed_vars[i][j] = 0; 
                            index++;
                        }
                    }
                }
                else{
                    memcpy(reconstructed_vars[i].data(), reconstructed_data, num_elements*sizeof(T));
                }
            }
            Vx_dec = reconstructed_vars[0].data();
            Vy_dec = reconstructed_vars[1].data();
            Vz_dec = reconstructed_vars[2].data();
            P_dec = reconstructed_vars[3].data();
            D_dec = reconstructed_vars[4].data();
            MGARD::print_statistics(Vx_ori.data(), Vx_dec, num_elements);
            MGARD::print_statistics(Vy_ori.data(), Vy_dec, num_elements);
            MGARD::print_statistics(Vz_ori.data(), Vz_dec, num_elements);
            MGARD::print_statistics(P_ori.data(), P_dec, num_elements);
            MGARD::print_statistics(D_ori.data(), D_dec, num_elements);
            error_Mach = std::vector<T>(num_elements);
            error_est_Mach = std::vector<T>(num_elements);
            std::cout << "iter" << iter << ": The old ebs are:" << std::endl;
            MDR::print_vec(ebs);
            tolerance_met = halfing_error_Mach_uniform(Vx_dec, Vy_dec, Vz_dec, P_dec, D_dec, num_elements, mask, tau, ebs);
            std::cout << "iter" << iter << ": The new ebs are:" << std::endl;
            MDR::print_vec(ebs);
            // std::cout << names[1] << " requested error = " << tau << std::endl;
            max_act_error = print_max_abs(names[1] + " error", error_Mach);
            max_est_error = print_max_abs(names[1] + " error_est", error_est_Mach);   	
        }
    }
    else{
        std::vector<PDR::WeightedApproximationBasedReconstructor<T, PDR::DummyApproximator<T>, MDR::WeightedNegaBinaryBPEncoder<T, uint32_t>, AdaptiveLevelCompressor, SignExcludeGreedyBasedSizeInterpreter<MDR::MaxErrorEstimatorHB<T>>, MaxErrorEstimatorHB<T>, ConcatLevelFileRetriever>> reconstructors;
        std::vector<std::vector<int>> weights(n_variable, std::vector<int>(num_elements, 0));
        for(int i=0; i<n_variable; i++){
            std::string rdir_prefix = rdata_file_prefix + varlist[i];
            std::string metadata_file = rdir_prefix + "_refactored/metadata.bin";
            std::vector<std::string> files;
            int num_levels = 1;
            for(int i=0; i<num_levels; i++){
                std::string filename = rdir_prefix + "_refactored/level_" + std::to_string(i) + ".bin";
                files.push_back(filename);
            }
            auto approximator = PDR::DummyApproximator<T>();
            auto encoder = WeightedNegaBinaryBPEncoder<T, uint32_t>();
            auto compressor = AdaptiveLevelCompressor(64);
            auto estimator = MaxErrorEstimatorHB<T>();
            auto interpreter = SignExcludeGreedyBasedSizeInterpreter<MaxErrorEstimatorHB<T>>(estimator);
            auto retriever = ConcatLevelFileRetriever(metadata_file, files);
            reconstructors.push_back(generateWBPReconstructor<T>(approximator, encoder, compressor, estimator, interpreter, retriever));
            reconstructors.back().load_metadata();
            reconstructors.back().load_weight();
            reconstructors.back().span_weight();
            weights[i] = reconstructors.back().get_int_weights();
        }
        weight_file_size = reconstructors[n_variable - 1].get_weight_file_size();
        while((!tolerance_met) && (iter < max_iter)){
            iter ++;
            for(int i=0; i<n_variable; i++){
                auto reconstructed_data = reconstructors[i].progressive_reconstruct(ebs[i] / static_cast<T>(std::pow(2.0, reconstructors[i].get_max_weight())), -1);
                total_retrieved_size[i] = reconstructors[i].get_retrieved_size();
                if(i < 3){
                    // reconstruct with mask
                    int index = 0;
                    for(int j=0; j<num_elements; j++){
                        if(mask[j]){
                            reconstructed_vars[i][j] = reconstructed_data[index ++];
                        }
                        else{
                            reconstructed_vars[i][j] = 0; 
                            index++;
                        }
                    }
                }
                else{
                    memcpy(reconstructed_vars[i].data(), reconstructed_data, num_elements*sizeof(T));
                }
            }
            Vx_dec = reconstructed_vars[0].data();
            Vy_dec = reconstructed_vars[1].data();
            Vz_dec = reconstructed_vars[2].data();
            P_dec = reconstructed_vars[3].data();
            D_dec = reconstructed_vars[4].data();
            MGARD::print_statistics(Vx_ori.data(), Vx_dec, num_elements);
            MGARD::print_statistics(Vy_ori.data(), Vy_dec, num_elements);
            MGARD::print_statistics(Vz_ori.data(), Vz_dec, num_elements);
            MGARD::print_statistics(P_ori.data(), P_dec, num_elements);
            MGARD::print_statistics(D_ori.data(), D_dec, num_elements);
            error_Mach = std::vector<T>(num_elements);
            error_est_Mach = std::vector<T>(num_elements);
            std::cout << "iter" << iter << ": The old ebs are:" << std::endl;
            MDR::print_vec(ebs);
            tolerance_met = halfing_error_Mach_uniform(Vx_dec, Vy_dec, Vz_dec, P_dec, D_dec, num_elements, mask, tau, ebs, weights);
            std::cout << "iter" << iter << ": The new ebs are:" << std::endl;
            MDR::print_vec(ebs);
            /* test
            std::string filename = "./Result/Temp_err.dat";
            std::ofstream outfile1(filename, std::ios::binary);
            if (!outfile1.is_open()) {
                std::cerr << "Failed to open file for writing: " << filename << std::endl;
                exit(-1);
            }

            outfile1.write(reinterpret_cast<const char*>(error_est_Mach.data()), error_est_Mach.size() * sizeof(float));
        
            outfile1.close();
            std::cout << "Data saved successfully to " << filename << std::endl;
            //*/
            // std::cout << names[1] << " requested error = " << tau << std::endl;
            max_act_error = print_max_abs(names[1] + " error", error_Mach);
            max_est_error = print_max_abs(names[1] + " error_est", error_est_Mach);   	
        }
    }
    return total_retrieved_size;
}

template<class T>
std::vector<size_t> retrieve_Mach_SZ3(std::string rdata_file_prefix, T tau, std::vector<T> ebs, size_t num_elements, const std::vector<unsigned char>& mask, int weighted, T & max_act_error, T & max_est_error, size_t & weight_file_size){
    int max_iter = 10;
	bool tolerance_met = false;
	int n_variable = ebs.size();
	std::vector<std::vector<T>> reconstructed_vars(n_variable, std::vector<T>(num_elements));
	std::vector<size_t> total_retrieved_size(n_variable, 0);
    if(!weighted){
        std::vector<PDR::ApproximationBasedReconstructor<T, PDR::SZApproximator<T>, MDR::NegaBinaryBPEncoder<T, uint32_t>, AdaptiveLevelCompressor, SignExcludeGreedyBasedSizeInterpreter<MDR::MaxErrorEstimatorHB<T>>, MaxErrorEstimatorHB<T>, ConcatLevelFileRetriever>> reconstructors;
        for(int i=0; i<n_variable; i++){
            std::string rdir_prefix = rdata_file_prefix + varlist[i];
            std::string metadata_file = rdir_prefix + "_refactored/metadata.bin";
            std::vector<std::string> files;
            int num_levels = 1;
            for(int i=0; i<num_levels; i++){
                std::string filename = rdir_prefix + "_refactored/level_" + std::to_string(i) + ".bin";
                files.push_back(filename);
            }
            auto approximator = PDR::SZApproximator<T>();
            auto encoder = NegaBinaryBPEncoder<T, uint32_t>();
            auto compressor = AdaptiveLevelCompressor(64);
            auto estimator = MaxErrorEstimatorHB<T>();
            auto interpreter = SignExcludeGreedyBasedSizeInterpreter<MaxErrorEstimatorHB<T>>(estimator);
            auto retriever = ConcatLevelFileRetriever(metadata_file, files);
            reconstructors.push_back(generateBPReconstructor<T>(approximator, encoder, compressor, estimator, interpreter, retriever));
            reconstructors.back().load_metadata();
        }    
        while((!tolerance_met) && (iter < max_iter)){
            iter ++;
            for(int i=0; i<n_variable; i++){
                auto reconstructed_data = reconstructors[i].progressive_reconstruct(ebs[i], -1);
                total_retrieved_size[i] = reconstructors[i].get_retrieved_size();
                if(i < 3){
                    // reconstruct with mask
                    int index = 0;
                    for(int j=0; j<num_elements; j++){
                        if(mask[j]){
                            reconstructed_vars[i][j] = reconstructed_data[index ++];
                        }
                        else{
                            reconstructed_vars[i][j] = 0; 
                            index++;
                        }
                    }
                }
                else{
                    memcpy(reconstructed_vars[i].data(), reconstructed_data, num_elements*sizeof(T));
                }
            }
            Vx_dec = reconstructed_vars[0].data();
            Vy_dec = reconstructed_vars[1].data();
            Vz_dec = reconstructed_vars[2].data();
            P_dec = reconstructed_vars[3].data();
            D_dec = reconstructed_vars[4].data();
            MGARD::print_statistics(Vx_ori.data(), Vx_dec, num_elements);
            MGARD::print_statistics(Vy_ori.data(), Vy_dec, num_elements);
            MGARD::print_statistics(Vz_ori.data(), Vz_dec, num_elements);
            MGARD::print_statistics(P_ori.data(), P_dec, num_elements);
            MGARD::print_statistics(D_ori.data(), D_dec, num_elements);
            error_Mach = std::vector<T>(num_elements);
            error_est_Mach = std::vector<T>(num_elements);
            std::cout << "iter" << iter << ": The old ebs are:" << std::endl;
            MDR::print_vec(ebs);
            tolerance_met = halfing_error_Mach_uniform(Vx_dec, Vy_dec, Vz_dec, P_dec, D_dec, num_elements, mask, tau, ebs);
            std::cout << "iter" << iter << ": The new ebs are:" << std::endl;
            MDR::print_vec(ebs);
            // std::cout << names[1] << " requested error = " << tau << std::endl;
            max_act_error = print_max_abs(names[1] + " error", error_Mach);
            max_est_error = print_max_abs(names[1] + " error_est", error_est_Mach);   	
        }
    }
    else{
        std::vector<PDR::WeightedApproximationBasedReconstructor<T, PDR::SZApproximator<T>, MDR::WeightedNegaBinaryBPEncoder<T, uint32_t>, AdaptiveLevelCompressor, SignExcludeGreedyBasedSizeInterpreter<MDR::MaxErrorEstimatorHB<T>>, MaxErrorEstimatorHB<T>, ConcatLevelFileRetriever>> reconstructors;
        std::vector<std::vector<int>> weights(n_variable, std::vector<int>(num_elements, 0));
        for(int i=0; i<n_variable; i++){
            std::string rdir_prefix = rdata_file_prefix + varlist[i];
            std::string metadata_file = rdir_prefix + "_refactored/metadata.bin";
            std::vector<std::string> files;
            int num_levels = 1;
            for(int i=0; i<num_levels; i++){
                std::string filename = rdir_prefix + "_refactored/level_" + std::to_string(i) + ".bin";
                files.push_back(filename);
            }
            auto approximator = PDR::SZApproximator<T>();
            auto encoder = WeightedNegaBinaryBPEncoder<T, uint32_t>();
            auto compressor = AdaptiveLevelCompressor(64);
            auto estimator = MaxErrorEstimatorHB<T>();
            auto interpreter = SignExcludeGreedyBasedSizeInterpreter<MaxErrorEstimatorHB<T>>(estimator);
            auto retriever = ConcatLevelFileRetriever(metadata_file, files);
            reconstructors.push_back(generateWBPReconstructor<T>(approximator, encoder, compressor, estimator, interpreter, retriever));
            reconstructors.back().load_metadata();
            reconstructors.back().load_weight();
            reconstructors.back().span_weight();
            weights[i] = reconstructors.back().get_int_weights();
        }    
        weight_file_size = reconstructors[n_variable - 1].get_weight_file_size();
        while((!tolerance_met) && (iter < max_iter)){
            iter ++;
            for(int i=0; i<n_variable; i++){
                auto reconstructed_data = reconstructors[i].progressive_reconstruct(ebs[i] / static_cast<T>(std::pow(2.0, reconstructors[i].get_max_weight())), -1);
                total_retrieved_size[i] = reconstructors[i].get_retrieved_size();
                if(i < 3){
                    // reconstruct with mask
                    int index = 0;
                    for(int j=0; j<num_elements; j++){
                        if(mask[j]){
                            reconstructed_vars[i][j] = reconstructed_data[index ++];
                        }
                        else{
                            reconstructed_vars[i][j] = 0; 
                            index++;
                        }
                    }
                }
                else{
                    memcpy(reconstructed_vars[i].data(), reconstructed_data, num_elements*sizeof(T));
                }
            }
            Vx_dec = reconstructed_vars[0].data();
            Vy_dec = reconstructed_vars[1].data();
            Vz_dec = reconstructed_vars[2].data();
            P_dec = reconstructed_vars[3].data();
            D_dec = reconstructed_vars[4].data();
            MGARD::print_statistics(Vx_ori.data(), Vx_dec, num_elements);
            MGARD::print_statistics(Vy_ori.data(), Vy_dec, num_elements);
            MGARD::print_statistics(Vz_ori.data(), Vz_dec, num_elements);
            MGARD::print_statistics(P_ori.data(), P_dec, num_elements);
            MGARD::print_statistics(D_ori.data(), D_dec, num_elements);
            error_Mach = std::vector<T>(num_elements);
            error_est_Mach = std::vector<T>(num_elements);
            std::cout << "iter" << iter << ": The old ebs are:" << std::endl;
            MDR::print_vec(ebs);
            tolerance_met = halfing_error_Mach_uniform(Vx_dec, Vy_dec, Vz_dec, P_dec, D_dec, num_elements, mask, tau, ebs, weights);
            std::cout << "iter" << iter << ": The new ebs are:" << std::endl;
            MDR::print_vec(ebs);
            /* test
            std::string filename = "./Result/Temp_err.dat";
            std::ofstream outfile1(filename, std::ios::binary);
            if (!outfile1.is_open()) {
                std::cerr << "Failed to open file for writing: " << filename << std::endl;
                exit(-1);
            }

            outfile1.write(reinterpret_cast<const char*>(error_est_Mach.data()), error_est_Mach.size() * sizeof(float));
        
            outfile1.close();
            std::cout << "Data saved successfully to " << filename << std::endl;
            //*/
            // std::cout << names[1] << " requested error = " << tau << std::endl;
            max_act_error = print_max_abs(names[1] + " error", error_Mach);
            max_est_error = print_max_abs(names[1] + " error_est", error_est_Mach);   	
        }
    }
    return total_retrieved_size;
}

template<class T>
std::vector<size_t> retrieve_Mach_PMGARD(std::string rdata_file_prefix, T tau, std::vector<T> ebs, size_t num_elements, const std::vector<unsigned char>& mask, int weighted, T & max_act_error, T & max_est_error, size_t & weight_file_size){
    int max_iter = 10;
	bool tolerance_met = false;
	int n_variable = ebs.size();
	std::vector<std::vector<T>> reconstructed_vars(n_variable, std::vector<T>(num_elements));
	std::vector<size_t> total_retrieved_size(n_variable, 0);
    if(!weighted){
        std::vector<MDR::ComposedReconstructor<T, MGARDHierarchicalDecomposer<T>, DirectInterleaver<T>, NegaBinaryBPEncoder<T, uint32_t>, AdaptiveLevelCompressor, SignExcludeGreedyBasedSizeInterpreter<MaxErrorEstimatorHB<T>>, MaxErrorEstimatorHB<T>, ConcatLevelFileRetriever>> reconstructors;
        for(int i=0; i<n_variable; i++){
            std::string rdir_prefix = rdata_file_prefix + varlist[i];
            std::string metadata_file = rdir_prefix + "_refactored/metadata.bin";
            std::vector<std::string> files;
            int num_levels = 9;
            for(int i=0; i<num_levels; i++){
                std::string filename = rdir_prefix + "_refactored/level_" + std::to_string(i) + ".bin";
                files.push_back(filename);
            }
            auto decomposer = MDR::MGARDHierarchicalDecomposer<T>();
            auto interleaver = DirectInterleaver<T>();
            auto encoder = NegaBinaryBPEncoder<T, uint32_t>();
            auto compressor = AdaptiveLevelCompressor(64);
            auto retriever = ConcatLevelFileRetriever(metadata_file, files);
            auto estimator = MaxErrorEstimatorHB<T>();
            auto interpreter = SignExcludeGreedyBasedSizeInterpreter<MaxErrorEstimatorHB<T>>(estimator);
            reconstructors.push_back(generateReconstructor<T>(decomposer, interleaver, encoder, compressor, estimator, interpreter, retriever));
            reconstructors.back().load_metadata();
        }
        while((!tolerance_met) && (iter < max_iter)){
            iter ++;
            for(int i=0; i<n_variable; i++){
                auto reconstructed_data = reconstructors[i].progressive_reconstruct(ebs[i], -1);
                total_retrieved_size[i] = reconstructors[i].get_retrieved_size();
                if(i < 3){
                    // reconstruct with mask
                    int index = 0;
                    for(int j=0; j<num_elements; j++){
                        if(mask[j]){
                            reconstructed_vars[i][j] = reconstructed_data[index ++];
                        }
                        else{
                            reconstructed_vars[i][j] = 0; 
                            index++;
                        }
                    }
                }
                else{
                    memcpy(reconstructed_vars[i].data(), reconstructed_data, num_elements*sizeof(T));
                }
            }
            Vx_dec = reconstructed_vars[0].data();
            Vy_dec = reconstructed_vars[1].data();
            Vz_dec = reconstructed_vars[2].data();
            P_dec = reconstructed_vars[3].data();
            D_dec = reconstructed_vars[4].data();
            MGARD::print_statistics(Vx_ori.data(), Vx_dec, num_elements);
            MGARD::print_statistics(Vy_ori.data(), Vy_dec, num_elements);
            MGARD::print_statistics(Vz_ori.data(), Vz_dec, num_elements);
            MGARD::print_statistics(P_ori.data(), P_dec, num_elements);
            MGARD::print_statistics(D_ori.data(), D_dec, num_elements);
            error_Mach = std::vector<T>(num_elements);
            error_est_Mach = std::vector<T>(num_elements);
            std::cout << "iter" << iter << ": The old ebs are:" << std::endl;
            MDR::print_vec(ebs);
            tolerance_met = halfing_error_Mach_uniform(Vx_dec, Vy_dec, Vz_dec, P_dec, D_dec, num_elements, mask, tau, ebs);
            std::cout << "iter" << iter << ": The new ebs are:" << std::endl;
            MDR::print_vec(ebs);
            // std::cout << names[1] << " requested error = " << tau << std::endl;
            max_act_error = print_max_abs(names[1] + " error", error_Mach);
            max_est_error = print_max_abs(names[1] + " error_est", error_est_Mach);   	
        }
    }
    else{
        std::vector<MDR::WeightReconstructor<T, MGARDHierarchicalDecomposer<T>, DirectInterleaver<T>, DirectInterleaver<int>, WeightedNegaBinaryBPEncoder<T, uint32_t>, AdaptiveLevelCompressor, SignExcludeGreedyBasedSizeInterpreter<MaxErrorEstimatorHB<T>>, MaxErrorEstimatorHB<T>, ConcatLevelFileRetriever>> reconstructors;
        std::vector<std::vector<int>> weights(n_variable, std::vector<int>(num_elements, 0));
        for(int i=0; i<n_variable; i++){
            std::string rdir_prefix = rdata_file_prefix + varlist[i];
            std::string metadata_file = rdir_prefix + "_refactored/metadata.bin";
            std::vector<std::string> files;
            int num_levels = 9;
            for(int i=0; i<num_levels; i++){
                std::string filename = rdir_prefix + "_refactored/level_" + std::to_string(i) + ".bin";
                files.push_back(filename);
            }
            auto decomposer = MDR::MGARDHierarchicalDecomposer<T>();
            auto interleaver = DirectInterleaver<T>();
            auto weight_interleaver = DirectInterleaver<int>();
            auto encoder = WeightedNegaBinaryBPEncoder<T, uint32_t>();
            auto compressor = AdaptiveLevelCompressor(64);
            auto retriever = ConcatLevelFileRetriever(metadata_file, files);
            auto estimator = MaxErrorEstimatorHB<T>();
            auto interpreter = SignExcludeGreedyBasedSizeInterpreter<MaxErrorEstimatorHB<T>>(estimator);
            reconstructors.push_back(generateReconstructor<T>(decomposer, interleaver, weight_interleaver, encoder, compressor, estimator, interpreter, retriever));
            reconstructors.back().load_metadata();
            reconstructors.back().load_weight();
            reconstructors.back().span_weight();
            weights[i] = reconstructors.back().get_int_weights();
        }
        weight_file_size = reconstructors[n_variable - 1].get_weight_file_size();
        while((!tolerance_met) && (iter < max_iter)){
            iter ++;
            for(int i=0; i<n_variable; i++){
                auto reconstructed_data = reconstructors[i].progressive_reconstruct(ebs[i] / static_cast<T>(std::pow(2.0, reconstructors[i].get_max_weight())), -1);
                total_retrieved_size[i] = reconstructors[i].get_retrieved_size();
                if(i < 3){
                    // reconstruct with mask
                    int index = 0;
                    for(int j=0; j<num_elements; j++){
                        if(mask[j]){
                            reconstructed_vars[i][j] = reconstructed_data[index ++];
                        }
                        else{
                            reconstructed_vars[i][j] = 0; 
                            index++;
                        }
                    }
                }
                else{
                    memcpy(reconstructed_vars[i].data(), reconstructed_data, num_elements*sizeof(T));
                }
            }
            Vx_dec = reconstructed_vars[0].data();
            Vy_dec = reconstructed_vars[1].data();
            Vz_dec = reconstructed_vars[2].data();
            P_dec = reconstructed_vars[3].data();
            D_dec = reconstructed_vars[4].data();
            MGARD::print_statistics(Vx_ori.data(), Vx_dec, num_elements);
            MGARD::print_statistics(Vy_ori.data(), Vy_dec, num_elements);
            MGARD::print_statistics(Vz_ori.data(), Vz_dec, num_elements);
            MGARD::print_statistics(P_ori.data(), P_dec, num_elements);
            MGARD::print_statistics(D_ori.data(), D_dec, num_elements);
            error_Mach = std::vector<T>(num_elements);
            error_est_Mach = std::vector<T>(num_elements);
            std::cout << "iter" << iter << ": The old ebs are:" << std::endl;
            MDR::print_vec(ebs);
            tolerance_met = halfing_error_Mach_uniform(Vx_dec, Vy_dec, Vz_dec, P_dec, D_dec, num_elements, mask, tau, ebs, weights);
            std::cout << "iter" << iter << ": The new ebs are:" << std::endl;
            MDR::print_vec(ebs);
            /* test
            std::string filename = "./Result/Temp_err.dat";
            std::ofstream outfile1(filename, std::ios::binary);
            if (!outfile1.is_open()) {
                std::cerr << "Failed to open file for writing: " << filename << std::endl;
                exit(-1);
            }

            outfile1.write(reinterpret_cast<const char*>(error_est_Mach.data()), error_est_Mach.size() * sizeof(float));
        
            outfile1.close();
            std::cout << "Data saved successfully to " << filename << std::endl;
            //*/
            // std::cout << names[1] << " requested error = " << tau << std::endl;
            max_act_error = print_max_abs(names[1] + " error", error_Mach);
            max_est_error = print_max_abs(names[1] + " error_est", error_est_Mach);   	
        }
    }
    return total_retrieved_size;
}


int main(int argc, char ** argv){

    using T = float;
	int argv_id = 1;
    int compressor = atoi(argv[argv_id++]);
    int weighted = atoi(argv[argv_id++]);
    T target_rel_eb = atof(argv[argv_id++]);
	std::string data_prefix_path = argv[argv_id++];
	std::string data_file_prefix = data_prefix_path + "/data/";
	std::string rdata_file_prefix = data_prefix_path + "/refactor/";

    size_t num_elements = 0;
    P_ori = MGARD::readfile<T>((data_file_prefix + "Pressure.dat").c_str(), num_elements);
    D_ori = MGARD::readfile<T>((data_file_prefix + "Density.dat").c_str(), num_elements);
    Vx_ori = MGARD::readfile<T>((data_file_prefix + "VelocityX.dat").c_str(), num_elements);
    Vy_ori = MGARD::readfile<T>((data_file_prefix + "VelocityY.dat").c_str(), num_elements);
    Vz_ori = MGARD::readfile<T>((data_file_prefix + "VelocityZ.dat").c_str(), num_elements);
    std::vector<T> ebs;
    T max_abs_values[3] = {compute_max_abs_value(Vx_ori.data(), Vx_ori.size()), compute_max_abs_value(Vy_ori.data(), Vy_ori.size()), compute_max_abs_value(Vz_ori.data(), Vz_ori.size())};
    T max_abs_value = max_abs_values[0];
    if(max_abs_value < max_abs_values[1]) max_abs_value = max_abs_values[1];
    if(max_abs_value < max_abs_values[2]) max_abs_value = max_abs_values[2];
    ebs.push_back(max_abs_value*target_rel_eb);
    ebs.push_back(max_abs_value*target_rel_eb);
    ebs.push_back(max_abs_value*target_rel_eb);
    ebs.push_back(compute_max_abs_value(P_ori.data(), P_ori.size())*target_rel_eb);
    ebs.push_back(compute_max_abs_value(D_ori.data(), D_ori.size())*target_rel_eb);
	int n_variable = ebs.size();
    std::vector<std::vector<T>> vars_vec = {Vx_ori, Vy_ori, Vz_ori, P_ori, D_ori};
    std::vector<T> var_range(n_variable);
    for(int i=0; i<n_variable; i++){
        var_range[i] = compute_value_range(vars_vec[i]);
    } 

	struct timespec start, end;
	int err;
	T elapsed_time;

	err = clock_gettime(CLOCK_REALTIME, &start);

    std::vector<T> Mach(num_elements);
    compute_Mach(Vx_ori.data(), Vy_ori.data(), Vz_ori.data(), P_ori.data(), D_ori.data(), num_elements, Mach.data());
	Mach_ori = Mach.data();
    T tau = compute_value_range(Mach)*target_rel_eb;

    std::string mask_file = rdata_file_prefix + "mask.bin";
    size_t num_valid_data = 0;
    auto mask = MGARD::readfile<unsigned char>(mask_file.c_str(), num_valid_data);
    T max_act_error = 0, max_est_error = 0;
    size_t weight_file_size = 0;
    std::vector<size_t> total_retrieved_size;
    
    switch (compressor)
	{
	case Dummy:
		total_retrieved_size = retrieve_Mach_Dummy(rdata_file_prefix, tau, ebs, num_elements, mask, weighted, max_act_error, max_est_error, weight_file_size);
		break;
	case SZ3:
		total_retrieved_size = retrieve_Mach_SZ3(rdata_file_prefix, tau, ebs, num_elements, mask, weighted, max_act_error, max_est_error, weight_file_size);
		break;
	case PMGARD:
		total_retrieved_size = retrieve_Mach_PMGARD(rdata_file_prefix, tau, ebs, num_elements, mask, weighted, max_act_error, max_est_error, weight_file_size);
		break;
	default:
		break;
	}

	err = clock_gettime(CLOCK_REALTIME, &end);
	elapsed_time = (T)(end.tv_sec - start.tv_sec) + (T)(end.tv_nsec - start.tv_nsec)/(T)1000000000;

	std::cout << "requested error = " << tau << std::endl;
	std::cout << "max_est_error = " << max_est_error << std::endl;
	std::cout << "max_act_error = " << max_act_error << std::endl;
	std::cout << "iter = " << iter << std::endl;
   
   	size_t total_size = std::accumulate(total_retrieved_size.begin(), total_retrieved_size.end(), 0) + weight_file_size;
	T cr = n_variable * num_elements * sizeof(T) * 1.0 / total_size;
	std::cout << "each retrieved size:";
    for(int i=0; i<n_variable; i++){
        std::cout << total_retrieved_size[i] << ", ";
    }
    std::cout << "weight_file_size = " << weight_file_size << std::endl;
	// MDR::print_vec(total_retrieved_size);
	std::cout << "aggregated cr = " << cr << std::endl;
	printf("elapsed_time = %.6f\n", elapsed_time);

    return 0;
}