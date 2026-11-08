#include "il2cpp_dumper.h"
#include <fstream>
#include <cstring>
#include <algorithm>
#include <sstream>

Il2CppDumper::Il2CppDumper() {
    LOGI("Il2CppDumper created");
}

Il2CppDumper::~Il2CppDumper() {
    cleanup();
    LOGI("Il2CppDumper destroyed");
}

void Il2CppDumper::cleanup() {
    if (metadataBuffer_) {
        delete[] metadataBuffer_;
        metadataBuffer_ = nullptr;
    }
    types_.clear();
    fields_.clear();
    methods_.clear();
    properties_.clear();
    assemblies_.clear();
    images_.clear();
    strings_.clear();
    initialized_ = false;
    LOGI("Cleanup completed");
}

bool Il2CppDumper::parseMetadata(const char* metadataPath) {
    std::ifstream file(metadataPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        LOGE("Failed to open metadata file: %s", metadataPath);
        return false;
    }

    metadataSize_ = file.tellg();
    file.seekg(0, std::ios::beg);

    metadataBuffer_ = new uint8_t[metadataSize_];
    file.read(reinterpret_cast<char*>(metadataBuffer_), metadataSize_);
    file.close();

    LOGI("Metadata file loaded: %zu bytes", metadataSize_);

    // Parse metadata header (simplified - based on Il2CppDumper format)
    if (metadataSize_ < 128) {
        LOGE("Metadata file too small");
        return false;
    }

    // Check for metadata signature
    uint32_t* header = reinterpret_cast<uint32_t*>(metadataBuffer_);
    if (header[0] != 0xFAB11BAF && header[0] != 0xAF11B1FA) {
        LOGW("Unknown metadata signature: 0x%08X", header[0]);
        // Try to continue anyway
    }

    // Parse basic header fields (offsets vary by Unity version)
    // This is a simplified parser - real implementation would detect version
    metadataVersion_ = header[1];
    
    // Try common offsets for different Unity versions
    // v24+ (Unity 2017+)
    typeDefinitionsOffset_ = header[2];
    typeDefinitionsCount_ = header[3];
    fieldDefinitionsOffset_ = header[4];
    fieldDefinitionsCount_ = header[5];
    methodDefinitionsOffset_ = header[6];
    methodDefinitionsCount_ = header[7];
    propertyDefinitionsOffset_ = header[8];
    propertyDefinitionsCount_ = header[9];
    assemblyDefinitionsOffset_ = header[10];
    assemblyDefinitionsCount_ = header[11];
    imageDefinitionsOffset_ = header[12];
    imageDefinitionsCount_ = header[13];
    stringLiteralOffset_ = header[14];
    stringLiteralCount_ = header[15];
    stringLiteralDataOffset_ = header[16];
    stringLiteralDataSize_ = header[17];

    LOGI("Metadata version: %u", metadataVersion_);
    LOGI("Types: %u at offset %u", typeDefinitionsCount_, typeDefinitionsOffset_);
    LOGI("Fields: %u at offset %u", fieldDefinitionsCount_, fieldDefinitionsOffset_);
    LOGI("Methods: %u at offset %u", methodDefinitionsCount_, methodDefinitionsOffset_);
    LOGI("Assemblies: %u at offset %u", assemblyDefinitionsCount_, assemblyDefinitionsOffset_);

    // Parse string literals
    if (stringLiteralDataOffset_ > 0 && stringLiteralDataSize_ > 0 && 
        stringLiteralDataOffset_ + stringLiteralDataSize_ <= metadataSize_) {
        const char* strData = reinterpret_cast<const char*>(metadataBuffer_ + stringLiteralDataOffset_);
        size_t pos = 0;
        while (pos < stringLiteralDataSize_) {
            const char* str = strData + pos;
            strings_.push_back(str);
            pos += strlen(str) + 1;
        }
        LOGI("Parsed %zu string literals", strings_.size());
    }

    // Parse assemblies
    if (assemblyDefinitionsOffset_ > 0 && assemblyDefinitionsCount_ > 0 &&
        assemblyDefinitionsOffset_ + assemblyDefinitionsCount_ * 12 <= metadataSize_) {
        const Il2CppMetadataAssembly* asmDefs = 
            reinterpret_cast<const Il2CppMetadataAssembly*>(metadataBuffer_ + assemblyDefinitionsOffset_);
        assemblies_.assign(asmDefs, asmDefs + assemblyDefinitionsCount_);
    }

    // Parse images
    if (imageDefinitionsOffset_ > 0 && imageDefinitionsCount_ > 0 &&
        imageDefinitionsOffset_ + imageDefinitionsCount_ * 24 <= metadataSize_) {
        const Il2CppMetadataImage* imgDefs = 
            reinterpret_cast<const Il2CppMetadataImage*>(metadataBuffer_ + imageDefinitionsOffset_);
        images_.assign(imgDefs, imgDefs + imageDefinitionsCount_);
    }

    // Parse types
    if (typeDefinitionsOffset_ > 0 && typeDefinitionsCount_ > 0 &&
        typeDefinitionsOffset_ + typeDefinitionsCount_ * 32 <= metadataSize_) {
        const Il2CppMetadataType* typeDefs = 
            reinterpret_cast<const Il2CppMetadataType*>(metadataBuffer_ + typeDefinitionsOffset_);
        types_.assign(typeDefs, typeDefs + typeDefinitionsCount_);
    }

    // Parse fields
    if (fieldDefinitionsOffset_ > 0 && fieldDefinitionsCount_ > 0 &&
        fieldDefinitionsOffset_ + fieldDefinitionsCount_ * 20 <= metadataSize_) {
        const Il2CppMetadataField* fieldDefs = 
            reinterpret_cast<const Il2CppMetadataField*>(metadataBuffer_ + fieldDefinitionsOffset_);
        fields_.assign(fieldDefs, fieldDefs + fieldDefinitionsCount_);
    }

    // Parse methods
    if (methodDefinitionsOffset_ > 0 && methodDefinitionsCount_ > 0 &&
        methodDefinitionsOffset_ + methodDefinitionsCount_ * 36 <= metadataSize_) {
        const Il2CppMetadataMethod* methodDefs = 
            reinterpret_cast<const Il2CppMetadataMethod*>(metadataBuffer_ + methodDefinitionsOffset_);
        methods_.assign(methodDefs, methodDefs + methodDefinitionsCount_);
    }

    // Parse properties
    if (propertyDefinitionsOffset_ > 0 && propertyDefinitionsCount_ > 0 &&
        propertyDefinitionsOffset_ + propertyDefinitionsCount_ * 24 <= metadataSize_) {
        const Il2CppMetadataProperty* propDefs = 
            reinterpret_cast<const Il2CppMetadataProperty*>(metadataBuffer_ + propertyDefinitionsOffset_);
        properties_.assign(propDefs, propDefs + propertyDefinitionsCount_);
    }

    // Try to detect Unity version from metadata
    if (strings_.size() > 0) {
        for (const auto& str : strings_) {
            if (str.find("Unity") != std::string::npos && str.find("20") != std::string::npos) {
                unityVersion_ = str;
                break;
            }
        }
        if (unityVersion_.empty()) {
            unityVersion_ = "Unknown (metadata parsed)";
        }
    } else {
        unityVersion_ = "Unknown";
    }

    return true;
}

