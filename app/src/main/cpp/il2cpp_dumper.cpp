#include "il2cpp_dumper.h"
#include <fstream>
#include <cstring>
#include <algorithm>
#include <sstream>
#include <cstdio>

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
    stringLiterals_.clear();
    stringLiteralData_.clear();
    assemblies_.clear();
    images_.clear();
    typeDefs_.clear();
    methodDefs_.clear();
    fieldDefs_.clear();
    propertyDefs_.clear();
    eventDefs_.clear();
    parameterDefs_.clear();
    genericContainers_.clear();
    genericParameters_.clear();
    nestedTypes_.clear();
    interfaces_.clear();
    vtableMethods_.clear();
    interfaceOffsets_.clear();
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

    if (metadataSize_ < sizeof(Il2CppGlobalMetadataHeader)) {
        LOGE("Metadata file too small for header");
        return false;
    }

    header_ = reinterpret_cast<Il2CppGlobalMetadataHeader*>(metadataBuffer_);
    
    // Check sanity (magic number)
    if (header_->sanity != 0xFAB11BAF) {
        LOGE("Invalid metadata signature: 0x%08X (expected 0xFAB11BAF)", header_->sanity);
        return false;
    }

    metadataVersion_ = header_->version;
    LOGI("Metadata version: %d", metadataVersion_);

    if (metadataVersion_ < 16 || metadataVersion_ > 31) {
        LOGE("Unsupported metadata version: %d (supported: 16-31)", metadataVersion_);
        return false;
    }

    // Version flags
    isVersion24Plus_ = metadataVersion_ >= 24;
    isVersion27Plus_ = metadataVersion_ >= 27;
    isVersion29Plus_ = metadataVersion_ >= 29;

    // Parse all tables
    if (!parseStrings()) return false;
    if (!parseStringLiterals()) return false;
    if (!parseAssemblies()) return false;
    if (!parseImages()) return false;
    if (!parseTypeDefinitions()) return false;
    if (!parseMethodDefinitions()) return false;
    if (!parseFieldDefinitions()) return false;
    if (!parsePropertyDefinitions()) return false;
    if (!parseEventDefinitions()) return false;
    if (!parseParameterDefinitions()) return false;
    if (!parseGenericContainers()) return false;
    if (!parseGenericParameters()) return false;
    
    adjustForVersion();

    LOGI("Parsed: %zu strings, %zu assemblies, %zu images, %zu types, %zu methods, %zu fields, %zu properties, %zu events",
         strings_.size(), assemblies_.size(), images_.size(), typeDefs_.size(),
         methodDefs_.size(), fieldDefs_.size(), propertyDefs_.size(), eventDefs_.size());

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
        LOGI("No string literals table");
        return true;
    }

    if (header_->stringLiteralOffset + header_->stringLiteralCount * sizeof(Il2CppStringLiteral) > metadataSize_) {
        LOGE("String literals table out of bounds");
        return false;
    }

    const Il2CppStringLiteral* literals = reinterpret_cast<const Il2CppStringLiteral*>(
        metadataBuffer_ + header_->stringLiteralOffset
    );

    stringLiterals_.assign(literals, literals + header_->stringLiteralCount);

    // Parse string literal data
    if (header_->stringLiteralDataOffset > 0 && header_->stringLiteralDataCount > 0) {
        const char* data = reinterpret_cast<const char*>(metadataBuffer_ + header_->stringLiteralDataOffset);
        stringLiteralData_.reserve(header_->stringLiteralCount);
        
        for (const auto& lit : stringLiterals_) {
            if (lit.dataIndex < static_cast<uint32_t>(header_->stringLiteralDataCount)) {
                stringLiteralData_.push_back(data + lit.dataIndex);
            } else {
                stringLiteralData_.push_back("");
            }
        }
    }

    LOGI("Parsed %zu string literals", stringLiterals_.size());
    return true;
}

