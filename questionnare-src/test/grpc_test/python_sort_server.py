#python3 python_sort_server.py

from concurrent import futures
import grpc
import sort_pb2
import sort_pb2_grpc

class SortService(sort_pb2_grpc.SortServiceServicer):
    def SortNumbers(self, request, context):
        # 打印收到的数字列表
        print("Received numbers:", request.numbers)

        # 排序请求的数字
        sorted_numbers = sorted(request.numbers)

        # 返回排序结果
        return sort_pb2.SortResponse(sorted_numbers=sorted_numbers)

def serve():
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    sort_pb2_grpc.add_SortServiceServicer_to_server(SortService(), server)
    server.add_insecure_port('[::]:50051')
    print("Python gRPC server is running on port 50051...")
    server.start()
    server.wait_for_termination()

if __name__ == '__main__':
    serve()

