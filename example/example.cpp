#include <iostream>
#include "../FStruct.h"
#include <typeinfo>
#include <iostream>
#include <list>
#include <functional>
#include <time.h>//clock()需要的头文件
using namespace std;

//g++ -std=c++11 ../FStruct.cpp example.cpp -o main -w

int main()
{
    cout << "===============================================================" << endl;
    cout << "\n1：结构体只包含基础类型------------------------------------\n";
    //1.结构体只包含基础类型(int,char,char*,string,以及由基础类型构成的数组,或者是STL容器(map暂不支持全类型))，则只需要注册成员即可。
    REGISTEREDMEMBER(student, name, age);  //注册student成员
    student stu;
    stu.name = "yujing";
    stu.age = 21;
    string stu_json = "";
    //结构体转json
    Fdog::FJson(stu_json, stu);  
    //结果 输出stu_json为： {"name":"yujing","age":21}
    cout << stu_json << endl;
    student stu2;
    string stu2_json = "{\"name\":\"zhangxv\",\"age\":21}"; //引号要做处理
    //json转结构体
    Fdog::FObject(stu2, stu2_json);  
    //结果  stu2.name = zhangxv   stu2.age = 21
    cout << "name = " << stu2.name << " age = " << stu2.age << endl;

    cout << "\n2：结构体中除了基础类型，还包括自定义结构体------------------------------------\n";

    //2.结构体中除了基础类型，还包括自定义结构体
    REGISTEREDMEMBER(teacher, name, age);  //注册teacher成员
    REGISTEREDMEMBER(school, stu, tea);     //注册school成员
    school sch;
    sch.stu.name = "liuliu";
    sch.stu.age = 18;
    sch.tea.name = "wufang";
    sch.tea.age = 48;
    string sch_json = "";
    //结构体转json
    Fdog::FJson(sch_json, sch); 
    //结果 输出sch_json为：{"stu":{"name":"liuliu","age":18},"tea":{"name":"wufang","age":48}}
  	cout << sch_json << endl;
    //json转结构体
    school sch2;
    string sch2_json = "{\"stu\":{\"name\":\"liuliu\",\"age\":18},\"tea\":{\"name\":\"wufang\",\"age\":48}}";
    Fdog::FObject(sch2, sch2_json);
    //结果  
    cout << "name = " << sch.stu.name << " age = " << sch.stu.age << endl;
    cout << "name = " << sch.tea.name << " age = " << sch.tea.age << endl;

    cout << "\n3：结构体中使用STL容器map------------------------------------\n";

    //3.结构体中使用STL容器map
    REGISTEREDMEMBER(class_map, grade);
    class_map cm;
    cm.grade["zhangxu"] = 88;
    cm.grade["yujing"] = 99;
    string cm_json;
    Fdog::FJson(cm_json, cm);
    cout << cm_json << endl;
    class_map cm2;
    string cm2_json = "{\"grade\":{\"lisi\":33,\"angwu\":11}}";
    Fdog::FObject(cm2, cm2_json);
    for(auto j : cm2.grade){
        cout << "name = " << j.first << " age = " << j.second << endl;
    }

    cout << "\n4：结构体成员存在数组，且数组存储的基础类型数据------------------------------------\n";

    //4.结构体成员存在数组，且数组存储的基础类型数据
    REGISTEREDMEMBER(class_base_array, numbers)
    class_base_array cba;
    cba.numbers[0] = 0;
    cba.numbers[1] = 1;
    cba.numbers[2] = 2;
    cba.numbers[3] = 3;
    cba.numbers[4] = 4;
    string cba_json = "";
    //结构体转json
    Fdog::FJson(cba_json, cba);
    cout << cba_json << endl;
    //json转结构体
    class_base_array cba2;
    string cba2_json = "{\"cba2_json\":{\"numbers\":[0,1,2,3,4]}}";
    Fdog::FObject(cba2, cba2_json);
    cout << "numbers is " << cba2.numbers[0] << " " << cba2.numbers[1] << " " << cba2.numbers[2] << " " << cba2.numbers[3] << " " << cba2.numbers[4] << endl;

    cout << "\n5：结构体成员存在STL容器，且容器为基础类型------------------------------------\n";

    //5.结构体成员存在STL容器，且容器为基础类型
    REGISTEREDMEMBER(class_base_vector, numbers)
    class_base_vector cbv; //要注意刚声明的变量 vector长度为0，使用下角标赋值会出错
    cbv.numbers.push_back(0);
    cbv.numbers.push_back(1);
    cbv.numbers.push_back(2);
    cbv.numbers.push_back(3);
    cbv.numbers.push_back(4);
    string cbv_json = "";
    //结构体转json
    Fdog::FJson(cbv_json, cbv);
    cout << cbv_json << endl;
    //json转结构体
    class_base_vector cbv2;
    string cbv2_json = "{\"cbv2_json\":{\"numbers\":[0,1,2,3,4]}}";
    Fdog::FObject(cbv2, cbv2_json);
    cout << "numbers is " << cbv2.numbers[0] << " " << cbv2.numbers[1] << " " << cbv2.numbers[2] << " " << cbv2.numbers[3] << " " << cbv2.numbers[4] << endl;

    cout << "\n6：结构体成员存在自定义类型的数组------------------------------------\n";

    //6.结构体成员存在自定义类型的数组
    REGISTEREDMEMBER(class_object_array, stus)
    class_object_array coa;
    coa.stus[0].name = "zhangxu";
    coa.stus[0].age = 21;
    coa.stus[1].name = "yujing";
    coa.stus[1].age = 22;
    string coa_json = "";
    //结构体转json
    Fdog::FJson(coa_json, coa);
    cout << coa_json << endl;
    //json转结构体
    class_object_array coa2;
    string coa2_json = "{\"stus\":[{\"name\":\"jinqi\",\"age\":25},{\"name\":\"ruiming\",\"age\":12}]}}";
    Fdog::FObject(coa2, coa2_json);
    cout << " name = " << coa2.stus[0].name << " age = " << coa2.stus[0].age << endl;
    cout << " name = " << coa2.stus[1].name << " age = " << coa2.stus[1].age << endl;

    cout << "\n7：结构体成员存在自定义类型STL容器------------------------------------\n";

    //7.结构体成员存在自定义类型STL容器
    REGISTEREDMEMBER(class_object_vector, stus)
    class_object_vector cov;
    student stu3;
    stu3.name = "zhangxu";
    stu3.age = 21;
    student stu4;
    stu4.name = "yujing";
    stu4.age = 21;
    cov.stus.push_back(stu3);
    cov.stus.push_back(stu4);
    string cov_json = "";
    //结构体转json
    Fdog::FJson(cov_json, cov);
    cout << cov_json << endl;
    //json转结构体
    class_object_vector cov2; //记得初始化长度 这个后期要改成自动
    cov2.stus.resize(2);
    string cov2_json = "{\"stus\":[{\"name\":\"jinqi\",\"age\":25},{\"name\":\"ruiming\",\"age\":12}]}";
    Fdog::FObject(cov2, cov2_json);
    cout << " name = " << cov2.stus[0].name << " age = " << cov2.stus[0].age << endl;
    cout << " name = " << cov2.stus[1].name << " age = " << cov2.stus[1].age << endl;

    cout << "\n8：支持别名------------------------------------\n";

    //8.支持别名
    Fdog::setAliasName("student", "name", "Aliasname"); //第一个参数为类型，第二参数为原名，第三个参数为别名
    stu_json = "";
    Fdog::FJson(stu_json, stu);  
    //结果 输出stu_json为： {"Aliasname":"yujing","age":21}
    cout << stu_json << endl;

    cout << "\n9：支持字段忽略------------------------------------\n";

    //9.支持字段忽略
    Fdog::setIgnoreField("student", "name");  //第一个参数为类型，第二参数为需要忽略的字段
    stu_json = "";
    Fdog::FJson(stu_json, stu);
    //结果 输出stu_json为： {"age":21}  //name字段的数据将被忽略
    cout <<  stu_json << endl;

    cout << "\n10：支持忽略字段大小写------------------------------------\n";

    //10.支持忽略字段大小写，当将json转为对象时，如json中的键值与对象中的成员名存在大小写不同，可以设定忽略大小写。
    Fdog::setIgnoreLU("student", "name");
    Fdog::setIgnoreLU("student", "age");
    stu_json = "{\"Name\":\"yujing\", \"AGE\":21}";
    Fdog::FObject(stu, stu_json);  //将Name对应name，AGE对应age
    cout << "name = " << stu.name << " age = " << stu.age << endl;

    //11.针对7，8，9接口增加对应的一次性接口，避免有多个字段需要设置，从而多次调用接口
    
    
    //12.默认支持模糊匹配
    //马上支持，当不小心写错字段名时，程序将自动进行模糊匹配，最大可能完成转换。
    
    
    //13.检测Json格式是否正确
    //马上支持
    
    
    //14.查找json中某个字段是否存在
    //马上支持
    
    //15.支持获取某个字段的值(返回类型支持int, double, string, bool)
    //马上支持
    
    //16.支持其他类型指针(指针类型将拥有可选字段属性，对于指针变量，在转换时，将先判断指针地址是否为空，若为空，将不进行转换，类似于忽略字段)
    //下个版本
    
    //17.支持xml序列化
    //下下版本～

    // REGISTEREDMEMBER(maptest, name);
    // maptest mt;
    // mt.name["zhangxu"] = 22;
    // mt.name["yujing"] = 23;
    // string str;
    // Fdog::FJson(str, mt);
    // cout << str << endl;
    // maptest mt2;
    // string str2 = "{\"name\":{\"lisi\":33,\"angwu\":11}}";
    // Fdog::FObject(mt2, str2);
    // for(auto j : mt2.name){
    //     cout << "first = " << j.first << " second = " << j.second << endl;
    // }
    // clock_t start, finish;
    // double totaltime;
    // start = clock();
    // for(int i = 0; i < 10000; i++){
    //     student stu;
    //     stu.name = "yujing";
    //     stu.age = 21;
    //     string stu_json = ""; 
    //     //结构体转json
    //     //Fdog::setAliasName("student", "name", "Zname");
    //     Fdog::FJson(stu_json, stu);  //结果 stu_json = 
    //     //cout << stu_json << endl;
    //     auto res = Fdog::JsonValidS(stu_json);
    //     //cout << "字符串是否正确：" << res.code << endl;
    //     if(Fdog::Exist(stu_json, "age")) {
    //         //cout << "age的值:" << Fdog::GetStringValue(stu_json, "age") << endl; 
    //     }
    //     student stu2;
    //     string stu2_json = "{\"name\":\"zhangxv\",\"age\":21}";
    //     //json转结构体
    //     Fdog::FObject(stu2, stu2_json);  //结果  stu2
    //     //cout << stu2.name << "-" << stu2.age << endl;
    // }
    // finish = clock();
    // totaltime = (double)(finish - start) / CLOCKS_PER_SEC;
    // cout << "clock " << totaltime << "s" << endl;  
    //打印      clock 20.7354s 
    //不打印    clock 5.48499s
    cout << "===============================================================" << endl;
    return 0;
}

