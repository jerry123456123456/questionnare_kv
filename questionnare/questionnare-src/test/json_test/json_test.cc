//g++ json_test.cc -o json_test -I../../jsoncpp -L/usr/local/lib -ljsoncpp -lstdc++


#include <iostream>
#include <json/json.h>
#include <string>
#include <memory> // 添加对 unique_ptr 的支持

// 辅助函数，用于打印 JSON 对象（格式化输出）
void printJson(const Json::Value& root) {
    Json::StyledWriter writer;
    std::string jsonString = writer.write(root);
    std::cout << jsonString << std::endl;
}

int main() {
    // 创建一个空的 JSON 对象
    Json::Value root;

    // 添加一些不同类型的数据到 JSON 对象中
    root["name"] = "John Doe";
    root["city"] = "Anytown";
    root["age"] = 30;
    root["isStudent"] = false;

    // 添加数组类型数据
    Json::Value hobbies;
    hobbies.append("Reading");
    hobbies.append("Running");
    root["hobbies"] = hobbies;

    // 添加嵌套的 JSON 对象
    Json::Value address;
    address["street"] = "123 Main St";
    address["zipCode"] = "12345";
    root["address"] = address;

    // 打印 JSON 对象
    std::cout << "Serialized JSON Object:" << std::endl;
    printJson(root);

    // 反序列化 JSON 字符串为 JSON 对象
    Json::Value newRoot;
    Json::Reader reader;
    std::string jsonString = Json::StyledWriter().write(root);
    bool parsingSuccessful = reader.parse(jsonString, newRoot);
    if (!parsingSuccessful) {
        std::cerr << "Failed to parse JSON: " << reader.getFormattedErrorMessages() << std::endl;
        return 1;
    }

    // 访问反序列化后的 JSON 对象中的数据
    std::cout << "Name: " << newRoot["name"].asString() << std::endl;
    std::cout << "Age: " << newRoot["age"].asInt() << std::endl;
    std::cout << "Is Student: " << (newRoot["isStudent"].asBool() ? "Yes" : "No") << std::endl;

    // 访问数组数据
    std::cout << "Hobbies: ";
    for (const auto& hobby : newRoot["hobbies"]) {
        std::cout << hobby.asString() << " ";
    }
    std::cout << std::endl;

    // 访问嵌套对象数据
    std::cout << "Address Street: " << newRoot["address"]["street"].asString() << std::endl;
    std::cout << "Address Zip Code: " << newRoot["address"]["zipCode"].asString() << std::endl;

    return 0;
}

/*结果
root@k8s-master1:/home/jerry/Desktop/questionnare/questionnare-src/test/json_test# ls
json_test  json_test.cc
root@k8s-master1:/home/jerry/Desktop/questionnare/questionnare-src/test/json_test# ./json_test 
Serialized JSON Object:
{
   "address" : {
      "street" : "123 Main St",
      "zipCode" : "12345"
   },
   "age" : 30,
   "city" : "Anytown",
   "hobbies" : [ "Reading", "Running" ],
   "isStudent" : false,
   "name" : "John Doe"
}

Name: John Doe
Age: 30
Is Student: No
Hobbies: Reading Running 
Address Street: 123 Main St
Address Zip Code: 12345

*/