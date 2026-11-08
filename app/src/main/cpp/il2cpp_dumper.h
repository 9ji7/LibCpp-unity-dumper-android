#pragma once

#include <jni.h>
#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <android/log.h>

#define LOG_TAG "LibCppDumper"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

// IL2CPP Metadata structures (simplified for common Unity versions)
struct Il2CppMetadataType {
    uint32_t nameIndex;
    uint32_t namespaceIndex;
    uint32_t byvalTypeIndex;
    uint32_t thisTypeIndex;
    uint32_t elementTypeIndex;
    int32_t genericContainerIndex;
    uint32_t flags;
    uint32_t token;
};

struct Il2CppMetadataField {
    uint32_t nameIndex;
    uint32_t typeIndex;
    uint32_t parentIndex;
    uint32_t offset;
    uint32_t token;
};

struct Il2CppMetadataMethod {
    uint32_t nameIndex;
    uint32_t returnTypeIndex;
    uint32_t declaringTypeIndex;
    uint32_t parameterStart;
    uint16_t parameterCount;
    uint16_t genericParameterCount;
    uint32_t flags;
    uint32_t iflags;
    uint32_t slot;
    uint32_t token;
};

struct Il2CppMetadataProperty {
    uint32_t nameIndex;
    uint32_t getterIndex;
    uint32_t setterIndex;
    uint32_t typeIndex;
    uint32_t parentIndex;
    uint32_t token;
};

struct Il2CppMetadataAssembly {
    uint32_t nameIndex;
    uint32_t token;
    uint32_t imageIndex;
};

struct Il2CppMetadataImage {
    uint32_t nameIndex;
    uint32_t assemblyIndex;
    uint32_t typeStart;
    uint32_t typeCount;
    uint32_t exportedTypeStart;
    uint32_t exportedTypeCount;
};

class Il2CppDumper {
public:
    Il2CppDumper();
    ~Il2CppDumper();

    int initialize(const char* soPath, const char* metadataPath);
    int dump(const char* outputPath);
    const char* getUnityVersion() const;
    int getAssemblyCount() const;
    int getTypeCount() const;
    int getMethodCount() const;
    int getFieldCount() const;
    void cleanup();

private:
    bool parseMetadata(const char* metadataPath);
    bool parsePE(const char* soPath);
    bool writeDump(const char* outputPath);
    void parseTypes();
    void parseMethods();
    void parseFields();
    void writeAssembly(std::FILE* fp, const Il2CppMetadataAssembly& assembly);
    void writeType(std::FILE* fp, const Il2CppMetadataType& type);
    void writeMethod(std::FILE* fp, const Il2CppMetadataMethod& method);
    void writeField(std::FILE* fp, const Il2CppMetadataField& field);
    const char* getString(uint32_t index) const;

    // Metadata pointers
    uint8_t* metadataBuffer_ = nullptr;
    size_t metadataSize_ = 0;
    
    // Parsed data
    std::vector<Il2CppMetadataType> types_;
    std::vector<Il2CppMetadataField> fields_;
    std::vector<Il2CppMetadataMethod> methods_;
    std::vector<Il2CppMetadataProperty> properties_;
    std::vector<Il2CppMetadataAssembly> assemblies_;
    std::vector<Il2CppMetadataImage> images_;
    std::vector<std::string> strings_;
    
    // Metadata header info
    uint32_t metadataVersion_ = 0;
    uint32_t typeDefinitionsOffset_ = 0;
    uint32_t typeDefinitionsCount_ = 0;
    uint32_t fieldDefinitionsOffset_ = 0;
    uint32_t fieldDefinitionsCount_ = 0;
    uint32_t methodDefinitionsOffset_ = 0;
    uint32_t methodDefinitionsCount_ = 0;
    uint32_t propertyDefinitionsOffset_ = 0;
    uint32_t propertyDefinitionsCount_ = 0;
    uint32_t assemblyDefinitionsOffset_ = 0;
    uint32_t assemblyDefinitionsCount_ = 0;
    uint32_t imageDefinitionsOffset_ = 0;
    uint32_t imageDefinitionsCount_ = 0;
    uint32_t stringLiteralOffset_ = 0;
    uint32_t stringLiteralCount_ = 0;
    uint32_t stringLiteralDataOffset_ = 0;
    uint32_t stringLiteralDataSize_ = 0;
    
    std::string unityVersion_;
    bool initialized_ = false;
};