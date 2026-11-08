#pragma once

#include <jni.h>
#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <dlfcn.h>
#include <android/log.h>

#define LOG_TAG "LibCppDumper"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

// ============================================================================
// IL2CPP Metadata Types (matching il2cpp-metadata.h)
// ============================================================================

using TypeIndex = int32_t;
using TypeDefinitionIndex = int32_t;
using FieldIndex = int32_t;
using DefaultValueIndex = int32_t;
using DefaultValueDataIndex = int32_t;
using CustomAttributeIndex = int32_t;
using ParameterIndex = int32_t;
using MethodIndex = int32_t;
using GenericMethodIndex = int32_t;
using PropertyIndex = int32_t;
using EventIndex = int32_t;
using GenericContainerIndex = int32_t;
using GenericParameterIndex = int32_t;
using GenericParameterConstraintIndex = int16_t;
using NestedTypeIndex = int32_t;
using InterfacesIndex = int32_t;
using VTableIndex = int32_t;
using InterfaceOffsetIndex = int32_t;
using RGCTXIndex = int32_t;
using StringIndex = int32_t;
using StringLiteralIndex = int32_t;
using GenericInstIndex = int32_t;
using ImageIndex = int32_t;
using AssemblyIndex = int32_t;
using InteropDataIndex = int32_t;
using GuidIndex = int32_t;

constexpr TypeIndex kTypeIndexInvalid = -1;
constexpr TypeDefinitionIndex kTypeDefinitionIndexInvalid = -1;
constexpr FieldIndex kFieldIndexInvalid = -1;
constexpr MethodIndex kMethodIndexInvalid = -1;
constexpr PropertyIndex kPropertyIndexInvalid = -1;
constexpr EventIndex kEventIndexInvalid = -1;
constexpr GenericContainerIndex kGenericContainerIndexInvalid = -1;
constexpr GenericParameterIndex kGenericParameterIndexInvalid = -1;
constexpr RGCTXIndex kRGCTXIndexInvalid = -1;
constexpr StringLiteralIndex kStringLiteralIndexInvalid = -1;
constexpr InteropDataIndex kInteropDataIndexInvalid = -1;

// Metadata header from global-metadata.dat
struct Il2CppGlobalMetadataHeader {
    int32_t sanity;
    int32_t version;
    int32_t stringLiteralOffset;
    int32_t stringLiteralCount;
    int32_t stringLiteralDataOffset;
    int32_t stringLiteralDataCount;
    int32_t stringOffset;
    int32_t stringCount;
    int32_t eventsOffset;
    int32_t eventsCount;
    int32_t propertiesOffset;
    int32_t propertiesCount;
    int32_t methodsOffset;
    int32_t methodsCount;
    int32_t parameterDefaultValuesOffset;
    int32_t parameterDefaultValuesCount;
    int32_t fieldDefaultValuesOffset;
    int32_t fieldDefaultValuesCount;
    int32_t fieldAndParameterDefaultValueDataOffset;
    int32_t fieldAndParameterDefaultValueDataCount;
    int32_t fieldMarshaledSizesOffset;
    int32_t fieldMarshaledSizesCount;
    int32_t parametersOffset;
    int32_t parametersCount;
    int32_t fieldsOffset;
    int32_t fieldsCount;
    int32_t genericParametersOffset;
    int32_t genericParametersCount;
    int32_t genericParameterConstraintsOffset;
    int32_t genericParameterConstraintsCount;
    int32_t genericContainersOffset;
    int32_t genericContainersCount;
    int32_t nestedTypesOffset;
    int32_t nestedTypesCount;
    int32_t interfacesOffset;
    int32_t interfacesCount;
    int32_t vtableMethodsOffset;
    int32_t vtableMethodsCount;
    int32_t interfaceOffsetsOffset;
    int32_t interfaceOffsetsCount;
    int32_t typeDefinitionsOffset;
    int32_t typeDefinitionsCount;
    int32_t rgctxEntriesOffset;
    int32_t rgctxEntriesCount;
    int32_t imagesOffset;
    int32_t imagesCount;
    int32_t assembliesOffset;
    int32_t assembliesCount;
    int32_t metadataUsageListsOffset;
    int32_t metadataUsageListsCount;
    int32_t metadataUsagePairsOffset;
    int32_t metadataUsagePairsCount;
    int32_t fieldRefsOffset;
    int32_t fieldRefsCount;
    int32_t referencedAssembliesOffset;
    int32_t referencedAssembliesCount;
    int32_t attributesInfoOffset;
    int32_t attributesInfoCount;
    int32_t attributeTypesOffset;
    int32_t attributeTypesCount;
    int32_t unresolvedVirtualCallParameterTypesOffset;
    int32_t unresolvedVirtualCallParameterTypesCount;
    int32_t unresolvedVirtualCallParameterRangesOffset;
    int32_t unresolvedVirtualCallParameterRangesCount;
    int32_t windowsRuntimeTypeNamesOffset;
    int32_t windowsRuntimeTypeNamesSize;
    int32_t exportedTypeDefinitionsOffset;
    int32_t exportedTypeDefinitionsCount;
    int32_t attributeDataRangeOffset;
    int32_t attributeDataRangeSize;
};

