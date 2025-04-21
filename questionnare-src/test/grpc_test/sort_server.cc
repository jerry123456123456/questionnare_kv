//g++ -std=c++11 -o sort_server sort_server.cc sort.pb.cc sort.grpc.pb.cc -lgrpc++ -lprotobuf -lpthread

#include <grpcpp/grpcpp.h>
#include "sort.pb.h"          // 由 protoc 生成的消息类型
#include "sort.grpc.pb.h"     // 由 protoc 生成的服务接口
#include <grpcpp/support/status_code_enum.h>
#include <iostream>
#include <vector>

#include <grpcpp/grpcpp.h>
#include "sort.pb.h"          // 由 protoc 生成的消息类型
#include "sort.grpc.pb.h"     // 由 protoc 生成的服务接口
#include <iostream>
#include <vector>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using mynamespace::SortService;    // 自定义命名空间
using mynamespace::SortRequest;    // 自定义命名空间
using mynamespace::SortResponse;   // 自定义命名空间

class SortServiceImpl final : public SortService::Service{
public:
    Status SortNumbers(ServerContext *context,const SortRequest *request,SortResponse *response) override {
        // 转发请求到 Python 服务
        std::cout << "Forwarding request to Python service..." << std::endl;
        
        //定义通道
        grpc::ChannelArguments args;
        //创建一个grpc通道对象，用于与grpc服务器建立连接。使用自定义的参数来配置这个通道
        //"localhost:50051"：这是一个字符串参数，指定了要连接的 gRPC 服务器的地址和端口号
        //这是用于指定通道的安全凭证相关信息。InsecureChannelCredentials 表示创建的是一个不安全的通道连接
        auto channel = grpc::CreateCustomChannel("127.0.0.1:50051",grpc::InsecureChannelCredentials(),args);
        //这个代码的目的是基于前面创建好的grpc通道对象来创建一个grpc存根，它是客户端服务器交互的关键
        //它隐藏了底层网络通信的细节，提供了类似本地函数调用的方式来发起远程调用
        //NewStub(channel)：这是 SortService 类的一个静态成员函数，用于创建该服务对应的存根对象，
        //传入的参数 channel 就是前面刚刚创建好的用于和服务器通信的通道，存根对象会利用这个通道来发送请求到服务器并接收服务器的响应，
        //从而实现客户端与服务器端服务方法的调用交互
        auto stub = mynamespace::SortService::NewStub(channel);

        //grpc::ClientContext 类用于封装一次 gRPC 远程调用的上下文信息
        grpc::ClientContext clientContext;
        //SortResponse pythonResponse; 这行代码就是创建了一个 SortResponse 类型的对象 pythonResponse，
        //用于后续接收从服务器端（这里推测是和 Python 相关的服务端，
        //可能是 Python 实现的某个服务，通过 gRPC 来与 C++ 客户端交互）返回的响应数据
        SortResponse pythonResponse;

        //将请求数据传给python服务
        Status status = stub->SortNumbers(&clientContext,*request,&pythonResponse);
        //如果成功
        if(status.ok()){
            //将从python服务器接收到并存储在pythonResponse对象中的排序后的数据，复制到上层调用者response对象的相应字段中
            response->mutable_sorted_numbers()->CopyFrom(pythonResponse.sorted_numbers());
            return Status::OK;
        }else{
            return Status(grpc::StatusCode::UNKNOWN,"Failed to contact Python service");
        }
    }
};

void RunServer(){
    std::string server_address("0.0.0.0:50052");
    SortServiceImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(server_address,grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "C++ gRPC server is running on " << server_address << std::endl;
    server->Wait();
}

int main(){
    RunServer();
    return 0;
}
