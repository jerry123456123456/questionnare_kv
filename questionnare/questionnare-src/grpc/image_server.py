#python3 -m grpc_tools.protoc -I. --python_out=. --grpc_python_out=. image_service.proto

#运行  ：  python3 image_server.py

import grpc
from concurrent import futures
import image_service_pb2
import image_service_pb2_grpc
import os
import random

# 定义 gRPC 服务
class ImageService(image_service_pb2_grpc.ImageServiceServicer):
    def GetImage(self, request, context):
        # 指定图片存放的目录
        image_dir = '/home/jerry/Desktop/questionnare/questionnare-src/grpc'
        
        # 获取该目录下所有 PNG 文件
        image_files = [f for f in os.listdir(image_dir) if f.endswith('.png')]
        
        # 如果没有图片文件，返回错误
        if not image_files:
            context.set_details("No images found")
            context.set_code(grpc.StatusCode.NOT_FOUND)
            return image_service_pb2.ImageResponse()

        # 随机选择一张图片
        image_file = random.choice(image_files)
        image_path = os.path.join(image_dir, image_file)

        # 读取图片文件并返回二进制数据
        with open(image_path, 'rb') as img_file:
            image_data = img_file.read()

        # 返回图片的二进制数据
        return image_service_pb2.ImageResponse(image_data=image_data)

# 启动 gRPC 服务器
def serve():
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    image_service_pb2_grpc.add_ImageServiceServicer_to_server(ImageService(), server)
    server.add_insecure_port('[::]:50051')  # 启动服务器，监听50051端口
    server.start()
    print("Server started at 127.0.0.1:50051")
    server.wait_for_termination()

if __name__ == '__main__':
    serve()
