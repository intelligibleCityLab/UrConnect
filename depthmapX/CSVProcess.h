#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <mutex>

class CSVProcess {
public:
    static CSVProcess& getInstance() {
        static CSVProcess instance; // 局部静态变量，保证只实例化一次
        return instance;
    }
    bool open(const std::string& filePath);
    void close();
    int getFieldIndex(const std::string& fieldName) const;
    void addField(const std::string& fieldName);
    void renameField(const std::string& oldName, const std::string& newName);
    void deleteField(const std::string& fieldName);
    void updateField(const std::string& fieldName, const std::map<int, double>& data);
    std::vector<std::string> getHeader();
private:
    CSVProcess() = default; // 私有构造函数
    ~CSVProcess() {
        close(); // 确保关闭文件
    }

    // 禁止拷贝和赋值
    CSVProcess(const CSVProcess&) = delete;
    CSVProcess& operator=(const CSVProcess&) = delete;
    void saveToFile();
    void loadFromFile();

    std::string m_filePath;
    std::ifstream file;
    std::vector<std::string> header;
    std::vector<std::vector<std::string>> rows;
    mutable std::mutex m_mutex; // 互斥锁，用于保护共享资源
};