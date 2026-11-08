#include "il2cpp_dumper.h"
#include <fstream>
#include <cstring>
#include <algorithm>
#include <sstream>
#include <dlfcn.h>

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
    header_ = nullptr;
    strings_.clear();
    stringLiteralData_.clear();
    assemblies_.clear();
    images_.clear();
    typeDefs_.clear();
    fieldDefs_.clear();
    methodDefs_.clear();
    propertyDefs_.clear();
    eventDefs_.clear();
    parameterDefs_.clear();
    
    if (il2cppHandle_) {
        dlclose(il2cppHandle_);
        il2cppHandle_ = nullptr;
    }
    metadataRegistration_ = nullptr;
    il2cppTypes_ = nullptr;
    il2cppTypesCount_ = 0;
    
    initialized_ = false;
    LOGI("Cleanup completed");
}

bool Il2CppDumper::loadIl2CppSo(const char* soPath) {
    il2cppHandle_ = dlopen(soPath, RTLD_LAZY | RTLD_GLOBAL);
    if (!il2cppHandle_) {
        LOGE("Failed to load libil2cpp.so: %s", dlerror());
        return false;
    }
    LOGI("Loaded libil2cpp.so successfully");
    return true;
}

bool Il2CppDumper::findMetadataRegistration() {
    const char* symbols[] = {
        "g_MetadataRegistration",
        "il2cpp_g_MetadataRegistration",
        "s_MetadataRegistration",
        "metadataRegistration"
    };
    
    for (const char* sym : symbols) {
        metadataRegistration_ = reinterpret_cast<const Il2CppMetadataRegistration*>(
            dlsym(il2cppHandle_, sym)
        );
        if (metadataRegistration_) {
            LOGI("Found MetadataRegistration via symbol: %s", sym);
            break;
        }
    }
    
    if (!metadataRegistration_) {
        LOGE("Failed to find MetadataRegistration symbol");
        return false;
    }
    
    LOGI("MetadataRegistration: typesCount=%d", metadataRegistration_->typesCount);
    return true;
}

bool Il2CppDumper::parseIl2CppTypes() {
    if (!metadataRegistration_ || metadataRegistration_->typesCount == 0) {
        LOGE("No types in metadata registration");
        return false;
    }
    
    il2cppTypes_ = metadataRegistration_->types;
    il2cppTypesCount_ = metadataRegistration_->typesCount;
    
    LOGI("Loaded %d Il2CppTypes from libil2cpp.so", il2cppTypesCount_);
    return true;
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

    if (metadataSize_ < 128) {
        LOGE("Metadata file too small");
        return false;
    }

    uint32_t* header = reinterpret_cast<uint32_t*>(metadataBuffer_);
    if (header[0] != 0xFAB11BAF && header[0] != 0xAF11B1FA) {
        LOGW("Unknown metadata signature: 0x%08X", header[0]);
    }

    metadataVersion_ = header[1];
    
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
    stringLiteralDataCount_ = header[17];

    header_ = reinterpret_cast<Il2CppGlobalMetadataHeader*>(metadataBuffer_);

    LOGI("Metadata version: %u", metadataVersion_);
    LOGI("Types: %u at offset %u", typeDefinitionsCount_, typeDefinitionsOffset_);
    LOGI("Fields: %u at offset %u", fieldDefinitionsCount_, fieldDefinitionsOffset_);
    LOGI("Methods: %u at offset %u", methodDefinitionsCount_, methodDefinitionsOffset_);
    LOGI("Assemblies: %u at offset %u", assemblyDefinitionsCount_, assemblyDefinitionsOffset_);

    if (!parseStrings()) return false;
    if (!parseStringLiterals()) return false;
    if (!parseAssemblies()) return false;
    if (!parseImages()) return false;
    if (!parseTypeDefinitions()) return false;
    if (!parseFieldDefinitions()) return false;
    if (!parseMethodDefinitions()) return false;
    if (!parsePropertyDefinitions()) return false;
    if (!parseParameterDefinitions()) return false;

    adjustForVersion();

    LOGI("Parsed: strings=%zu, assemblies=%zu, images=%zu, types=%zu, methods=%zu, fields=%zu, properties=%zu",
         strings_.size(), assemblies_.size(), images_.size(), typeDefs_.size(),
         methodDefs_.size(), fieldDefs_.size(), propertyDefs_.size());

    return true;
}

