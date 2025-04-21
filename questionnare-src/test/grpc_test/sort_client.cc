//g++ -std=c++11 -o sort_client sort_client.cc sort.pb.cc sort.grpc.pb.cc -lgrpc++ -lprotobuf -lpthread

#include <grpcpp/grpcpp.h>
#include "sort.pb.h"
#include "sort.grpc.pb.h"
#include <iostream>
#include <vector>
#include <sstream>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using mynamespace::SortService;    // 自定义命名空间
using mynamespace::SortRequest;    // 自定义命名空间
using mynamespace::SortResponse;   // 自定义命名空间

class SortClient{
public:
    SortClient(std::shared_ptr<Channel> channel) : stub_(SortService::NewStub(channel)){}

    //这是 SortClient 类中定义的一个公共成员函数，名为 SortNumbers，它的作用是向 gRPC 服务端发起请求，调用服务端对应的排序服务，并获取和返回排序后的结果（如果调用成功的话）
    std::vector<int> SortNumbers(const std::vector<int>& numbers){
        SortRequest request;
        for(int num : numbers){
            request.add_numbers(num);
        }

        SortResponse response;
        ClientContext context;
        Status status = stub_->SortNumbers(&context,request,&response);

        if (status.ok()) {
            return {response.sorted_numbers().begin(), response.sorted_numbers().end()};
        } else {
            std::cerr << "gRPC call failed: " << status.error_message() << std::endl;
            return {};
        }
    }

private:
    std::unique_ptr<SortService::Stub> stub_;
};

int main() {
    SortClient client(grpc::CreateChannel("localhost:50052", grpc::InsecureChannelCredentials()));

    std::cout << "Enter numbers to sort (separated by space): ";
    std::string line;
    std::getline(std::cin, line);

    std::stringstream ss(line);
    std::vector<int> numbers;
    int num;

    while (ss >> num) {
        numbers.push_back(num);
    }

    std::cout << "Unsorted numbers: ";
    for (int num : numbers) std::cout << num << " ";
    std::cout << std::endl;

    std::vector<int> sorted_numbers = client.SortNumbers(numbers);

    std::cout << "Sorted numbers: ";
    for (int num : sorted_numbers) std::cout << num << " ";
    std::cout << std::endl;

    return 0;
}