#include <iostream>
#include "../base/tc_thread_pool.h"
#include <vector>
#include <random>
#include <algorithm>
#include <fstream>
#include <thread>
#include <future>
#include <numeric>

std::vector<int> generated_data;

// 1. 生成随机数据
void generate_data(int count) {
    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(1, 100);
    generated_data.clear();

    for (int i = 0; i < count; ++i) {
        int value = distribution(generator);
        std::cout << value << " ";
        generated_data.push_back(value);
    }
    std::cout << std::endl;
    std::cout << "Generated " << count << " random data points.\n";
}

// 2. 数据过滤
std::vector<int> filter_data(const std::vector<int>& data, int threshold) {
    std::vector<int> filtered_data;
    std::copy_if(data.begin(), data.end(), std::back_inserter(filtered_data),
                 [threshold](int value) { return value > threshold; });
    std::cout << "Filtered data to include values greater than " << threshold << ".\n";
    return filtered_data;
}

// 3. 数据处理
std::vector<int> process_data(const std::vector<int>& data) {
    std::vector<int> processed_data;
    std::transform(data.begin(), data.end(), std::back_inserter(processed_data),
                   [](int value) { return value * value; });
    std::cout << "Processed data (squared values).\n";
    return processed_data;
}

// 5. 结果存储
void aggregate_data(double average) {
    std::ofstream out("result.txt");
    if (out.is_open()) {
        out << "Average value: " << average << "\n";
        out.close();
        std::cout << "Stored result in result.txt.\n";
    } else {
        std::cerr << "Failed to open result.txt for writing.\n";
    }
}

// 6. 结果报告
void report_result(double average) {
    std::cout << "Report: The average value is " << average << ".\n";
}

int main() {
    ThreadPool tpool;
    tpool.Init(5);
    // 启动线程池
    tpool.Start();

    // 生成随机数据
    auto generate_future = tpool.Exec(generate_data, 100);
    generate_future.get(); // 等待生成数据的任务完成

    // 过滤数据
    auto future_result = tpool.Exec(filter_data, generated_data, 50);
    std::vector<int> filtered_result = future_result.get(); // 获取过滤后的结果
    std::cout << "Filtered Result: ";
    for (int i = 0; i < filtered_result.size(); ++i) {
        std::cout << filtered_result[i] << " ";
    }
    std::cout << std::endl;

    // 处理数据
    auto processed_future = tpool.Exec(process_data, filtered_result);
    std::vector<int> processed_result = processed_future.get(); // 获取处理后的结果
    std::cout << "Processed Result: ";
    for (const auto &num : processed_result) {
        std::cout << num << " ";
    }
    std::cout << std::endl;

    // 计算平均值
    if (processed_result.empty()) {
        std::cerr << "No processed data available to compute average.\n";
        return 1; // 或者处理其他逻辑
    }
    double average = static_cast<double>(std::accumulate(processed_result.begin(), processed_result.end(), 0)) / processed_result.size();
    
    // 存储结果
    tpool.Exec(aggregate_data, average);

    // 报告结果
    tpool.Exec(report_result, average);

    return 0;
}
