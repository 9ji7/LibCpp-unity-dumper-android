#pragma once

#include <jni.h>
#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <unordered_map>
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

struct Il2CppGlobalMetadataHeader {
    int32_t sanity;
    int32_t version;
    
    // String literals (for managed code)
    int32_t stringLiteralOffset;
    int32_t stringLiteralCount;
    int32_t stringLiteralDataOffset;
    int32_t stringLiteralDataCount;
    
    // Strings (for metadata - type/method/field names)
    int32_t stringOffset;
    int32_t stringCount;
    
    // Events
    int32_t eventsOffset;
    int32_t eventsCount;
    
    // Properties
    int32_t propertiesOffset;
    int32_t propertiesCount;
    
    // Methods
    int32_t methodsOffset;
    int32_t methodsCount;
    
    // Parameter default values
    int32_t parameterDefaultValuesOffset;
    int32_t parameterDefaultValuesCount;
    
    // Field default values
    int32_t fieldDefaultValuesOffset;
    int32_t fieldDefaultValuesCount;
    
    // Field and parameter default value data
    int32_t fieldAndParameterDefaultValueDataOffset;
    int32_t fieldAndParameterDefaultValueDataCount;
    
    // Field marshaled sizes
    int32_t fieldMarshaledSizesOffset;
    int32_t fieldMarshaledSizesCount;
    
    // Parameters
    int32_t parametersOffset;
    int32_t parametersCount;
    
    // Fields
    int32_t fieldsOffset;
    int32_t fieldsCount;
    
    // Generic parameters
    int32_t genericParametersOffset;
    int32_t genericParametersCount;
    
    // Generic parameter constraints
    int32_t genericParameterConstraintsOffset;
    int32_t genericParameterConstraintsCount;
    
    // Generic containers
    int32_t genericContainersOffset;
    int32_t genericContainersCount;
    
    // Nested types
    int32_t nestedTypesOffset;
    int32_t nestedTypesCount;
    
    // Interfaces
    int32_t interfacesOffset;
    int32_t interfacesCount;
    
    // VTable methods
    int32_t vtableMethodsOffset;
    int32_t vtableMethodsCount;
    
    // Interface offsets
    int32_t interfaceOffsetsOffset;
    int32_t interfaceOffsetsCount;
    
    // Type definitions
    int32_t typeDefinitionsOffset;
    int32_t typeDefinitionsCount;
    
    // RGCTX entries
    int32_t rgctxEntriesOffset;
    int32_t rgctxEntriesCount;
    
    // Images
    int32_t imagesOffset;
    int32_t imagesCount;
    
    // Assemblies
    int32_t assembliesOffset;
    int32_t assembliesCount;
    
    // Metadata usage lists (v19+)
    int32_t metadataUsageListsOffset;
    int32_t metadataUsageListsCount;
    
    // Metadata usage pairs (v19+)
    int32_t metadataUsagePairsOffset;
    int32_t metadataUsagePairsCount;
    
    // Field refs (v19+)
    int32_t fieldRefsOffset;
    int32_t fieldRefsCount;
    
    // Referenced assemblies (v20+)
    int32_t referencedAssembliesOffset;
    int32_t referencedAssembliesCount;
    
    // Attributes info (v21-v27)
    int32_t attributesInfoOffset;
    int32_t attributesInfoCount;
    
    // Attribute types (v21-v27)
    int32_t attributeTypesOffset;
    int32_t attributeTypesCount;
    
    // Unresolved virtual call parameter types (v22+)
    int32_t unresolvedVirtualCallParameterTypesOffset;
    int32_t unresolvedVirtualCallParameterTypesCount;
    
    // Unresolved virtual call parameter ranges (v22+)
    int32_t unresolvedVirtualCallParameterRangesOffset;
    int32_t unresolvedVirtualCallParameterRangesCount;
    
    // Windows runtime type names (v24+)
    int32_t windowsRuntimeTypeNamesOffset;
    int32_t windowsRuntimeTypeNamesSize;
    
    // Exported type definitions (v24+)
    int32_t exportedTypeDefinitionsOffset;
    int32_t exportedTypeDefinitionsCount;
    
    // For v29+
    int32_t attributeDataRangeOffset;
    int32_t attributeDataRangeSize;
};

// Type definition (v27+ / v29+ layout - 88 bytes)
struct Il2CppTypeDefinition {
    StringIndex nameIndex;
    StringIndex namespaceIndex;
    TypeIndex byvalTypeIndex;
    TypeIndex declaringTypeIndex;
    TypeIndex parentIndex;
    TypeIndex elementTypeIndex;
    GenericContainerIndex genericContainerIndex;
    uint32_t flags;
    FieldIndex fieldStart;
    MethodIndex methodStart;
    EventIndex eventStart;
    PropertyIndex propertyStart;
    NestedTypeIndex nestedTypesStart;
    InterfacesIndex interfacesStart;
    VTableIndex vtableStart;
    InterfacesIndex interfaceOffsetsStart;
    uint16_t method_count;
    uint16_t property_count;
    uint16_t field_count;
    uint16_t event_count;
    uint16_t nested_type_count;
    uint16_t vtable_count;
    uint16_t interfaces_count;
    uint16_t interface_offsets_count;
    uint32_t bitfield;
    uint32_t token;
};

// Method definition (v24.2+ layout - 32 bytes)
struct Il2CppMethodDefinition {
    StringIndex nameIndex;
    TypeDefinitionIndex declaringType;
    TypeIndex returnType;
    ParameterIndex parameterStart;
    CustomAttributeIndex customAttributeIndex;
    GenericContainerIndex genericContainerIndex;
    MethodIndex methodIndex;
    MethodIndex invokerIndex;
    MethodIndex delegateWrapperIndex;
    RGCTXIndex rgctxStartIndex;
    int32_t rgctxCount;
    uint32_t token;
    uint16_t flags;
    uint16_t iflags;
    uint16_t slot;
    uint16_t parameterCount;
};