bool Il2CppDumper::parsePE(const char* soPath) {
    // For now, we just verify the file exists and is readable
    std::ifstream file(soPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        LOGE("Failed to open SO file: %s", soPath);
        return false;
    }
    size_t size = file.tellg();
    file.close();
    LOGI("SO file size: %zu bytes", size);
    return true;
}

int Il2CppDumper::initialize(const char* soPath, const char* metadataPath) {
    LOGI("Initializing dumper with SO: %s, Metadata: %s", soPath, metadataPath);
    
    cleanup();
    
    if (!parseMetadata(metadataPath)) {
        LOGE("Failed to parse metadata");
        return -1;
    }
    
    if (!parsePE(soPath)) {
        LOGE("Failed to parse SO file");
        return -2;
    }
    
    initialized_ = true;
    LOGI("Initialization successful");
    return 0;
}

const char* Il2CppDumper::getUnityVersion() const {
    return unityVersion_.c_str();
}

int Il2CppDumper::getAssemblyCount() const {
    return static_cast<int>(assemblies_.size());
}

int Il2CppDumper::getTypeCount() const {
    return static_cast<int>(types_.size());
}

int Il2CppDumper::getMethodCount() const {
    return static_cast<int>(methods_.size());
}

int Il2CppDumper::getFieldCount() const {
    return static_cast<int>(fields_.size());
}

const char* Il2CppDumper::getString(uint32_t index) const {
    if (index < strings_.size()) {
        return strings_[index].c_str();
    }
    return "";
}

void Il2CppDumper::writeAssembly(std::FILE* fp, const Il2CppMetadataAssembly& assembly) {
    const char* name = getString(assembly.nameIndex);
    fprintf(fp, "// Assembly: %s (Token: 0x%08X)\n", name, assembly.token);
    fprintf(fp, "// Image Index: %u\n\n", assembly.imageIndex);
}

void Il2CppDumper::writeType(std::FILE* fp, const Il2CppMetadataType& type) {
    const char* name = getString(type.nameIndex);
    const char* ns = getString(type.namespaceIndex);
    
    if (ns && strlen(ns) > 0) {
        fprintf(fp, "namespace %s {\n", ns);
    }
    
    fprintf(fp, "    // Type: %s (Token: 0x%08X, Flags: 0x%08X)\n", name, type.token, type.flags);
    fprintf(fp, "    class %s\n", name);
    fprintf(fp, "    {\n");
}

void Il2CppDumper::writeMethod(std::FILE* fp, const Il2CppMetadataMethod& method) {
    const char* name = getString(method.nameIndex);
    const char* retType = getString(method.returnTypeIndex);
    fprintf(fp, "        // Method: %s (Token: 0x%08X, Flags: 0x%08X, Slot: %u)\n", 
            name, method.token, method.flags, method.slot);
    fprintf(fp, "        %s %s();\n", retType ? retType : "void", name);
}

