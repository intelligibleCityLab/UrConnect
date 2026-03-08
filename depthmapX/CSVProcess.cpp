#include "CSVProcess.h"
std::string join(const std::vector<std::string>& vec, const std::string& delimiter) {
    std::ostringstream oss;
    for (size_t i = 0; i < vec.size(); ++i) {
        oss << vec[i];
        if (i < vec.size() - 1) {
            oss << delimiter;
        }
    }
    return oss.str();
}
bool CSVProcess::open(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_filePath = filePath;
    file.open(filePath);
    if (file.is_open()) {
        loadFromFile();
        file.close();
        return true;
    }
    std::cerr << "Error: Unable to open file at " << filePath << std::endl;
    return false;
}

void CSVProcess::close() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (file.is_open()) {
        saveToFile();
        file.close();
    }
}

int CSVProcess::getFieldIndex(const std::string& fieldName) const {
    auto it = std::find(header.begin(), header.end(), fieldName);
    return (it != header.end()) ? std::distance(header.begin(), it) : -1;
}

void CSVProcess::addField(const std::string& fieldName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (getFieldIndex(fieldName) == -1) {
        header.push_back(fieldName);
        for (auto& row : rows) {
            row.resize(header.size()); // Add empty value for new field
        }
    }
}

void CSVProcess::renameField(const std::string& oldName, const std::string& newName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    int index = getFieldIndex(oldName);
    if (index != -1 && getFieldIndex(newName) == -1) {
        header[index] = newName;
    }
}

void CSVProcess::deleteField(const std::string& fieldName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    int index = getFieldIndex(fieldName);
    if (index != -1) {
        header.erase(header.begin() + index);
        for (auto& row : rows) {
            row.erase(row.begin() + index);
        }
    }
}

void CSVProcess::updateField(const std::string& fieldName, const std::map<int, double>& data) {
    std::lock_guard<std::mutex> lock(m_mutex);
    int index = getFieldIndex(fieldName);
    if (index == -1) return;

    for (const auto& entry : data) {
        int rowIndex = entry.first;
        if (rowIndex >= 0 && rowIndex < rows.size()) {
            rows[rowIndex][index] = std::to_string(entry.second);
        }
    }
}

void CSVProcess::saveToFile() {
    std::ofstream outputFile(m_filePath, std::ios::out | std::ios::trunc);
    if (!outputFile.is_open()) {
        std::cerr << "Error: Unable to open file at " << m_filePath << std::endl;
        return;
    }

    // Write header
    outputFile << join(header, ",") << "\n";

    // Write rows
    for (const auto& row : rows) {
        outputFile << join(row, ",") << "\n";
    }

    outputFile.close();
}

void CSVProcess::loadFromFile() {
    std::string line;

    // 清空之前的数据
    header.clear();
    rows.clear();

    // 读取表头
    if (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string field;
        while (std::getline(ss, field, ',')) {
            header.push_back(field);
        }
    }

    // 读取每一行数据
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::vector<std::string> row;
        std::string value;

        // 读取当前行的每个值
        while (std::getline(ss, value, ',')) {
            row.push_back(value);
        }

        // 根据表头大小调整行的大小
        while (row.size() < header.size()) {
            row.push_back(""); // 填充空值
        }

        // 检查行的大小是否符合要求
        if (row.size() > header.size()) {
            std::cerr << "Warning: Row size exceeds header size; extra data will be ignored." << std::endl;
            row.resize(header.size()); // 截断多余的字段
        }

        rows.push_back(row);
    }
}

std::vector<std::string> CSVProcess::getHeader(){
    return header;
}