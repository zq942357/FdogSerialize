/*
该项目签署了Apache-2.0 License，详情请参见LICENSE
根据 Apache 许可，版本 2.0（“许可”）获得许可
除非遵守许可，否则您不得使用此文件。

Copyright 2021-2022 花狗Fdog(张旭)
*/

#ifndef FSTRUCT_H
#define FSTRUCT_H

#include "definition.h"
#include <algorithm>
#include <cxxabi.h>
#include <cstring>
#include <ctime>
#include <deque>
#include <mutex>
#include <map>
#include <map>
#include <list>
#include <sstream>
#include <string>
#include <set>
#include <vector>
#include <regex>
using namespace std;


static vector<string> baseType = {
        "bool", "bool*"
        "char", "unsigned char", "unsigned char*",
        "int", "unsigned int", "int*", "unsigned int*",
        "short", "unsigned short", "short*", "unsigned short*",
        "long", "unsigned long int", "long*", "unsigned long*",
        "long long", "unsigned long long", "long long*", "unsigned long long*",
        "float", "double", "long double", "float*", "double*", "long double*",
        "std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >",
        "char*"
};

//有符号类型应该拥有正负号，正号忽视 ^(-|+)? 匹配负号
static map<string, string> baseRegex = {
        {"bool", "(\\d+)"},
        {"float", "(\\d+.\\d+)"}, 
        {"double", "(\\d+.\\d+)"},
        {"long double", "(\\d+.\\d+)"},
        {"char*", "\"(.*?)\""},
        {"std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >", "\"(.*?)\""},
        {"char", "(\\d+)"}, {"unsigned char", "(\\d+)"}, 
        {"int", "(\\d+)"}, {"unsigned int", "(\\d+)"},
        {"short", "(\\d+)"}, {"unsigned short", "(\\d+)"}, 
        {"long", "(\\d+)"}, {"unsigned long", "(\\d+)"},
        {"long long", "(\\d+)"}, {"unsigned long long", "(\\d+)"}, 
};

static map<int, string> complexRegex = {
    {4, "(.*?) (\\[)(\\d+)(\\])"},
    {5, "std::vector<(.*?),"},     //这里存在问题，如果是string，只会截取不完整类型
    {6, "std::map<(.*?), (.*?),"}, //string也存在问题
    {62, "std::map<(.*?), (.*?), (.*?), (.*?),"},
    {7, "std::__cxx11::list<(.*?),"},
    {8, "std::set<(.*?),"},
    {9, "std::deque<(.*?),"},
    {10,"std::pair<(.*?) const, (.*?)>"} //
};


//用于数组类型整体提取
const string arrayRegex = "(\\[(.*?)\\])";
//用于Map类型整体提取
const string mapRegex = "(\\{(.*?)\\})";
//匹配数组
const string patternArray = "((A)(\\d+)_\\d?(\\D+))";
//匹配别名
const string patterAlias = "(\\(A:(.*?)\\))";
//用于结构体整体提取
const string objectRegex = "(\\{(.*?)\\})";
//匹配结构体
const string patternObject = "((\\d+)(\\D+))";

enum ObjectType{
    OBJECT_BASE = 1,
    OBJECT_STRUCT,
    OBJECT_CLASS,
    OBJECT_ARRAY,
    OBJECT_VECTOR,
    OBJECT_MAP,
    OBJECT_LIST,
    OBJECT_SET,
    OBJECT_DEQUE,
};


/***********************************
*   存储结构体元信息
************************************/
typedef struct MetaInfo{
    string memberName;          //成员名
    string memberAliasName;     //成员别名
    string memberType;          //成员类型
    size_t memberOffset;        //偏移值
    size_t memberTypeSize;      //类型大小
    size_t memberArraySize;     //如果类型是数组，表示数组大小
    int    memberTypeInt;       //成员类型 数值型
    string first;               //如果是map类型 first表示key的类型，如果是其他类型，表示value类型
    string second;              //如果是map类型，second表示value类型
    bool   memberIsIgnore = false;    //是否忽略字段
    bool   memberIsIgnoreLU = false;  //是否忽略大小写                                                                                                                                                                               
}MetaInfo;

/***********************************
*   存储对象元信息
************************************/
typedef struct ObjectInfo{
    string objectType;                      //结构体类型 字符串表示
    int objectTypeInt;                      //结构体类型 数值表示
    int objectSize;                         //结构体大小
    vector<MetaInfo *> metaInfoObjectList;  //结构体元信息 
}ObjectInfo;


typedef struct FdogMap{
    string first;
    string second;
};

/***********************************
*   存储成员类型，数组大小
************************************/
struct memberAttribute {
    string valueType;
    int valueTypeInt;   //类型 数值表示
    string first;       //如果是map类型 first表示key的类型，如果是其他类型，表示value类型
    string second;      //如果是map类型，second表示value类型
    int ArraySize;
};

//结构体用于返回信息
struct result {
    int code;           //1.正确 0.错误
    string message;     //如果错误，返回错误提示
};

//声明序列化base类
class FdogSerializerBase {
    private:
    static mutex * mutex_base;
    static FdogSerializerBase * FdogSerializerbase;

    public:
    static FdogSerializerBase * Instance();

    template<class T>
    string removeLastZero(T & return_){
        std::ostringstream oss; 
        oss << return_;
        return oss.str();
    }

    template<class T>
    string getValueByAddress(string valueType, T & object, int offsetValue){
        if(valueType == "char*"){
            auto value = *((const char **)((void *)&object + offsetValue));
            string str_value = value;
            return "\"" + str_value  + "\"";
        }
        if(valueType == "string" || valueType == "std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >"){
            auto value = *((string *)((void *)&object + offsetValue));
            string str_value = value;
            return "\"" + str_value  + "\"";
        }
        if(valueType == "bool"){
            auto value = *((bool *)((char *)&object + offsetValue));
            if(value){
                return "true";
            }else{
                return "false";
            }
        }
        if(valueType == "char"){
            auto value = *((char *)((void *)&object + offsetValue));
            return to_string((int)value);
        }
        if(valueType == "unsigned char"){
            auto value = *((char *)((void *)&object + offsetValue));
            return to_string((unsigned int)value);
        }
        if(valueType == "int"){
            //cout << "get = " << (int *)((char *)&object + offsetValue) << endl;
            auto value = *((int *)((char *)&object + offsetValue));
            return to_string(value);
        }
        if(valueType == "unsigned int"){
            auto value = *((unsigned int *)((char *)&object + offsetValue));
            return to_string(value);
        }
        if(valueType == "short"){
            auto value = *((short int *)((char *)&object + offsetValue));
            return to_string(value);
        }
        if(valueType == "unsigned short"){
            auto value = *((unsigned short int *)((char *)&object + offsetValue));
            return to_string(value);
        }
        if(valueType == "long"){
            auto value = *((long int *)((char *)&object + offsetValue));
            return to_string(value);
        }
        if(valueType == "unsigned long"){
            auto value = *((unsigned long int *)((char *)&object + offsetValue));
            return to_string(value);
        }
        if(valueType == "long long"){
            auto value = *((long long int *)((char *)&object + offsetValue));
            return to_string(value);
        }
        if(valueType == "unsigned long long"){
            auto value = *((unsigned long long int *)((char *)&object + offsetValue));
            return to_string(value);
        }        
        if(valueType == "float"){
            auto value = *((float *)((char *)&object + offsetValue));
            return removeLastZero(value);
        }
        if(valueType == "double"){
            auto value = *((double *)((char *)&object + offsetValue));
            return removeLastZero(value);
        }
        if(valueType == "long double"){
            auto value = *((long double *)((char *)&object + offsetValue));
            return removeLastZero(value);
        }
        return "";
    }

    template<class T>
    void setValueByAddress(string valueType, T &object, int offsetValue, string value){
        if(valueType == "char*"){
            *((char **)((void *)&object + offsetValue)) = new char[strlen(value.c_str())];
            strcpy(*((char **)((void *)&object + offsetValue)), value.c_str());
        }
        if(valueType == "string" || valueType == "std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >"){
            *((string *)((void *)&object + offsetValue)) = value;
        }
        std::stringstream ss;
        ss.str(value);
        if(valueType == "bool"){
            ss >> *((bool *)((void *)&object + offsetValue));
        }
        if(valueType == "char"){
            ss >> *((char *)((void *)&object + offsetValue));
        }
        if(valueType == "unsigned char"){
            ss >> *((unsigned char *)((void *)&object + offsetValue));
        }
        if(valueType == "int"){
            ss >> *((int *)((char *)&object + offsetValue));
        }
        if(valueType == "unsigned int"){
            ss >> *((unsigned int *)((char *)&object + offsetValue));
        }
        if(valueType == "short"){
            ss >> *((short int *)((char *)&object + offsetValue));
        }
        if(valueType == "unsigned short"){
            ss >> *((unsigned short int *)((char *)&object + offsetValue));
        }
        if(valueType == "long"){
            ss >> *((long int *)((char *)&object + offsetValue));
        }
        if(valueType == "unsigned long"){
            ss >> *((unsigned long int *)((char *)&object + offsetValue));
        }
        if(valueType == "long long"){
            ss >> *((long long int *)((char *)&object + offsetValue));
        }
        if(valueType == "unsigned long long"){
            ss >> *((unsigned long long  int *)((char *)&object + offsetValue));
        }  
        if(valueType == "float"){
            ss >> *((float *)((char *)&object + offsetValue));
        }
        if(valueType == "double"){
            ss >> *((double *)((char *)&object + offsetValue));
        }
        if(valueType == "long double"){
            ss >> *((long double *)((char *)&object + offsetValue));
        }
    }

    // 基础类型转json
    template<class T>
    void BaseToJson(string & json_, MetaInfo * metainfoobject, T & object_){
        
        string value = getValueByAddress(metainfoobject->memberType, object_, metainfoobject->memberOffset);
        if(metainfoobject->memberAliasName != ""){
            json_ = json_ + "\"" + metainfoobject->memberAliasName + "\"" + ":" + value + ",";
        }else{
            json_ = json_ + "\"" + metainfoobject->memberName + "\"" + ":" + value + ",";
        }
    }

    // //基础类型转json
    template<class T>
    void BaseToJsonA(string & json_, MetaInfo * metainfoobject, T & object_){
        string value = getValueByAddress(metainfoobject->memberType, object_, metainfoobject->memberOffset);
        json_ = json_ + value + ",";
    }    

    // //json转基础类型
    template<class T>
    void JsonToBase(T & object_, MetaInfo * metainfoobject, string json_){
        setValueByAddress(metainfoobject->memberType, object_, metainfoobject->memberOffset, json_);
    }
};


//BaseTag ArrayTag MapTag 用于处理在Serialize，Deserialize上的分发
//处理 base
struct BaseTag {};
//处理 array vector list deque set
struct ArrayTag {};
//处理 map  
struct MapTag {};

template<typename T> struct TagDispatchTrait {
    using Tag = BaseTag;
};