void Il2CppDumper::writeField(std::FILE* fp, const Il2CppMetadataField& field) {
    const char* name = getString(field.nameIndex);
    const char* type = getString(field.typeIndex);
    fprintf(fp, "        // Field: %s (Token: 0x%08X, Offset: %u)\n", 
            name, field.token, field.offset);
    fprintf(fp, "        %s %s;\n", type ? type : "unknown", name);
}

bool Il2CppDumper::writeDump(const char* outputPath) {
    std::FILE* fp = std::fopen(outputPath, "w");
    if (!fp) {
        LOGE("Failed to open output file: %s", outputPath);
        return false;
    }

    fprintf(fp, "// IL2CPP Dump generated by LibCpp Unity Dumper\n");
    fprintf(fp, "// Unity Version: %s\n", unityVersion_.c_str());
    fprintf(fp, "// Assemblies: %zu, Types: %zu, Methods: %zu, Fields: %zu\n\n",
            assemblies_.size(), types_.size(), methods_.size(), fields_.size());

    // Write assemblies and their types
    for (size_t imgIdx = 0; imgIdx < images_.size(); ++imgIdx) {
        const auto& image = images_[imgIdx];
        const char* imgName = getString(image.nameIndex);
        
        if (imgName && strlen(imgName) > 0) {
            fprintf(fp, "// ===== Image: %s =====\n", imgName);
        }

        // Find assemblies for this image
        for (const auto& assembly : assemblies_) {
            if (assembly.imageIndex == imgIdx) {
                writeAssembly(fp, assembly);
            }
        }

        // Write types in this image
        uint32_t typeEnd = image.typeStart + image.typeCount;
        for (uint32_t i = image.typeStart; i < typeEnd && i < types_.size(); ++i) {
            const auto& type = types_[i];
            writeType(fp, type);

            // Write fields for this type
            for (const auto& field : fields_) {
                if (field.parentIndex == i) {
                    writeField(fp, field);
                }
            }

            // Write methods for this type
            for (const auto& method : methods_) {
                if (method.declaringTypeIndex == i) {
                    writeMethod(fp, method);
                }
            }

            fprintf(fp, "    }\n\n");
        }
    }

    std::fclose(fp);
    LOGI("Dump written to: %s", outputPath);
    return true;
}

int Il2CppDumper::dump(const char* outputPath) {
    if (!initialized_) {
        LOGE("Dumper not initialized");
        return -1;
    }
    
    LOGI("Starting dump to: %s", outputPath);
    
    if (!writeDump(outputPath)) {
        LOGE("Failed to write dump");
        return -2;
    }
    
    LOGI("Dump completed successfully");
    return 0;
}

// Global instance
static Il2CppDumper g_dumper;

extern "C" {

JNIEXPORT jint JNICALL
Java_com_libcpp_dumper_MainActivity_initDumper(JNIEnv* env, jobject /* this */, 
                                                jstring soPath, jstring metadataPath) {
    const char* so = env->GetStringUTFChars(soPath, nullptr);
    const char* metadata = env->GetStringUTFChars(metadataPath, nullptr);
    
    int result = g_dumper.initialize(so, metadata);
    
    env->ReleaseStringUTFChars(soPath, so);
    env->ReleaseStringUTFChars(metadataPath, metadata);
    
    return result;
}

JNIEXPORT jint JNICALL
Java_com_libcpp_dumper_MainActivity_startDump(JNIEnv* env, jobject /* this */, 
                                               jstring outputPath) {
    const char* output = env->GetStringUTFChars(outputPath, nullptr);
    
    int result = g_dumper.dump(output);
    
    env->ReleaseStringUTFChars(outputPath, output);
    
    return result;
}

JNIEXPORT jstring JNICALL
Java_com_libcpp_dumper_MainActivity_getUnityVersion(JNIEnv* env, jobject /* this */) {
    return env->NewStringUTF(g_dumper.getUnityVersion());
}

JNIEXPORT jint JNICALL
Java_com_libcpp_dumper_MainActivity_getAssemblyCount(JNIEnv* env, jobject /* this */) {
    return g_dumper.getAssemblyCount();
}

JNIEXPORT jint JNICALL
Java_com_libcpp_dumper_MainActivity_getTypeCount(JNIEnv* env, jobject /* this */) {
    return g_dumper.getTypeCount();
}

JNIEXPORT jint JNICALL
Java_com_libcpp_dumper_MainActivity_getMethodCount(JNIEnv* env, jobject /* this */) {
    return g_dumper.getMethodCount();
}

JNIEXPORT jint JNICALL
Java_com_libcpp_dumper_MainActivity_getFieldCount(JNIEnv* env, jobject /* this */) {
    return g_dumper.getFieldCount();
}

JNIEXPORT void JNICALL
Java_com_libcpp_dumper_MainActivity_cleanup(JNIEnv* env, jobject /* this */) {
    g_dumper.cleanup();
}

}