// Type definition (v24+ layout - 32 bytes)
struct Il2CppTypeDefinition {
    StringIndex nameIndex;
    StringIndex namespaceIndex;
    uint32_t byvalTypeIndex;
    uint32_t thisTypeIndex;
    uint32_t elementTypeIndex;
    int32_t genericContainerIndex;
    uint32_t flags;
    uint32_t token;
    uint32_t fieldStart;
    uint32_t methodStart;
    uint32_t eventStart;
    uint32_t propertyStart;
    uint32_t nestedTypesStart;
    uint32_t interfacesStart;
    uint32_t vtableStart;
    uint32_t interfaceOffsetsStart;
    uint16_t method_count;
    uint16_t property_count;
    uint16_t field_count;
    uint16_t event_count;
    uint16_t nested_type_count;
    uint16_t vtable_count;
    uint16_t interfaces_count;
    uint16_t interface_offsets_count;
};

// Field definition (20 bytes)
struct Il2CppFieldDefinition {
    uint32_t nameIndex;
    uint32_t typeIndex;
    uint32_t parentIndex;
    uint32_t offset;
    uint32_t token;
};

// Method definition (36 bytes)
struct Il2CppMethodDefinition {
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

// Property definition (24 bytes)
struct Il2CppPropertyDefinition {
    uint32_t nameIndex;
    uint32_t getterIndex;
    uint32_t setterIndex;
    uint32_t typeIndex;
    uint32_t parentIndex;
    uint32_t token;
};

// Event definition (24 bytes)
struct Il2CppEventDefinition {
    uint32_t nameIndex;
    uint32_t typeIndex;
    uint32_t addIndex;
    uint32_t removeIndex;
    uint32_t raiseIndex;
    uint32_t token;
};

// Parameter definition (16 bytes)
struct Il2CppParameterDefinition {
    uint32_t nameIndex;
    uint32_t typeIndex;
    uint32_t parentIndex;
    uint32_t token;
};

// String literal (8 bytes)
struct Il2CppStringLiteral {
    uint32_t length;
    uint32_t dataIndex;
};

// Assembly definition (12 bytes)
struct Il2CppMetadataAssembly {
    uint32_t nameIndex;
    uint32_t token;
    uint32_t imageIndex;
};

// Image definition (24 bytes)
struct Il2CppMetadataImage {
    uint32_t nameIndex;
    uint32_t assemblyIndex;
    uint32_t typeStart;
    uint32_t typeCount;
    uint32_t exportedTypeStart;
    uint32_t exportedTypeCount;
};

// ============================================================================
// IL2CPP Runtime Types (from libil2cpp.so via MetadataRegistration)
// ============================================================================

enum Il2CppTypeEnum {
    IL2CPP_TYPE_END = 0x00,
    IL2CPP_TYPE_VOID = 0x01,
    IL2CPP_TYPE_BOOLEAN = 0x02,
    IL2CPP_TYPE_CHAR = 0x03,
    IL2CPP_TYPE_I1 = 0x04,
    IL2CPP_TYPE_U1 = 0x05,
    IL2CPP_TYPE_I2 = 0x06,
    IL2CPP_TYPE_U2 = 0x07,
    IL2CPP_TYPE_I4 = 0x08,
    IL2CPP_TYPE_U4 = 0x09,
    IL2CPP_TYPE_I8 = 0x0A,
    IL2CPP_TYPE_U8 = 0x0B,
    IL2CPP_TYPE_R4 = 0x0C,
    IL2CPP_TYPE_R8 = 0x0D,
    IL2CPP_TYPE_STRING = 0x0E,
    IL2CPP_TYPE_PTR = 0x0F,
    IL2CPP_TYPE_BYREF = 0x10,
    IL2CPP_TYPE_VALUETYPE = 0x11,
    IL2CPP_TYPE_CLASS = 0x12,
    IL2CPP_TYPE_VAR = 0x13,
    IL2CPP_TYPE_ARRAY = 0x14,
    IL2CPP_TYPE_GENERICINST = 0x15,
    IL2CPP_TYPE_TYPEDBYREF = 0x16,
    IL2CPP_TYPE_I = 0x17,
    IL2CPP_TYPE_U = 0x18,
    IL2CPP_TYPE_FNPTR = 0x1B,
    IL2CPP_TYPE_OBJECT = 0x1C,
    IL2CPP_TYPE_SZARRAY = 0x1D,
    IL2CPP_TYPE_MVAR = 0x1E,
    IL2CPP_TYPE_CMOD_REQD = 0x1F,
    IL2CPP_TYPE_CMOD_OPT = 0x20,
    IL2CPP_TYPE_INTERNAL = 0x21,
    IL2CPP_TYPE_MODIFIER = 0x40,
    IL2CPP_TYPE_SENTINEL = 0x41,
    IL2CPP_TYPE_PINNED = 0x45,
    IL2CPP_TYPE_ENUM = 0x55,
};

struct Il2CppType {
    union {
        void* dummy;
        TypeDefinitionIndex klassIndex;
        const Il2CppType* type;
        void* array;
        GenericParameterIndex genericParameterIndex;
        void* generic_class;
    } data;
    unsigned int attrs : 16;
    uint8_t type : 8;
    unsigned int num_mods : 6;
    unsigned int byref : 1;
    unsigned int pinned : 1;
};

struct Il2CppGenericInst {
    uint32_t type_argc;
    const Il2CppType** type_argv;
};

struct Il2CppGenericClass {
    TypeDefinitionIndex typeDefinitionIndex;
    const Il2CppGenericInst* context;
};

struct Il2CppGenericContext {
    const Il2CppGenericInst* class_inst;
    const Il2CppGenericInst* method_inst;
};

struct Il2CppMetadataRegistration {
    int32_t typesCount;
    const Il2CppType** types;
    int32_t fieldOffsetsCount;
    const int32_t* fieldOffsets;
    int32_t genericClassesCount;
    const void** genericClasses;
    int32_t methodSpecsCount;
    const void** methodSpecs;
    int32_t imagesCount;
    const void** images;
    int32_t assembliesCount;
    const void** assemblies;
    int32_t typeDefinitionsSizesCount;
    const void** typeDefinitionsSizes;
    int32_t fieldDefinitionsCount;
    const void** fieldDefinitions;
    int32_t methodDefinitionsCount;
    const void** methodDefinitions;
    int32_t propertyDefinitionsCount;
    const void** propertyDefinitions;
    int32_t eventDefinitionsCount;
    const void** eventDefinitions;
    int32_t parameterDefinitionsCount;
    const void** parameterDefinitions;
    int32_t nestedTypesCount;
    const void** nestedTypes;
    int32_t interfacesCount;
    const void** interfaces;
    int32_t vtableMethodsCount;
    const void** vtableMethods;
    int32_t interfaceOffsetsCount;
    const void** interfaceOffsets;
};

// ============================================================================
// Il2CppDumper Class
// ============================================================================

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
    // Parsing metadata file (global-metadata.dat)
    bool parseMetadata(const char* metadataPath);
    bool parseStrings();
    bool parseAssemblies();
    bool parseImages();
    bool parseTypeDefinitions();
    bool parseFieldDefinitions();
    bool parseMethodDefinitions();
    bool parsePropertyDefinitions();
    bool parseParameterDefinitions();
    bool parseStringLiterals();
    void adjustForVersion();