bool Il2CppDumper::parseAssemblies() {
    if (header_->assembliesOffset == 0 || header_->assembliesCount == 0) {
        LOGW("No assemblies table");
        return true;
    }

    size_t assemblySize = isVersion27Plus_ ? 16 : 12;
    if (header_->assembliesOffset + header_->assembliesCount * assemblySize > metadataSize_) {
        LOGE("Assemblies table out of bounds");
        return false;
    }

    const Il2CppAssemblyDefinition* asmDefs = reinterpret_cast<const Il2CppAssemblyDefinition*>(
        metadataBuffer_ + header_->assembliesOffset
    );
    assemblies_.assign(asmDefs, asmDefs + header_->assembliesCount);

    LOGI("Parsed %zu assemblies", assemblies_.size());
    return true;
}

bool Il2CppDumper::parseImages() {
    if (header_->imagesOffset == 0 || header_->imagesCount == 0) {
        LOGW("No images table");
        return true;
    }

    if (header_->imagesOffset + header_->imagesCount * sizeof(Il2CppImageDefinition) > metadataSize_) {
        LOGE("Images table out of bounds");
        return false;
    }

    const Il2CppImageDefinition* imgDefs = reinterpret_cast<const Il2CppImageDefinition*>(
        metadataBuffer_ + header_->imagesOffset
    );
    images_.assign(imgDefs, imgDefs + header_->imagesCount);

    LOGI("Parsed %zu images", images_.size());
    return true;
}

bool Il2CppDumper::parseTypeDefinitions() {
    if (header_->typeDefinitionsOffset == 0 || header_->typeDefinitionsCount == 0) {
        LOGE("No type definitions table");
        return false;
    }

    if (header_->typeDefinitionsOffset + header_->typeDefinitionsCount * sizeof(Il2CppTypeDefinition) > metadataSize_) {
        LOGE("Type definitions table out of bounds");
        return false;
    }

    const Il2CppTypeDefinition* typeDefs = reinterpret_cast<const Il2CppTypeDefinition*>(
        metadataBuffer_ + header_->typeDefinitionsOffset
    );
    typeDefs_.assign(typeDefs, typeDefs + header_->typeDefinitionsCount);

    LOGI("Parsed %zu type definitions", typeDefs_.size());
    return true;
}

bool Il2CppDumper::parseMethodDefinitions() {
    if (header_->methodsOffset == 0 || header_->methodsCount == 0) {
        LOGW("No methods table");
        return true;
    }

    if (header_->methodsOffset + header_->methodsCount * sizeof(Il2CppMethodDefinition) > metadataSize_) {
        LOGE("Methods table out of bounds");
        return false;
    }

    const Il2CppMethodDefinition* methodDefs = reinterpret_cast<const Il2CppMethodDefinition*>(
        metadataBuffer_ + header_->methodsOffset
    );
    methodDefs_.assign(methodDefs, methodDefs + header_->methodsCount);

    LOGI("Parsed %zu method definitions", methodDefs_.size());
    return true;
}

bool Il2CppDumper::parseFieldDefinitions() {
    if (header_->fieldsOffset == 0 || header_->fieldsCount == 0) {
        LOGW("No fields table");
        return true;
    }

    if (header_->fieldsOffset + header_->fieldsCount * sizeof(Il2CppFieldDefinition) > metadataSize_) {
        LOGE("Fields table out of bounds");
        return false;
    }

    const Il2CppFieldDefinition* fieldDefs = reinterpret_cast<const Il2CppFieldDefinition*>(
        metadataBuffer_ + header_->fieldsOffset
    );
    fieldDefs_.assign(fieldDefs, fieldDefs + header_->fieldsCount);

    LOGI("Parsed %zu field definitions", fieldDefs_.size());
    return true;
}