template<> struct TagDispatchTrait<vector<int>> {
    using Tag = ArrayTag;
};

template<> struct TagDispatchTrait<list<int>> {
    using Tag = ArrayTag;
};

template<> struct TagDispatchTrait<set<int>> {
    using Tag = ArrayTag;
};

template<> struct TagDispatchTrait<deque<int>> {
    using Tag = ArrayTag;
};

template<> struct TagDispatchTrait<map<int,int>> {
    using Tag = MapTag;
};

// 区分非字符串和字符串 处理在FSerialize(map) key的值，可能是数值，可能是非数值
struct NoStringTag{};
struct StringTag{};

template<typename T> struct TagString {
    using Tag = NoStringTag;
};

template<> struct TagString<char *> {
    using Tag = StringTag;
};

template<> struct TagString<string> {
    using Tag = StringTag;
};

template<typename T>
string F_toString_s(T object, NoStringTag){
    return to_string(object);
}
template<typename T>
string F_toString_s(T object, StringTag){
    return object;
}
template<typename T>
string F_toString(T object){
    return F_toString_s(object, typename TagString<T>::Tag{});
}

// 用于反序列化时对象为空的情况，需要根据数据进行扩展长度 Init  分为数值和非数值类型
struct InitBaseTag{};

struct InitVectorTag{};
struct InitVectorStrTag{};

struct InitDequeTag{};
struct InitDequeStrTag{};

struct InitListTag{};
struct InitListStrTag{};

struct InitMapTag{};
struct InitMapStrTag{};

struct InitSetTag{};
struct InitSetStrTag{};


template<typename T> struct TagSTLType {
    using Tag = InitBaseTag;
};

template<> struct TagSTLType<vector<int>> {
    using Tag = InitVectorTag;
};

template<> struct TagSTLType<vector<char *>> {
    using Tag = InitVectorStrTag;
};

template<> struct TagSTLType<vector<string>> {
    using Tag = InitVectorStrTag;
};

template<> struct TagSTLType<list<int>> {
    using Tag = InitListTag;
};

template<> struct TagSTLType<list<char *>> {
    using Tag = InitListStrTag;
};

template<> struct TagSTLType<list<string>> {
    using Tag = InitListStrTag;
};

template<> struct TagSTLType<set<int>> {
    using Tag = InitSetTag;
};

template<> struct TagSTLType<set<char *>> {
    using Tag = InitSetStrTag;
};

template<> struct TagSTLType<set<string>> {
    using Tag = InitSetStrTag;
};

template<> struct TagSTLType<deque<int>> {
    using Tag = InitDequeTag;
};

template<> struct TagSTLType<deque<char *>> {
    using Tag = InitDequeStrTag;
};

template<> struct TagSTLType<deque<string>> {
    using Tag = InitDequeStrTag;
};

template<> struct TagSTLType<map<int,int>> {
    using Tag = InitMapTag;
};

template<> struct TagSTLType<map<string,int>> {
    using Tag = InitMapStrTag;
};

template<typename T>
void F_init_s(T & object, InitBaseTag, string first, string second = "", string key = ""){

}

template<typename T>
void F_init_s(T & object, InitVectorTag, string first, string second = "", string key = ""){
    if (first == "int"){
        object.push_back(0);
    }
    if (first == "char"){
        object.push_back('0');
    }else if (first == "unsigned char"){
        object.push_back(0);
    }else if (first == "short"){
        object.push_back(0);
    }else if (first == "unsigned short"){
        object.push_back(0);
    }else if (first == "int"){
        object.push_back(0);
    }else if (first == "unsigned int"){
        object.push_back(0);
    }else if (first == "long"){
        object.push_back(0);
    }else if (first == "unsigned long"){
        object.push_back(0);
    }else if (first == "long long"){
        object.push_back(0);
    }else if (first == "unsigned long long"){
        object.push_back(0);
    }else if (first == "float"){
        object.push_back(0.1f);
    }else if (first == "double"){
        object.push_back(0.1);
    }else if (first == "long double"){
        object.push_back(0.1);
    }else{

    }
}

template<typename T>
void F_init_s(T & object, InitVectorStrTag, string first, string second = "", string key = ""){
    if (first == "char*"){
        object.push_back("");
    }else if (first == "string" || first == "std::__cxx11::basic_string<char"){
        object.push_back("");
    }else{

    }
}

template<typename T>
void F_init_s(T & object, InitDequeTag, string first, string second = "", string key = ""){
    if (first == "char"){
        object.push_back('0');
    }else if (first == "unsigned char"){
        object.push_back(0);
    }else if (first == "short"){
        object.push_back(0);
    }else if (first == "unsigned short"){
        object.push_back(0);
    }else if (first == "int"){
        object.push_back(0);
    }else if (first == "unsigned int"){
        object.push_back(0);
    }else if (first == "long"){
        object.push_back(0);
    }else if (first == "unsigned long"){
        object.push_back(0);
    }else if (first == "long long"){
        object.push_back(0);
    }else if (first == "unsigned long long"){
        object.push_back(0);
    }else if (first == "float"){
        object.push_back(0.1f);
    }else if (first == "double"){
        object.push_back(0.1);
    }else if (first == "long double"){
        object.push_back(0.1);
    }else {

    }
}

template<typename T>
void F_init_s(T & object, InitDequeStrTag, string first, string second = "", string key = ""){
    if (first == "char*"){
        object.push_back("");
    }else if (first == "string" || first == "std::__cxx11::basic_string<char"){
        object.push_back("");
    }else{

    }
}

template<typename T>
void F_init_s(T & object, InitListTag, string first, string second = "", string key = ""){
    if (first == "int"){
        object.push_back(0);
    }
    if (first == "char"){
        object.push_back('0');
    }else if (first == "unsigned char"){
        object.push_back(0);
    }else if (first == "short"){
        object.push_back(0);
    }else if (first == "unsigned short"){
        object.push_back(0);
    }else if (first == "int"){
        object.push_back(0);
    }else if (first == "unsigned int"){
        object.push_back(0);
    }else if (first == "long"){
        object.push_back(0);
    }else if (first == "unsigned long"){
        object.push_back(0);
    }else if (first == "long long"){
        object.push_back(0);
    }else if (first == "unsigned long long"){
        object.push_back(0);
    }else if (first == "float"){
        object.push_back(0.1f);
    }else if (first == "double"){
        object.push_back(0.1);
    }else if (first == "long double"){
        object.push_back(0.1);
    }else {

    }
}

template<typename T>
void F_init_s(T & object, InitListStrTag, string first, string second = "", string key = ""){
    if (first == "char*"){
        object.push_back("");
    }else if (first == "string" || first == "std::__cxx11::basic_string<char"){
        object.push_back("");
    }else{

    }
}

template<typename T>
void F_init_s(T & object, InitSetTag, string first, string second = "", string key = ""){
        //set是不可以重复的，可以拿随机数
    int a = rand()%100;
    if (first == "int"){
        object.insert(a);
    }
    if (first == "char"){
        stringstream sstr;
        sstr << a;
        object.insert(sstr.str()[0]);
    }else if (first == "unsigned char"){
        object.insert(a);
    }else if (first == "short"){
        object.insert(a);
    }else if (first == "unsigned short"){
        object.insert(a);
    }else if (first == "int"){
        object.insert(a);
    }else if (first == "unsigned int"){
        object.insert(a);
    }else if (first == "long"){
        object.insert(a);
    }else if (first == "unsigned long"){
        object.insert(a);
    }else if (first == "long long"){
        object.insert(a);
    }else if (first == "unsigned long long"){
        object.insert(a);
    }else if (first == "float"){
        object.insert(static_cast<float>(a));
    }else if (first == "double"){
        object.insert(static_cast<double>(a));
    }else if (first == "long double"){
        object.insert(static_cast<long double>(a));
    }else {

    }
}

//如果是char * 类型，每次delete之后 地址都是相同的，所以不能在内部使用完归零，要在外面整体赋完值delete
static vector<char *> temp;

template<typename T>
void F_init_s(T & object, InitSetStrTag, string first, string second = "", string key = ""){
    int a = rand()%100;
    //cout << "a = " << a << endl;
    if (first == "char*"){
        stringstream sstr;
        sstr << a;
        char * cc = new char[4];
        //cout << "cc = " << (void *)cc << "---"<< *cc << "---" << &cc << " sstr 地址：" << &sstr << endl;
        strcpy(cc, (char *)sstr.str().data());
        //cout << "cc = " << (void *)cc << "---"<< *cc << "---" << &cc << " sstr 地址：" << &sstr << endl;
        object.insert(cc);
        temp.push_back(cc);
    }else if (first == "string" || first == "std::__cxx11::basic_string<char"){
        stringstream sstr;
        sstr << a;
        object.insert((char *)sstr.str().c_str());
    }else{

    }
    //cout << "长度：" << object.size() << endl;
}

template<typename T>
void F_init_s(T & object, InitMapTag, string first, string second = "", string key = ""){
    int a = atoi(key.c_str());
    if(first == "int" && second == "int"){
        object.insert(make_pair(a, a));
    }
}

template<typename T>
void F_init_s(T & object, InitMapStrTag, string first, string second = "", string key = ""){
    int a = rand()%100;
    if(first == "std::__cxx11::basic_string<char" && second == "int"){
        stringstream sstr;
        sstr << a;
        //string value = key;
        object.insert(make_pair(key, a));
    }
}

template<typename T>
void F_init(T & object, int stlType, string first, string second = "", string key = ""){

    if(stlType == OBJECT_VECTOR){
        F_init_s(object, typename TagSTLType<T>::Tag{}, first);
    }
    if(stlType == OBJECT_LIST){
        F_init_s(object, typename TagSTLType<T>::Tag{}, first); 
    }
    if(stlType == OBJECT_DEQUE){
        F_init_s(object, typename TagSTLType<T>::Tag{}, first);   
    }
    if(stlType == OBJECT_SET){
        F_init_s(object, typename TagSTLType<T>::Tag{}, first);      
    }
    if(stlType == OBJECT_MAP){
        F_init_s(object, typename TagSTLType<T>::Tag{}, first, second, key);    
    }
}

//用于区分(基础类型结构体/数组)
struct BaseAndStructTag{};
struct BaseArrayTag{};

template<typename T> struct TagSTLAAAType {
    using Tag = BaseAndStructTag;
};

// template<> struct TagSTLAAAType<student[2]> {
//     using Tag = BaseArrayTag;
// };

template<> struct TagSTLAAAType<int[2]> {
    using Tag = BaseArrayTag;
};

template<> struct TagSTLAAAType<list<int>> {
    using Tag = BaseAndStructTag;
};

class FdogSerializer {

    private:
    static mutex * mutex_serialize;
    static FdogSerializer * fdogSerializer;
    vector<ObjectInfo *> objectInfoList;
    vector<MetaInfo *> baseInfoList;

    FdogSerializer();
    ~FdogSerializer();

    public:
    //获取实例
    static FdogSerializer * Instance();