bool Il2CppDumper::parseStrings() {
    if (header_->stringOffset == 0 || header_->stringCount == 0) {
        LOGW("No string table found");
        return true;
    }

    if (header_->stringOffset + header_->stringCount > metadataSize_) {
        LOGE("String table out of bounds");
        return false;
    }

    const char* stringData = reinterpret_cast<const char*>(metadataBuffer_ + header_->stringOffset);
    size_t pos = 0;
    for (int i = 0; i < header_->stringCount; ++i) {
        if (pos >= static_cast<size_t>(header_->stringCount)) break;
        const char* str = stringData + pos;
        strings_.push_back(str);
        pos += strlen(str) + 1;
        if (pos > static_cast<size_t>(header_->stringCount)) break;
    }

    LOGI("Parsed %zu strings", strings_.size());
    return true;
}

bool Il2CppDumper::parseStringLiterals() {
    if (header_->stringLiteralOffset == 0 || header_->stringLiteralCount == 0) {
        return true;
    }

    if (header_->stringLiteralDataOffset == 0 || header_->stringLiteralDataCount == 0) {
        return true;
    }

    const char* data = reinterpret_cast<const char*>(metadataBuffer_ + header_->stringLiteralDataOffset);
    stringLiteralData_.reserve(header_->stringLiteralCount);
    
    const Il2CppStringLiteral* literals = reinterpret_cast<const Il2CppStringLiteral*>(
        metadataBuffer_ + header_->stringLiteralOffset
    );
    
    for (int i = 0; i < header_->stringLiteralCount; ++i) {
        const Il2CppStringLiteral& lit = literals[i];
        if (lit.dataIndex < static_cast<uint32_t>(header_->stringLiteralDataCount)) {
            stringLiteralData_.push_back(data + lit.dataIndex);
        } else {
            stringLiteralData_.push_back("");
        }
    }

    LOGI("Parsed %zu string literals", stringLiteralData_.size());
    return true;
}

bool Il2CppDumper::parseAssemblies() {
    if (assemblyDefinitionsOffset_ == 0 || assemblyDefinitionsCount_ == 0) {
        return true;
    }

    const Il2CppAssemblyDefinition* asmDefs = 
        reinterpret_cast<const Il2CppAssemblyDefinition*>(metadataBuffer_ + assemblyDefinitionsOffset_);
    assemblies_.assign(asmDefs, asmDefs + assemblyDefinitionsCount_);

    LOGI("Parsed %zu assemblies", assemblies_.size());
    return true;
}

bool Il2CppDumper::parseImages() {
    if (imageDefinitionsOffset_ == 0 || imageDefinitionsCount_ == 0) {
        return true;
    }

    const Il2CppImageDefinition* imgDefs = 
        reinterpret_cast<const Il2CppImageDefinition*>(metadataBuffer_ + imageDefinitionsOffset_);
    images_.assign(imgDefs, imgDefs + imageDefinitionsCount_);

    LOGI("Parsed %zu images", images_.size());
    return true;
}

bool Il2CppDumper::parseTypeDefinitions() {
    if (typeDefinitionsOffset_ == 0 || typeDefinitionsCount_ == 0) {
        LOGE("No type definitions table");
        return false;
    }

    const Il2CppTypeDefinition* typeDefs = 
        reinterpret_cast<const Il2CppTypeDefinition*>(metadataBuffer_ + typeDefinitionsOffset_);
    typeDefs_.assign(typeDefs, typeDefs + typeDefinitionsCount_);

    LOGI("Parsed %zu type definitions", typeDefs_.size());
    return true;
}