bool Il2CppDumper::parsePropertyDefinitions() {
    if (header_->propertiesOffset == 0 || header_->propertiesCount == 0) {
        LOGI("No properties table");
        return true;
    }

    if (header_->propertiesOffset + header_->propertiesCount * sizeof(Il2CppPropertyDefinition) > metadataSize_) {
        LOGE("Properties table out of bounds");
        return false;
    }

    const Il2CppPropertyDefinition* propDefs = reinterpret_cast<const Il2CppPropertyDefinition*>(
        metadataBuffer_ + header_->propertiesOffset
    );
    propertyDefs_.assign(propDefs, propDefs + header_->propertiesCount);

    LOGI("Parsed %zu property definitions", propertyDefs_.size());
    return true;
}

bool Il2CppDumper::parseEventDefinitions() {
    if (header_->eventsOffset == 0 || header_->eventsCount == 0) {
        LOGI("No events table");
        return true;
    }

    if (header_->eventsOffset + header_->eventsCount * sizeof(Il2CppEventDefinition) > metadataSize_) {
        LOGE("Events table out of bounds");
        return false;
    }

    const Il2CppEventDefinition* eventDefs = reinterpret_cast<const Il2CppEventDefinition*>(
        metadataBuffer_ + header_->eventsOffset
    );
    eventDefs_.assign(eventDefs, eventDefs + header_->eventsCount);

    LOGI("Parsed %zu event definitions", eventDefs_.size());
    return true;
}

bool Il2CppDumper::parseParameterDefinitions() {
    if (header_->parametersOffset == 0 || header_->parametersCount == 0) {
        LOGI("No parameters table");
        return true;
    }

    if (header_->parametersOffset + header_->parametersCount * sizeof(Il2CppParameterDefinition) > metadataSize_) {
        LOGE("Parameters table out of bounds");
        return false;
    }

    const Il2CppParameterDefinition* paramDefs = reinterpret_cast<const Il2CppParameterDefinition*>(
        metadataBuffer_ + header_->parametersOffset
    );
    parameterDefs_.assign(paramDefs, paramDefs + header_->parametersCount);

    LOGI("Parsed %zu parameter definitions", parameterDefs_.size());
    return true;
}

bool Il2CppDumper::parseGenericContainers() {
    if (header_->genericContainersOffset == 0 || header_->genericContainersCount == 0) {
        LOGI("No generic containers table");
        return true;
    }

    if (header_->genericContainersOffset + header_->genericContainersCount * sizeof(Il2CppGenericContainer) > metadataSize_) {
        LOGE("Generic containers table out of bounds");
        return false;
    }

    const Il2CppGenericContainer* containers = reinterpret_cast<const Il2CppGenericContainer*>(
        metadataBuffer_ + header_->genericContainersOffset
    );
    genericContainers_.assign(containers, containers + header_->genericContainersCount);

    LOGI("Parsed %zu generic containers", genericContainers_.size());
    return true;
}

bool Il2CppDumper::parseGenericParameters() {
    if (header_->genericParametersOffset == 0 || header_->genericParametersCount == 0) {
        LOGI("No generic parameters table");
        return true;
    }

    if (header_->genericParametersOffset + header_->genericParametersCount * sizeof(Il2CppGenericParameter) > metadataSize_) {
        LOGE("Generic parameters table out of bounds");
        return false;
    }

    const Il2CppGenericParameter* params = reinterpret_cast<const Il2CppGenericParameter*>(
        metadataBuffer_ + header_->genericParametersOffset
    );
    genericParameters_.assign(params, params + header_->genericParametersCount);

    LOGI("Parsed %zu generic parameters", genericParameters_.size());
    return true;
}