// Field definition (16 bytes)
struct Il2CppFieldDefinition {
    StringIndex nameIndex;
    TypeIndex typeIndex;
    CustomAttributeIndex customAttributeIndex;
    uint32_t token;
};

// Property definition (24 bytes)
struct Il2CppPropertyDefinition {
    StringIndex nameIndex;
    MethodIndex getter;
    MethodIndex setter;
    TypeIndex typeIndex;
    TypeDefinitionIndex parent;
    uint32_t token;
};

// Event definition (24 bytes)
struct Il2CppEventDefinition {
    StringIndex nameIndex;
    TypeIndex typeIndex;
    MethodIndex add;
    MethodIndex remove;
    MethodIndex raise;
    uint32_t token;
};

// Parameter definition (16 bytes)
struct Il2CppParameterDefinition {
    StringIndex nameIndex;
    uint32_t token;
    CustomAttributeIndex customAttributeIndex;
    TypeIndex typeIndex;
};

// Image definition (24 bytes)
struct Il2CppImageDefinition {
    StringIndex nameIndex;
    AssemblyIndex assemblyIndex;
    TypeDefinitionIndex typeStart;
    uint32_t typeCount;
    MethodIndex entryPointIndex;
    uint32_t token;
};

// Assembly definition (12 bytes for v24.2+, 16 bytes for v27+)
struct Il2CppAssemblyDefinition {
    StringIndex nameIndex;
    uint32_t token;
    ImageIndex imageIndex;
};

// Generic container (24 bytes)
struct Il2CppGenericContainer {
    TypeDefinitionIndex ownerIndex;
    GenericParameterIndex genericParameterStart;
    int32_t genericParameterCount;
    GenericInstIndex genericInstIndex;
    int32_t type_argc;
    TypeIndex type_argv;
};

// Generic parameter (12 bytes)
struct Il2CppGenericParameter {
    TypeDefinitionIndex ownerIndex;
    StringIndex nameIndex;
    GenericContainerIndex constraintsStart;
    int16_t constraintsCount;
    uint16_t num;
    uint16_t flags;
};

// String literal (8 bytes)
struct Il2CppStringLiteral {
    uint32_t length;
    uint32_t dataIndex;
};

// Type definition (for metadata)
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
    // Parsing
    bool parseMetadata(const char* metadataPath);
    bool parseHeader();
    bool parseStrings();
    bool parseAssemblies();
    bool parseImages();
    bool parseTypeDefinitions();
    bool parseMethodDefinitions();
    bool parseFieldDefinitions();
    bool parsePropertyDefinitions();
    bool parseEventDefinitions();
    bool parseParameterDefinitions();
    bool parseGenericContainers();
    bool parseGenericParameters();
    bool parseStringLiterals();
    
    // Version-specific parsing
    void adjustForVersion();
    
    // Dumping
    bool writeDump(const char* outputPath);
    void writeAssembly(FILE* fp, int assemblyIndex);
    void writeImage(FILE* fp, int imageIndex);
    void writeType(FILE* fp, int typeIndex, int indent = 1);
    void writeMethod(FILE* fp, int methodIndex, int indent);
    void writeField(FILE* fp, int fieldIndex, int indent);
    void writeProperty(FILE* fp, int propertyIndex, int indent);
    void writeEvent(FILE* fp, int eventIndex, int indent);
    
    // Helpers
    const char* getString(uint32_t index) const;
    const char* getStringLiteral(uint32_t index) const;
    std::string getTypeName(TypeIndex typeIndex) const;
    std::string getTypeNameFromDefinition(const Il2CppTypeDefinition* typeDef) const;
    bool isValueType(const Il2CppTypeDefinition* typeDef) const;
    bool isEnum(const Il2CppTypeDefinition* typeDef) const;
    std::string getMethodSignature(int methodIndex) const;
    std::string getFieldTypeName(int fieldIndex) const;
    
    // Metadata buffer
    uint8_t* metadataBuffer_ = nullptr;
    size_t metadataSize_ = 0;
    Il2CppGlobalMetadataHeader* header_ = nullptr;
    
    // Parsed data
    std::vector<std::string> strings_;
    std::vector<Il2CppStringLiteral> stringLiterals_;
    std::vector<const char*> stringLiteralData_;
    std::vector<Il2CppAssemblyDefinition> assemblies_;
    std::vector<Il2CppImageDefinition> images_;
    std::vector<Il2CppTypeDefinition> typeDefs_;
    std::vector<Il2CppMethodDefinition> methodDefs_;
    std::vector<Il2CppFieldDefinition> fieldDefs_;
    std::vector<Il2CppPropertyDefinition> propertyDefs_;
    std::vector<Il2CppEventDefinition> eventDefs_;
    std::vector<Il2CppParameterDefinition> parameterDefs_;
    std::vector<Il2CppGenericContainer> genericContainers_;
    std::vector<Il2CppGenericParameter> genericParameters_;
    std::vector<int32_t> nestedTypes_;
    std::vector<int32_t> interfaces_;
    std::vector<int32_t> vtableMethods_;
    std::vector<int32_t> interfaceOffsets_;
    
    // Version info
    int metadataVersion_ = 0;
    bool isVersion24Plus_ = false;
    bool isVersion27Plus_ = false;
    bool isVersion29Plus_ = false;
    
    std::string unityVersion_;
    bool initialized_ = false;
};