bool Il2CppDumper::parseFieldDefinitions() {
    if (fieldDefinitionsOffset_ == 0 || fieldDefinitionsCount_ == 0) {
        return true;
    }

    const Il2CppFieldDefinition* fieldDefs = 
        reinterpret_cast<const Il2CppFieldDefinition*>(metadataBuffer_ + fieldDefinitionsOffset_);
    fieldDefs_.assign(fieldDefs, fieldDefs + fieldDefinitionsCount_);

    LOGI("Parsed %zu field definitions", fieldDefs_.size());
    return true;
}

bool Il2CppDumper::parseMethodDefinitions() {
    if (methodDefinitionsOffset_ == 0 || methodDefinitionsCount_ == 0) {
        return true;
    }

    const Il2CppMethodDefinition* methodDefs = 
        reinterpret_cast<const Il2CppMethodDefinition*>(metadataBuffer_ + methodDefinitionsOffset_);
    methodDefs_.assign(methodDefs, methodDefs + methodDefinitionsCount_);

    LOGI("Parsed %zu method definitions", methodDefs_.size());
    return true;
}

bool Il2CppDumper::parsePropertyDefinitions() {
    if (propertyDefinitionsOffset_ == 0 || propertyDefinitionsCount_ == 0) {
        return true;
    }

    const Il2CppPropertyDefinition* propDefs = 
        reinterpret_cast<const Il2CppPropertyDefinition*>(metadataBuffer_ + propertyDefinitionsOffset_);
    propertyDefs_.assign(propDefs, propDefs + propertyDefinitionsCount_);

    LOGI("Parsed %zu property definitions", propertyDefs_.size());
    return true;
}

bool Il2CppDumper::parseParameterDefinitions() {
    if (header_->parametersOffset == 0 || header_->parametersCount == 0) {
        return true;
    }

    const Il2CppParameterDefinition* paramDefs = 
        reinterpret_cast<const Il2CppParameterDefinition*>(metadataBuffer_ + header_->parametersOffset);
    parameterDefs_.assign(paramDefs, paramDefs + header_->parametersCount);

    LOGI("Parsed %zu parameter definitions", parameterDefs_.size());
    return true;
}

void Il2CppDumper::adjustForVersion() {
    for (const auto& str : strings_) {
        if (str.find("Unity") != std::string::npos && str.find("20") != std::string::npos) {
            unityVersion_ = str;
            break;
        }
    }
    
    if (unityVersion_.empty()) {
        unityVersion_ = "Unknown (metadata version " + std::to_string(metadataVersion_) + ")";
    }
}