    // Loading libil2cpp.so and finding metadata registration
    bool loadIl2CppSo(const char* soPath);
    bool findMetadataRegistration();
    bool parseIl2CppTypes();
    
    // Type resolution using Il2CppType from metadata registration
    std::string getTypeName(TypeIndex typeIndex) const;
    std::string getTypeNameFromDefinition(const Il2CppTypeDefinition* typeDef) const;
    std::string getFieldTypeName(int fieldIndex) const;
    std::string getMethodSignature(int methodIndex) const;
    bool isValueType(const Il2CppTypeDefinition* typeDef) const;
    bool isEnum(const Il2CppTypeDefinition* typeDef) const;
    
    // Dumping
    bool writeDump(const char* outputPath);
    void writeAssembly(FILE* fp, const Il2CppMetadataAssembly& assembly);
    void writeImage(FILE* fp, int imageIndex);
    void writeType(FILE* fp, int typeIndex);
    void writeMethod(FILE* fp, int methodIndex);
    void writeField(FILE* fp, int fieldIndex);
    void writeProperty(FILE* fp, int propertyIndex);
    void writeEvent(FILE* fp, int eventIndex);
    
    // Helpers
    const char* getString(uint32_t index) const;
    const char* getStringLiteral(uint32_t index) const;

    // Metadata file (global-metadata.dat)
    uint8_t* metadataBuffer_ = nullptr;
    size_t metadataSize_ = 0;
    Il2CppGlobalMetadataHeader* header_ = nullptr;
    
    // Metadata header info (cached for easy access)
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
    
    // Parsed metadata tables
    std::vector<std::string> strings_;
    std::vector<const char*> stringLiteralData_;
    std::vector<Il2CppMetadataAssembly> assemblies_;
    std::vector<Il2CppMetadataImage> images_;
    std::vector<Il2CppTypeDefinition> typeDefs_;
    std::vector<Il2CppFieldDefinition> fieldDefs_;
    std::vector<Il2CppMethodDefinition> methodDefs_;
    std::vector<Il2CppPropertyDefinition> propertyDefs_;
    std::vector<Il2CppEventDefinition> eventDefs_;
    std::vector<Il2CppParameterDefinition> parameterDefs_;
    
    // IL2CPP runtime types (from libil2cpp.so)
    void* il2cppHandle_ = nullptr;
    const Il2CppMetadataRegistration* metadataRegistration_ = nullptr;
    const Il2CppType** il2cppTypes_ = nullptr;
    int32_t il2cppTypesCount_ = 0;
    
    // Version info
    std::string unityVersion_;
    bool initialized_ = false;
};