    //添加objectinfo
    void addObjectInfo(ObjectInfo * objectinfo);

    //获取对应Info
    ObjectInfo & getObjectInfo(string objectName);

    //获取对于Info 针对基础类型获取
    MetaInfo * getMetaInfo(string TypeName);

    //设置别名
    void __setAliasName(string Type, string memberName, string AliasName);

    //设置是否忽略该字段序列化
    void __setIgnoreField(string Type, string memberName);

	//设置是否忽略大小写
    void __setIgnoreLU(string Type, string memberName);

	//设置进行模糊转换 结构体转json不存在这个问题主要是针对json转结构体的问题，如果存在分歧，可以尝试进行模糊转换
	void __setFuzzy(string Type);


    //一次性设置多个别名
    template<class T, class ...Args>
    void setAliasNameAll();

    //一次性设置忽略多个字段序列化
    template<class T, class ...Args>
    void setIgnoreFieldAll();

    void removeFirstComma(string & return_);

    void removeLastComma(string & return_);

    void removeNumbers(string & return_);

    template<class T, class ...Args>
    void setIgnoreLUAll();

    //获取key值
    string getKey(string json);

    //获取成员属性
    memberAttribute getMemberAttribute(string key);

    //获取object类型
    int getObjectTypeInt(string objectName, string typeName);

    //获取基础类型 只有base和struct两种
    ObjectInfo getObjectInfoByType(string typeName, int objectTypeInt);

    //通过宏定义加载的信息获取
    int getObjectTypeByObjectInfo(string objectName);

    //判断是否是基础类型
    bool isBaseType(string typeName);

    //在map的基础上判断是否是基础类型
    bool isBaseTypeByMap(string typeName);

    //判断是否为vector类型
    bool isVectorType(string objectName, string typeName);
    
    //获取vector中的类型
    string getTypeOfVector(string objectName, string typeName);

    //判断是否为map类型
    bool isMapType(string objectName, string typeName);

    //获取map中的key，value类型
    FdogMap getTypeOfMap(string objectName, string typeName);

    //判断是否是list类型
    bool isListType(string objectName, string typeName);

    //获取list中的类型
    string getTypeOfList(string objectName, string typeName);

    //判断是否是deque类型
    bool isDequeType(string objectName, string typeName);

    //判断是否是set类型
    bool isSetType(string objectName, string typeName);

    //判断是否是结构体类型
    bool isStructType(string objectName, string typeName);

    //判断是否是数组
    bool isArrayType(string objectName, string typeName);

    //解析数组
    vector<string> CuttingArray(string data);

    //切割
    vector<string> split(string str, string pattern);

    //判断json格式是否正确
    vector<string> CuttingJson(string json_);

    //判断方括号是否匹配
    bool IsSquareBracket(string json_);

    //判断花括号是否匹配
    bool IsCurlyBraces(string json_);

    //判断总符号数是否匹配
    bool isMatch(string json_);
    
    //判断json正确性
    result __JsonValidS(string json_);
    
    //判断字段是否存在
    bool __Exist(string json_, string key);
    
    //获取字段的值
    string __GetStringValue(string json_, string key);

    //获取字段的值
    int __GetIntValue(string json_, string key);

    //获取字段的值
    double __GetDoubleValue(string json_, string key);

    //获取字段的值
    long __GetLongValue(string json_, string key);

    //获取字段的值
    bool __GetBoolValue(string json_, string key);