void Il2CppDumper::adjustForVersion() {
    // Try to detect Unity version from string literals
    for (const auto& str : strings_) {
        if (str.find("Unity") != std::string::npos && str.find("20") != std::string::npos) {
            unityVersion_ = str;
            break;
        }
    }
    
    if (unityVersion_.empty()) {
        // Try string literals
        for (const auto& data : stringLiteralData_) {
            if (data && strstr(data, "Unity") && strstr(data, "20")) {
                unityVersion_ = data;
                break;
            }
        }
    }
    
    if (unityVersion_.empty()) {
        unityVersion_ = "Unknown (metadata version " + std::to_string(metadataVersion_) + ")";
    }
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

bool Il2CppDumper::isValueType(const Il2CppTypeDefinition* typeDef) const {
    return (typeDef->bitfield & 1) != 0;
}

bool Il2CppDumper::isEnum(const Il2CppTypeDefinition* typeDef) const {
    return (typeDef->bitfield & 2) != 0;
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

std::string Il2CppDumper::getTypeName(TypeIndex typeIndex) const {
    if (typeIndex == kTypeIndexInvalid) return "void";
    
    // For now, we need to resolve through type definitions
    // This is simplified - real implementation would use Il2CppType from metadata registration
    if (typeIndex >= 0 && typeIndex < static_cast<int>(typeDefs_.size())) {
        return getTypeNameFromDefinition(&typeDefs_[typeIndex]);
    }
    
    // Handle primitive types based on Il2CppTypeEnum
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

std::string Il2CppDumper::getMethodSignature(int methodIndex) const {
    if (methodIndex < 0 || methodIndex >= static_cast<int>(methodDefs_.size())) {
        return "/* invalid method */";
    }
    
    const auto& method = methodDefs_[methodIndex];
    const char* name = getString(method.nameIndex);
    std::string returnType = getTypeName(method.returnType);
    
    std::string sig = returnType + " " + name + "(";
    
    // Parameters
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

std::string Il2CppDumper::getFieldTypeName(int fieldIndex) const {
    if (fieldIndex < 0 || fieldIndex >= static_cast<int>(fieldDefs_.size())) {
        return "/* invalid field */";
    }
    return getTypeName(fieldDefs_[fieldIndex].typeIndex);
}

bool Il2CppDumper::writeDump(const char* outputPath) {
    FILE* fp = fopen(outputPath, "w");
    if (!fp) {
        LOGE("Failed to open output file: %s", outputPath);
        return false;
    }

    fprintf(fp, "// IL2CPP Dump generated by LibCpp Unity Dumper\n");
    fprintf(fp, "// Unity Version: %s\n", unityVersion_.c_str());
    fprintf(fp, "// Metadata Version: %d\n", metadataVersion_);
    fprintf(fp, "// Assemblies: %zu, Types: %zu, Methods: %zu, Fields: %zu, Properties: %zu, Events: %zu\n\n",
            assemblies_.size(), typeDefs_.size(), methodDefs_.size(), 
            fieldDefs_.size(), propertyDefs_.size(), eventDefs_.size());

    // Write each assembly/image
    for (size_t i = 0; i < images_.size(); ++i) {
        writeImage(fp, static_cast<int>(i));
    }

    fclose(fp);
    LOGI("Dump written to: %s", outputPath);
    return true;
}

void Il2CppDumper::writeAssembly(FILE* fp, int assemblyIndex) {
    if (assemblyIndex < 0 || assemblyIndex >= static_cast<int>(assemblies_.size())) return;
    
    const auto& assembly = assemblies_[assemblyIndex];
    const char* name = getString(assembly.nameIndex);
    
    fprintf(fp, "// ============================================================\n");
    fprintf(fp, "// Assembly: %s (Token: 0x%08X)\n", name, assembly.token);
    fprintf(fp, "// Image Index: %u\n", assembly.imageIndex);
    fprintf(fp, "// ============================================================\n\n");
}

void Il2CppDumper::writeImage(FILE* fp, int imageIndex) {
    if (imageIndex < 0 || imageIndex >= static_cast<int>(images_.size())) return;
    
    const auto& image = images_[imageIndex];
    const char* name = getString(image.nameIndex);
    
    fprintf(fp, "// ===== Image: %s =====\n", name ? name : "Unknown");
    
    // Find and write assembly for this image
    for (size_t i = 0; i < assemblies_.size(); ++i) {
        if (assemblies_[i].imageIndex == static_cast<ImageIndex>(imageIndex)) {
            writeAssembly(fp, static_cast<int>(i));
            break;
        }
    }
    
    // Write types in this image
    uint32_t typeEnd = image.typeStart + image.typeCount;
    for (uint32_t i = image.typeStart; i < typeEnd && i < typeDefs_.size(); ++i) {
        writeType(fp, static_cast<int>(i));
    }
    
    fprintf(fp, "\n");
}

void Il2CppDumper::writeType(FILE* fp, int typeIndex, int indent) {
    if (typeIndex < 0 || typeIndex >= static_cast<int>(typeDefs_.size())) return;
    
    const auto& typeDef = typeDefs_[typeIndex];
    const char* name = getString(typeDef.nameIndex);
    const char* ns = getString(typeDef.namespaceIndex);
    
    // Namespace
    if (ns && strlen(ns) > 0) {
        for (int i = 0; i < indent - 1; ++i) fprintf(fp, "    ");
        fprintf(fp, "namespace %s\n", ns);
        for (int i = 0; i < indent - 1; ++i) fprintf(fp, "    ");
        fprintf(fp, "{\n");
    }
    
    // Type declaration
    for (int i = 0; i < indent - 1; ++i) fprintf(fp, "    ");
    
    // Type kind
    const char* kind = "class";
    if (isValueType(&typeDef)) {
        if (isEnum(&typeDef)) kind = "enum";
        else kind = "struct";
    } else if ((typeDef.flags & 0x20) != 0) { // interface
        kind = "interface";
    }
    
    fprintf(fp, "// Type: %s (Token: 0x%08X, Flags: 0x%08X)\n", name, typeDef.token, typeDef.flags);
    for (int i = 0; i < indent - 1; ++i) fprintf(fp, "    ");
    fprintf(fp, "%s %s\n", kind, name);
    for (int i = 0; i < indent - 1; ++i) fprintf(fp, "    ");
    fprintf(fp, "{\n");
    
    // Fields
    for (uint16_t i = 0; i < typeDef.field_count; ++i) {
        int fieldIdx = typeDef.fieldStart + i;
        if (fieldIdx >= 0 && fieldIdx < static_cast<int>(fieldDefs_.size())) {
            writeField(fp, fieldIdx, indent);
        }
    }
    
    // Properties
    for (uint16_t i = 0; i < typeDef.property_count; ++i) {
        int propIdx = typeDef.propertyStart + i;
        if (propIdx >= 0 && propIdx < static_cast<int>(propertyDefs_.size())) {
            writeProperty(fp, propIdx, indent);
        }
    }
    
    // Events
    for (uint16_t i = 0; i < typeDef.event_count; ++i) {
        int eventIdx = typeDef.eventStart + i;
        if (eventIdx >= 0 && eventIdx < static_cast<int>(eventDefs_.size())) {
            writeEvent(fp, eventIdx, indent);
        }
    }
    
    // Methods
    for (uint16_t i = 0; i < typeDef.method_count; ++i) {
        int methodIdx = typeDef.methodStart + i;
        if (methodIdx >= 0 && methodIdx < static_cast<int>(methodDefs_.size())) {
            writeMethod(fp, methodIdx, indent);
        }
    }
    
    // Nested types
    if (typeDef.nested_type_count > 0) {
        // Parse nested types indices
        if (header_->nestedTypesOffset > 0 && typeDef.nestedTypesStart >= 0) {
            const int32_t* nestedIndices = reinterpret_cast<const int32_t*>(
                metadataBuffer_ + header_->nestedTypesOffset
            );
            for (uint16_t i = 0; i < typeDef.nested_type_count; ++i) {
                int nestedIdx = nestedIndices[typeDef.nestedTypesStart + i];
                if (nestedIdx >= 0 && nestedIdx < static_cast<int>(typeDefs_.size())) {
                    writeType(fp, nestedIdx, indent + 1);
                }
            }
        }
    }
    
    for (int i = 0; i < indent - 1; ++i) fprintf(fp, "    ");
    fprintf(fp, "}\n\n");
    
    if (ns && strlen(ns) > 0) {
        for (int i = 0; i < indent - 1; ++i) fprintf(fp, "    ");
        fprintf(fp, "}\n\n");
    }
}

void Il2CppDumper::writeMethod(FILE* fp, int methodIndex, int indent) {
    if (methodIndex < 0 || methodIndex >= static_cast<int>(methodDefs_.size())) return;
    
    const auto& method = methodDefs_[methodIndex];
    std::string signature = getMethodSignature(methodIndex);
    
    for (int i = 0; i < indent; ++i) fprintf(fp, "    ");
    fprintf(fp, "// Method: Token: 0x%08X, Flags: 0x%04X, Slot: %u, RVA: 0x%08X\n", 
            method.token, method.flags, method.slot, method.methodIndex);
    for (int i = 0; i < indent; ++i) fprintf(fp, "    ");
    fprintf(fp, "%s;\n\n", signature.c_str());
}

void Il2CppDumper::writeField(FILE* fp, int fieldIndex, int indent) {
    if (fieldIndex < 0 || fieldIndex >= static_cast<int>(fieldDefs_.size())) return;
    
    const auto& field = fieldDefs_[fieldIndex];
    const char* name = getString(field.nameIndex);
    std::string typeName = getFieldTypeName(fieldIndex);
    
    for (int i = 0; i < indent; ++i) fprintf(fp, "    ");
    fprintf(fp, "// Field: Token: 0x%08X, Offset: %u\n", field.token, 0);
    for (int i = 0; i < indent; ++i) fprintf(fp, "    ");
    fprintf(fp, "%s %s;\n", typeName.c_str(), name);
}

void Il2CppDumper::writeProperty(FILE* fp, int propertyIndex, int indent) {
    if (propertyIndex < 0 || propertyIndex >= static_cast<int>(propertyDefs_.size())) return;
    
    const auto& prop = propertyDefs_[propertyIndex];
    const char* name = getString(prop.nameIndex);
    std::string typeName = getTypeName(prop.typeIndex);
    
    for (int i = 0; i < indent; ++i) fprintf(fp, "    ");
    fprintf(fp, "// Property: Token: 0x%08X\n", prop.token);
    for (int i = 0; i < indent; ++i) fprintf(fp, "    ");
    fprintf(fp, "%s %s { get; set; }\n", typeName.c_str(), name);
}

void Il2CppDumper::writeEvent(FILE* fp, int eventIndex, int indent) {
    if (eventIndex < 0 || eventIndex >= static_cast<int>(eventDefs_.size())) return;
    
    const auto& event = eventDefs_[eventIndex];
    const char* name = getString(event.nameIndex);
    std::string typeName = getTypeName(event.typeIndex);
    
    for (int i = 0; i < indent; ++i) fprintf(fp, "    ");
    fprintf(fp, "// Event: Token: 0x%08X\n", event.token);
    for (int i = 0; i < indent; ++i) fprintf(fp, "    ");
    fprintf(fp, "event %s %s;\n", typeName.c_str(), name);
}

int Il2CppDumper::initialize(const char* soPath, const char* metadataPath) {
    LOGI("Initializing dumper with SO: %s, Metadata: %s", soPath, metadataPath);
    
    cleanup();
    
    if (!parseMetadata(metadataPath)) {
        LOGE("Failed to parse metadata");
        return -1;
    }
    
    // Verify SO file exists
    std::ifstream soFile(soPath, std::ios::binary | std::ios::ate);
    if (!soFile.is_open()) {
        LOGE("Failed to open SO file: %s", soPath);
        return -2;
    }
    size_t soSize = soFile.tellg();
    soFile.close();
    LOGI("SO file size: %zu bytes", soSize);
    
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