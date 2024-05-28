//
// Created by shrevena on 23/05/24.
//

#include <fstream>
#include <vector>
#include <sstream>
#include <iomanip>
#include <complex>

int main() {
    std::ofstream koios_medium_info_file("koios_medium_heap_profiling_info.txt", std::ios::out);
    auto original_precision = koios_medium_info_file.precision();

    std::vector<std::string> koios_medium_test_types = {"attention_layer", "bnn", "dla_like.small", "robot_rl",
                                                        "tpu_like.small.os", "tpu_like.small.ws"};

    for (int i = 1; i <= 27; ++i) {
        std::string run_num;

        if (i < 10)
            run_num = "00" + std::to_string(i);
        else
            run_num = "0" + std::to_string(i);

        koios_medium_info_file << "run" + run_num + ": " << std::endl;

        std::vector<float> percents;
        for (const auto& test_type: koios_medium_test_types) {
            std::string file_path = "./fine-grained-parallel-router/testing/koios_medium/run" + run_num +
                                    "/k6FracN10LB_mem20K_complexDSP_customSB_22nm.xml/" + test_type +
                                    ".pre-vpr.blif/common/vpr.dot";

            std::ifstream dot_file(file_path);
            std::stringstream buffer;
            buffer << dot_file.rdbuf();
            std::string dot_file_text = buffer.str();

            float total_percent = 0.0;

            size_t add_pos = dot_file_text.find("BinaryHeap::add_to_heap");
            if (add_pos != std::string::npos) {
                std::string time_percent_str = dot_file_text.substr(add_pos + 25, 4);

                if (time_percent_str.back() == '%')
                    time_percent_str.pop_back();

                total_percent += std::stof(time_percent_str);
            }

            size_t get_head_pos = dot_file_text.find("BinaryHeap::get_heap_head");
            if (get_head_pos != std::string::npos) {
                std::string time_percent_str = dot_file_text.substr(get_head_pos + 27, 4);

                if (time_percent_str.back() == '%')
                    time_percent_str.pop_back();

                total_percent += std::stof(time_percent_str);
            }

            size_t build_pos = dot_file_text.find("BinaryHeap::build_heap");
            if (build_pos != std::string::npos) {
                std::string time_percent_str = dot_file_text.substr(build_pos + 24, 4);

                if (time_percent_str.back() == '%')
                    time_percent_str.pop_back();

                total_percent += std::stof(time_percent_str);
            }

            koios_medium_info_file << "\t" + test_type + ": " << std::setprecision(3) << total_percent << "%"
                                   << std::endl;

            percents.push_back(total_percent);
        }

        float product = 1.0;
        for (auto percent: percents) {
            product *= percent;
        }

        float geomean = std::pow(product, 1.0 / 6.0);

        koios_medium_info_file << "\tGEOMEAN = " << std::setprecision(original_precision) << geomean << "%" << std::endl
                               << std::endl;
    }

    koios_medium_info_file.close();

    std::ofstream titan_info_file("titan_heap_profiling_info.txt", std::ios::out);
    original_precision = titan_info_file.precision();

    std::vector<std::string> titan_test_types = {"bitcoin_miner_stratixiv_arch_timing",
                                                 "directrf_stratixiv_arch_timing", "LU230_stratixiv_arch_timing",
                                                 "mes_noc_stratixiv_arch_timing", "sparcT1_chip2_stratixiv_arch_timing",
                                                 "openCV_stratixiv_arch_timing"};

    for (int i = 1; i <= 44; ++i) {
        std::string run_num;

        if (i < 10)
            run_num = "00" + std::to_string(i);
        else
            run_num = "0" + std::to_string(i);

        titan_info_file << "run" + run_num + ": " << std::endl;

        std::vector<float> percents;
        for (const auto& test_type: titan_test_types) {
            std::string file_path = "./fine-grained-parallel-router/testing/titan/run" + run_num +
                                    "/stratixiv_arch.timing.xml/" + test_type +
                                    ".blif/common/vpr.dot";

            std::ifstream dot_file(file_path);
            std::stringstream buffer;
            buffer << dot_file.rdbuf();
            std::string dot_file_text = buffer.str();

            float total_percent = 0.0;

            size_t add_pos = dot_file_text.find("BinaryHeap::add_to_heap");
            if (add_pos != std::string::npos) {
                std::string time_percent_str = dot_file_text.substr(add_pos + 25, 4);

                if (time_percent_str.back() == '%')
                    time_percent_str.pop_back();

                total_percent += std::stof(time_percent_str);
            }

            size_t get_head_pos = dot_file_text.find("BinaryHeap::get_heap_head");
            if (get_head_pos != std::string::npos) {
                std::string time_percent_str = dot_file_text.substr(get_head_pos + 27, 4);

                if (time_percent_str.back() == '%')
                    time_percent_str.pop_back();

                total_percent += std::stof(time_percent_str);
            }

            size_t build_pos = dot_file_text.find("BinaryHeap::build_heap");
            if (build_pos != std::string::npos) {
                std::string time_percent_str = dot_file_text.substr(build_pos + 24, 4);

                if (time_percent_str.back() == '%')
                    time_percent_str.pop_back();

                total_percent += std::stof(time_percent_str);
            }

            titan_info_file << "\t" + test_type + ": " << std::setprecision(3) << total_percent << "%" << std::endl;

            percents.push_back(total_percent);
        }

        float product = 1.0;
        for (auto percent: percents) {
            product *= percent;
        }

        float geomean = std::pow(product, 1.0 / 6.0);

        titan_info_file << "\tGEOMEAN = " << std::setprecision(original_precision) << geomean << "%" << std::endl
                        << std::endl;
    }

    titan_info_file.close();
}