    //序列化
    template<typename T>
    void Serialize(string & json_, T & object_, string name = ""){
        //通过传进来的T判断是什么复合类型，ObjectInfo只保存结构体,如果是NULL可以确定传进来的不是struct类型
        ObjectInfo objectinfo = FdogSerializer::Instance()->getObjectInfo(abi::__cxa_demangle(typeid(T).name(),0,0,0));
        //获取的只能是结构体的信息，无法知道是什么复合类型，尝试解析类型 objectType其实是一个结构体类型名称
        int objectType = getObjectTypeInt(objectinfo.objectType, abi::__cxa_demangle(typeid(T).name(),0,0,0));
        if(objectinfo.objectType == "NULL" && objectType != OBJECT_BASE && objectType != OBJECT_STRUCT){
            //说明不是struct类型和base类型尝试，尝试解析类型
            objectinfo = getObjectInfoByType(abi::__cxa_demangle(typeid(T).name(),0,0,0), objectType);
            objectType = getObjectTypeInt(objectinfo.objectType, abi::__cxa_demangle(typeid(T).name(),0,0,0));
            //这里这个objectinfo应该还是空 所以拿objecttype的数值判断
        }
        int sum = objectinfo.metaInfoObjectList.size();
        //cout << "sum：" << sum << endl;
        int i = 1;
        //获取到的objectType才是真正的类型，根据这个类型进行操作
        //cout << "objectType type = " << objectType  << " json_ : " << json_ << endl;
        switch(objectType){
            //第一次调用进来表示其本身类型 只有两种 结构体或着基础类型
            case OBJECT_BASE:
            {
                MetaInfo * metainfo1 = nullptr;
                metainfo1 = getMetaInfo(abi::__cxa_demangle(typeid(object_).name(),0,0,0));
                if (metainfo1 != nullptr){
                    //cout << "==================" << endl;
                    FdogSerializerBase::Instance()->BaseToJsonA(json_, metainfo1, object_);
                } else {
                    //cout << "获取MetaInfo失败" << endl;
                }
            }
            break;
            case OBJECT_STRUCT:
            {
                for(auto metainfoObject : objectinfo.metaInfoObjectList){
                    string json_s;
                    //cout <<"成员类型：" << metainfoObject->memberType << "--" << metainfoObject->memberTypeInt << "--" << metainfoObject->first << "--" << metainfoObject->memberOffset << endl;
                    if(metainfoObject->memberTypeInt == OBJECT_BASE && metainfoObject->memberIsIgnore != true){
                        FdogSerializerBase::Instance()->BaseToJson(json_s, metainfoObject, object_);
                        json_ = json_ + json_s;
                    }
                    if(metainfoObject->memberTypeInt == OBJECT_ARRAY && metainfoObject->memberIsIgnore != true){
                        if(metainfoObject->first == "bool"){
                            for(int i = 0; i < metainfoObject->memberArraySize; i++){
                                string value = FdogSerializerBase::Instance()->getValueByAddress(metainfoObject->first, object_, metainfoObject->memberOffset+ (i * sizeof(bool)));
                                json_s = json_s + value + ",";
                            }
                        }                        
                        if(metainfoObject->first == "char"){
                            for(int i = 0; i < metainfoObject->memberArraySize; i++){
                                string value = FdogSerializerBase::Instance()->getValueByAddress(metainfoObject->first, object_, metainfoObject->memberOffset+ (i * sizeof(char)));
                                json_s = json_s + value + ",";
                            }
                        }
                        if(metainfoObject->first == "unsigned char"){
                            for(int i = 0; i < metainfoObject->memberArraySize; i++){
                                string value = FdogSerializerBase::Instance()->getValueByAddress(metainfoObject->first, object_, metainfoObject->memberOffset+ (i * sizeof(unsigned char)));
                                json_s = json_s + value + ",";
                            }
                        }
                        if(metainfoObject->first == "char*"){
                            for(int i = 0; i < metainfoObject->memberArraySize; i++){
                                string value = FdogSerializerBase::Instance()->getValueByAddress(metainfoObject->first, object_, metainfoObject->memberOffset+ (i * sizeof(char*)));
                                json_s = json_s + value + ",";
                            }
                        }
                        if(metainfoObject->first == "string" || metainfoObject->first == "std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >"){
                            for(int i = 0; i < metainfoObject->memberArraySize; i++){
                                string value = FdogSerializerBase::Instance()->getValueByAddress(metainfoObject->first, object_, metainfoObject->memberOffset+ (i * sizeof(string)));
                                json_s = json_s + value + ",";
                            }
                        }
                        if(metainfoObject->first == "short"){
                            for(int i = 0; i < metainfoObject->memberArraySize; i++){
                                string value = FdogSerializerBase::Instance()->getValueByAddress(metainfoObject->first, object_, metainfoObject->memberOffset+ (i * sizeof(short)));
                                json_s = json_s + value + ",";
                            }
                        }
                        if(metainfoObject->first == "unsigned short"){
                            for(int i = 0; i < metainfoObject->memberArraySize; i++){
                                string value = FdogSerializerBase::Instance()->getValueByAddress(metainfoObject->first, object_, metainfoObject->memberOffset+ (i * sizeof(unsigned short)));
                                json_s = json_s + value + ",";
                            }
                        }
                        if(metainfoObject->first == "int"){
                            for(int i = 0; i < metainfoObject->memberArraySize; i++){
                                string value = FdogSerializerBase::Instance()->getValueByAddress(metainfoObject->first, object_, metainfoObject->memberOffset+ (i * sizeof(int)));
                                json_s = json_s + value + ",";
                            }
                        }
                        if(metainfoObject->first == "unsigned int"){
                            for(int i = 0; i < metainfoObject->memberArraySize; i++){
                                string value = FdogSerializerBase::Instance()->getValueByAddress(metainfoObject->first, object_, metainfoObject->memberOffset+ (i * sizeof(unsigned int)));
                                json_s = json_s + value + ",";
                            }
                        }
                        if(metainfoObject->first == "long"){
                            for(int i = 0; i < metainfoObject->memberArraySize; i++){
                                string value = FdogSerializerBase::Instance()->getValueByAddress(metainfoObject->first, object_, metainfoObject->memberOffset+ (i * sizeof(long)));
                                json_s = json_s + value + ",";
                            }
                        }
                        if(metainfoObject->first == "unsigned long"){
                            for(int i = 0; i < metainfoObject->memberArraySize; i++){
                                string value = FdogSerializerBase::Instance()->getValueByAddress(metainfoObject->first, object_, metainfoObject->memberOffset+ (i * sizeof(unsigned long)));
                                json_s = json_s + value + ",";
                            }
                        }
                        if(metainfoObject->first == "long long"){
                            for(int i = 0; i < metainfoObject->memberArraySize; i++){
                                string value = FdogSerializerBase::Instance()->getValueByAddress(metainfoObject->first, object_, metainfoObject->memberOffset+ (i * sizeof(long long)));
                                json_s = json_s + value + ",";
                            }
                        }
                        if(metainfoObject->first == "unsigned long long"){
                            for(int i = 0; i < metainfoObject->memberArraySize; i++){
                                string value = FdogSerializerBase::Instance()->getValueByAddress(metainfoObject->first, object_, metainfoObject->memberOffset+ (i * sizeof(unsigned long long)));
                                json_s = json_s + value + ",";
                            }
                        }
                        if(metainfoObject->first == "float"){
                            for(int i = 0; i < metainfoObject->memberArraySize; i++){
                                string value = FdogSerializerBase::Instance()->getValueByAddress(metainfoObject->first, object_, metainfoObject->memberOffset+ (i * sizeof(float)));
                                json_s = json_s + value + ",";
                            }
                        }
                        if(metainfoObject->first == "double"){
                            for(int i = 0; i < metainfoObject->memberArraySize; i++){
                                string value = FdogSerializerBase::Instance()->getValueByAddress(metainfoObject->first, object_, metainfoObject->memberOffset+ (i * sizeof(double)));
                                json_s = json_s + value + ",";
                            }
                        }
                        if(metainfoObject->first == "long double"){
                            for(int i = 0; i < metainfoObject->memberArraySize; i++){
                                string value = FdogSerializerBase::Instance()->getValueByAddress(metainfoObject->first, object_, metainfoObject->memberOffset + (i * sizeof(long double)));
                                json_s = json_s + value + ",";
                            }
                        }
                        //添加容器自定义参数宏
                        Serialize_arraytype_judgment_all;

                        removeLastComma(json_s);
                        json_ = json_ + "\"" + metainfoObject->memberName + "\"" + ":" + "[" + json_s + "]" + ",";                
                    }
                    if(metainfoObject->memberTypeInt == OBJECT_VECTOR && metainfoObject->memberIsIgnore != true){
                        //cout << "====获取的值：" << metainfoObject->first << endl;
                        if(metainfoObject->first == "char"){
                            FSerialize(json_s, *(vector<char> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<vector<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned char"){
                            FSerialize(json_s, *(vector<unsigned char> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<vector<int>>::Tag{});
                        }
                        if(metainfoObject->first == "char*"){
                            //cout << "zhaodaoleix1" << endl;
                            FSerialize(json_s, *(vector<char *> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<vector<int>>::Tag{});
                        }
                        if(metainfoObject->first == "string" || metainfoObject->first == "std::__cxx11::basic_string<char"){
                            //cout << "zhaodaoleix1" << endl;
                            FSerialize(json_s, *(vector<string> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<vector<int>>::Tag{});
                        }
                        if(metainfoObject->first == "short"){
                            FSerialize(json_s, *(vector<short> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<vector<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned short"){
                            FSerialize(json_s, *(vector<unsigned short> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<vector<int>>::Tag{});
                        }
                        if(metainfoObject->first == "int"){
                            FSerialize(json_s, *(vector<int> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<vector<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned int"){
                            FSerialize(json_s, *(vector<unsigned int> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<vector<int>>::Tag{});
                        }
                        if(metainfoObject->first == "long"){
                            FSerialize(json_s, *(vector<long> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<vector<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned long"){
                            FSerialize(json_s, *(vector<unsigned long> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<vector<int>>::Tag{});
                        }
                        if(metainfoObject->first == "long long"){
                            FSerialize(json_s, *(vector<long long> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<vector<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned long long"){
                            FSerialize(json_s, *(vector<unsigned long long> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<vector<int>>::Tag{});
                        }
                        if(metainfoObject->first == "float"){
                            FSerialize(json_s, *(vector<float> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<vector<int>>::Tag{});
                        }
                        if(metainfoObject->first == "double"){
                            FSerialize(json_s, *(vector<double> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<vector<int>>::Tag{});
                        }
                        if(metainfoObject->first == "long double"){
                            FSerialize(json_s, *(vector<long double> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<vector<int>>::Tag{});
                        }
                        Serialize_vector_type_judgment_all;
                        // if(metainfoObject->first == "student"){
                        //     for(int i = 0; i < 2; i++){
                        //         cout << "aaaaa" << endl;
                        //         string json_z = "";
                        //         FSerialize(json_z, *( *)((void *)&object_ + metainfoObject->memberOffset + (i * sizeof(TYPE))), TagDispatchTrait<TYPE>::Tag{});
                        //         cout << "aaaaa2" << endl;
                        //         json_s = json_s + "{" + json_z + "}" + ",";
                        //     }
                        // }                                                                                                                                                                                                                                                                                     
                        json_ = json_ + "\"" + metainfoObject->memberName + "\"" + ":" + "[" + json_s + "]" + ",";
                    }
                    if(metainfoObject->memberTypeInt == OBJECT_LIST && metainfoObject->memberIsIgnore != true){
                        //cout << "====获取的值：" << metainfoObject->first << endl;
                        if(metainfoObject->first == "bool"){
                            FSerialize(json_s, *(list<bool> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<list<bool>>::Tag{});
                        } 
                        if(metainfoObject->first == "char"){
                            FSerialize(json_s, *(list<char> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<list<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned char"){
                            FSerialize(json_s, *(list<unsigned char> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<list<int>>::Tag{});
                        }
                        if(metainfoObject->first == "char*"){
                            FSerialize(json_s, *(list<char *> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<list<int>>::Tag{});
                        }
                        if(metainfoObject->first == "string" || metainfoObject->first == "std::__cxx11::basic_string<char"){
                            FSerialize(json_s, *(list<string> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<list<int>>::Tag{});
                        }
                        if(metainfoObject->first == "short"){
                            FSerialize(json_s, *(list<short> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<list<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned short"){
                            FSerialize(json_s, *(list<unsigned short> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<list<int>>::Tag{});
                        }
                        if(metainfoObject->first == "int"){                    
                            FSerialize(json_s, *(list<int> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<list<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned int"){
                            FSerialize(json_s, *(list<unsigned int> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<list<int>>::Tag{});
                        }
                        if(metainfoObject->first == "long"){
                            FSerialize(json_s, *(list<long> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<list<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned long"){
                            FSerialize(json_s, *(list<unsigned long> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<list<int>>::Tag{});
                        }
                        if(metainfoObject->first == "long long"){
                            FSerialize(json_s, *(list<long long> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<list<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned long long"){
                            FSerialize(json_s, *(list<unsigned long long> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<list<int>>::Tag{});
                        }
                        if(metainfoObject->first == "float"){
                            FSerialize(json_s, *(list<float> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<list<int>>::Tag{});
                        }
                        if(metainfoObject->first == "double"){
                            FSerialize(json_s, *(list<double> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<list<int>>::Tag{});
                        }
                        if(metainfoObject->first == "long double"){
                            FSerialize(json_s, *(list<long double> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<list<int>>::Tag{});
                        }    
                        json_ = json_ + "\"" + metainfoObject->memberName + "\"" + ":" + "[" + json_s + "]" + ",";
                    }
                    if(metainfoObject->memberTypeInt == OBJECT_DEQUE && metainfoObject->memberIsIgnore != true){
                        if(metainfoObject->first == "bool"){
                            FSerialize(json_s, *(deque<bool> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<deque<bool>>::Tag{});
                        } 
                        if(metainfoObject->first == "char"){
                            FSerialize(json_s, *(deque<char> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<deque<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned char"){
                            FSerialize(json_s, *(deque<unsigned char> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<deque<int>>::Tag{});
                        }
                        if(metainfoObject->first == "char*"){
                            FSerialize(json_s, *(deque<char *> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<deque<int>>::Tag{});
                        }
                        if(metainfoObject->first == "string" || metainfoObject->first == "std::__cxx11::basic_string<char"){
                            FSerialize(json_s, *(deque<string> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<deque<int>>::Tag{});
                        }
                        if(metainfoObject->first == "short"){
                            FSerialize(json_s, *(deque<short> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<deque<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned short"){
                            FSerialize(json_s, *(deque<unsigned short> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<deque<int>>::Tag{});
                        }
                        if(metainfoObject->first == "int"){                        
                            FSerialize(json_s, *(deque<int> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<deque<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned int"){
                            FSerialize(json_s, *(deque<unsigned int> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<deque<int>>::Tag{});
                        }
                        if(metainfoObject->first == "long"){
                            FSerialize(json_s, *(deque<long> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<deque<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned long"){
                            FSerialize(json_s, *(deque<unsigned long> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<deque<int>>::Tag{});
                        }
                        if(metainfoObject->first == "long long"){
                            FSerialize(json_s, *(deque<long long> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<deque<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned long long"){
                            FSerialize(json_s, *(deque<unsigned long long> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<deque<int>>::Tag{});
                        }
                        if(metainfoObject->first == "float"){
                            FSerialize(json_s, *(deque<float> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<deque<int>>::Tag{});
                        }
                        if(metainfoObject->first == "double"){
                            FSerialize(json_s, *(deque<double> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<deque<int>>::Tag{});
                        }
                        if(metainfoObject->first == "long double"){
                            FSerialize(json_s, *(deque<long double> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<deque<int>>::Tag{});
                        }    
                        json_ = json_ + "\"" + metainfoObject->memberName + "\"" + ":" + "[" + json_s + "]" + ",";
                    }
                    if(metainfoObject->memberTypeInt == OBJECT_SET && metainfoObject->memberIsIgnore != true){
                        if(metainfoObject->first == "bool"){
                            FSerialize(json_s, *(set<bool> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<set<bool>>::Tag{});
                        } 
                        if(metainfoObject->first == "char"){
                            FSerialize(json_s, *(set<char> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<set<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned char"){
                            FSerialize(json_s, *(set<unsigned char> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<set<int>>::Tag{});
                        }
                        if(metainfoObject->first == "char*"){
                            FSerialize(json_s, *(set<char *> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<set<int>>::Tag{});
                        }
                        if(metainfoObject->first == "string" || metainfoObject->first == "std::__cxx11::basic_string<char"){
                            FSerialize(json_s, *(set<string> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<set<int>>::Tag{});
                        }
                        if(metainfoObject->first == "short"){
                            FSerialize(json_s, *(set<short> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<set<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned short"){
                            FSerialize(json_s, *(set<unsigned short> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<set<int>>::Tag{});
                        }
                        if(metainfoObject->first == "int"){                        
                            FSerialize(json_s, *(set<int> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<set<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned int"){
                            FSerialize(json_s, *(set<unsigned int> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<set<int>>::Tag{});
                        }
                        if(metainfoObject->first == "long"){
                            FSerialize(json_s, *(set<long> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<set<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned long"){
                            FSerialize(json_s, *(set<unsigned long> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<set<int>>::Tag{});
                        }
                        if(metainfoObject->first == "long long"){
                            FSerialize(json_s, *(set<long long> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<set<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned long long"){
                            FSerialize(json_s, *(set<unsigned long long> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<set<int>>::Tag{});
                        }
                        if(metainfoObject->first == "float"){
                            FSerialize(json_s, *(set<float> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<set<int>>::Tag{});
                        }
                        if(metainfoObject->first == "double"){
                            FSerialize(json_s, *(set<double> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<set<int>>::Tag{});
                        }
                        if(metainfoObject->first == "long double"){
                            FSerialize(json_s, *(set<long double> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<set<int>>::Tag{});
                        }    
                        json_ = json_ + "\"" + metainfoObject->memberName + "\"" + ":" + "[" + json_s + "]" + ",";                       
                    }
                    if(metainfoObject->memberTypeInt == OBJECT_MAP && metainfoObject->memberIsIgnore != true){
                        if(metainfoObject->first == "char*" && metainfoObject->second == "int"){
                            FSerialize(json_s, *(map<char *, int> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<map<int,int>>::Tag{});
                        }
                        if(metainfoObject->first == "std::__cxx11::basic_string<char" && metainfoObject->second == "int"){
                            FSerialize(json_s, *(map<string, int> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<map<int,int>>::Tag{});
                        }
                        if(metainfoObject->first == "std::__cxx11::basic_string<char" && metainfoObject->second == "float"){
                            FSerialize(json_s, *(map<string, float> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<map<int,int>>::Tag{});
                        }
                        if(metainfoObject->first == "std::__cxx11::basic_string<char" && metainfoObject->second == "bool"){
                            FSerialize(json_s, *(map<string, int> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<map<int,int>>::Tag{});
                        }                                                                                                                                                 
                        if(metainfoObject->first == "int" && metainfoObject->second == "int"){
                            FSerialize(json_s, *(map<int, int> *)((void *)&object_ + metainfoObject->memberOffset), TagDispatchTrait<map<int,int>>::Tag{});
                        }
                        json_ = json_ + "\"" + metainfoObject->memberName + "\"" + ":" + "{" + json_s + "}" + ",";
                    }

                    //还需要添加容器自定义参数宏
                    //这个宏用于进入OBJECT_STRUCT
                    Serialize_type_judgment_all;
                    if(i == sum){
                        if(json_.length() > 0){
                            removeLastComma(json_);
                        }
                    }
                    json_s = "";
                    i++;
                }
            }
            break;
            //普通map需要在这里定义
            //处理直接是STL类型
        }
    }

    template<typename T>
    void SerializeS(string & json_, T & object_, BaseAndStructTag, string name = ""){
        //cout << "SerializeS1" << endl;
        Serialize(json_, object_, name);
    }

    template<typename T>
    void SerializeS(string & json_, T & object_, BaseArrayTag, string name = ""){
        //cout << "SerializeS2" << endl;
        for(auto & object_one : object_){
            Serialize(json_, object_one);
        }
    }

    template<typename T>
    void SerializeS_s(string & json_, T & object_, bool isArray, string name = ""){
        // if(isArray){
        //     SerializeS(json_, object_, TagSTLAAAType<int[2]>::Tag{}, name);
        // }else{
        // SerializeS(json_, object_, TagSTLAAAType<list<int>>::Tag{}, name);
        // }
        SerializeS(json_, object_, typename TagSTLAAAType<T>::Tag{}, name);
    }


    //用于解析基础类型，数组(只需要判断有没有[]就能确定是不是数组，结构体和基础类型都不具备[]条件)，结构体
    template<typename T>
    void FSerialize(string & json_, T & object_, BaseTag, string name = ""){
        //cout << "类型：" << abi::__cxa_demangle(typeid(T).name(),0,0,0) << endl;
        bool isArray = isArrayType("", abi::__cxa_demangle(typeid(T).name(),0,0,0));
        //cout << "是否是数组 ： " << isArray << endl;
        SerializeS_s(json_, object_, isArray, name);
        //Serialize(json_, object_, name);
        //这里需要判断类型 如果是基础类型直接使用name 不是基础类型，可以使用
        if(isBaseType(abi::__cxa_demangle(typeid(T).name(),0,0,0))) {
            removeLastComma(json_);
            json_ = "{\"" + name + "\":" + json_ + "}";
            return ;
        }
        if(isArrayType("", abi::__cxa_demangle(typeid(T).name(),0,0,0))){
            removeLastComma(json_);
            json_ = "{\"" + name + "\":" + "[" + json_ + "]" + "}";
            return ;    
        }
        json_ = "{" + json_ + "}";
    }
    //用于解析STL（map除外）其实上面接口也可以处理vector，但其他类型无法处理，所以这个处理STL
    template<typename T>
    void FSerialize(string & json_, T & object_, ArrayTag, string name = ""){
        //cout << "进入array==========1" << typeid(T).name() << endl;
        for(auto & object_one : object_){
            //ji
            int objectType = isBaseType(abi::__cxa_demangle(typeid(object_one).name(),0,0,0));
            if (!objectType){
                json_ = json_ + "{";
            }
            //cout << "1=====" << endl;
            Serialize(json_, object_one);
            //cout << "进入array==========2" << typeid(object_one).name() << endl;
            if (!objectType){
                json_ = json_ + "},";
            }
        }
        if(json_.length() > 0){
            removeLastComma(json_);
        }
        //json_ = "{\"" + name + "\":[" + json_ + "]}";
        //如果转换对象直接就是数组，可以再额外提供一个，或者说其他
    }
    //用于解析map
    template<typename T>
    void FSerialize(string & json_, T & object_, MapTag, string name = ""){
        int i = 0;
        int len = object_.size();
        for(auto & object_one : object_){
            //看情况，如果是结构体，需要花括号，基本类型不需要
            json_ = json_ + "\"" + F_toString(object_one.first) + "\"" + ":";
            Serialize(json_, object_one.second);
            removeLastComma(json_);
            json_ = json_ + ",";
            i++;
        }
        removeLastComma(json_);
        //json_ = "{" + json_ + "}";
    }

    //反序列化
    template<typename T>
    void Deserialize(T & object_, string & json_, string name = ""){
        ObjectInfo & objectinfo = FdogSerializer::Instance()->getObjectInfo(abi::__cxa_demangle(typeid(T).name(),0,0,0));
        int objectType = getObjectTypeInt(objectinfo.objectType, abi::__cxa_demangle(typeid(T).name(),0,0,0));
        if(objectinfo.objectType == "NULL" && objectType != OBJECT_BASE && objectType != OBJECT_STRUCT){
            //说明不是struct类型和base类型尝试，尝试解析类型
            objectinfo = getObjectInfoByType(abi::__cxa_demangle(typeid(T).name(),0,0,0), objectType);
            objectType = getObjectTypeInt(objectinfo.objectType, abi::__cxa_demangle(typeid(T).name(),0,0,0));
        }
        if (OBJECT_BASE == objectType) {
            MetaInfo * metainfo1 = getMetaInfo(abi::__cxa_demangle(typeid(object_).name(),0,0,0));
            smatch result;
            string regex_key;
            string regex_value = baseRegex[metainfo1->memberType];
            regex * pattern;
            if (name == "") {
                pattern = new regex(regex_value);
            } else {
                pattern = new regex(regex_key + ":" +regex_value);
                // if (metainfoObject->memberIsIgnoreLU == false){
                //     pattern = new regex(regex_key + ":" +regex_value);
                // }else{
                //     pattern = new regex(regex_key + ":" +regex_value,regex::icase);//icase用于忽略大小写
                // }
            }
            //cout << "       反序列化获取的regex_value：" << regex_value << "  memberType = " << metainfo1->memberType << endl;
            if(regex_search(json_, result, *pattern)){
                string value = result.str(2).c_str();
                if (value == ""){
                    value = result.str(1).c_str();
                }
                //cout << "@@@@@@@@@@@@@反序列化value = " << value << endl;
                FdogSerializerBase::Instance()->JsonToBase(object_, metainfo1, value);
            }
        }

        if (OBJECT_STRUCT == objectType) {
            for(auto metainfoObject : objectinfo.metaInfoObjectList){
                //通过正则表达式获取对应的json
                smatch result;
                string regex_key = "(\"" + metainfoObject->memberName +"\")";
                string regex_value = baseRegex[metainfoObject->memberType];
                //cout << "       反序列化获取的regex_value：" << regex_value << "  memberType = " << metainfoObject->memberType << endl;
                if(regex_value == ""){
                    if(metainfoObject->memberTypeInt == OBJECT_STRUCT){
                        //cout << "------------" << "struct类型" << endl;
                        regex_value = objectRegex;
                    }
                    if(metainfoObject->memberTypeInt == OBJECT_ARRAY){
                        //cout << "------------" << "appay类型" << endl;
                        regex_value = arrayRegex;
                    }
                    if(metainfoObject->memberTypeInt == OBJECT_VECTOR){
                        //cout << "------------" << "vector类型" << endl;
                        regex_value = arrayRegex;
                    }
                    if(metainfoObject->memberTypeInt == OBJECT_LIST){
                        //cout << "------------" << "list类型" << endl;
                        regex_value = arrayRegex;
                    }
                    if(metainfoObject->memberTypeInt == OBJECT_SET){
                        //cout << "------------" << "set类型" << endl;
                        regex_value = arrayRegex;
                    }
                    if(metainfoObject->memberTypeInt == OBJECT_DEQUE){
                        //cout << "------------" << "deque类型" << endl;
                        regex_value = arrayRegex;
                    }
                    if(metainfoObject->memberTypeInt == OBJECT_MAP){
                        //cout << "------------" << "map类型" << endl;
                        regex_value = mapRegex;
                    }                                       
                } else {
                    //cout << "------------" << "base类型" << endl;
                }
                //根据大小写判断
                regex * pattern = nullptr;
                if (metainfoObject->memberIsIgnoreLU == false){
                    pattern = new regex(regex_key + ":" +regex_value);
                }else{
                    pattern = new regex(regex_key + ":" +regex_value,regex::icase);//icase用于忽略大小写
                }
                if(regex_search(json_, result, *pattern)){
                    string value = result.str(2).c_str();
                    //cout << endl << "正则表达式 获取的值：" << value << "   type = " << metainfoObject->memberTypeInt << endl;
                    if(metainfoObject->memberTypeInt == OBJECT_BASE && metainfoObject->memberIsIgnore != true){
                        //cout << "反序列化进入base：" << value << endl << endl;
                        FdogSerializerBase::Instance()->JsonToBase(object_, metainfoObject, value);
                    }
                    if(metainfoObject->memberTypeInt == OBJECT_ARRAY && metainfoObject->memberIsIgnore != true){
                        vector<string> json_array;
                        objectType = isBaseType(metainfoObject->first);
                        if (objectType){
                                smatch result;
                                regex pattern(arrayRegex);
                                if(regex_search(json_, result, pattern)){
                                    string value = result.str(2).c_str();
                                    json_array = split(value, ",");
                                }
                        }else{
                            removeFirstComma(json_);
                            removeLastComma(json_);
                            json_array = FdogSerializer::Instance()->CuttingArray(json_);
                        }
                        int j = 0;
                        if(metainfoObject->first == "bool"){
                            for(int i = 0; i < metainfoObject->memberArraySize; i++){
                                FdogSerializerBase::Instance()->setValueByAddress(metainfoObject->first, object_, metainfoObject->memberOffset+ (i * sizeof(bool)), json_array[j++]);
                            }
                        }                        
                        if(metainfoObject->first == "char"){
                            for(int i = 0; i < metainfoObject->memberArraySize; i++){
                                FdogSerializerBase::Instance()->setValueByAddress(metainfoObject->first, object_, metainfoObject->memberOffset+ (i * sizeof(char)), json_array[j++]);
                            }
                        }
                        if(metainfoObject->first == "unsigned char"){
                            for(int i = 0; i < metainfoObject->memberArraySize; i++){
                                FdogSerializerBase::Instance()->setValueByAddress(metainfoObject->first, object_, metainfoObject->memberOffset+ (i * sizeof(unsigned char)), json_array[j++]);
                            }
                        }
                        if(metainfoObject->first == "char*"){
                            for(int i = 0; i < metainfoObject->memberArraySize; i++){
                                FdogSerializerBase::Instance()->setValueByAddress(metainfoObject->first, object_, metainfoObject->memberOffset+ (i * sizeof(char*)), json_array[j++]);
                            }
                        }
                        if(metainfoObject->first == "string" || metainfoObject->first == "std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >"){
                            for(int i = 0; i < metainfoObject->memberArraySize; i++){
                                FdogSerializerBase::Instance()->setValueByAddress(metainfoObject->first, object_, metainfoObject->memberOffset+ (i * sizeof(string)), json_array[j++]);
                            }
                        }
                        if(metainfoObject->first == "short"){
                            for(int i = 0; i < metainfoObject->memberArraySize; i++){
                                FdogSerializerBase::Instance()->setValueByAddress(metainfoObject->first, object_, metainfoObject->memberOffset+ (i * sizeof(short)), json_array[j++]);
                            }
                        }
                        if(metainfoObject->first == "unsigned short"){
                            for(int i = 0; i < metainfoObject->memberArraySize; i++){
                                FdogSerializerBase::Instance()->setValueByAddress(metainfoObject->first, object_, metainfoObject->memberOffset+ (i * sizeof(unsigned short)), json_array[j++]);
                            }
                        }
                        if(metainfoObject->first == "int"){
                            for(int i = 0; i < metainfoObject->memberArraySize; i++){
                                FdogSerializerBase::Instance()->setValueByAddress(metainfoObject->first, object_, metainfoObject->memberOffset+ (i * sizeof(int)), json_array[j++]);

                            }
                        }
                        if(metainfoObject->first == "unsigned int"){
                            for(int i = 0; i < metainfoObject->memberArraySize; i++){
                                FdogSerializerBase::Instance()->setValueByAddress(metainfoObject->first, object_, metainfoObject->memberOffset+ (i * sizeof(unsigned int)), json_array[j++]);
                            }
                        }
                        if(metainfoObject->first == "long"){
                            for(int i = 0; i < metainfoObject->memberArraySize; i++){
                                FdogSerializerBase::Instance()->setValueByAddress(metainfoObject->first, object_, metainfoObject->memberOffset+ (i * sizeof(long)), json_array[j++]);
                            }
                        }
                        if(metainfoObject->first == "unsigned long"){
                            for(int i = 0; i < metainfoObject->memberArraySize; i++){
                                FdogSerializerBase::Instance()->setValueByAddress(metainfoObject->first, object_, metainfoObject->memberOffset+ (i * sizeof(unsigned long)), json_array[j++]);
                            }
                        }
                        if(metainfoObject->first == "long long"){
                            for(int i = 0; i < metainfoObject->memberArraySize; i++){
                                FdogSerializerBase::Instance()->setValueByAddress(metainfoObject->first, object_, metainfoObject->memberOffset+ (i * sizeof(long long)), json_array[j++]);
                            }
                        }
                        if(metainfoObject->first == "unsigned long long"){
                            for(int i = 0; i < metainfoObject->memberArraySize; i++){
                                FdogSerializerBase::Instance()->setValueByAddress(metainfoObject->first, object_, metainfoObject->memberOffset+ (i * sizeof(unsigned long long)), json_array[j++]);
                            }
                        }
                        if(metainfoObject->first == "float"){
                            for(int i = 0; i < metainfoObject->memberArraySize; i++){
                                FdogSerializerBase::Instance()->setValueByAddress(metainfoObject->first, object_, metainfoObject->memberOffset+ (i * sizeof(float)), json_array[j++]);
                            }
                        }
                        if(metainfoObject->first == "double"){
                            for(int i = 0; i < metainfoObject->memberArraySize; i++){
                                FdogSerializerBase::Instance()->setValueByAddress(metainfoObject->first, object_, metainfoObject->memberOffset+ (i * sizeof(double)), json_array[j++]);
                            }
                        }
                        if(metainfoObject->first == "long double"){
                            for(int i = 0; i < metainfoObject->memberArraySize; i++){
                                FdogSerializerBase::Instance()->setValueByAddress(metainfoObject->first, object_, metainfoObject->memberOffset+ (i * sizeof(long double)), json_array[j++]);
                            }
                        }
                        Deserialize_arraytype_judgment_all;
                    }
                    if(metainfoObject->memberTypeInt == OBJECT_VECTOR && metainfoObject->memberIsIgnore != true){
                        if(metainfoObject->first == "char"){
                            FDeserialize(*(vector<char> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<vector<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned char"){
                            FDeserialize(*(vector<unsigned char> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<vector<int>>::Tag{});
                        }
                        if(metainfoObject->first == "char*"){
                            FDeserialize(*(vector<char *> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<vector<int>>::Tag{});
                        }
                        if(metainfoObject->first == "string" || metainfoObject->first == "std::__cxx11::basic_string<char"){
                            //cout << "进入string" << endl;
                            FDeserialize(*(vector<string> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<vector<int>>::Tag{});
                        }
                        if(metainfoObject->first == "short"){
                            FDeserialize(*(vector<short> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<vector<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned short"){
                            FDeserialize(*(vector<unsigned short> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<vector<int>>::Tag{});
                        }
                        if(metainfoObject->first == "int"){
                            FDeserialize(*(vector<int> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<vector<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned int"){
                            FDeserialize(*(vector<unsigned int> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<vector<int>>::Tag{});
                        }
                        if(metainfoObject->first == "long"){
                            FDeserialize(*(vector<long> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<vector<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned long"){
                            FDeserialize(*(vector<unsigned long> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<vector<int>>::Tag{});
                        }
                        if(metainfoObject->first == "long long"){
                            FDeserialize(*(vector<long long> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<vector<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned long long"){
                            FDeserialize(*(vector<unsigned long long> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<vector<int>>::Tag{});
                        }
                        if(metainfoObject->first == "float"){
                            FDeserialize(*(vector<float> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<vector<int>>::Tag{});
                        }
                        if(metainfoObject->first == "double"){
                            FDeserialize(*(vector<double> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<vector<int>>::Tag{});
                        }
                        if(metainfoObject->first == "long double"){
                            FDeserialize(*(vector<long double> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<vector<int>>::Tag{});
                        }
                        Deserialize_vector_type_judgment_all;                                                                                                                                                                                                                                                                                       
                    }
                    if(metainfoObject->memberTypeInt == OBJECT_LIST && metainfoObject->memberIsIgnore != true){
                        if(metainfoObject->first == "bool"){
                            FDeserialize(*(list<bool> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<list<bool>>::Tag{});
                        } 
                        if(metainfoObject->first == "char"){
                            FDeserialize(*(list<char> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<list<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned char"){
                            FDeserialize(*(list<unsigned char> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<list<int>>::Tag{});
                        }
                        if(metainfoObject->first == "char*"){
                            FDeserialize(*(list<char *> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<list<int>>::Tag{});
                        }
                        if(metainfoObject->first == "string" || metainfoObject->first == "std::__cxx11::basic_string<char"){
                            FDeserialize(*(list<string> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<list<int>>::Tag{});
                        }
                        if(metainfoObject->first == "short"){
                            FDeserialize(*(list<short> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<list<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned short"){
                            FDeserialize(*(list<unsigned short> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<list<int>>::Tag{});
                        }
                        if(metainfoObject->first == "int"){
                            //cout << "dasdsa======进入list" << endl;                        
                            FDeserialize(*(list<int> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<list<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned int"){
                            FDeserialize(*(list<unsigned int> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<list<int>>::Tag{});
                        }
                        if(metainfoObject->first == "long"){
                            FDeserialize(*(list<long> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<list<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned long"){
                            FDeserialize(*(list<unsigned long> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<list<int>>::Tag{});
                        }
                        if(metainfoObject->first == "long long"){
                            FDeserialize(*(list<long long> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<list<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned long long"){
                            FDeserialize(*(list<unsigned long long> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<list<int>>::Tag{});
                        }
                        if(metainfoObject->first == "float"){
                            FDeserialize(*(list<float> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<list<int>>::Tag{});
                        }
                        if(metainfoObject->first == "double"){
                            FDeserialize(*(list<double> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<list<int>>::Tag{});
                        }
                        if(metainfoObject->first == "long double"){
                            FDeserialize(*(list<long double> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<list<int>>::Tag{});
                        }    
                    }
                    if(metainfoObject->memberTypeInt == OBJECT_DEQUE && metainfoObject->memberIsIgnore != true){
                        if(metainfoObject->first == "bool"){
                            FDeserialize(*(deque<bool> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<deque<bool>>::Tag{});
                        } 
                        if(metainfoObject->first == "char"){
                            FDeserialize(*(deque<char> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<deque<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned char"){
                            FDeserialize(*(deque<unsigned char> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<deque<int>>::Tag{});
                        }
                        if(metainfoObject->first == "char*"){
                            FDeserialize(*(deque<char *> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<deque<int>>::Tag{});
                        }
                        if(metainfoObject->first == "string" || metainfoObject->first == "std::__cxx11::basic_string<char"){
                            FDeserialize(*(deque<string> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<deque<int>>::Tag{});
                        }
                        if(metainfoObject->first == "short"){
                            FDeserialize(*(deque<short> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<deque<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned short"){
                            FDeserialize(*(deque<unsigned short> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<deque<int>>::Tag{});
                        }
                        if(metainfoObject->first == "int"){                        
                            FDeserialize(*(deque<int> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<deque<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned int"){
                            FDeserialize(*(deque<unsigned int> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<deque<int>>::Tag{});
                        }
                        if(metainfoObject->first == "long"){
                            FDeserialize(*(deque<long> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<deque<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned long"){
                            FDeserialize(*(deque<unsigned long> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<deque<int>>::Tag{});
                        }
                        if(metainfoObject->first == "long long"){
                            FDeserialize(*(deque<long long> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<deque<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned long long"){
                            FDeserialize(*(deque<unsigned long long> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<deque<int>>::Tag{});
                        }
                        if(metainfoObject->first == "float"){
                            FDeserialize(*(deque<float> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<deque<int>>::Tag{});
                        }
                        if(metainfoObject->first == "double"){
                            FDeserialize(*(deque<double> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<deque<int>>::Tag{});
                        }
                        if(metainfoObject->first == "long double"){
                            FDeserialize(*(deque<long double> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<deque<int>>::Tag{});
                        }    
                    }
                    if(metainfoObject->memberTypeInt == OBJECT_SET && metainfoObject->memberIsIgnore != true){
                        if(metainfoObject->first == "bool"){
                            FDeserialize(*(set<bool> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<set<bool>>::Tag{});
                        } 
                        if(metainfoObject->first == "char"){
                            FDeserialize(*(set<char> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<set<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned char"){
                            FDeserialize(*(set<unsigned char> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<set<int>>::Tag{});
                        }
                        if(metainfoObject->first == "char*"){
                            FDeserialize(*(set<char *> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<set<int>>::Tag{});
                        }
                        if(metainfoObject->first == "string" || metainfoObject->first == "std::__cxx11::basic_string<char"){
                            FDeserialize(*(set<string> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<set<int>>::Tag{});
                        }
                        if(metainfoObject->first == "short"){
                            FDeserialize(*(set<short> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<set<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned short"){
                            FDeserialize(*(set<unsigned short> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<set<int>>::Tag{});
                        }
                        if(metainfoObject->first == "int"){                        
                            FDeserialize(*(set<int> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<set<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned int"){
                            FDeserialize(*(set<unsigned int> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<set<int>>::Tag{});
                        }
                        if(metainfoObject->first == "long"){
                            FDeserialize(*(set<long> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<set<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned long"){
                            FDeserialize(*(set<unsigned long> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<set<int>>::Tag{});
                        }
                        if(metainfoObject->first == "long long"){
                            FDeserialize(*(set<long long> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<set<int>>::Tag{});
                        }
                        if(metainfoObject->first == "unsigned long long"){
                            FDeserialize(*(set<unsigned long long> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<set<int>>::Tag{});
                        }
                        if(metainfoObject->first == "float"){
                            FDeserialize(*(set<float> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<set<int>>::Tag{});
                        }
                        if(metainfoObject->first == "double"){
                            FDeserialize(*(set<double> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<set<int>>::Tag{});
                        }
                        if(metainfoObject->first == "long double"){
                            FDeserialize(*(set<long double> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<set<int>>::Tag{});
                        }                         
                    }
                    if(metainfoObject->memberTypeInt == OBJECT_MAP && metainfoObject->memberIsIgnore != true){
                        if(metainfoObject->first == "char*" && metainfoObject->second == "int"){
                            FDeserialize(*(map<char *, int> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<map<int,int>>::Tag{});
                        }
                        if(metainfoObject->first == "std::__cxx11::basic_string<char" && metainfoObject->second == "int"){
                            FDeserialize(*(map<string, int> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<map<int,int>>::Tag{});
                        }
                        if(metainfoObject->first == "std::__cxx11::basic_string<char" && metainfoObject->second == "float"){
                            FDeserialize(*(map<string, float> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<map<int,int>>::Tag{});
                        }
                        if(metainfoObject->first == "std::__cxx11::basic_string<char" && metainfoObject->second == "bool"){
                            FDeserialize(*(map<string, int> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<map<int,int>>::Tag{});
                        }                                                                                                                                                 
                        if(metainfoObject->first == "int" && metainfoObject->second == "int"){
                            FDeserialize(*(map<int, int> *)((void *)&object_ + metainfoObject->memberOffset), value, TagDispatchTrait<map<int,int>>::Tag{});
                        }
                    }
                    Deserialize_type_judgment_all;
                }
            }
        }
    }

    template<typename T>
    void DeserializeS(string & json_, T & object_, BaseAndStructTag, string name = ""){
        Deserialize(object_, json_, name);
    }

    template<typename T>
    void DeserializeS(string & json_, T & object_, BaseArrayTag, string name = ""){
        vector<string> json_array;
        json_array = FdogSerializer::Instance()->CuttingArray(json_);
        //cout << "----" << json_array.size() << endl;
        for(auto & object_one : object_){
            Deserialize(object_one, json_);
        }
    }

    template<typename T>
    void DeserializeS_s(T & object_, string & json_, bool isArray, string name = ""){
        // if(isArray){
        //     DeserializeS(json_, object_, TagSTLAAAType<int[2]>::Tag{}, name);
        // }else{

        DeserializeS(json_, object_, typename TagSTLAAAType<T>::Tag{}, name);
        //}
    }

    //用于解析基础类型，数组(只需要判断有没有[]就能确定是不是数组，结构体和基础类型都不具备[]条件)，结构体
    template<typename T>
    void FDeserialize(T & object_, string & json_, BaseTag, string name = ""){
        bool isArray = isArrayType("", abi::__cxa_demangle(typeid(T).name(),0,0,0));
        DeserializeS_s(object_, json_, isArray, name);
    }

    //用于解析STL（map除外）
    template<typename T>
    void FDeserialize(T & object_, string & json_, ArrayTag, string name = ""){
        //cout << "反序列化进入~array：" << json_  << endl;
        //cout << "类型" << abi::__cxa_demangle(typeid(object_).name(),0,0,0) << endl << endl;
        int objectType;
        for(auto & object_one : object_){
            //判断内部类型是否为基础类型
            objectType = isBaseType(abi::__cxa_demangle(typeid(object_one).name(),0,0,0));
            //cout << "objectType=" << objectType << "--" << abi::__cxa_demangle(typeid(object_one).name(),0,0,0)<< endl;
            break;
        }
        vector<string> json_array;
        if (objectType){
                smatch result;
                regex pattern(arrayRegex);
                if(regex_search(json_, result, pattern)){
                    string value = result.str(2).c_str();
                    json_array = split(value, ",");
                }
        }else{
            removeFirstComma(json_);
            removeLastComma(json_);
            json_array = FdogSerializer::Instance()->CuttingArray(json_);
        }
        int i = 0;
        //这里需要注意，进来的STL容器长度为0，需要重新指定长度 需要想办法
        //需要知道容易内部类型，然后作为参数传进去,如果长度小于需要转换的，就需要添加长度
        
        memberAttribute Member = getMemberAttribute(abi::__cxa_demangle(typeid(T).name(),0,0,0));
        srand((int)time(NULL)); //用于set
        for(int i = 0; i < json_array.size(); i++){
            //cout << "xunhuan :" << i << endl;
            if(json_array.size() > object_.size()){
                F_init(object_, Member.valueTypeInt, Member.first);
            }
        }
        // int len = json_array.size();
        // int len2 = object_.size();
        //cout << "changdu:" << len  << "----" << len2 << endl;
        for(auto & object_one : object_){
            //cout << "json_array[i] = " << json_array[i] << endl;
            Deserialize(object_one, json_array[i]);
            i++;
        }
        //这里释放init里面申请的内存
        if(1){
            vector<char *>::iterator it = temp.begin();
            while(it!= temp.end()){
                delete *it;
                ++it;
            }
        }
        temp.clear();    
    }
    //用于map
    template<typename T>
    void FDeserialize(T & object_, string & json_, MapTag){
        //cout << "反序列化进入~map：" << json_ << endl;
        //cout << "类型" << abi::__cxa_demangle(typeid(object_).name(),0,0,0) << endl << endl;
        int objectType;
        for(auto & object_one : object_){
            //判断内部类型是否为基础类型
            objectType = isBaseTypeByMap(abi::__cxa_demangle(typeid(object_one).name(),0,0,0));
            //cout << "objectType=" << objectType << "--" << abi::__cxa_demangle(typeid(object_one).name(),0,0,0)<< endl;
            break;
        }
        //这里有问题 objectType永远为0 找第二种方法
        vector<string> json_array;
        if (objectType){
                smatch result;
                regex pattern(mapRegex);
                if(regex_search(json_, result, pattern)){
                    string value = result.str(2).c_str();
                    json_array = split(value, ",");
                }
        }else{
            removeFirstComma(json_);
            removeLastComma(json_);
            json_array = FdogSerializer::Instance()->CuttingArray(json_);
        }
        int i = 0;
        int len = json_array.size();
        sort(json_array.begin(), json_array.end());
        //cout << "changdu:" << len << endl;
        memberAttribute Member = getMemberAttribute(abi::__cxa_demangle(typeid(T).name(),0,0,0));
        for(int i = 0; i < json_array.size(); i++){
            if(json_array.size() > object_.size()){
                F_init(object_, Member.valueTypeInt, Member.first, Member.second, getKey(json_array[i]));
            }
        }
        //这里有个问题，就是可能key的顺序不匹配
        //这里存在问题，进来的STL容器长度为0，需要重新指定长度
        for(auto & object_one : object_){
            //cout << "进来啦 object_one.second = " << object_one.second << " json_array[i] = " << json_array[i] << endl;
            //提取key:
            //object_one.first = getKey(json_array[i]);
            Deserialize(object_one.second, json_array[i]);
            i++;
        }
        //这里释放init里面申请的内存
        if(Member.valueTypeInt == OBJECT_MAP){
            vector<char *>::iterator it = temp.begin();
            while(it!= temp.end()){
                delete *it;
                ++it;
            }
        }
        temp.clear();    
    }
};

namespace Fdog{
 
template<typename T>
void FJson(string & json_, T & object_, string name = "") {
    FdogSerializer::Instance()->FSerialize(json_, object_, typename TagDispatchTrait<T>::Tag{}, name);
}

template<typename T>
void FObject(T & object_, string & json_, string name = ""){
  FdogSerializer::Instance()->FDeserialize(object_, json_, typename TagDispatchTrait<T>::Tag{}, name);
}

void setAliasName(string Type, string memberName, string AliasName);

//设置是否忽略该字段序列化
void setIgnoreField(string Type, string memberName);

//设置是否忽略大小写
void setIgnoreLU(string Type, string memberName);

//设置进行模糊转换 结构体转json不存在这个问题主要是针对json转结构体的问题，如果存在分歧，可以尝试进行模糊转换
void setFuzzy(string Type);

//判断json正确性
result JsonValidS(string json_);

//判断字段是否存在
bool Exist(string json_, string key);

//获取字段的值
string GetStringValue(string json_, string key);

//获取字段的值
int GetIntValue(string json_, string key);

//获取字段的值
double GetDoubleValue(string json_, string key);

//获取字段的值
long GetLongValue(string json_, string key);

//获取字段的值
bool GetBoolValue(string json_, string key);

// void setAliasName1(string Type, string memberName, string AliasName){
//     FdogSerializer::Instance()->setAliasName(Type, memberName, AliasName);
// }   

// //设置别名
// void setAliasName1(string Type, string memberName, string AliasName){
//   FdogSerializer::Instance()->setAliasName(Type, memberName, AliasName);
// }

// //设置是否忽略该字段序列化
// void setIgnoreField1(string Type, string memberName){
//   FdogSerializer::Instance()->setIgnoreField(Type, memberName);
// }

// //设置是否忽略大小写
// void setIgnoreLU(string Type, string memberName){
//   FdogSerializer::Instance()->setIgnoreLU(Type, memberName);
// }

/*
{
    "test1": "wx9fdb8ble7ce3c68f",
    "test2": "123456789",
    "testData1": {   
        "testdatason1": "97895455"
        "testdatason2":3,
    }
}

*/

/*
    "key" : "dsasadsadsadsa"
    "key" : 32131
    "key" : true
*/
    //string js = "{\"stu\":{\"name\":\"liuliu\",\"age\":18},\"tea\":{\"name\":\"wufang\",\"age\":48}}";
// //json是否正确
// bool JsonValid(string json_) {
//     //1. 两个花括号是否存在
//     string start = json_[0];
//     string end = json_[json_.length()-1];
//     if(start + end != "{}"){
//         return false;
//     }
//     //2. 将key:value 转化为 *：*  判断是否有不存在的情况
//     //3. 判断
//     return true;
// }

// //所查找字段是否存在 //如果存在其他成员也存在这个值，应当使用"xxx.xxx"来正确获取
// bool Exists(string field_) {
//     return true;
// }

// //获取int类型的值
// int GetIntValue(string json_, string field_) {
//     return 11;
// }

// //获取double类型的值
// double GetDoubleValue(string json_, string field_) {
//     return 11.3;
// }

// //获取string类型的值
// string GetStringValue(string json_, string field_) {
//     return "xxx";
// }

// //获取bool类型的值
// bool GetBoolValue(string json_, string field_) {
//     return true;
// }
}

#define NAME(x) #x

#define EXTAND_ARGS(args) args //__VA_ARGS__ 在vs中会被认为是一个实参，所以需要定义该宏过渡

#define ARG_N(...) \
    EXTAND_ARGS(ARG_N_(0, ##__VA_ARGS__, ARG_N_RESQ()))

#define ARG_N_(...) \
    EXTAND_ARGS(ARG_N_M(__VA_ARGS__))

#define ARG_N_M(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, _11, _12, _13, _14, _15, _16, _17, _18, _19,_20, N,...) N

#define ARG_N_RESQ() 20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0

#define MEMBERTYPE(TYPE, MEMBER) FdogSerializer::Instance()->getMemberAttribute(abi::__cxa_demangle(typeid(((TYPE *)0)->MEMBER).name(),0,0,0))

#define PLACEHOLDER(placeholder, ...) placeholder

#define REGISTEREDMEMBER(TYPE, ...) \
do{ \
    ObjectInfo * objectinfo_one = new ObjectInfo();\
    objectinfo_one->objectType = NAME(TYPE);\
    objectinfo_one->objectTypeInt = FdogSerializer::Instance()->getObjectTypeInt(objectinfo_one->objectType, abi::__cxa_demangle(typeid(TYPE).name(),0,0,0));\
    objectinfo_one->objectSize = sizeof(TYPE);\
    FdogSerializer::Instance()->addObjectInfo(objectinfo_one);\
    REGISTEREDMEMBER_s_1(TYPE, PLACEHOLDER(__VA_ARGS__), objectinfo_one->metaInfoObjectList, ARG_N(__VA_ARGS__) - 1, ##__VA_ARGS__, PLACEHOLDER(__VA_ARGS__));\
}while(0);

#define REGISTEREDMEMBER_s_1(TYPE, PLACE, metaInfoObjectList, size, arg1, ...) \
REGISTEREDMEMBER_s(TYPE, metaInfoObjectList, arg1); if (size > 0) REGISTEREDMEMBER_s_2(TYPE, PLACE, metaInfoObjectList, size-1, ##__VA_ARGS__, PLACE);

#define REGISTEREDMEMBER_s_2(TYPE, PLACE, metaInfoObjectList, size, arg1, ...) \
REGISTEREDMEMBER_s(TYPE, metaInfoObjectList, arg1); if (size > 0) REGISTEREDMEMBER_s_3(TYPE, PLACE, metaInfoObjectList, size-1, ##__VA_ARGS__, PLACE);

#define REGISTEREDMEMBER_s_3(TYPE, PLACE, metaInfoObjectList, size, arg1, ...) \
REGISTEREDMEMBER_s(TYPE, metaInfoObjectList, arg1); if (size > 0) REGISTEREDMEMBER_s_4(TYPE, PLACE, metaInfoObjectList, size-1, ##__VA_ARGS__, PLACE);

#define REGISTEREDMEMBER_s_4(TYPE, PLACE, metaInfoObjectList, size, arg1, ...) \
REGISTEREDMEMBER_s(TYPE, metaInfoObjectList, arg1); if (size > 0) REGISTEREDMEMBER_s_5(TYPE, PLACE, metaInfoObjectList, size-1, ##__VA_ARGS__, PLACE);

#define REGISTEREDMEMBER_s_5(TYPE, PLACE, metaInfoObjectList, size, arg1, ...) \
REGISTEREDMEMBER_s(TYPE, metaInfoObjectList, arg1); if (size > 0) REGISTEREDMEMBER_s_6(TYPE, PLACE, metaInfoObjectList, size-1, ##__VA_ARGS__, PLACE);

#define REGISTEREDMEMBER_s_6(TYPE, PLACE, metaInfoObjectList, size, arg1, ...) \
REGISTEREDMEMBER_s(TYPE, metaInfoObjectList, arg1); if (size > 0) REGISTEREDMEMBER_s_7(TYPE, PLACE, metaInfoObjectList, size-1, ##__VA_ARGS__, PLACE);

#define REGISTEREDMEMBER_s_7(TYPE, PLACE, metaInfoObjectList, size, arg1, ...) \
REGISTEREDMEMBER_s(TYPE, metaInfoObjectList, arg1); if (size > 0) REGISTEREDMEMBER_s_8(TYPE, PLACE, metaInfoObjectList, size-1, ##__VA_ARGS__, PLACE);

#define REGISTEREDMEMBER_s_8(TYPE, PLACE, metaInfoObjectList, size, arg1, ...) \
REGISTEREDMEMBER_s(TYPE, metaInfoObjectList, arg1); if (size > 0) REGISTEREDMEMBER_s_9(TYPE, PLACE, metaInfoObjectList, size-1, ##__VA_ARGS__, PLACE);

#define REGISTEREDMEMBER_s_9(TYPE, PLACE, metaInfoObjectList, size, arg1, ...) \
REGISTEREDMEMBER_s(TYPE, metaInfoObjectList, arg1); if (size > 0) REGISTEREDMEMBER_s_10(TYPE, PLACE, metaInfoObjectList, size-1, ##__VA_ARGS__, PLACE);

#define REGISTEREDMEMBER_s_10(TYPE, PLACE, metaInfoObjectList, size, arg1, ...) \
REGISTEREDMEMBER_s(TYPE, metaInfoObjectList, arg1); if (size > 0) REGISTEREDMEMBER_s_11(TYPE, PLACE, metaInfoObjectList, size-1, ##__VA_ARGS__, PLACE);

#define REGISTEREDMEMBER_s_11(TYPE, PLACE, metaInfoObjectList, size, arg1, ...) \
REGISTEREDMEMBER_s(TYPE, metaInfoObjectList, arg1); if (size > 0) REGISTEREDMEMBER_s_12(TYPE, PLACE, metaInfoObjectList, size-1, ##__VA_ARGS__, PLACE);

#define REGISTEREDMEMBER_s_12(TYPE, PLACE, metaInfoObjectList, size, arg1, ...) \
REGISTEREDMEMBER_s(TYPE, metaInfoObjectList, arg1); if (size > 0) REGISTEREDMEMBER_s_13(TYPE, PLACE, metaInfoObjectList, size-1, ##__VA_ARGS__, PLACE);

#define REGISTEREDMEMBER_s_13(TYPE, PLACE, metaInfoObjectList, size, arg1, ...) \
REGISTEREDMEMBER_s(TYPE, metaInfoObjectList, arg1); if (size > 0) REGISTEREDMEMBER_s_14(TYPE, PLACE, metaInfoObjectList, size-1, ##__VA_ARGS__, PLACE);

#define REGISTEREDMEMBER_s_14(TYPE, PLACE, metaInfoObjectList, size, arg1, ...) \
REGISTEREDMEMBER_s(TYPE, metaInfoObjectList, arg1); if (size > 0) REGISTEREDMEMBER_s_15(TYPE, PLACE, metaInfoObjectList, size-1, ##__VA_ARGS__, PLACE);

#define REGISTEREDMEMBER_s_15(TYPE, PLACE, metaInfoObjectList, size, arg1, ...) \
REGISTEREDMEMBER_s(TYPE, metaInfoObjectList, arg1); if (size > 0) REGISTEREDMEMBER_s_16(TYPE, PLACE, metaInfoObjectList, size-1, ##__VA_ARGS__, PLACE);

#define REGISTEREDMEMBER_s_16(TYPE, PLACE, metaInfoObjectList, size, arg1, ...) \
REGISTEREDMEMBER_s(TYPE, metaInfoObjectList, arg1); if (size > 0) REGISTEREDMEMBER_s_17(TYPE, PLACE, metaInfoObjectList, size-1, ##__VA_ARGS__, PLACE);

#define REGISTEREDMEMBER_s_17(TYPE, PLACE, metaInfoObjectList, size, arg1, ...) \
REGISTEREDMEMBER_s(TYPE, metaInfoObjectList, arg1); if (size > 0) REGISTEREDMEMBER_s_18(TYPE, PLACE, metaInfoObjectList, size-1, ##__VA_ARGS__, PLACE);

#define REGISTEREDMEMBER_s_18(TYPE, PLACE, metaInfoObjectList, size, arg1, ...) \
REGISTEREDMEMBER_s(TYPE, metaInfoObjectList, arg1); if (size > 0) REGISTEREDMEMBER_s_19(TYPE, PLACE, metaInfoObjectList, size-1, ##__VA_ARGS__, PLACE);

#define REGISTEREDMEMBER_s_19(TYPE, PLACE, metaInfoObjectList, size, arg1, ...) \
REGISTEREDMEMBER_s(TYPE, metaInfoObjectList, arg1); if (size > 0) REGISTEREDMEMBER_s_20(TYPE, PLACE, metaInfoObjectList, size-1, ##__VA_ARGS__, PLACE);

#define REGISTEREDMEMBER_s_20(TYPE, PLACE, metaInfoObjectList, size, arg1, ...) \
REGISTEREDMEMBER_s(TYPE, metaInfoObjectList, arg1);

#define REGISTEREDMEMBER_s(TYPE, metaInfoObjectList, arg) \
    do{\
        MetaInfo * metainfo_one = new MetaInfo();\
        metainfo_one->memberName = NAME(arg);\
        metainfo_one->memberAliasName = "";\
        metainfo_one->memberOffset = offsetof(TYPE, arg);\
        memberAttribute resReturn = MEMBERTYPE(TYPE, arg);\
        metainfo_one->memberType = resReturn.valueType;\
        metainfo_one->memberTypeSize = sizeof(TYPE);\
        metainfo_one->memberArraySize = resReturn.ArraySize;\
        metainfo_one->memberTypeInt = resReturn.valueTypeInt;\
        metainfo_one->first = resReturn.first;\
        metainfo_one->second = resReturn.second;\
        metainfo_one->memberIsIgnore = false;\
        metainfo_one->memberIsIgnoreLU = false;\
        metaInfoObjectList.push_back(metainfo_one);\
    }while(0);

#endif