bool Il2CppDumper::initialize(const char* soPath, const char* metadataPath) {
    LOGI("Initializing dumper with SO: %s, Metadata: %s", soPath, metadataPath);
    
    cleanup();
    
    if (!loadIl2CppSo(soPath)) {
        return -1;
    }
    
    if (!findMetadataRegistration()) {
        return -2;
    }
    
    if (!parseIl2CppTypes()) {
        return -3;
    }
    
    if (!parseMetadata(metadataPath)) {
        return -4;
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
    return static_cast<int>(typeDefs_.size());
}

int Il2CppDumper::getMethodCount() const {
    return static_cast<int>(methodDefs_.size());
}

int Il2CppDumper::getFieldCount() const {
    return static_cast<int>(fieldDefs_.size());
}

const char* Il2CppDumper::getString(uint32_t index) const {
    if (index < strings_.size()) {
        return strings_[index].c_str();
    }
    return "";
}

const char* Il2CppDumper::getStringLiteral(uint32_t index) const {
    if (index < stringLiteralData_.size()) {
        return stringLiteralData_[index];
    }
    return "";
}

const Il2CppTypeDefinition* Il2CppDumper::getTypeDefFromIndex(TypeDefinitionIndex index) const {
    if (index >= 0 && static_cast<size_t>(index) < typeDefs_.size()) {
        return &typeDefs_[index];
    }
    return nullptr;
}

std::string Il2CppDumper::resolveType(const Il2CppType* type, bool addNamespace) const {
    if (!type) return "void";
    
    switch (type->type) {
        case IL2CPP_TYPE_VOID: return "void";
        case IL2CPP_TYPE_BOOLEAN: return "bool";
        case IL2CPP_TYPE_CHAR: return "char";
        case IL2CPP_TYPE_I1: return "sbyte";
        case IL2CPP_TYPE_U1: return "byte";
        case IL2CPP_TYPE_I2: return "short";
        case IL2CPP_TYPE_U2: return "ushort";
        case IL2CPP_TYPE_I4: return "int";
        case IL2CPP_TYPE_U4: return "uint";
        case IL2CPP_TYPE_I8: return "long";
        case IL2CPP_TYPE_U8: return "ulong";
        case IL2CPP_TYPE_R4: return "float";
        case IL2CPP_TYPE_R8: return "double";
        case IL2CPP_TYPE_STRING: return "string";
        case IL2CPP_TYPE_OBJECT: return "object";
        
        case IL2CPP_TYPE_PTR: {
            std::string inner = resolveType(type->data.type, addNamespace);
            return inner + "*";
        }
        
        case IL2CPP_TYPE_BYREF: {
            std::string inner = resolveType(type->data.type, addNamespace);
            return inner + "&";
        }
        
        case IL2CPP_TYPE_SZARRAY: {
            std::string inner = resolveType(type->data.type, addNamespace);
            return inner + "[]";
        }
        
        case IL2CPP_TYPE_ARRAY: {
            // Array type - simplified
            std::string inner = resolveType(type->data.type, addNamespace);
            return inner + "[]";
        }
        
        case IL2CPP_TYPE_VALUETYPE:
        case IL2CPP_TYPE_CLASS: {
            TypeDefinitionIndex klassIndex = type->data.klassIndex;
            if (klassIndex != kTypeIndexInvalid && klassIndex >= 0 && 
                static_cast<size_t>(klassIndex) < typeDefs_.size()) {
                const Il2CppTypeDefinition* typeDef = &typeDefs_[klassIndex];
                return getTypeNameFromDefinition(typeDef);
            }
            return "unknown_type";
        }
        
        case IL2CPP_TYPE_GENERICINST: {
            return "generic_instance";
        }
        
        case IL2CPP_TYPE_VAR:
        case IL2CPP_TYPE_MVAR: {
            GenericParameterIndex paramIdx = type->data.genericParameterIndex;
            return "T" + std::to_string(paramIdx);
        }
        
        default:
            return "type_" + std::to_string(type->type);
    }
}

std::string Il2CppDumper::getTypeName(TypeIndex typeIndex) const {
    if (typeIndex == kTypeIndexInvalid || typeIndex < 0) {
        return "void";
    }
    
    if (il2cppTypes_ && typeIndex < il2cppTypesCount_) {
        const Il2CppType* il2cppType = il2cppTypes_[typeIndex];
        if (il2cppType) {
            return resolveType(il2cppType, true);
        }
    }
    
    // Fallback to primitive types
    switch (typeIndex) {
        case IL2CPP_TYPE_VOID: return "void";
        case IL2CPP_TYPE_BOOLEAN: return "bool";
        case IL2CPP_TYPE_CHAR: return "char";
        case IL2CPP_TYPE_I1: return "sbyte";
        case IL2CPP_TYPE_U1: return "byte";
        case IL2CPP_TYPE_I2: return "short";
        case IL2CPP_TYPE_U2: return "ushort";
        case IL2CPP_TYPE_I4: return "int";
        case IL2CPP_TYPE_U4: return "uint";
        case IL2CPP_TYPE_I8: return "long";
        case IL2CPP_TYPE_U8: return "ulong";
        case IL2CPP_TYPE_R4: return "float";
        case IL2CPP_TYPE_R8: return "double";
        case IL2CPP_TYPE_STRING: return "string";
        case IL2CPP_TYPE_OBJECT: return "object";
        default: return "type_" + std::to_string(typeIndex);
    }
}

std::string Il2CppDumper::getTypeNameFromDefinition(const Il2CppTypeDefinition* typeDef) const {
    const char* name = getString(typeDef->nameIndex);
    const char* ns = getString(typeDef->namespaceIndex);
    
    std::string result;
    if (ns && strlen(ns) > 0) {
        result = ns;
        result += ".";
    }
    result += name;
    return result;
}

std::string Il2CppDumper::getFieldTypeName(int fieldIndex) const {
    if (fieldIndex < 0 || fieldIndex >= static_cast<int>(fieldDefs_.size())) {
        return "unknown";
    }
    return getTypeName(fieldDefs_[fieldIndex].typeIndex);
}

std::string Il2CppDumper::getMethodSignature(int methodIndex) const {
    if (methodIndex < 0 || methodIndex >= static_cast<int>(methodDefs_.size())) {
        return "/* invalid method */";
    }
    
    const auto& method = methodDefs_[methodIndex];
    const char* name = getString(method.nameIndex);
    std::string returnType = getTypeName(method.returnType);
    
    std::string sig = returnType + " " + name + "(";
    
    if (method.parameterCount > 0 && method.parameterStart >= 0) {
        for (uint16_t i = 0; i < method.parameterCount; ++i) {
            int paramIdx = method.parameterStart + i;
            if (paramIdx >= 0 && paramIdx < static_cast<int>(parameterDefs_.size())) {
                const auto& param = parameterDefs_[paramIdx];
                const char* paramName = getString(param.nameIndex);
                std::string paramType = getTypeName(param.typeIndex);
                
                if (i > 0) sig += ", ";
                sig += paramType + " " + paramName;
            }
        }
    }
    
    sig += ")";
    return sig;
}

bool Il2CppDumper::isValueType(const Il2CppTypeDefinition* typeDef) const {
    return (typeDef->flags & 0x10000000) != 0;
}

bool Il2CppDumper::isEnum(const Il2CppTypeDefinition* typeDef) const {
    return (typeDef->flags & 0x20000000) != 0;
}

bool Il2CppDumper::writeDump(const char* outputPath) {
    FILE* fp = fopen(outputPath, "w");
    if (!fp) {
        LOGE("Failed to open output file: %s", outputPath);
        return false;
    }

    fprintf(fp, "// IL2CPP Dump generated by LibCpp Unity Dumper\n");
    fprintf(fp, "// Unity Version: %s\n", unityVersion_.c_str());
    fprintf(fp, "// Assemblies: %zu, Types: %zu, Methods: %zu, Fields: %zu, Properties: %zu\n\n",
            assemblies_.size(), typeDefs_.size(), methodDefs_.size(), fieldDefs_.size(), propertyDefs_.size());

    for (size_t imgIdx = 0; imgIdx < images_.size(); ++imgIdx) {
        const auto& image = images_[imgIdx];
        const char* imgName = getString(image.nameIndex);
        
        if (imgName && strlen(imgName) > 0) {
            fprintf(fp, "// ===== Image: %s =====\n", imgName);
        }

        for (const auto& assembly : assemblies_) {
            if (assembly.imageIndex == imgIdx) {
                writeAssembly(fp, assembly);
            }
        }

        uint32_t typeEnd = image.typeStart + image.typeCount;
        for (uint32_t i = image.typeStart; i < typeEnd && i < typeDefs_.size(); ++i) {
            writeType(fp, static_cast<int>(i));
        }
        
        fprintf(fp, "\n");
    }

    fclose(fp);
    LOGI("Dump written to: %s", outputPath);
    return true;
}

void Il2CppDumper::writeAssembly(FILE* fp, const Il2CppAssemblyDefinition& assembly) {
    const char* name = getString(assembly.nameIndex);
    fprintf(fp, "// Assembly: %s (Token: 0x%08X)\n", name, assembly.token);
    fprintf(fp, "// Image Index: %u\n\n", assembly.imageIndex);
}

void Il2CppDumper::writeType(FILE* fp, int typeIndex) {
    if (typeIndex < 0 || typeIndex >= static_cast<int>(typeDefs_.size())) return;
    
    const auto& typeDef = typeDefs_[typeIndex];
    const char* name = getString(typeDef.nameIndex);
    const char* ns = getString(typeDef.namespaceIndex);
    
    if (ns && strlen(ns) > 0) {
        fprintf(fp, "namespace %s\n", ns);
        fprintf(fp, "{\n");
    }
    
    const char* kind = "class";
    if (isValueType(&typeDef)) {
        if (isEnum(&typeDef)) kind = "enum";
        else kind = "struct";
    } else if ((typeDef.flags & 0x20) != 0) {
        kind = "interface";
    }
    
    fprintf(fp, "// Type: %s (Token: 0x%08X, Flags: 0x%08X)\n", name, typeDef.token, typeDef.flags);
    fprintf(fp, "%s %s\n", kind, name);
    fprintf(fp, "{\n");
    
    for (uint16_t i = 0; i < typeDef.field_count; ++i) {
        int fieldIdx = typeDef.fieldStart + i;
        if (fieldIdx >= 0 && fieldIdx < static_cast<int>(fieldDefs_.size())) {
            writeField(fp, fieldIdx);
        }
    }
    
    for (uint16_t i = 0; i < typeDef.property_count; ++i) {
        int propIdx = typeDef.propertyStart + i;
        if (propIdx >= 0 && propIdx < static_cast<int>(propertyDefs_.size())) {
            writeProperty(fp, propIdx);
        }
    }
    
    for (uint16_t i = 0; i < typeDef.event_count; ++i) {
        int eventIdx = typeDef.eventStart + i;
        if (eventIdx >= 0 && eventIdx < static_cast<int>(eventDefs_.size())) {
            writeEvent(fp, eventIdx);
        }
    }
    
    for (uint16_t i = 0; i < typeDef.method_count; ++i) {
        int methodIdx = typeDef.methodStart + i;
        if (methodIdx >= 0 && methodIdx < static_cast<int>(methodDefs_.size())) {
            writeMethod(fp, methodIdx);
        }
    }
    
    fprintf(fp, "}\n\n");
    
    if (ns && strlen(ns) > 0) {
        fprintf(fp, "}\n\n");
    }
}

void Il2CppDumper::writeMethod(FILE* fp, int methodIndex) {
    if (methodIndex < 0 || methodIndex >= static_cast<int>(methodDefs_.size())) return;
    
    std::string signature = getMethodSignature(methodIndex);
    const auto& method = methodDefs_[methodIndex];
    
    fprintf(fp, "    // Method: Token: 0x%08X, Flags: 0x%04X, Slot: %u\n", 
            method.token, method.flags, method.slot);
    fprintf(fp, "    %s;\n\n", signature.c_str());
}

void Il2CppDumper::writeField(FILE* fp, int fieldIndex) {
    if (fieldIndex < 0 || fieldIndex >= static_cast<int>(fieldDefs_.size())) return;
    
    const auto& field = fieldDefs_[fieldIndex];
    const char* name = getString(field.nameIndex);
    std::string typeName = getFieldTypeName(fieldIndex);
    
    // Get field offset from metadata registration
    uint32_t offset = 0;
    if (metadataRegistration_ && metadataRegistration_->fieldOffsetsCount > fieldIndex) {
        offset = metadataRegistration_->fieldOffsets[fieldIndex];
    }
    
    fprintf(fp, "    // Field: Token: 0x%08X, Offset: %u\n", field.token, offset);
    fprintf(fp, "    %s %s;\n", typeName.c_str(), name);
}

void Il2CppDumper::writeProperty(FILE* fp, int propertyIndex) {
    if (propertyIndex < 0 || propertyIndex >= static_cast<int>(propertyDefs_.size())) return;
    
    const auto& prop = propertyDefs_[propertyIndex];
    const char* name = getString(prop.nameIndex);
    std::string typeName = getTypeName(prop.typeIndex);
    
    fprintf(fp, "    // Property: Token: 0x%08X\n", prop.token);
    fprintf(fp, "    %s %s { get; set; }\n", typeName.c_str(), name);
}

void Il2CppDumper::writeEvent(FILE* fp, int eventIndex) {
    if (eventIndex < 0 || eventIndex >= static_cast<int>(eventDefs_.size())) return;
    
    const auto& event = eventDefs_[eventIndex];
    const char* name = getString(event.nameIndex);
    std::string typeName = getTypeName(event.typeIndex);
    
    fprintf(fp, "    // Event: Token: 0x%08X\n", event.token);
    fprintf(fp, "    event %s %s;\n", typeName.c_str(), name);
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